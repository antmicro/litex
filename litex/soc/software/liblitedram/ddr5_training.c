#include <liblitedram/ddr5_training.h>
#ifdef LIBLITEDRAM_DDR5_TRAINING_H
#include <liblitedram/ddr5_helpers.h>
#include <stdio.h>
#include <inttypes.h>

#ifdef MEMORY_TYPE_DDR5
//#define DEBUG_DDR5

#ifndef SDRAM_PHY_ADDRESS_LINES
#define SDRAM_PHY_ADDRESS_LINES 13
#endif

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

int ca_delays[2][14][2];
int par_delays[2][2];
//If per rank timings are available, the array above should be [SDRAM_PHY_RANKS][2][14][2]
//To cover clock/ca delays per rank
int cs_final_delay[2][SDRAM_PHY_RANKS];

int ca_final_delay[2][14];
int par_final_delay[2];

int WICA = 0;

static void CS_training(int32_t channel, uint8_t *success) {
    int32_t ckdly, csdly, coarse;
    int32_t rank, works;
    int32_t on_edge, _result, offset;
    // If we ever have multiple ranks, and independent timing for them
    // Uncomment loop below
    // for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
    {rank = 0;
        printf("Rank: %"PRId32"", rank);

        // Enter CS training MPC
        enter_cs(channel, rank);

        // Scan clock delays only if one of patterns work (0x55 or 0xAA)
        // If neither works then we are in meta state, both clock and CS change
        // to close to each other. In such situations we only check CS delays

        // Get working pattern
        on_edge = 0;
        for (offset = 0; offset < 2; offset++) {
            cs_sample_prep(channel, rank, 0, offset);
            _result =  or_sample(channel);
            if (_result)
                on_edge = offset<<1|1;
        }

        cs_delays[channel][rank][0] = UNSET_DELAY;
        cs_delays[channel][rank][1] = UNSET_DELAY;

        // We got one of the patterns to work flawlessly, check clock delays
        if (on_edge) {
            printf("|");
            offset = on_edge >> 1;
            works = 1;
            // loop over all delays, prevent too sudden clock change
            for(ckdly = 0; ckdly < SDRAM_PHY_DELAYS; ckdly++) {
                cs_sample_prep(channel, rank, 0, offset);
                _result = or_sample(channel);
                printf("%d", !!_result);
                if (!_result && works) {
                    works = 0;
                    cs_delays[channel][rank][0] = -(ckdly - 1);
                } else if (_result && works && ckdly == SDRAM_PHY_DELAYS - 1) {
                    cs_delays[channel][rank][0] = -ckdly;
                }
                ck_inc(channel, rank, 0);
            }
        }
        // After delaying clock, pattern could have changes, as we missed one clock cycle
        printf("|");

        works = -1;
        for (csdly = 0; csdly < SDRAM_PHY_DELAYS; csdly++) {
            if (works == -1) {
                for (offset = 0; offset < 2; offset++) {
                    cs_sample_prep(channel, rank, 0, offset);
                    _result =  or_sample(channel);
                    // Found working pattern
                    if (_result) {
                        works = offset;
                        break;
                    }
                }
            } else {
                cs_sample_prep(channel, rank, 0, works);
                _result =  or_sample(channel);
            }
            printf("%d", !!_result);
            if (_result && cs_delays[channel][rank][0] == UNSET_DELAY)
                cs_delays[channel][rank][0] = csdly;
            if ((!_result || csdly == SDRAM_PHY_DELAYS - 1) && \
                    cs_delays[channel][rank][0] != UNSET_DELAY && \
                    cs_delays[channel][rank][1] == UNSET_DELAY)
                cs_delays[channel][rank][1] = csdly;
            cs_inc(channel, rank, 0);
        }
        printf("|\n");

        cs_rst(channel, rank, 0);

        // Set up coarse delay adjustment until we get CA results
        printf("Rank delays: %d:%d\n", cs_delays[channel][rank][0], cs_delays[channel][rank][1]);
        coarse = (cs_delays[channel][rank][0]+cs_delays[channel][rank][1]) / 2;
        coarse = coarse < 0 ? 0 : coarse;
        printf("Coarse adjustment:%"PRId32"\n", coarse);

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
    if (and_sample(channel))
        ca_line_count = 14;
    else
        ca_line_count = 13;
    cmd_injector(channel, 0xf, 0, 0x1f, 0, 0, 0, 0);
    exit_ca(channel, 0);
    printf("DDR5 module has %d address lines\n", ca_line_count);
}

static void CA_training(int32_t channel) {
    int32_t left_side, right_side;
    int32_t rank, address;
    int32_t on_edge, works, _result;
    int32_t ckdly, cadly;

    // If we ever have multiple ranks, and independent timing for them
    // Uncomment loop below
    // for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
    {rank = 0;
        printf("Rank:%02"PRId32"\n", rank);
        // Enter CA training MPC
        enter_ca(channel, rank);

        for (address = 0; address < ca_line_count; address++) {
            // Reset CA delay
            ca_rst(channel, rank, address);

            // Check if address line is correct with 0 tap.
            on_edge = 0;
            ca_sample_prep_current_period(channel, rank, address, 1);
            if (and_sample(channel))
                on_edge = 1;
            ca_sample_prep_current_period(channel, rank, address, 0);
            if (or_sample(channel))
                on_edge &= 1;
            printf("CA line:%"PRId32"", address);

            left_side = UNSET_DELAY; right_side = UNSET_DELAY;
            // Address line works with no delays no clock or ca delay
            if (on_edge) {
                printf("|");
                works = 1;
                // loop over all delays, prevent too sudden clock change
                for(ckdly = 0; ckdly < SDRAM_PHY_DELAYS; ckdly++) {
                    ca_sample_prep_current_period(channel, rank, address, 1);
                    _result = and_sample(channel);
                    ca_sample_prep_current_period(channel, rank, address, 0);
                    _result &= or_sample(channel);
                    printf("%d", !!_result);
                    if (!_result && works) {
                        works = 0;
                        right_side = -(ckdly - 1);
                    } else if(_result && works && ckdly == SDRAM_PHY_DELAYS - 1) {
                        right_side = -ckdly;
                    }
                    ck_inc(channel, rank, 0);
                }
            }
            printf("|");
            for (cadly = 0; cadly < SDRAM_PHY_DELAYS; cadly++) {
                ca_sample_prep_current_period(channel, rank, address, 1);
                _result = and_sample(channel);
                ca_sample_prep_current_period(channel, rank, address, 0);
                _result &= or_sample(channel);
                printf("%d", !!_result);
                if (_result && right_side == UNSET_DELAY)
                    right_side = cadly;
                if ((!_result || cadly == SDRAM_PHY_DELAYS - 1) && \
                        right_side != UNSET_DELAY && \
                        left_side == UNSET_DELAY)
                    left_side = cadly;
                ca_inc(channel, rank, address);
            }
            printf("|\n");

            ca_rst(channel, rank, address);

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
        cmd_injector(channel, 0xf, 0, 0x1f, 0, 0, 0, 0);
        exit_ca(channel, rank);
    }
}

static void CA_check_values(int32_t channel, uint8_t *success) {
    int32_t address;
    for (address = 0; address < ca_line_count; address++) {
        if (ca_delays[channel][address][1] == UNSET_DELAY || ca_delays[channel][address][0] == UNSET_DELAY) {
            printf("CA:%02"PRId32" Eye width:0 Failed\n", address);
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

#ifdef SDRAM_PHY_SUBCHANNELS
    for (channel = 0; channel < 2; channel++) {
        printf("Subchannel:%c Timings\n", 'A'+channel);
#else
    {channel = 0;
        printf("Timings\n");
#endif // SDRAM_PHY_SUBCHANNELS
        // If we ever have multiple ranks, and independent timing for them
        // Uncomment loop below
        // for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
        {rank = 0;
            temp = (cs_delays[channel][rank][0] + cs_delays[channel][rank][1])/2;
            printf("Rank:%d: min delay %d, max delay %d, center %d\n",
                rank, cs_delays[channel][rank][0], cs_delays[channel][rank][1], temp);

            cs_final_delay[channel][rank] = temp;
            min = min > temp ? temp : min;
            max = max < temp ? temp : max;
        }
        for (address = 0; address < ca_line_count; address++) {
            temp = (ca_delays[channel][address][0] + ca_delays[channel][address][1])/2;
            printf("CA:%d: min delay %d, max delay %d, center %d\n",
                address, ca_delays[channel][address][0], ca_delays[channel][address][1], temp);

            ca_final_delay[channel][address] = temp;
            min = min > temp ? temp : min;
            max = max < temp ? temp : max;
        }
#ifdef DDR5_RDIMM
        temp = (par_delays[channel][0] + par_delays[channel][1])/2;
        printf("PAR: min delay %d, max delay %d, center %d\n",
            par_delays[channel][0], par_delays[channel][1], temp);

        par_final_delay[channel] = temp;
        min = min > temp ? temp : min;
        max = max < temp ? temp : max;
#endif // DDR5_RDIMM
    }
    printf("Max center point delay:%d, min center point delay:%d, spread:%d\n", max, min, max-min);

    printf("Adjusting clock delay, so min center point is at delay 0\n");
    newdly = (SDRAM_PHY_DELAYS - min) % SDRAM_PHY_DELAYS;
    printf("New clock delay:%d\n", newdly);

    ck_rst(0, 0, 0);
    for (cntdly = 0; cntdly < newdly; ++cntdly)
        ck_inc(0, 0, 0);

#ifdef SDRAM_PHY_SUBCHANNELS
    for (channel = 0; channel < 2; channel++) {
        printf("Subchannel:%c Adjusted Timings\n", 'A'+channel);
#else
    {channel = 0;
        printf("Adjusted Timings\n");
#endif // SDRAM_PHY_SUBCHANNELS
        // If we ever have multiple ranks, and independent timing for them
        // Uncomment loop below
        // for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
        {rank = 0;
            cs_final_delay[channel][rank] -= min;
            printf("Rank:%d center point delay:%d\n", rank, cs_final_delay[channel][rank]);
            cs_rst(channel, rank, 0);
            for (cntdly = 0; cntdly < cs_final_delay[channel][rank]; ++cntdly)
                cs_inc(channel, rank, 0);

        }
        for (address = 0; address < ca_line_count; address++) {
            ca_final_delay[channel][address] -= min;
            printf("CA:%d center point delay:%d\n", address, ca_final_delay[channel][address]);
            ca_rst(channel, 0, address);
            for (cntdly = 0; cntdly < ca_final_delay[channel][address]; ++cntdly)
                ca_inc(channel, 0, address);
        }
#ifdef DDR5_RDIMM
        par_final_delay[channel] -= min;
        printf("PAR center point delay:%d\n", address, par_final_delay[channel]);
            par_rst(channel, 0, 0);
            for (cntdly = 0; cntdly < pra_final_delay[channel]; ++cntdly)
                par_inc(channel, 0, 0);
#endif // DDR5_RDIMM
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
#ifdef SDRAM_PHY_SUBCHANNELS
    for (channel = 0; channel < 2; channel++) {
        printf("Subchannel:%c CS training\n", (char)('A'+channel));
#else
    {channel = 0;
        printf("CS training\n");
#endif // SDRAM_PHY_SUBCHANNELS
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
#ifdef SDRAM_PHY_SUBCHANNELS
        for (channel = 0; channel < 2; channel++) {
#else
        {channel = 0;
#endif // SDRAM_PHY_SUBCHANNELS
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
#ifdef SDRAM_PHY_SUBCHANNELS
    for (channel = 0; channel < 2; channel++) {
        printf("Subchannel:%c CS training\n", (char)('A'+channel));
#else
    {channel = 0;
        printf("CS training\n");
#endif // SDRAM_PHY_SUBCHANNELS
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
#ifdef SDRAM_PHY_SUBCHANNELS
        for (channel = 0; channel < 2; channel++) {
#else
        {channel = 0;
#endif // SDRAM_PHY_SUBCHANNELS
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
#ifdef SDRAM_PHY_SUBCHANNELS
    if (SDRAM_PHY_MODULES/2 > 15) {
#else
    if (SDRAM_PHY_MODULES > 15) {
#endif // SDRAM_PHY_SUBCHANNELS
        printf("Too many modules on single rank to enumerate,\n"
               "maximum is 15 but this design has %d\n", SDRAM_PHY_MODULES);
        enumerated = 0;
        return;
    }
#ifdef SDRAM_PHY_SUBCHANNELS
    for (channel = 0; channel < 2; channel++) {
        printf("Enumerating subchannel:%c\n", (char)('A'+channel));
#else
    {channel = 0;
        printf("Enumerating\n");
#endif // SDRAM_PHY_SUBCHANNELS
        // If we ever have multiple ranks, and independent timing for them
        // Uncomment loop below
        // for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
        {rank = 0;
            printf("\tEnumerating rank:%d\n", rank);
            // Enter PDA Enumerate Programming Mode
            send_mpc(channel, rank, 0xB);
#ifdef SDRAM_PHY_SUBCHANNELS
            for (module = 0; module < SDRAM_PHY_MODULES/2; module++) {
#else
            for (module = 0; module < SDRAM_PHY_MODULES; module++) {
#endif // SDRAM_PHY_SUBCHANNELS
                setup_enumerate(channel, rank, module);
            }
            // Exit PDA Enumerate Programming Mode
            send_mpc(channel, rank, 0xA);
        }
    }
    enumerated = 1;
}

int seeds0[] = {0x1c, 0x5a, 0x24, 0x36};
int seeds1[] = {0x59, 0x3c, 0x48, 0x72};

int seeds_count = sizeof(seeds0)/sizeof(int);

void sdram_ddr5_read_training(void) {
    int channel, rank, module, i, seed;
    int cycle, delay, preamble, got, works;
    int start_cycle, start_delay,   // First working cycle delay pair
        middle_cycle, middle_delay, // Middle between first and last working
        end_cycle, end_delay;       // First cycle delay pair that does not work after working
    uint32_t eye_width;             // In taps
#ifdef SDRAM_PHY_SUBCHANNELS
    for (channel = 0; channel < 2; channel++) {
        printf("Subchannel:%c Read training\n", (char)('A'+channel));
#else
    {channel = 0;
        printf("Read training\n");
#endif // SDRAM_PHY_SUBCHANNELS
        setup_rddata_cnt(channel, 8);

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

            printf("Training rank%d\n", rank);
#ifdef SDRAM_PHY_SUBCHANNELS
            for (module = 0; module < SDRAM_PHY_MODULES/2; module++) {
#else
            for (module = 0; module < SDRAM_PHY_MODULES; module++) {
#endif // SDRAM_PHY_SUBCHANNELS
                printf("Training module%d\n", module);
                start_cycle = -1;
                end_cycle = 100;
                /* Coarse alignment */
                rd_rst(channel, module);
                got = 0;
                for (cycle = 0; cycle < 67 && got < 2; cycle ++) {
                    idly_rst(channel, module);
                    for (delay = 0; delay < SDRAM_PHY_DELAYS; delay++) {
                        send_mrr(channel, rank, 31);
                        preamble = captured_preamble(channel, module);
                        if (preamble == 4 && got == 0) {
                            start_cycle = cycle;
                            got = 1;
                        } else if (preamble != 4 && got == 1) {
                            end_cycle = cycle;
                            got = 2;
                        }
                        idly_inc(channel, module);
                    }
                    rd_inc(channel, module);
                }
                if (start_cycle == -1) {
                    printf("Failed to find result for %d\n", module);
                    continue;
                }
                printf("Preamble starts in cycle:%d\n", start_cycle);
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
                    printf("%d|", cycle);
                    idly_rst(channel, module);
                    for(delay = 0; delay < SDRAM_PHY_DELAYS; ++delay){
                        works = 1;
                        for (seed = 0; seed < seeds_count && works; ++seed){
                            /* Setup MRs */
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
                    }
                    printf("|\n");
                    ++cycle;
                    rd_inc(channel, module);
                }
                printf("m%d|start cycle:%d, delay:%d; end cycle:%d, delay:%d|",
                    module, start_cycle, start_delay, end_cycle, end_delay);
                eye_width = (end_cycle-start_cycle)*SDRAM_PHY_DELAYS + end_delay - start_delay;
                middle_cycle = start_cycle + (start_delay + eye_width/2)/SDRAM_PHY_DELAYS;
                middle_delay = (start_delay + eye_width/2)%SDRAM_PHY_DELAYS;
                printf("eye_width:%"PRIu32"; eye center: cycle:%d,delay:%d\n",
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
            send_mpc(channel, rank, 0x7f);
        }

        printf("Simple read check\n");
        // If we ever have multiple ranks, and independent timing for them
        // Uncomment loop below
        // for(rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
        {rank =0;
#ifdef SDRAM_PHY_SUBCHANNELS
            for (module = 0; module < SDRAM_PHY_MODULES/2; module++) {
                printf("Channel:%c rank:%d module:%d serial number:", (char)('A'+channel), rank, module);
#else
            for (module = 0; module < SDRAM_PHY_MODULES; module++) {
                printf("Rank:%d module:%d serial number:", rank, module);
#endif // SDRAM_PHY_SUBCHANNELS
                for (i = 0; i < 5; ++i) {
                    send_mrr(channel, rank, 65+i);
                    printf("%02"PRIX8, recover_mrr_value(channel, module));
                }
                printf("\n");
#ifdef DEBUG_DDR5
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
#endif // DEBUG_DDR5
            }
#ifdef DEBUG_DDR5
            // Check if registers are correct
#ifdef SDRAM_PHY_SUBCHANNELS
            for (module = 0; module < SDRAM_PHY_MODULES/2; module++) {
                printf("Channel:%c rank:%d module:%d\n", (char)('A'+channel), rank, module);
#else
            for (module = 0; module < SDRAM_PHY_MODULES; module++) {
                printf("Rank:%d module:%d\n", rank, module);
#endif // SDRAM_PHY_SUBCHANNELS
                for (i = 0; i < 256; ++i) {
                    send_mrr(channel, rank, i);
                    printf("\tMR:%d %02"PRIX8"\n", i, recover_mrr_value(channel, module));
                }
            }
#endif // DEBUG_DDR5
            send_mpc(channel, rank, 0x7f);
        }
    }
}

void sdram_ddr5_write_training(void) {
    int channel, rank, module;
    int cycle, delay, got, sample, it;
    int start_cycle, start_delay;   // First working cycle delay pair
    WICA = 1<<7;
#if 1
#ifdef SDRAM_PHY_SUBCHANNELS
    for (channel = 0; channel < 2; channel++) {
        printf("Subchannel:%c Write leveling\n", (char)('A'+channel));
#else
    {channel = 0;
        printf("Write leveling\n");
#endif // SDRAM_PHY_SUBCHANNELS
        enter_write_leveling(channel);
        /* Coarse alignment */
        // If we ever have multiple ranks, and independent timing for them
        // Uncomment loop below
        // for(rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
        {rank = 0;
#ifdef SDRAM_PHY_SUBCHANNELS
            for (module = 0; module < SDRAM_PHY_MODULES/2; module++) {
#else
            for (module = 0; module < SDRAM_PHY_MODULES; module++) {
#endif // SDRAM_PHY_SUBCHANNELS
                start_cycle = -1;
                wr_rst(channel, module);
                /* Setup MRs */
                send_mrw(channel, rank, module, 2, 2);
                wr_rst(channel, module);
                cycle = 0;
                for(delay = SDRAM_PHY_MIN_WR_LATENCY; delay < SDRAM_PHY_CWL/2; ++delay, ++cycle) {
                    wr_inc(channel, module);
                }
                got = 0;
                for (; cycle < 63 && got < 1; ++cycle) {
                    sample = 1;
                    // Check multiple times, as we can be on the edge of transition
                    // Make sure we aren't in meta stable delay
                    for (it = 0; it<16; it++) {
                        send_wleveling_write(channel, rank);
                        sample &= wleveling_sample(channel);
                    }
                    if (sample && got == 0 && cycle > start_cycle) {
                        start_cycle = cycle;
                        got = 1;
                    }
                    wr_inc(channel, module);
                }
                if (start_cycle == -1) {
                    printf("Failed to find result for %d\n", module);
                    continue;
                }
                wr_rst(channel, module);
                printf("DQS write leveling starts in cycle:%d (adjusted %d)\n",
                    start_cycle, start_cycle + SDRAM_PHY_MIN_WR_LATENCY);
                /* Pull back 1 cycle */
                start_cycle -= 1;
                wr_rst(channel, module);
                odly_dq_rst(channel, module);
                odly_dqs_rst(channel, module);
                for (it = 0; it < start_cycle; ++it) {
                    wr_inc(channel, module);
                }

                got = 0;
                cycle = start_cycle;
                start_cycle = -1; start_delay = 1;
                printf("DQS edge scan:\n");
                // Break out when 0 to 1 transition was found
                while (got != 1 && cycle < 67) {
                    printf("%d|", cycle);
                    odly_dq_rst(channel, module);
                    odly_dqs_rst(channel, module);
                    for(delay = 0; delay < SDRAM_PHY_DELAYS; ++delay) {
                        sample = 1;
                        // Check multiple times, as we can be on the edge of transition
                        // Make sure we aren't in meta stable delay
                        for (it = 0; it<16; it++) {
                            send_wleveling_write(channel, rank);
                            sample &= wleveling_sample(channel);
                        }
                        printf("%d", sample);
                        if (sample && got == 0) {
                            start_cycle = cycle;
                            start_delay = delay;
                            got = 1;
                        }
                        odly_dq_inc(channel, module);
                        odly_dqs_inc(channel, module);
                    }
                    printf("|\n");
                    ++cycle;
                    wr_inc(channel, module);
                }
                // Pull back 0.75 clock as specified by JEDEC
                // to train WICA

                start_cycle -= 1;
                start_delay += SDRAM_PHY_DELAYS/4;
                if (start_delay >= SDRAM_PHY_DELAYS) {
                    start_cycle += 1;
                    start_delay -= SDRAM_PHY_DELAYS;
                }

                wr_rst(channel, module);
                odly_dq_rst(channel, module);
                odly_dqs_rst(channel, module);
                for (it = 0; it < start_cycle; ++it) {
                    wr_inc(channel, module);
                }
                for (it = 0; it < start_delay; ++it) {
                    odly_dq_inc(channel, module);
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
                        sample &= wleveling_sample(channel);
                    }
                    printf("WICA:%d,%d|", cycle, sample);
                    ++cycle;
                    if (sample && got == 0) {
                        got = 1;
                    }
                } while (got != 1 && cycle < 7 );

                start_cycle -= 1;
                wr_rst(channel, module);
                odly_dq_rst(channel, module);
                odly_dqs_rst(channel, module);
                for (it = 0; it < start_cycle; ++it) {
                    wr_inc(channel, module);
                }

                got = 0;
                cycle = start_cycle;
                start_cycle = -1; start_delay = 1;
                printf("Scan for internal edge:\n");
                // Break out when 0 to 1 transition was found
                while (got != 1) {
                    printf("%d|", cycle);
                    odly_dq_rst(channel, module);
                    odly_dqs_rst(channel, module);
                    for(delay = 0; delay < SDRAM_PHY_DELAYS; ++delay) {
                        sample = 1;
                        // Check multiple times, as we can be on the edge of transition
                        // Make sure we aren't in meta stable delay
                        for (it = 0; it<16; it++) {
                            send_wleveling_write(channel, rank);
                            sample &= wleveling_sample(channel);
                        }
                        printf("%d", sample);
                        if (sample && got == 0) {
                            start_cycle = cycle;
                            start_delay = delay;
                            got = 1;
                        }
                        odly_dq_inc(channel, module);
                        odly_dqs_inc(channel, module);
                    }
                    printf("|\n");
                    ++cycle;
                    wr_inc(channel, module);
                }

                // Push forward 1.25 clock as specified by JEDEC
                // after training WICA

                start_cycle += 1;
                start_delay += SDRAM_PHY_DELAYS/4;
                if (start_delay >= SDRAM_PHY_DELAYS) {
                    start_cycle += 1;
                    start_delay -= SDRAM_PHY_DELAYS;
                }

                printf("Final timing values: cycles:%d(adjusted %d) delay:%d\n",
                    start_cycle, start_cycle + SDRAM_PHY_MIN_WR_LATENCY, start_delay);

                wr_rst(channel, module);
                odly_dq_rst(channel, module);
                odly_dqs_rst(channel, module);
                for (it = 0; it < start_cycle; ++it) {
                    wr_inc(channel, module);
                }
                for (it = 0; it < start_delay; ++it) {
                    odly_dq_inc(channel, module);
                    odly_dqs_inc(channel, module);
                }

                send_mrw(channel, rank, module, 2, 0|WICA);
            }
        }
        exit_write_leveling(channel);
    }
#endif
}

#endif // MEMORY_TYPE_DDR5
#endif // LIBLITEDRAM_DDR5_TRAINING_H
