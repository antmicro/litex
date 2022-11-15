#ifndef LIBLITEDRAM_DDR5_TRAINING_H
#define LIBLITEDRAM_DDR5_TRAINING_H
#include <liblitedram/accessors.h>

#define DEBUG_DDR5

static int prep_payload (int cs, int command, int wrdata_en,
                         int wrdata_mask, int rddata_en) {
    int payload;
#ifdef SDRAM_PHY_SUBCHANNELS
    payload = cs << CSR_SDRAM_DFII_A_CMDINJECTOR_COMMAND_STORAGE_CS_OFFSET | \
              command << CSR_SDRAM_DFII_A_CMDINJECTOR_COMMAND_STORAGE_CA_OFFSET | \
              wrdata_en << CSR_SDRAM_DFII_A_CMDINJECTOR_COMMAND_STORAGE_WRDATA_EN_OFFSET | \
              wrdata_mask << CSR_SDRAM_DFII_A_CMDINJECTOR_COMMAND_STORAGE_WRDATA_MASK_OFFSET | \
              rddata_en << CSR_SDRAM_DFII_A_CMDINJECTOR_COMMAND_STORAGE_RDDATA_EN_OFFSET;
#else
    payload = cs << CSR_SDRAM_DFII_CMDINJECTOR_COMMAND_STORAGE_CS_OFFSET | \
              command << CSR_SDRAM_DFII_CMDINJECTOR_COMMAND_STORAGE_CA_OFFSET | \
              wrdata_en << CSR_SDRAM_DFII_CMDINJECTOR_COMMAND_STORAGE_WRDATA_EN_OFFSET | \
              wrdata_mask << CSR_SDRAM_DFII_CMDINJECTOR_COMMAND_STORAGE_WRDATA_MASK_OFFSET | \
              rddata_en << CSR_SDRAM_DFII_CMDINJECTOR_COMMAND_STORAGE_RDDATA_EN_OFFSET;
#endif
    return payload;
}

static void upload_payload(int channel, int phases, int payload) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        sdram_dfii_B_cmdinjector_command_storage_write(payload);
        sdram_dfii_B_cmdinjector_phase_addr_write(phases);
    } else {
        sdram_dfii_A_cmdinjector_command_storage_write(payload);
        sdram_dfii_A_cmdinjector_phase_addr_write(phases);
    }
#else
    sdram_dfii_cmdinjector_command_storage_write(payload);
    sdram_dfii_cmdinjector_phase_addr_write(phases);
#endif
}

static void store_payload(int channel, int single) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        if (single == 0)
            sdram_dfii_cmdinjector_store_continuous_cmd_write(1);
        else
            sdram_dfii_cmdinjector_store_singleshot_cmd_write(1);
    } else {
        if (single == 0)
            sdram_dfii_cmdinjector_store_continuous_cmd_write(1);
        else
            sdram_dfii_cmdinjector_store_singleshot_cmd_write(1);
    }
#else
    if (single == 0)
        sdram_dfii_cmdinjector_store_continuous_cmd_write(1);
    else
        sdram_dfii_cmdinjector_store_singleshot_cmd_write(1);
#endif
}

static void cmd_injector(int channel, int phases, int cs, int command,
                         int wrdata_en, int wrdata_mask, int rddata_en, int single) {
    int payload = prep_payload(cs, command, wrdata_en, wrdata_mask, rddata_en);
    upload_payload(channel, phases, payload);
    store_payload(channel, single);
}

static void issue_single(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        sdram_dfii_B_cmdinjector_single_shot_write(1);
        sdram_dfii_B_cmdinjector_issue_command_write(1);
        sdram_dfii_B_cmdinjector_single_shot_write(0);
    } else {
        sdram_dfii_A_cmdinjector_single_shot_write(1);
        sdram_dfii_A_cmdinjector_issue_command_write(1);
        sdram_dfii_A_cmdinjector_single_shot_write(0);
    }
#else
    sdram_dfii_cmdinjector_single_shot_write(1);
    sdram_dfii_cmdinjector_issue_command_write(1);
    sdram_dfii_cmdinjector_single_shot_write(0);
#endif
}

static void setup_capture(int channel, int setup) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        sdram_dfii_B_cmdinjector_sample_write(0);
        sdram_dfii_B_cmdinjector_setup_write(setup);
        sdram_dfii_B_cmdinjector_reset_write(1);
    } else {
        sdram_dfii_A_cmdinjector_sample_write(0);
        sdram_dfii_A_cmdinjector_setup_write(setup);
        sdram_dfii_A_cmdinjector_reset_write(1);
    }
#else
    sdram_dfii_cmdinjector_sample_write(0);
    sdram_dfii_cmdinjector_setup_write(setup);
    sdram_dfii_cmdinjector_reset_write(1);
#endif
}

static void start_capture(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        sdram_dfii_B_cmdinjector_sample_write(1);
    } else {
        sdram_dfii_A_cmdinjector_sample_write(1);
    }
#else
    sdram_dfii_cmdinjector_sample_write(1);
#endif
}

static void stop_capture(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        sdram_dfii_B_cmdinjector_sample_write(0);
    } else {
        sdram_dfii_A_cmdinjector_sample_write(0);
    }
#else
    sdram_dfii_cmdinjector_sample_write(0);
#endif
}

static uint32_t capture_result(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        return sdram_dfii_B_cmdinjector_result_read();
    } else {
        return sdram_dfii_A_cmdinjector_result_read();
    }
#else
    return sdram_dfii_cmdinjector_result_read();
#endif
}

static void disable_2n_mode() {
    int value = sdram_dfii_control_read();
    value &= ~DFII_CONTROL_2N_MODE;
    sdram_dfii_control_write(value);
    printf("Switching to 1N mode\n");
}

static void enable_2n_mode() {
    int value = sdram_dfii_control_read();
    value |= DFII_CONTROL_2N_MODE;
    sdram_dfii_control_write(value);
    printf("Switching to 2N mode\n");
}

#if defined(SDRAM_PHY_CLK_DELAY_CAPABLE) && defined(SDRAM_PHY_ADDRESS_DELAY_CAPABLE)
static char _taps_result[SDRAM_PHY_DELAYS];
static char _ca_results[SDRAM_PHY_ADDRESS_LINES][SDRAM_PHY_DELAYS];

static void sdram_ddr5_cs_ca_training() {
    int channel, rank, address;
    int offset, delay, phase, dq;
    int32_t left_side, right_side, mid_point;
    uint32_t _result, state;
    bool CS_success, CA_success;
    int debug;
    disable_2n_mode();
#ifdef DEBUG_DDR5
    debug = 1;
#else
    debug = 0;
#endif
    CS_success = false;
    CA_success = false;
#ifdef SDRAM_PHY_SUBCHANNELS
    for (channel = 0; channel < 2; channel++) {
        printf("Subchannel:%c CS training\n", 'A'+channel);
#else
    {channel = 0;
        printf("CS training\n");
#endif
        for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
            printf("Rank: %d\n", rank);
            // Enter CS training MPC
            cmd_injector(channel, 0xf, 0, 0xf | (1<<5), 0, 0, 0, 0);
            cmd_injector(channel, 0xf, 1<<rank, 0xf | (1<<5), 0, 0, 0, 0);
            cmd_injector(channel, 0xf, 0, 0xf | (1<<5), 0, 0, 0, 0);
            cdelay(100);
            for (offset = 0; offset < 2; offset ++) {
                cs_ca_select(channel, rank);
                cs_rst_delay(channel);
                cs_ca_deselect(channel, rank);
                for (delay = 0; delay < SDRAM_PHY_DELAYS; delay++) {
                    setup_capture(channel, 0);
                    cmd_injector(channel, 0xf, 0, 0x1f, 0, 0, 0, 0);
                    cmd_injector(channel, 0x5<<offset, 1<<rank, 0x1f, 0, 0, 0, 0);
                    cdelay(10);
                    start_capture(channel);
                    cdelay(200);
                    stop_capture(channel);
                    _result = 1;
                    _result = capture_result(channel);
                    if(debug)
                        printf("CLK_offst 0, CS_pattern:0x%x, CS_offset %d:%d\n", 0x5<<offset, delay, !_result);
                    _taps_result[delay] |= !_result;
                    cs_ca_select(channel, rank);
                    cs_inc_delay(channel);
                    cs_ca_deselect(channel, rank);
                }
            }
            // Exit CS training MPC
            cmd_injector(channel, 0xf, 0, 0xf, 0, 0, 0, 0);
            cmd_injector(channel, 0xf, 1<<rank, 0xf, 0, 0, 0, 0);
            cmd_injector(channel, 0xf, 0, 0xf, 0, 0, 0, 0);
            printf("Rank:%d|", rank);
            cdelay(100);
            state = 0; left_side = 0; right_side = 0;
            for (delay = 0; delay < SDRAM_PHY_DELAYS; delay++) {
                printf("%c", '0'+_taps_result[delay]);
                if (state < 2 && _taps_result[delay]) {
                    state = 1;
                    left_side += 1;
                } else if (state == 1 && !_taps_result[delay]) {
                    state = 2;
                } else if (state == 2 && _taps_result[delay]) {
                    right_side -=1;
                }
            }
            printf("\nEye width:%d ", left_side - right_side);
            mid_point = (left_side - right_side) / 2 + right_side;
            if (mid_point < 0)
                mid_point += SDRAM_PHY_DELAYS;

            printf("Mid point:%d\n", mid_point);
            cs_ca_select(channel, rank);
            cs_rst_delay(channel);
            cs_ca_deselect(channel, rank);
            for (delay = 0; delay < mid_point; delay++) {
                cs_ca_select(channel, rank);
                cs_inc_delay(channel);
                cs_ca_deselect(channel, rank);
            }
            CS_success &= !!state;
        }

        for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
            // Enter CA training MPC
            cmd_injector(channel, 0xf, 0, 0xf | (3<<5), 0, 0, 0, 0);
            cmd_injector(channel, 0xf, 1<<rank, 0xf | (3<<5), 0, 0, 0, 0);
            cmd_injector(channel, 0xf, 0, 0xf | (3<<5), 0, 0, 0, 0);
            cdelay(100);
            for (address = 0; address < SDRAM_PHY_ADDRESS_LINES; address++) {
                cs_ca_select(channel, rank);
                ca_rst_delay(address);
                cs_ca_deselect(channel, rank);
                if (debug)
                    printf("CLK_offst 0, Rank:%d, CA_line:%02d|",rank, address);
                for (delay = 0; delay < SDRAM_PHY_DELAYS; delay++) {
                    setup_capture(channel, 3);
                    cmd_injector(channel, 0xf, 0, 0, 0, 0, 0, 0);
                    cmd_injector(channel, 0x1, 1<<rank, 1<<address, 0, 0, 0, 0);
                    cdelay(10);
                    start_capture(channel);
                    cdelay(200);
                    stop_capture(channel);
                    _result = 0;
                    _result = capture_result(channel);
                    if(debug)
                        printf("%d", _result);
                    _ca_results[rank][delay] |= _result;
                    cs_ca_select(channel, rank);
                    ca_inc_delay(address);
                    cs_ca_deselect(channel, rank);
                }
                if(debug)
                    printf("\n");
            }
            // Exit CA training multiple NOPs
            cmd_injector(channel, 0xff, 1<<rank, 0x1f, 0, 0, 0, 1);
            issue_single(channel);
            cdelay(100);
        }
    }
    if (~(CS_success & CA_success))
        enable_2n_mode();
    return;
}
#elif defined(SDRAM_PHY_ADDRESS_DELAY_CAPABLE)
static void sdram_ddr5_cs_ca_training() {
    bool CS_CA_failed;
    CS_CA_failed = true;
    if(CS_CA_failed) {
        printf("CS/CA training impossible\n");
        enable_2n_mode();
    }
}
#elif defined(SDRAM_PHY_CLK_DELAY_CAPABLE)
static void sdram_ddr5_cs_ca_training() {
    bool CS_CA_failed;
    CS_CA_failed = true;
    if(CS_CA_failed) {
        printf("CS/CA training impossible\n");
        enable_2n_mode();
    }
}
#else
static void sdram_ddr5_cs_ca_training() {
    printf("CS/CA training impossible\n"
           "Keeping DRAM in 2N mode\n");
}
#endif

#endif // LIBLITEDRAM_DDR5_TRAINING_H
