#include <liblitedram/ddr5_training.h>
#ifdef LIBLITEDRAM_DDR5_TRAINING_H
#include <liblitedram/ddr5_helpers.h>
#include <stdio.h>
#include <inttypes.h>

#ifdef MEMORY_TYPE_DDR5
//#define INFO_DDR5
//#define DEBUG_CA_DDR5
//#define DEBUG_DDR5

#define BYTES_PER_MODULE (SDRAM_PHY_DQ_DQS_RATIO/4)

int N2_mode = 1;
int enumerated = 0;

// Addressing: channel, pin, 0-right eye closing, 1-left eye closing
//      \______________/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
//      --------<============>-------------
//              | valid data |
//most left point            most right point
//
// Delay clock has effects of moving signal to "the left"
//      ‾‾‾‾‾\______________/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
//      --------<============>-------------
// while delaying signal itself to "the right"
//      \______________/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
//      --------------<============>-------------

int ca_line_count = 14;
int has_parity = 0;
int max_value = 0;

int cs_delays[2][SDRAM_PHY_RANKS][2];
int cs_coarse_delays[2][SDRAM_PHY_RANKS];

int ca_delays[2][14][2];
int par_delays[2][2];
//If per rank timings are available, the array above should be [SDRAM_PHY_RANKS][2][14][2]
//To cover clock/ca delays per rank
int cs_final_delay[2][SDRAM_PHY_RANKS];

int ca_final_delay[2][14];
int par_final_delay[2];

int WICA = 0;

#ifdef SDRAM_PHY_SUBCHANNELS
#define CHANNELS 2
#else
#define CHANNELS 1
#endif

static int CS_on_edge_detect(int32_t channel, int32_t rank) {
    int offset, _result;
    for (offset = 0; offset < 2; offset++) {
        cs_sample_prep(channel, rank, 0, offset);
        if (or_sample(channel))
            _result = offset<<1|1;
    }
    return _result;
}

static int CS_ck_scan(int32_t channel, int32_t rank, int offset) {
    int works, last_good, _result, ckdly;
    works = 1;
    printf("|");
    last_good = 0;
    cs_rst(channel, rank, 0);
    for(ckdly = 0; ckdly < SDRAM_PHY_DELAYS && works; ckdly++) {
        cs_sample_prep(channel, rank, 0, offset);
        _result = or_sample(channel);
        printf("%d", !!_result);
        if (!_result && works) {
            works = 0;
            last_good = -(ckdly - 1);
        } else if (_result && works && ckdly == SDRAM_PHY_DELAYS - 1) {
            last_good = -ckdly;
        }
        ck_inc(channel, rank, 0);
    }
    ck_rst(channel, rank, 0);
    return last_good;
}

static int CS_find_offset(int32_t channel, int32_t rank) {
    int csdly, offset;
    int offset_result[2];
    cs_rst(channel, rank, 0);
    for (offset = 0; offset < 2; offset++) {
        for (csdly = 0; csdly < SDRAM_PHY_DELAYS; csdly++) {
            cs_sample_prep(channel, rank, 0, offset);
            // Found working pattern
            if (or_sample(channel)) {
                offset_result[offset] = csdly;
                break;
            }
            cs_inc(channel, rank, 0);
        }
        cs_rst(channel, rank, 0);
    }
    return offset_result[0] < offset_result[1] ? 0 : 1;
}

static void CS_scan(int32_t channel, int32_t rank, int* left, int* right) {
    int works, csdly, offset;
    offset = CS_find_offset(channel, rank);
    cs_rst(channel, rank, 0);
    for (csdly = 0; csdly < SDRAM_PHY_DELAYS; csdly++) {
        cs_sample_prep(channel, rank, 0, offset);
        works = or_sample(channel);
        printf("%d", !!works);
        if (works && *right  == UNSET_DELAY)
            *right = csdly;
        if ((!works || csdly == SDRAM_PHY_DELAYS - 1) && *right != UNSET_DELAY && *left == UNSET_DELAY)
            *left = csdly;
        cs_inc(channel, rank, 0);
    }
    cs_rst(channel, rank, 0);
}

static void CS_training(int32_t channel, uint8_t *success) {
    int32_t csdly, coarse;
    int32_t rank;
    int32_t on_edge;
    // If we ever have multiple ranks, and independent timing for them
    // Uncomment loop below
    // for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
    {rank = 0;
        printf("Rank: %2"PRId32"", rank);

        // Enter CS training MPC
        enter_cs(channel, rank);

        // Scan clock delays only if one of patterns work (0x55 or 0xAA)
        // If neither works then we are in meta state, both clock and CS change
        // to close to each other. In such situations we only check CS delays

        // Get working pattern
        on_edge = CS_on_edge_detect(channel, rank);

        cs_delays[channel][rank][0] = UNSET_DELAY;
        cs_delays[channel][rank][1] = UNSET_DELAY;

        // We got one of the patterns to work flawlessly, check clock delays
        if (on_edge) {
            cs_delays[channel][rank][0] = CS_ck_scan(channel, rank, on_edge >> 1);
        }
        // After delaying clock, pattern could have changes, as we missed one clock cycle
        printf("|");
        CS_scan(channel, rank, &cs_delays[channel][rank][1], &cs_delays[channel][rank][0]);
        printf("|\n");

        cs_rst(channel, rank, 0);

        // Set up coarse delay adjustment until we get CA results
        printf("Rank delays: %2d:%2d\n", cs_delays[channel][rank][0], cs_delays[channel][rank][1]);
        coarse = (cs_delays[channel][rank][0]+cs_delays[channel][rank][1]) / 2;
        coarse = coarse < 0 ? 0 : coarse;
        printf("Coarse adjustment:%"PRId32"\n", coarse);
        cs_coarse_delays[channel][rank] = coarse;

        for (csdly = 0; csdly < coarse; ++csdly)
            cs_inc(channel, rank, 0);

        // Exit CS training MPC
        exit_cs(channel, rank);
    }
}

static void CA_setup_array(void) {
    int channel, address;
    for (channel = 0; channel < 2; ++channel) {
        for (address = 0; address <14; ++address) {
           ca_delays[channel][address][0] = -SDRAM_PHY_DELAYS;
           ca_delays[channel][address][1] = SDRAM_PHY_DELAYS;
        }
    }
}

static void CA_check_lines(int32_t channel) {
    enter_ca(channel, 0);
    cmd_injector(channel, 0xf, 0, 1<<13, 0, 0, 1, 0);
    cmd_injector(channel, 0x1, 1, 1<<13, 0, 0, 1, 0);
    store_continuous(channel);
    if (and_sample(channel))
        ca_line_count = 14;
    else
        ca_line_count = 13;
    exit_ca(channel, 0);
    printf("DDR5 module has %d address lines\n", ca_line_count);
}

static int CA_detect(int32_t channel, int32_t rank, int32_t address, int cs_dly) {
    int _result;
    ca_sample_prep_current_period(channel, rank, address, 1, cs_dly);
    _result = and_sample(channel);
    ca_sample_prep_current_period(channel, rank, address, 0, cs_dly);
    _result &= or_sample(channel);
    return _result;
}

static int CA_ck_scan(int32_t channel, int32_t rank, int32_t address, int32_t csdly_base) {
    int works, last_good, _result, ckdly, csdly;
    printf("|");
    works = 1;
    last_good = 0;
    csdly = csdly_base;
    ca_rst(channel, rank, address);
    for(ckdly = 0; ckdly < SDRAM_PHY_DELAYS && works; ++ckdly, ++csdly) {
        _result = CA_detect(channel, rank, address, csdly/SDRAM_PHY_DELAYS);
        printf("%d", !!_result);
        if (!_result && works) {
            works = 0;
            last_good = -(ckdly - 1);
        } else if(_result && works && ckdly == SDRAM_PHY_DELAYS - 1) {
            last_good = -ckdly;
        }
        ck_inc(channel, rank, 0);
        cs_inc(channel, rank, 0);
    }
    ck_rst(channel, rank, 0);

    cs_rst(channel, rank, 0);
    for (csdly = 0; csdly < csdly_base; ++csdly)
        cs_inc(channel, rank, 0);

    return last_good;
}

static void CA_scan(int32_t channel, int32_t rank, int32_t address, int* left, int* right) {
    int cadly, _result;
    ca_rst(channel, rank, address);
    for (cadly = 0; cadly < SDRAM_PHY_DELAYS; cadly++) {
#ifdef DEBUG_CA_DDR5
        printf("CA%2"PRIu32" dly:%"PRIu16"\n", address,
               get_ca_dly(channel, rank, address));
#endif // DEBUG_CA_DDR5
        _result = CA_detect(channel, rank, address, 0);
        printf("%d", !!_result);
#ifdef DEBUG_CA_DDR5
        printf("\n");
#endif // DEBUG_CA_DDR5
        if (_result && *right == UNSET_DELAY)
            *right = cadly;
        if ((!_result || cadly == SDRAM_PHY_DELAYS - 1) && *right != UNSET_DELAY && *left == UNSET_DELAY)
            *left = cadly;
        ca_inc(channel, rank, address);
    }
    ca_rst(channel, rank, address);
}

static void CA_training(int32_t channel) {
    int left_side, right_side;
    int32_t rank, address;
    int32_t on_edge;

    // If we ever have multiple ranks, and independent timing for them
    // Uncomment loop below
    // for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
    {rank = 0;
        printf("Rank:%2"PRId32"\n", rank);
        // Enter CA training MPC
        enter_ca(channel, rank);

        for (address = 0; address < ca_line_count; address++) {
            printf("CA line:%2"PRId32"", address);

            // Reset CA delay
            ca_rst(channel, rank, address);

            // Check if address line is correct with 0 tap.
            on_edge = CA_detect(channel, rank, address, 0);
            on_edge &= CA_detect(channel, rank, address, 0);
            on_edge &= CA_detect(channel, rank, address, 0);

            left_side = UNSET_DELAY;
            right_side = UNSET_DELAY;
            // Address line works with no delays no clock or ca delay
            if (on_edge) {
                right_side = CA_ck_scan(channel, rank, address, cs_coarse_delays[channel][rank]);
            }
            printf("|");
            CA_scan(channel, rank, address, &left_side, &right_side);
            printf("|\n");

            if(right_side == UNSET_DELAY) {
                ca_delays[channel][address][0] = UNSET_DELAY;
                break;
            } else if (right_side > ca_delays[channel][address][0]) {
                ca_delays[channel][address][0] = right_side;
            }

            if(left_side == UNSET_DELAY) {
                ca_delays[channel][address][1] = UNSET_DELAY;
                break;
            } else if (left_side < ca_delays[channel][address][1]) {
                ca_delays[channel][address][1] = left_side;
            }
        }
        // Exit CA training multiple NOPs
        exit_ca(channel, rank);
    }
}

static void CA_check_values(int32_t channel, uint8_t *success) {
    int32_t address;
    for (address = 0; address < ca_line_count; address++) {
        if (ca_delays[channel][address][1] == UNSET_DELAY || ca_delays[channel][address][0] == UNSET_DELAY) {
            printf("CA:%2"PRId32" Eye width:0 Failed\n", address);
            *success &= 0;
            return;
        }
    }
}

static void CS_CA_best_timings(void) {
    int channel, rank, address, newdly, cntdly;
    int min, max, temp;
    min = SDRAM_PHY_DELAYS;
    max = -SDRAM_PHY_DELAYS;

    for (channel = 0; channel < CHANNELS; channel++) {
        printf("Subchannel:%c Timings\n", 'A'+channel);
        // If we ever have multiple ranks, and independent timing for them
        // Uncomment loop below
        // for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
        {rank = 0;
            temp = (cs_delays[channel][rank][0] + cs_delays[channel][rank][1])/2;
            printf("Rank:%2d: min delay %2d, max delay %2d, center %2d\n",
                rank, cs_delays[channel][rank][0], cs_delays[channel][rank][1], temp);

            cs_final_delay[channel][rank] = temp;
            min = min > temp ? temp : min;
            max = max < temp ? temp : max;
        }
        for (address = 0; address < ca_line_count; address++) {
            temp = (ca_delays[channel][address][0] + ca_delays[channel][address][1])/2;
            printf("CA:%2d: min delay %2d, max delay %2d, center %2d\n",
                address, ca_delays[channel][address][0], ca_delays[channel][address][1], temp);

            ca_final_delay[channel][address] = temp;
            min = min > temp ? temp : min;
            max = max < temp ? temp : max;
        }
#ifdef DDR5_RDIMM
        temp = (par_delays[channel][0] + par_delays[channel][1])/2;
        printf("PAR: min delay %2d, max delay %2d, center %2d\n",
            par_delays[channel][0], par_delays[channel][1], temp);

        par_final_delay[channel] = temp;
        min = min > temp ? temp : min;
        max = max < temp ? temp : max;
#endif // DDR5_RDIMM
    }
    printf("Max center point delay:%2d, min center point delay:%2d, spread:%2d\n", max, min, max-min);

    printf("Adjusting clock delay, so min center point is at delay 0\n");

    newdly = (SDRAM_PHY_DELAYS - min) % SDRAM_PHY_DELAYS;
    printf("New clock delay:%2d\n", newdly);

    ck_rst(0, 0, 0);
    for (cntdly = 0; cntdly < newdly; ++cntdly)
        ck_inc(0, 0, 0);

        // If we ever have multiple ranks, and independent timing for them
    for (channel = 0; channel < CHANNELS; channel++) {
        // Uncomment loop below
        // for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
        {rank = 0;
            cs_final_delay[channel][rank] -= min;
            printf("Rank:%2d center point delay:%2d\n", rank, cs_final_delay[channel][rank]);
            cs_rst(channel, rank, 0);
            for (cntdly = 0; cntdly < cs_final_delay[channel][rank]; ++cntdly)
                cs_inc(channel, rank, 0);

        }
        for (address = 0; address < ca_line_count; address++) {
            ca_final_delay[channel][address] -= min;
            printf("CA:%2d center point delay:%2d\n", address, ca_final_delay[channel][address]);
            ca_rst(channel, 0, address);
            for (cntdly = 0; cntdly < ca_final_delay[channel][address]; ++cntdly)
                ca_inc(channel, 0, address);
        }
#ifdef DDR5_RDIMM
        par_final_delay[channel] -= min;
        printf("PAR center point delay:%2d\n", address, par_final_delay[channel]);
            par_rst(channel, 0, 0);
            for (cntdly = 0; cntdly < pra_final_delay[channel]; ++cntdly)
                par_inc(channel, 0, 0);
#endif // DDR5_RDIMM
    }

    printf("Re-scan CS/CA\n");
        // If we ever have multiple ranks, and independent timing for them
        // Uncomment loop below
        // for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
        {rank = 0;
            printf("Rank:%d\n", rank);
            enter_cs(channel, rank);
            temp = CS_find_offset(channel, rank);
            CS_ck_scan(channel, rank, temp);
            ck_rst(0, 0, 0);
            for (cntdly = 0; cntdly < newdly; ++cntdly)
                ck_inc(0, 0, 0);
            printf("|");
            CS_scan(channel, rank, &temp, &temp);
            printf("\n");
            exit_cs(channel, rank);

            cs_rst(channel, rank, 0);
            for (cntdly = 0; cntdly < cs_final_delay[channel][rank]; ++cntdly)
                cs_inc(channel, rank, 0);

            enter_ca(channel, rank);
            for (address = 0; address < ca_line_count; address++) {
                printf("Address:%2d\n", address);
                CA_ck_scan(channel, rank, address, cs_final_delay[channel][rank]);
                ck_rst(0, 0, 0);
                for (cntdly = 0; cntdly < newdly; ++cntdly)
                    ck_inc(0, 0, 0);
                printf("|");
                CA_scan(channel, rank, address, &temp, &temp);
                printf("\n");
                ca_rst(channel, rank, address);
                for (cntdly = 0; cntdly < ca_final_delay[channel][address]; ++cntdly)
                    ca_inc(channel, rank, address);
            }
            exit_ca(channel, rank);
        }
    }
    max_value = max - min;
}

#if defined(SDRAM_PHY_ADDRESS_DELAY_CAPABLE)
void sdram_ddr5_cs_ca_training(void) {
    int32_t channel, rank;
    uint8_t CS_success, CA_success;
    disable_dfi_2n_mode();

    CS_success = 1;
    CA_success = 1;
    CA_setup_array();
    for (channel = 0; channel < CHANNELS; channel++) {
        printf("Subchannel:%c CS training\n", (char)('A'+channel));
        CS_training(channel, &CS_success);
        printf("CA training\n");
        CA_check_lines(channel);
        CA_training(channel);
        CA_check_values(channel, &CA_success);
    }
    if (!(CS_success & CA_success)) {
        enable_dfi_2n_mode();
    } else {
        CS_CA_best_timings();
        for (channel = 0; channel < CHANNELS; channel++) {
            for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
                disable_dram_2n_mode(channel, rank);
            }
        }
    }
    return;
}
#else
void sdram_ddr5_cs_ca_training(void) {
#ifndef SKIP_NO_DELAYS
    printf("WARNING:\n"
           "PHY does not have io delays on address lines!!!\n"
           "BIOS will try to check if 1N mode is possible,\n"
           "but it may be unstable.\n"
           "Build bios with -DSKIP_NO_DELAYS, to stay in 2N mode.\n");
    int32_t channel, rank;
    uint8_t CS_success, CA_success;
    disable_dfi_2n_mode();

    CS_success = 1;
    CA_success = 1;
    CA_setup_array();
    for (channel = 0; channel < CHANNELS; channel++) {
        printf("Subchannel:%c CS training\n", (char)('A'+channel));
        CS_training(channel, &CS_success);
        printf("CA training\n");
        CA_check_lines(channel);
        CA_training(channel);
        CA_check_values(channel, &CA_success);
    }
    if (!(CS_success & CA_success)) {
        enable_dfi_2n_mode();
    } else {
        CS_CA_best_timings();
        for (channel = 0; channel < CHANNELS; channel++) {
            // If we ever have multiple ranks, and independent timing for them
            // Uncomment loop below
            // for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
            {rank = 0;
                disable_dram_2n_mode(channel, rank);
            }
        }
    }
#else
    printf("CS/CA training impossible\n"
           "Keeping DRAM in 2N mode\n");
#endif
    return;
}
#endif // defined(SDRAM_PHY_ADDRESS_DELAY_CAPABLE)

void sdram_ddr5_module_enumerate(void) {
    int channel, rank, module;
    if (SDRAM_PHY_MODULES/CHANNELS > 15) {
        printf("Too many modules on single rank to enumerate,\n"
               "maximum is 15 but this design has %2d\n", SDRAM_PHY_MODULES);
        enumerated = 0;
        return;
    }
    for (channel = 0; channel < CHANNELS; channel++) {
        printf("Enumerating subchannel:%c\n", (char)('A'+channel));
        // If we ever have multiple ranks, and independent timing for them
        // Uncomment loop below
        // for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
        {rank = 0;
            printf("\tEnumerating rank:%2d\n", rank);
            // Enter PDA Enumerate Programming Mode
            send_mpc(channel, rank, 0xB);
            for (module = 0; module < SDRAM_PHY_MODULES/CHANNELS; module++) {
                setup_enumerate(channel, rank, module);
            }
            // Exit PDA Enumerate Programming Mode
            send_mpc(channel, rank, 0xA);
        }
    }
    enumerated = 1;
}

#ifndef DDR5_TRAINING_SIM
int seeds0[] = {0x1c, 0x5a, 0x24,
#else
int seeds0[] = {
#endif
                0x36, 0xaa, 0xc1};

#ifndef DDR5_TRAINING_SIM
int seeds1[] = {0x59, 0x3c, 0x48,
#else
int seeds1[] = {
#endif
                0x72, 0x55, 0x95};

int seeds_count = sizeof(seeds0)/sizeof(int);

uint16_t serial[] = {0x0000, 0xffff,
                     0xfffe, 0xfffd, 0xfffb, 0xfff7, 0xffef, 0xffdf, 0xffbf, 0xff7f,
                     0xfeff, 0xfdff, 0xfbff, 0xf7ff, 0xefff, 0xdfff, 0xbfff, 0x7fff,
                     0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
                     0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000};
int serial_count = sizeof(serial)/sizeof(uint16_t);

void sdram_ddr5_read_training(void) {
    int channel, rank, module, i, seed;
    int cycle, delay, preamble, got, works;
    int start_cycle, start_delay,   // First working cycle delay pair
        middle_cycle, middle_delay, // Middle between first and last working
        end_cycle, end_delay;       // First cycle delay pair that does not work after working
    uint32_t eye_width;             // In taps
    for (channel = 0; channel < CHANNELS; channel++) {
        printf("Subchannel:%c Read training\n", (char)('A'+channel));

        /* All PHYs so far have support for single delay far all ranks,
           use only first rank, leave all other as inactive*/

        // If we ever have multiple ranks, and independent timing for them
        // Uncomment loop below
        // for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
        {rank = 0;
            /* Setup MRs */
            send_mrw(channel, rank, 0xf, 2, 1|WICA);
            send_mrw(channel, rank, 0xf, 25, 1);
            send_mrw(channel, rank, 0xf, 28, 0xA5);
            send_mrw(channel, rank, 0xf, 29, 0xA5);
            send_mrw(channel, rank, 0xf, 30, 0x33);

            printf("Training rank%2d\n", rank);
            for (module = 0; module < SDRAM_PHY_MODULES/CHANNELS; module++) {
                printf("Training module%2d\n", module);
                start_cycle = -1;
                end_cycle = 100;
                /* Coarse alignment */
                rd_rst(channel, module);
                got = 0;
#ifdef INFO_DDR5
                printf("Preamble\n");
#endif // INFO_DDR5
                for (cycle = 0; cycle < 67 && got < 2; cycle ++) {
#ifdef INFO_DDR5
                    printf("%2d|", cycle);
#endif // INFO_DDR5
                    idly_rst(channel, module);
                    for (delay = 0; delay < SDRAM_PHY_DELAYS; delay++) {
                        send_mrr(channel, rank, 31);
                        preamble = captured_preamble(channel, module);
#ifdef INFO_DDR5
                        printf("%01x", preamble);
#endif // INFO_DDR5
                        if (preamble == 4 && got == 0) {
                            start_cycle = cycle;
                            got = 1;
                        } else if (preamble != 4 && got == 1) {
                            end_cycle = cycle;
                            got = 2;
                        }
                        idly_inc(channel, module);
                    }
#ifdef INFO_DDR5
                    printf("\n");
#endif // INFO_DDR5
                    rd_inc(channel, module);
                }
                if (start_cycle == -1) {
                    printf("Failed to find result for %2d\n", module);
                    continue;
                }
                printf("Preamble starts in cycle:%2d\n", start_cycle);
                /* Pull back 1 cycle */
                start_cycle -= 1;
                rd_rst(channel, module);
                idly_rst(channel, module);
                for (i = 0; i < start_cycle; ++i) {
                    rd_inc(channel, module);
                }
                got = 0;
                cycle = start_cycle;
                start_cycle = -1; start_delay = 1;
                end_cycle = -1; end_delay = -1;
                printf("Data scan:\n");
                while (got != 2 && cycle < 67) {
                    printf("%2d|", cycle);
#ifdef DEBUG_DDR5
                    printf("\n");
#endif // DEBUG_DDR5
                    idly_rst(channel, module);
                    for(delay = 0; delay < SDRAM_PHY_DELAYS; ++delay){
#ifdef DEBUG_DDR5
                        printf("DQ dly:%"PRIu16"\n",
                               get_rd_dq_dly(channel, module));
#endif // DEBUG_DDR5
                        works = 1;
#ifndef DDR5_TRAINING_SIM
                        for (seed = 0; seed < serial_count && works; ++seed){
                            /* Setup MRs */
                            send_mrw(channel, rank, 0xf, 25, 0);
                            send_mrw(channel, rank, module, 26, serial[seed]&0xff);
                            send_mrw(channel, rank, module, 27, serial[seed]>>8);
                            send_mrr(channel, rank, 31);
                            works &= compare_serial(channel, module, serial[seed],
                                                    0xA5, 0x33);
                        }
#endif // DDR5_TRAINING_SIM
                        for (seed = 0; seed < seeds_count && works; ++seed){
                            /* Setup MRs */
                            send_mrw(channel, rank, 0xf, 25, 1);
                            send_mrw(channel, rank, module, 26, seeds0[seed]);
                            send_mrw(channel, rank, module, 27, seeds1[seed]);
                            send_mrr(channel, rank, 31);
                            works &= compare(channel, module,
                                             seeds0[seed], seeds1[seed],
                                             0xA5, 0x33);
                        }
                        printf("%d", works);
                        if (works && got == 0) {
                            start_cycle = cycle;
                            start_delay = delay;
                            got = 1;
                        } else if (!works && got == 1) {
                            end_cycle = cycle;
                            end_delay = delay;
                            got = 2;
                        }
                        idly_inc(channel, module);
#ifdef DEBUG_DDR5
                        printf("\n");
#endif // DEBUG_DDR5
                    }
                    printf("|\n");
                    ++cycle;
                    rd_inc(channel, module);
                }
                printf("m%2d|start cycle:%2d, delay:%2d; end cycle:%2d, delay:%2d|",
                    module, start_cycle, start_delay, end_cycle, end_delay);
                eye_width = (end_cycle-start_cycle)*SDRAM_PHY_DELAYS + end_delay - start_delay;
                middle_cycle = start_cycle + (start_delay + eye_width/2)/SDRAM_PHY_DELAYS;
                middle_delay = (start_delay + eye_width/2)%SDRAM_PHY_DELAYS;
                printf("eye_width:%2"PRIu32"; eye center: cycle:%2d,delay:%2d\n",
                    eye_width, middle_cycle, middle_delay);

                // Setting read delay to eye center
                rd_rst(channel, module);
                idly_rst(channel, module);
                for (i = 0; i < middle_cycle; ++i) {
                    rd_inc(channel, module);
                }
                for (i = 0; i < middle_delay; ++i) {
                    idly_inc(channel, module);
                }

                send_mrw(channel, rank, module, 25, 0);
                send_mrw(channel, rank, module, 26, 0xff);
                send_mrw(channel, rank, module, 27, 0xff);
                send_mrw(channel, rank, module, 28, 0);
                send_mrw(channel, rank, module, 29, 0);
            }
            /* Finish preamble and read training*/
            send_mrw(channel, rank, 0xf, 2, 0|WICA);
        }

        printf("Simple read check\n");
        // If we ever have multiple ranks, and independent timing for them
        // Uncomment loop below
        // for(rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
        {rank =0;
            for (module = 0; module < SDRAM_PHY_MODULES/CHANNELS; module++) {
                printf("Channel:%c rank:%2d module:%2d serial number:", (char)('A'+channel), rank, module);
                for (i = 0; i < 5; ++i) {
                    send_mrr(channel, rank, 65+i);
                    printf("%02"PRIX8, recover_mrr_value(channel, module));
                }
                printf("\n");
#ifdef INFO_DDR5
                // Check if data is read correctly
                send_mrw(channel, rank, module, 63, 0xDE);
                send_mrr(channel, rank, 63);
                printf("%"PRIX8, recover_mrr_value(channel, module));
                send_mrw(channel, rank, module, 63, 0xAD);
                send_mrr(channel, rank, 63);
                printf("%"PRIX8, recover_mrr_value(channel, module));
                send_mrw(channel, rank, module, 63, 0xBE);
                send_mrr(channel, rank, 63);
                printf("%"PRIX8, recover_mrr_value(channel, module));
                send_mrw(channel, rank, module, 63, 0xEF);
                send_mrr(channel, rank, 63);
                printf("%"PRIX8"\n", recover_mrr_value(channel, module));
#endif //INFO_DDR5
            }
#ifdef INFO_DDR5
            // Check if registers are correct
            for (module = 0; module < SDRAM_PHY_MODULES/CHANNELS; module++) {
                printf("Channel:%c rank:%d module:%d\n", (char)('A'+channel), rank, module);
                read_registers(channel, rank, module);
            }
#endif // INFO_DDR5
        }
    }
}

void sdram_ddr5_write_training(void) {
    int channel, rank, module, seed, cnt_seed, byte;
    int cycle, delay, got, sample, it, works;
    int start_cycle, start_delay,   // First working cycle delay pair
        middle_cycle, middle_delay, // Middle between first and last working
        end_cycle, end_delay;       // First cycle delay pair that does not work after working
    uint8_t lfsr, temp;
    uint8_t mr5;
    uint16_t wrdata, rddata;
    uint32_t eye_width;             // In taps
    WICA = 1<<7;
    for (channel = 0; channel < CHANNELS; channel++) {
        printf("Subchannel:%c Write leveling\n", (char)('A'+channel));
        /* Coarse alignment */
        // If we ever have multiple ranks, and independent timing for them
        // Uncomment loop below
        // for(rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
        {rank = 0;
            enter_write_leveling(channel);
            /* Setup MRs */
            send_mrw(channel, rank, 0xf, 2, 2);
            for (module = 0; module < SDRAM_PHY_MODULES/CHANNELS; module++) {
                printf("WL m:%2d\n", module);
                start_cycle = -1;
                wr_dqs_rst(channel, module);
                cycle = 0;
                for(delay = SDRAM_PHY_MIN_WR_LATENCY; delay < SDRAM_PHY_CWL/2; ++delay, ++cycle) {
                    wr_dqs_inc(channel, module);
                }
                got = 0;
                for (; cycle < 63 && got < 1; ++cycle) {
                    sample = 1;
                    // Check multiple times, as we can be on the edge of transition
                    // Make sure we aren't in meta stable delay
                    printf("%2d|", cycle);
                    for (it = 0; it<16; it++) {
                        send_wleveling_write(channel, rank);
                        temp = wleveling_sample(channel, module);
                        sample &= temp;
#ifdef DEBUG_DDR5
                        printf("%d:%d|", sample, temp);
#else
                        printf("%d", sample);
#endif // DEBUG_DDR5
                    }
                    printf("|\n");
                    if (sample && got == 0) {
                        start_cycle = cycle;
                        got = 1;
                    }
                    wr_dqs_inc(channel, module);
                }
                if (start_cycle == -1) {
                    printf("Failed to find result for %2d\n", module);
                    continue;
                }
                wr_dqs_rst(channel, module);
                printf("DQS write leveling starts in cycle:%2d (adjusted %2d)\n",
                    start_cycle, start_cycle + SDRAM_PHY_MIN_WR_LATENCY);
                /* Pull back 1 cycle */
                start_cycle -= 1;
                wr_dqs_rst(channel, module);
                odly_dqs_rst(channel, module);
                for (it = 0; it < start_cycle; ++it) {
                    wr_dqs_inc(channel, module);
                }

                got = 0;
                cycle = start_cycle;
                start_cycle = -1; start_delay = 1;
                printf("DQS edge scan:\n");
                // Break out when 0 to 1 transition was found
                do {
                    wleveling_scan(&cycle, &got, &start_cycle, &start_delay, channel, rank, module);
                } while (got != 1);

#ifdef INFO_DDR5
                printf("cycle:%2d delay:%2d\n", start_cycle, start_delay);
#endif // INFO_DDR5

                // Pull back 0.75 clock as specified by JEDEC
                // to train WICA
                start_cycle -= 1;
                start_delay += SDRAM_PHY_DELAYS/4;
                if (start_delay >= SDRAM_PHY_DELAYS) {
                    start_cycle += 1;
                    start_delay -= SDRAM_PHY_DELAYS;
                }

#ifdef INFO_DDR5
                printf("After -0.75; cycle:%2d delay:%2d\n", start_cycle, start_delay);
#endif // INFO_DDR5

                wr_dqs_rst(channel, module);
                odly_dqs_rst(channel, module);
                for (it = 0; it < start_cycle; ++it) {
                    wr_dqs_inc(channel, module);
                }
                for (it = 0; it < start_delay; ++it) {
                    odly_dqs_inc(channel, module);
                }

                printf("DQS internal cycle alignment\n|");
                send_mrw(channel, rank, module, 2, 2|WICA);
                got = 0;
                cycle = 0;
                do {
                    send_mrw(channel, rank, module, 3, cycle);
                    sample = 1;
                    // Check multiple times, as we can be on the edge of transition
                    // Make sure we aren't in meta stable delay
                    for (it = 0; it<16; it++) {
                        send_wleveling_write(channel, rank);
                        sample &= wleveling_sample(channel, module);
                    }
                    printf("WICA:%d,%d|", cycle, sample);
                    ++cycle;
                    if (sample && got == 0) {
                        got = 1;
                    }
                } while (got != 1 && cycle < 7 );
                printf("\n");

                start_cycle -= 1;

                wr_dqs_rst(channel, module);
                odly_dqs_rst(channel, module);
                for (it = 0; it < start_cycle; ++it) {
                    wr_dqs_inc(channel, module);
                }

                got = 0;
                cycle = start_cycle;
                start_cycle = -1; start_delay = -1;
                printf("Scan for internal edge:\n");
                // Break out when 0 to 1 transition was found
                do {
                    wleveling_scan(&cycle, &got, &start_cycle, &start_delay, channel, rank, module);
                } while (got != 1);

                // Push forward 1.25 clock as specified by JEDEC
                // after training WICA

                start_cycle += 1;
                start_delay += SDRAM_PHY_DELAYS/4;
                if (start_delay >= SDRAM_PHY_DELAYS) {
                    start_cycle += 1;
                    start_delay -= SDRAM_PHY_DELAYS;
                }

                printf("Final timing values: cycles:%2d(adjusted %2d) delay:%2d\n",
                    start_cycle, start_cycle + SDRAM_PHY_MIN_WR_LATENCY, start_delay);

                wr_dqs_rst(channel, module);
                odly_dqs_rst(channel, module);
                for (it = 0; it < start_cycle; ++it) {
                    wr_dqs_inc(channel, module);
                }
                for (it = 0; it < start_delay; ++it) {
                    odly_dqs_inc(channel, module);
                }
            }
            send_mrw(channel, rank, 0xf, 2, 0|WICA);
            exit_write_leveling(channel);
#ifdef DEBUG_DDR5
            for (module = 0; module < SDRAM_PHY_MODULES/CHANNELS; module++) {
                read_registers(channel, rank, module);
            }
#endif // DEBUG_DDR5
            printf("DQ write training\n");
            mr5 = 0;
            for (module = 0; module < SDRAM_PHY_MODULES/CHANNELS; module++) {
                send_mrr(channel, rank, 5);
                mr5 = recover_mrr_value(channel, module);
                printf("mr5:%02"PRIx8"\n", mr5);
                send_mrw(channel, rank, module, 5, mr5 & 0xDF); // Disable DM

                wr_dq_rst(channel, module);
                printf("m%2d|\n", module);
                cycle = 0;
                got = 0;
                start_cycle = -1; start_delay = 1;
                end_cycle = -1; end_delay = -1;
                printf("Data scan:\n");
                while (got != 2 && cycle < 65) {
                    printf("%2d|", cycle);
#ifdef DEBUG_DDR5
                    printf("\n");
#endif // DEBUG_DDR5
                    odly_dq_rst(channel, module);
                    for(delay = 0; delay < SDRAM_PHY_DELAYS; ++delay){
#ifdef DEBUG_DDR5
                        printf("DQ dly:%"PRIu16"\n", get_wr_dq_dly(channel, module));
#endif // DEBUG_DDR5
                        works = 1;
#ifndef DDR5_TRAINING_SIM
                        for (cnt_seed = 0; cnt_seed < serial_count && works; ++cnt_seed) {
                            for (it =0; it <8; ++it) {
                                wrdata = 0;
                                for (temp = 0; temp < SDRAM_PHY_DQ_DQS_RATIO; ++temp) {
                                    wrdata |= ((serial[cnt_seed]>>(2*it))&1) << temp;
                                }
                                for (temp = 0; temp < SDRAM_PHY_DQ_DQS_RATIO; ++temp) {
                                    wrdata |= ((serial[cnt_seed]>>(2*it+1))&1) << (temp + SDRAM_PHY_DQ_DQS_RATIO);
                                }
#ifdef DEBUG_DDR5
                                printf("wrdata:%04"PRIx16"|", wrdata);
#endif // DEBUG_DDR5
                                set_data_module_phase(channel, module, it, wrdata);
                            }
#ifdef DEBUG_DDR5
                            printf("\n");
#endif // DEBUG_DDR5
                            send_write(channel, rank);
                            send_read(channel, rank);

                            for (it =0; it <8; ++it) {
                                rddata = get_data_module_phase(channel, module, it);
#ifdef DEBUG_DDR5
                                printf("rddata:%04"PRIx16"|", rddata);
#endif // DEBUG_DDR5
                                for (temp = 0; temp < SDRAM_PHY_DQ_DQS_RATIO; ++temp) {
                                    works &= !!(((rddata>>temp)&1) == ((serial[cnt_seed]>>(2*it))&1));
                                }
                                for (temp = 0; temp < SDRAM_PHY_DQ_DQS_RATIO; ++temp) {
                                    works &= !!(((rddata>>(temp + SDRAM_PHY_DQ_DQS_RATIO))&1) == ((serial[cnt_seed]>>(2*it+1))&1));
                                }
                            }
                            for (it =0; it <8; ++it) {
                                set_data_module_phase(channel, module, it, 0);
                            }
                            send_write(channel, rank);
#ifdef DEBUG_DDR5
                            printf("\n");
#endif // DEBUG_DDR5
                        }
#endif // DDR5_TRAINING_SIM
                        for (cnt_seed = 0; cnt_seed < seeds_count * 2 && works; ++cnt_seed) {
                            if(cnt_seed < seeds_count)
                                seed = seeds0[cnt_seed];
                            else
                                seed = seeds1[cnt_seed - seeds_count];

                            lfsr = seed;
                            for (it =0; it <8; ++it) {
                                wrdata = lfsr;
                                lfsr = lfsr_next(lfsr);
                                if (BYTES_PER_MODULE > 1) {
                                    wrdata |= lfsr << 8;
                                    lfsr = lfsr_next(lfsr);
                                }
#ifdef DEBUG_DDR5
                                printf("wrdata:%04"PRIx16"|", wrdata);
#endif // DEBUG_DDR5
                                set_data_module_phase(channel, module, it, wrdata);
                            }
#ifdef DEBUG_DDR5
                            printf("\n");
#endif // DEBUG_DDR5
                            send_write(channel, rank);
                            send_read(channel, rank);

                            lfsr = seed;
                            for (it =0; it <8; ++it) {
                                rddata = get_data_module_phase(channel, module, it);
#ifdef DEBUG_DDR5
                                printf("rddata:%04"PRIx16"|", rddata);
#endif // DEBUG_DDR5
                                works &= ((rddata&0xff) == lfsr);
                                lfsr = lfsr_next(lfsr);
                                if (BYTES_PER_MODULE > 1) {
                                    rddata >>= 8;
                                    works &= (rddata == lfsr);
                                    lfsr = lfsr_next(lfsr);
                                }
                            }
                            for (it =0; it <8; ++it) {
                                set_data_module_phase(channel, module, it, 0);
                            }
                            send_write(channel, rank);
#ifdef DEBUG_DDR5
                            printf("\n");
#endif // DEBUG_DDR5
                        }
                        printf("%d", works);
#ifdef DEBUG_DDR5
                        printf("\n");
#endif // DEBUG_DDR5
                        if (works && got == 0) {
                            start_cycle = cycle;
                            start_delay = delay;
                            got = 1;
                        } else if (!works && got == 1) {
                            end_cycle = cycle;
                            end_delay = delay;
                            got = 2;
                        }
                        odly_dq_inc(channel, module);
                    }
                    printf("|\n");
                    ++cycle;
                    wr_dq_inc(channel, module);
                }
                printf("m%2d|start cycle:%2d, delay:%2d; end cycle:%2d, delay:%2d|",
                    module, start_cycle, start_delay, end_cycle, end_delay);
                eye_width = (end_cycle-start_cycle)*SDRAM_PHY_DELAYS + end_delay - start_delay;
                middle_cycle = start_cycle + (start_delay + eye_width/2)/SDRAM_PHY_DELAYS;
                middle_delay = (start_delay + eye_width/2)%SDRAM_PHY_DELAYS;
                printf("eye_width:%2"PRIu32"; eye center: cycle:%2d,delay:%2d\n",
                    eye_width, middle_cycle, middle_delay);

                // Setting read delay to eye center
                wr_dq_rst(channel, module);
                odly_dq_rst(channel, module);
                for (it = 0; it < middle_cycle; ++it) {
                    wr_dq_inc(channel, module);
                }
                for (it = 0; it < middle_delay; ++it) {
                    odly_dq_inc(channel, module);
                }

                // DM training
                printf("%x\n", mr5);
                if (mr5 & 0x20) { // DM was enabled
                    printf("DM scan\nm:%2d DM|", module);
                    odly_dm_rst(channel, module);
                    got = 0;
                    start_delay = 1; end_delay = -1;
                    for(delay = 0; delay < SDRAM_PHY_DELAYS; ++delay){
#ifdef DEBUG_DDR5
                        printf("DM dly:%"PRIu16"\n", get_wr_dm_dly(channel, module));
#endif // DEBUG_DDR5
                        works = 1;
                        for (byte = 0; byte < 16 && works; ++byte) {
#ifndef DDR5_TRAINING_SIM
                            for (cnt_seed = 0; cnt_seed < seeds_count * 2 && works; ++cnt_seed) {
#else
                            {cnt_seed = 0;
#endif // DDR5_TRAINING_SIM
                                send_mrw(channel, rank, module, 5, mr5 & 0xDF); // Disable DM
                                if(cnt_seed < seeds_count)
                                    seed = seeds0[cnt_seed];
                                else
                                    seed = seeds1[cnt_seed - seeds_count];

                                for (it =0; it <8; ++it) {
                                    set_data_module_phase(channel, module, it, 0);
                                }
                                send_write(channel, rank);

                                send_mrw(channel, rank, module, 5, mr5); // Enable DM
                                lfsr = seed;
                                for (it =0; it <8; ++it) {
                                    wrdata = lfsr;
                                    lfsr = lfsr_next(lfsr);
                                    if (BYTES_PER_MODULE > 1) {
                                        wrdata |= lfsr << 8;
                                        lfsr = lfsr_next(lfsr);
                                    }
#ifdef DEBUG_DDR5
                                    printf("wrdata:%04"PRIx16"|", wrdata);
#endif // DEBUG_DDR5
                                    set_data_module_phase(channel, module, it, wrdata);
                                }
#ifdef DEBUG_DDR5
                                printf("\n");
#endif // DEBUG_DDR5
                                send_write_byte(channel, rank, module, byte);
                                send_read(channel, rank);

                                lfsr = seed;
                                for (it =0; it <8; ++it) {
                                    rddata = get_data_module_phase(channel, module, it);
#ifdef DEBUG_DDR5
                                    printf("rddata:%04"PRIx16"|", rddata);
#endif // DEBUG_DDR5
                                    if ((byte >> 1) == it) {
                                        if(!(byte & 1)) {
                                            works &= ((rddata&0xff) == lfsr);
                                        }
                                        lfsr = lfsr_next(lfsr);
                                        rddata >>= 8;
                                        if(byte & 1) {
                                            works &= ((rddata&0xff) == lfsr);
                                        }
                                        lfsr = lfsr_next(lfsr);
                                    } else {
                                        lfsr = lfsr_next(lfsr);
                                        lfsr = lfsr_next(lfsr);
                                    }
                                }
                                for (it =0; it <8; ++it) {
                                    set_data_module_phase(channel, module, it, 0);
                                }
                                send_write(channel, rank);
#ifdef DEBUG_DDR5
                                printf("\n");
#endif // DEBUG_DDR5
                            }
                        }
                        printf("%d", works);
#ifdef DEBUG_DDR5
                        printf("\n");
#endif // DEBUG_DDR5
                        if (works && got == 0) {
                            start_delay = delay;
                            got = 1;
                        }
                        if (!works && got == 1) {
                            end_delay = delay;
                            got = 2;
                        } else if (delay == SDRAM_PHY_DELAYS-1 && got == 1) {
                            end_delay = delay + 1;
                            got = 2;
                        }
                        odly_dm_inc(channel, module);
                    }
                    printf("|\n");
                }
                printf("m%2d|DM start delay:%2d; delay:%2d|",
                    module, start_delay, end_delay);
                eye_width = end_delay - start_delay;
                middle_delay = (start_delay + eye_width/2)%SDRAM_PHY_DELAYS;
                printf("eye_width:%2"PRIu32"; eye center: delay:%2d\n",
                    eye_width, middle_delay);

                // Setting read delay to eye center
                odly_dm_rst(channel, module);
                for (it = 0; it < middle_delay; ++it) {
                    odly_dm_inc(channel, module);
                }
            }
        }
    }
}

int in_2n_mode(void) {
    return N2_mode;
}

#endif // MEMORY_TYPE_DDR5
#endif // LIBLITEDRAM_DDR5_TRAINING_H
