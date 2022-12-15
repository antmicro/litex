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

static void cs_non_negative_tap(int channel, int rank, int offset, int32_t *left, int32_t *right, const char* format) {
    int _result, delay;
    cs_rst(channel, rank, 0);
    cs_sample_prep(channel, rank, 0, offset);
#ifdef DEBUG_DDR5
    printf(format, 0xa>>offset);
#endif // DEBUG_DDR5
    _result = 1;
    for (delay = 0; delay < SDRAM_PHY_DELAYS && (_result || *right == UNSET_DELAY); delay++) {
        _result =  or_sample(channel);
        if (_result && *right == UNSET_DELAY) {
            *right = delay;
        }
        else if (!_result && *right != UNSET_DELAY && *left == UNSET_DELAY) {
            *left = delay;
        }
        else if (_result && *left == UNSET_DELAY && delay+1 == SDRAM_PHY_DELAYS) {
            *left = delay + 1;
        }
#ifdef DEBUG_DDR5
        printf("%d", _result);
#endif // DEBUG_DDR5
        cs_inc(channel, rank, 0);
    }
}

static void cs_on_edge(int channel, int rank, int32_t *left, int32_t *right, int on_edge) {
    int _result, delay, offset;
    offset = on_edge >> 1;
    cs_rst(channel, rank, 0);
    cs_sample_prep(channel, rank, 0, (offset+1)%2);
#ifdef DEBUG_DDR5
    printf("CS right_edge|");
#endif // DEBUG_DDR5
    *right = 0;
    _result = 0;
    for (delay = 0; delay < SDRAM_PHY_DELAYS; delay++) {
        _result =  or_sample(channel);
        if (_result && *right == UNSET_DELAY)
            *right = delay + 1 - SDRAM_PHY_DELAYS;
#ifdef DEBUG_DDR5
        printf("%d", _result);
#endif // DEBUG_DDR5
        cs_inc(channel, rank, 0);
    }
    cs_non_negative_tap(channel, rank, offset, left, right, "\n   left_edge|");
#ifdef DEBUG_DDR5
    printf(";%"PRId32":%"PRId32"\n", *right, *left);
#endif // DEBUG_DDR5
}

static void cs_in_middle(int channel, int rank, int32_t *left, int32_t *right) {
    int32_t temp_l, temp_r, offset;
    for (offset = 0; offset < 2; offset++) {
        temp_l = UNSET_DELAY; temp_r = UNSET_DELAY;
        cs_non_negative_tap(channel, rank, offset, &temp_l, &temp_r, "CS_pattern:0x%x|");
        if (temp_l != UNSET_DELAY && temp_r != UNSET_DELAY) {
            if (*left == UNSET_DELAY) {
                *left = temp_l; *right = temp_r;
            } else if (*left - *right < temp_l - temp_r) {
                *left = temp_l; *right = temp_r;
            }
        }
#ifdef DEBUG_DDR5
        printf("\n");
#endif // DEBUG_DDR5
    }
#ifdef DEBUG_DDR5
    printf(";%" PRId32 ":%" PRId32 "\n", *right, *left);
#endif // DEBUG_DDR5
}

static void ca_on_edge(int channel, int rank, int address, int32_t *right) {
    int _result, delay;

    ca_rst(channel, rank, address);
#ifdef DEBUG_DDR5
    printf("CA line:%02d\nright_edge|", address);
#endif // DEBUG_DDR5
    _result = 0;
    for (delay = 0; delay < SDRAM_PHY_DELAYS; delay++) {
        ca_sample_prep_previous_period(channel, rank, address, 1);
        _result = and_sample(channel);
        ca_sample_prep_previous_period(channel, rank, address, 0);
        _result &= or_sample(channel);
        if (_result && *right == UNSET_DELAY)
            *right = delay + 1 - SDRAM_PHY_DELAYS;
#ifdef DEBUG_DDR5
        printf("%d", _result);
#endif // DEBUG_DDR5
        ca_inc(channel, rank, address);
    }
    if (*right == UNSET_DELAY)
        *right = 0;
}

static void ca_non_negative_taps(int channel, int rank, int address, int32_t *left, int32_t *right, const char* format) {
    int _result, delay;
    ca_rst(channel, rank, address);
#ifdef DEBUG_DDR5
    printf(format, address);
#endif // DEBUG_DDR5
    _result = 1;
    for (delay = 0; delay < SDRAM_PHY_DELAYS && (_result || *right == UNSET_DELAY); delay++) {
        ca_sample_prep_current_period(channel, rank, address, 1);
        _result = and_sample(channel);
        ca_sample_prep_current_period(channel, rank, address, 0);
        _result &= or_sample(channel);
        if (_result && *right == UNSET_DELAY)
            *right = delay;
        else if (!_result && *right != UNSET_DELAY && *left == UNSET_DELAY)
            *left = delay;
        else if (_result && *left == UNSET_DELAY&& delay+1 == SDRAM_PHY_DELAYS)
            *left = delay + 1;
#ifdef DEBUG_DDR5
        printf("%d", _result);
#endif // DEBUG_DDR5
        ca_inc(channel, rank, address);
    }
#ifdef DEBUG_DDR5
    printf("\n");
    printf(";%"PRId32":%"PRId32"\n", *right, *left);
#endif // DEBUG_DDR5
}

static void CS_training(int32_t channel, uint8_t *success, int debug) {
    int32_t left_side, right_side;
    int32_t rank;
    int32_t on_edge, _result, offset;
    for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
        if (debug)
            printf("Rank: %"PRId32"\n", rank);
        // Enter CS training MPC
        enter_cs(channel, rank);

        on_edge = 0;
        for (offset = 0; offset < 2; offset++) {
            cs_sample_prep(channel, rank, 0, offset);
            _result =  or_sample(channel);
            if (_result)
                on_edge = offset<<1|1;
        }

        left_side = UNSET_DELAY; right_side = UNSET_DELAY;
        if (on_edge)
            cs_on_edge(channel, rank, &left_side, &right_side, on_edge);
        else
            cs_in_middle(channel, rank, &left_side, &right_side);

        cs_rst(channel, rank, 0);

        // Exit CS training MPC
        exit_cs(channel, rank);

        mid_point_calc_and_set(success, "Rank:%d Eye width:%d ", channel, rank,
                               -1, rank, left_side, right_side, cs_inc, 1);
    }
}

static void CA_training(int32_t channel, int debug, int limit) {
    int32_t left_side, right_side;
    int32_t rank, address;
    int32_t on_edge;

    for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
        if (debug)
            printf("Rank:%02"PRId32"\n", rank);
        // Enter CA training MPC
        enter_ca(channel, rank);

        for (address = 0; address < limit; address++) {
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

            left_side = UNSET_DELAY; right_side = UNSET_DELAY;
            if (on_edge) {
                ca_on_edge(channel, rank, address, &right_side);
                ca_non_negative_taps(channel, rank, address, &left_side, &right_side, "\n left_edge|");
            } else {
                ca_non_negative_taps(channel, rank, address, &left_side, &right_side, "CA line:%02d|");
            }


            if(right_side == UNSET_DELAY) {
                _ca_results[address][0] = UNSET_DELAY;
                break;
            } else if (right_side > _ca_results[address][0]) {
                _ca_results[address][0] = right_side;
            }

            if(left_side == UNSET_DELAY) {
                _ca_results[address][1] = UNSET_DELAY;
                break;
            } else if (left_side < _ca_results[address][1]) {
                _ca_results[address][1] = left_side;
            }
        }
        // Exit CA training multiple NOPs
        cmd_injector(channel, 0xf, 0, 0x1f, 0, 0, 0, 0);
        exit_ca(channel, rank);
    }
}

static void CA_setup_values(int32_t channel, uint8_t *success, int debug, int limit) {
    int32_t left_side, right_side;
    int32_t address;
    for (address = 0; address < limit; address++) {
        right_side = _ca_results[address][0];
        left_side  = _ca_results[address][1];

        if (left_side == UNSET_DELAY || right_side == UNSET_DELAY) {
            printf("CA:%02"PRId32" Eye width:0 Failed\n", address);
            *success &= 0;
            return;
        }
        ca_rst(channel, -1, address);
        mid_point_calc_and_set(success, "CA:%02d Eye width:%d ", channel, -1,
                               address, address, left_side, right_side, ca_inc, 0);
    }
}

#if defined(SDRAM_PHY_ADDRESS_DELAY_CAPABLE)
void sdram_ddr5_cs_ca_training(void) {
    int32_t channel, rank;
    uint8_t CS_success, CA_success;
    int debug;
    disable_dfi_2n_mode();

#ifdef DEBUG_DDR5
    debug = 1;
#else
    debug = 0;
#endif // DEBUG_DDR5

    CS_success = 1;
    CA_success = 1;
#ifdef SDRAM_PHY_SUBCHANNELS
    for (channel = 0; channel < 2; channel++) {
        printf("Subchannel:%c CS training\n", 'A'+channel);
#else
    {channel = 0;
        printf("CS training\n");
#endif // SDRAM_PHY_SUBCHANNELS
        CS_training(channel, &CS_success, debug);
        printf("CA training\n");
        setup_ca_results();
        CA_training(channel, debug, SDRAM_PHY_ADDRESS_LINES);
        CA_setup_values(channel, &CA_success, debug, SDRAM_PHY_ADDRESS_LINES);
    }
    if (!(CS_success & CA_success)) {
        enable_dfi_2n_mode();
    } else {
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
    int debug;
    disable_dfi_2n_mode();

#ifdef DEBUG_DDR5
    debug = 1;
#else
    debug = 0;
#endif // DEBUG_DDR5

    CS_success = 1;
    CA_success = 1;
#ifdef SDRAM_PHY_SUBCHANNELS
    for (channel = 0; channel < 2; channel++) {
        printf("Subchannel:%c CS training\n", (char)('A'+channel));
#else
    {channel = 0;
        printf("CS training\n");
#endif // SDRAM_PHY_SUBCHANNELS
        CS_training(channel, &CS_success, debug);
        printf("CA training\n");
        setup_ca_results();
        CA_training(channel, debug, SDRAM_PHY_ADDRESS_LINES);
        CA_setup_values(channel, &CA_success, debug, SDRAM_PHY_ADDRESS_LINES);
    }
    if (!(CS_success & CA_success)) {
        enable_dfi_2n_mode();
    } else {
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
#else
    printf("CS/CA training impossible\n"
           "Keeping DRAM in 2N mode\n");
#endif
    return;
}
#endif // defined(SDRAM_PHY_ADDRESS_DELAY_CAPABLE)

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
        /* Setup MRs */
        for(rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
            send_mrw(channel, rank, 2, 1);
            send_mrw(channel, rank, 25, 1);
            send_mrw(channel, rank, 28, 0xA5);
            send_mrw(channel, rank, 29, 0xA5);
            send_mrw(channel, rank, 30, 0x33);
        }

        /* All PHYs so far have support for single delay far all ranks */
        /* Change order from module,rank to rank,module when per rank training will be supported */

#ifdef SDRAM_PHY_SUBCHANNELS
        for (module = 0; module < SDRAM_PHY_MODULES/2; module++) {
#else
        for (module = 0; module < SDRAM_PHY_MODULES; module++) {
#endif // SDRAM_PHY_SUBCHANNELS
            start_cycle = -1;
            end_cycle = 100;
            rd_rst(channel, module);
            /* Coarse alignment */
            for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
                got = 0;
                for (cycle = 0; cycle < 67; cycle ++) {
                    idly_rst(channel, module);
                    for (delay = 0; delay < SDRAM_PHY_DELAYS; delay++) {
                        send_mrr(channel, rank, 31);
                        preamble = captured_preamble(channel, module);
                        if (preamble == 4 && got == 0 && cycle > start_cycle) {
                            start_cycle = cycle;
                            got = 1;
                        } else if (preamble != 4 && got == 1 && end_cycle > cycle) {
                            end_cycle = cycle;
                        }
                        idly_inc(channel, module);
                    }
                    rd_inc(channel, module);
                }
            }
            if (start_cycle == -1) {
                printf("Failed to find result for %d\n", module);
                continue;
            }
#ifdef DEBUG_DDR5
            printf("Preamble starts in cycle:%d\n", start_cycle);
#endif
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
            while (got != 2 && cycle < 67) {
#ifdef DEBUG_DDR5
                printf("%d\n", cycle);
#endif
                idly_rst(channel, module);
                for(delay = 0; delay < SDRAM_PHY_DELAYS; ++delay){
                    works = 1;
                    for (seed = 0; seed < seeds_count && works; ++seed){
                        /* Setup MRs */
                        for(rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
                            send_mrw(channel, rank, 26, seeds0[seed]);
                            send_mrw(channel, rank, 27, seeds1[seed]);
                            send_mrr(channel, rank, 31);
                            works &= compare(channel, module,
                                             seeds0[seed], seeds1[seed],
                                             0xA5, 0x33);
                        }
                    }
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
                ++cycle;
                rd_inc(channel, module);
            }
            printf("m%d|cyc%d, dly%d: cyc%d, dly%d ", module, start_cycle, start_delay, end_cycle, end_delay);
            eye_width = (end_cycle-start_cycle)*SDRAM_PHY_DELAYS + end_delay - start_delay;
            middle_cycle = start_cycle + (start_delay + eye_width/2)/SDRAM_PHY_DELAYS;
            middle_delay = (start_delay + eye_width/2)%SDRAM_PHY_DELAYS;
            printf("eye_width:%"PRIu32", chosen cyc:%d,dly:%d\n", eye_width, middle_cycle, middle_delay);
            rd_rst(channel, module);
            idly_rst(channel, module);
            for (i = 0; i < middle_cycle; ++i) {
                rd_inc(channel, module);
            }
            for (i = 0; i < middle_delay; ++i) {
                idly_inc(channel, module);
            }
        }
        /* Finish preamble and read training*/
        for(rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
            send_mrw(channel, rank, 2, 0);
        }
#ifdef DEBUG_DDR5
        for (rank = 0; rank < SDRAM_PHY_RANKS; ++rank) {
            send_mrw(channel, rank, 63, 0xDE);
            send_mrr(channel, rank, 63);
            printf("%"PRIX8, recover_mrr_value(channel, 0));
            send_mrw(channel, rank, 63, 0xAD);
            send_mrr(channel, rank, 63);
            printf("%"PRIX8, recover_mrr_value(channel, 0));
            send_mrw(channel, rank, 63, 0xBE);
            send_mrr(channel, rank, 63);
            printf("%"PRIX8, recover_mrr_value(channel, 0));
            send_mrw(channel, rank, 63, 0xEF);
            send_mrr(channel, rank, 63);
            printf("%"PRIX8"\n", recover_mrr_value(channel, 0));
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
            }
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
        }
#endif // DEBUG_DDR5
    }
}

#endif // MEMORY_TYPE_DDR5
#endif // LIBLITEDRAM_DDR5_TRAINING_H
