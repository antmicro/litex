#ifndef LIBLITEDRAM_DDR5_TRAINING_H
#define LIBLITEDRAM_DDR5_TRAINING_H
#include <liblitedram/accessors.h>

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

static void sample_rddata(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        sdram_dfii_B_cmdinjector_rddata_capture_write(1);
    } else {
        sdram_dfii_A_cmdinjector_rddata_capture_write(1);
    }
#else
    sdram_dfii_cmdinjector_rddata_capture_write(1);
#endif
}

static uint32_t get_rddata(int channel, int phase) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        sdram_dfii_B_cmdinjector_rddata_select_write(phase);
        return sdram_dfii_B_cmdinjector_rddata_read();
    } else {
        sdram_dfii_A_cmdinjector_rddata_select_write(phase);
        return sdram_dfii_A_cmdinjector_rddata_read();
    }
#else
    sdram_dfii_cmdinjector_rddata_select_write(phase);
    return sdram_dfii_cmdinjector_rddata_read();
#endif
}

static void disable_2n_mode() {
    int value = sdram_dfii_control_read();
    value &= ~DFII_CONTROL_2N_MODE;
    sdram_dfii_control_write(value);
}

static void enable_2n_mode() {
    int value = sdram_dfii_control_read();
    value |= DFII_CONTROL_2N_MODE;
    sdram_dfii_control_write(value);
    printf("Switching to 2N mode\n");
}

int cs_state[2][SDRAM_PHY_DELAYS];

#if defined(SDRAM_PHY_CLK_DELAY_CAPABLE) && defined(SDRAM_PHY_ADDRESS_DELAY_CAPABLE)
static void sdram_ddr5_cs_ca_training() {
    int channel, rank, offset, delay, phase, dq;
    uint32_t data, left_hand_side, right_hand_side;
    bool CS_failed, CA_failed;
    disable_2n_mode();
    CS_failed = true;
    CA_failed = true;
#if 0
#ifdef SDRAM_PHY_SUBCHANNELS
    for (channel = 0; channel < 2; channel++) {
        printf("Subchannel:%c CS training\n", 'A'+channel);
#else
    {channel = 0;
        printf("CS training\n");
#endif
        for  (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
            printf("Rank: %d|", rank);
            // Emit CS training MPC
            cmd_injector(channel, 0xf, 0, 0xf | (1<<5), 0, 0, 0, 0);
            cmd_injector(channel, 0xf, 1<<rank, 0xf | (1<<5), 0, 0, 0, 0);
            cmd_injector(channel, 0xf, 0, 0xf | (1<<5), 0, 0, 0, 0);
            // Start with pattern 1010, then check pattern 0101
            for (offset = 0; offset < 2; offset ++) {
                cs_ca_select(channel, rank);
                cs_rst_delay(channel);
                cs_ca_deselect(channel, rank);
                cmd_injector(channel, 0xf, 0, 0x1f, 0, 0, 0, 0);
                cmd_injector(channel, 0x5<<offset, 1<<rank, 0x1f, 0, 0, 0, 0);
                left_hand_side = 0;
                right_hand_side = 0;

                // Sample at delay 0, if works, then move clock, equivalent to moving CS back
                cdelay(200);
                sample_rddata(channel);
                cs_state[offset][0] = 1;
                for (phase = 0; phase < SDRAM_PHY_PHASES; phase ++) {
                    data = get_rddata(channel, phase);
                    cs_state[offset][delay] &= !data;
                }
                for (delay = 0; delay < SDRAM_PHY_DELAYS; delay++) {
                
                }
                for (delay = 0; delay < SDRAM_PHY_DELAYS; delay++) {
                    cdelay(200);
                    sample_rddata(channel);
                    cs_state[offset][delay] = 1;
                    for (phase = 0; phase < SDRAM_PHY_PHASES; phase ++) {
                        data = get_rddata(channel, phase);
                        cs_state[offset][delay] &= !data;
                    }
                    printf("%d", cs_state[offset][delay]);
                    cs_ca_select(channel, rank);
                    cs_inc_delay(channel);
                    cs_ca_deselect(channel, rank);
                }
            }
            printf("\n");
            cmd_injector(channel, 0xf, 0, 0xf, 0, 0, 0, 0);
            cmd_injector(channel, 0xf, 1<<rank, 0xf, 0, 0, 0, 0);
            cmd_injector(channel, 0xf, 0, 0xf, 0, 0, 0, 0);
        }
    }
#endif
    if (CS_failed | CA_failed)
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
