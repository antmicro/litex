#include <liblitedram/accessors.h>
#include <liblitedram/ddr5_helpers.h>
#ifdef LIBLITEDRAM_DDR5_HELPERS_H
#include <stdio.h>

#ifdef MEMORY_TYPE_DDR5
//#define DEBUG_DDR5

extern int N2_mode;
extern int enumerated;

int prep_payload (int cs, int command, int wrdata_en,
                  uint32_t wrdata_mask, int rddata_en) {
    int payload;
#ifdef SDRAM_PHY_SUBCHANNELS
    payload = cs << CSR_SDRAM_DFII_A_CMDINJECTOR_COMMAND_STORAGE_CS_OFFSET | \
              command << CSR_SDRAM_DFII_A_CMDINJECTOR_COMMAND_STORAGE_CA_OFFSET | \
              wrdata_en << CSR_SDRAM_DFII_A_CMDINJECTOR_COMMAND_STORAGE_WRDATA_EN_OFFSET | \
              (wrdata_mask & WRDATA_BITMASK) << CSR_SDRAM_DFII_A_CMDINJECTOR_COMMAND_STORAGE_WRDATA_MASK_OFFSET | \
              rddata_en << CSR_SDRAM_DFII_A_CMDINJECTOR_COMMAND_STORAGE_RDDATA_EN_OFFSET;
#else
    payload = cs << CSR_SDRAM_DFII_CMDINJECTOR_COMMAND_STORAGE_CS_OFFSET | \
              command << CSR_SDRAM_DFII_CMDINJECTOR_COMMAND_STORAGE_CA_OFFSET | \
              wrdata_en << CSR_SDRAM_DFII_CMDINJECTOR_COMMAND_STORAGE_WRDATA_EN_OFFSET | \
              (wrdata_mask & WRDATA_BITMASK) << CSR_SDRAM_DFII_CMDINJECTOR_COMMAND_STORAGE_WRDATA_MASK_OFFSET | \
              rddata_en << CSR_SDRAM_DFII_CMDINJECTOR_COMMAND_STORAGE_RDDATA_EN_OFFSET;
#endif
    return payload;
}

void upload_payload(int channel, int phases, int payload) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        sdram_dfii_b_cmdinjector_command_storage_write(payload);
        sdram_dfii_b_cmdinjector_phase_addr_write(phases);
    } else {
        sdram_dfii_a_cmdinjector_command_storage_write(payload);
        sdram_dfii_a_cmdinjector_phase_addr_write(phases);
    }
#else
    sdram_dfii_cmdinjector_command_storage_write(payload);
    sdram_dfii_cmdinjector_phase_addr_write(phases);
#endif
}

void store_payload(int channel, int single) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        if (single == 0)
            sdram_dfii_b_cmdinjector_store_continuous_cmd_write(1);
        else
            sdram_dfii_b_cmdinjector_store_singleshot_cmd_write(1);
    } else {
        if (single == 0)
            sdram_dfii_a_cmdinjector_store_continuous_cmd_write(1);
        else
            sdram_dfii_a_cmdinjector_store_singleshot_cmd_write(1);
    }
#else
    if (single == 0)
        sdram_dfii_cmdinjector_store_continuous_cmd_write(1);
    else
        sdram_dfii_cmdinjector_store_singleshot_cmd_write(1);
#endif
}

void cmd_injector(int channel, int phases, int cs, int command,
                  int wrdata_en, uint32_t wrdata_mask, int rddata_en, int single) {
    int payload = prep_payload(cs, command, wrdata_en, wrdata_mask, rddata_en);
    upload_payload(channel, phases, payload);
    store_payload(channel, single);
}

void issue_single(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        sdram_dfii_b_cmdinjector_single_shot_write(1);
        sdram_dfii_b_cmdinjector_issue_command_write(1);
        sdram_dfii_b_cmdinjector_single_shot_write(0);
    } else {
        sdram_dfii_a_cmdinjector_single_shot_write(1);
        sdram_dfii_a_cmdinjector_issue_command_write(1);
        sdram_dfii_a_cmdinjector_single_shot_write(0);
    }
#else
    sdram_dfii_cmdinjector_single_shot_write(1);
    sdram_dfii_cmdinjector_issue_command_write(1);
    sdram_dfii_cmdinjector_single_shot_write(0);
#endif
}

void setup_rddata_cnt(int channel, int value) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        sdram_dfii_b_cmdinjector_rddata_capture_cnt_write(value);
    } else {
        sdram_dfii_a_cmdinjector_rddata_capture_cnt_write(value);
    }
#else
    sdram_dfii_cmdinjector_rddata_capture_cnt_write(value);
#endif
}

#ifndef SDRAM_PHY_SUBCHANNELS
#define DFII_CMDINJECTOR_DATA_BYTES (SDRAM_PHY_DFI_DATABITS/8)
#else
#define DFII_CMDINJECTOR_DATA_BYTES (SDRAM_PHY_DFI_DATABITS/16)
#endif
#define BYTES_PER_MODULE (SDRAM_PHY_DQ_DQS_RATIO/4)
#define MODULE_BITMASK ((1<<SDRAM_PHY_DQ_DQS_RATIO)-1)

uint16_t get_data_module_phase(int channel, int module, int phase) {
    uint16_t ret_value;
    int pebo;   // module's positive_edge_byte_offset
    int nebo;   // module's negative_edge_byte_offset, could be undefined if SDR DRAM is used
    int ibo;    // module's in byte offset (x4 ICs)
    uint8_t data[DFII_CMDINJECTOR_DATA_BYTES];
    ret_value = 0;
#ifdef SDRAM_PHY_SUBCHANNELS
    if (channel) {
        sdram_dfii_b_cmdinjector_rddata_select_write(phase);
        csr_rd_buf_uint8(CSR_SDRAM_DFII_B_CMDINJECTOR_RDDATA_ADDR, data, DFII_CMDINJECTOR_DATA_BYTES);
    } else {
        sdram_dfii_a_cmdinjector_rddata_select_write(phase);
        csr_rd_buf_uint8(CSR_SDRAM_DFII_A_CMDINJECTOR_RDDATA_ADDR, data, DFII_CMDINJECTOR_DATA_BYTES);
    }
#else
    sdram_dfii_cmdinjector_rddata_select_write(phase);
    csr_rd_buf_uint8(CSR_SDRAM_DFII_CMDINJECTOR_RDDATA_ADDR, data, DFII_CMDINJECTOR_DATA_BYTES);
#endif
    // CSR are read as BIG Endian
    nebo = ((DFII_CMDINJECTOR_DATA_BYTES / BYTES_PER_MODULE) - 1 - module) * BYTES_PER_MODULE;
    pebo = ((DFII_CMDINJECTOR_DATA_BYTES / BYTES_PER_MODULE) - 1 - module) * BYTES_PER_MODULE + BYTES_PER_MODULE/2;

    ibo = 0; // Non zero only if x4 ICs are used
    ret_value |= (data[pebo] >> ibo) & MODULE_BITMASK;
    ibo = (0x4*BYTES_PER_MODULE) % 8;
    ret_value |= ((data[nebo] >> ibo) & MODULE_BITMASK) << SDRAM_PHY_DQ_DQS_RATIO;
    return ret_value;
}

void set_data_module_phase(int channel, int module, int phase, uint16_t wrdata) {
    int pebo;   // module's positive_edge_byte_offset
    int nebo;   // module's negative_edge_byte_offset, could be undefined if SDR DRAM is used
    int ibo;    // module's in byte offset (x4 ICs)
    uint8_t data[DFII_CMDINJECTOR_DATA_BYTES];
#ifdef SDRAM_PHY_SUBCHANNELS
    if (channel) {
        sdram_dfii_b_cmdinjector_wrdata_select_write(phase);
        csr_rd_buf_uint8(CSR_SDRAM_DFII_B_CMDINJECTOR_WRDATA_S_ADDR, data, DFII_CMDINJECTOR_DATA_BYTES);
    } else {
        sdram_dfii_a_cmdinjector_wrdata_select_write(phase);
        csr_rd_buf_uint8(CSR_SDRAM_DFII_A_CMDINJECTOR_WRDATA_S_ADDR, data, DFII_CMDINJECTOR_DATA_BYTES);
    }
#else
    sdram_dfii_cmdinjector_wrdata_select_write(phase);
    csr_rd_buf_uint8(CSR_SDRAM_DFII_CMDINJECTOR_WRDATA_S_ADDR, data, DFII_CMDINJECTOR_DATA_BYTES);
#endif

    // CSR are read as BIG Endian
    nebo = ((DFII_CMDINJECTOR_DATA_BYTES / BYTES_PER_MODULE) - 1 - module) * BYTES_PER_MODULE;
    pebo = ((DFII_CMDINJECTOR_DATA_BYTES / BYTES_PER_MODULE) - 1 - module) * BYTES_PER_MODULE + BYTES_PER_MODULE/2;
    ibo = 0; // Non zero only if x4 ICs are used
    data[pebo] = (data[pebo]&(~MODULE_BITMASK)) | (wrdata & MODULE_BITMASK);
    ibo = (0x4*BYTES_PER_MODULE) % 8;
    data[nebo] = (data[nebo]&(~(MODULE_BITMASK << ibo))) | ((wrdata >> 8*(BYTES_PER_MODULE/2)) & (MODULE_BITMASK << ibo));
#ifdef SDRAM_PHY_SUBCHANNELS
    if (channel) {
        sdram_dfii_b_cmdinjector_wrdata_select_write(phase);
        csr_wr_buf_uint8(CSR_SDRAM_DFII_B_CMDINJECTOR_WRDATA_ADDR, data, DFII_CMDINJECTOR_DATA_BYTES);
        sdram_dfii_b_cmdinjector_wrdata_store_write(1);
    } else {
        sdram_dfii_a_cmdinjector_wrdata_select_write(phase);
        csr_wr_buf_uint8(CSR_SDRAM_DFII_A_CMDINJECTOR_WRDATA_ADDR, data, DFII_CMDINJECTOR_DATA_BYTES);
        sdram_dfii_a_cmdinjector_wrdata_store_write(1);
    }
#else
    sdram_dfii_cmdinjector_wrdata_select_write(phase);
    csr_wr_buf_uint8(CSR_SDRAM_DFII_CMDINJECTOR_WRDATA_ADDR, data, DFII_CMDINJECTOR_DATA_BYTES);
    sdram_dfii_cmdinjector_wrdata_store_write(1);
#endif
    return;
}

void setup_capture(int channel, int setup) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        sdram_dfii_b_cmdinjector_sample_write(0);
        sdram_dfii_b_cmdinjector_setup_write(setup);
        sdram_dfii_b_cmdinjector_reset_write(1);
    } else {
        sdram_dfii_a_cmdinjector_sample_write(0);
        sdram_dfii_a_cmdinjector_setup_write(setup);
        sdram_dfii_a_cmdinjector_reset_write(1);
    }
#else
    sdram_dfii_cmdinjector_sample_write(0);
    sdram_dfii_cmdinjector_setup_write(setup);
    sdram_dfii_cmdinjector_reset_write(1);
#endif
}

void start_capture(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        sdram_dfii_b_cmdinjector_sample_write(1);
    } else {
        sdram_dfii_a_cmdinjector_sample_write(1);
    }
#else
    sdram_dfii_cmdinjector_sample_write(1);
#endif
}

void stop_capture(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        sdram_dfii_b_cmdinjector_sample_write(0);
    } else {
        sdram_dfii_a_cmdinjector_sample_write(0);
    }
#else
    sdram_dfii_cmdinjector_sample_write(0);
#endif
}

// operation: 0->OR, 1-> AND
uint32_t capture_and_reduce_result(int channel, int operation) {
    uint8_t data[DFII_CMDINJECTOR_DATA_BYTES];
    int i;
#ifdef SDRAM_PHY_SUBCHANNELS
    if (channel) {
        csr_rd_buf_uint8(CSR_SDRAM_DFII_B_CMDINJECTOR_RESULT_ARRAY_ADDR, data, DFII_CMDINJECTOR_DATA_BYTES);
    } else {
        csr_rd_buf_uint8(CSR_SDRAM_DFII_A_CMDINJECTOR_RESULT_ARRAY_ADDR, data, DFII_CMDINJECTOR_DATA_BYTES);
    }
#else
    csr_rd_buf_uint8(CSR_SDRAM_DFII_CMDINJECTOR_RESULT_ARRAY_ADDR, data, DFII_CMDINJECTOR_DATA_BYTES);
#endif
    for (i = 1; i < DFII_CMDINJECTOR_DATA_BYTES; ++i) {
        data[0] = operation ? (data[0] & data[i]) : (data[0] | data[i]);
    }
    if (operation) {
        data[0] &= data[0]>>4;
        data[0] &= data[0]>>2;
        data[0] &= data[0]>>1;
    } else {
        data[0] |= data[0]>>4;
        data[0] |= data[0]>>2;
        data[0] |= data[0]>>1;
    }
    return data[0]&1;
}

uint32_t capture_and_reduce_module(int channel, int module, int operation) {
    uint16_t ret_value;
    int pebo;   // module's positive_edge_byte_offset
    int nebo;   // module's negative_edge_byte_offset, could be undefined if SDR DRAM is used
    int ibo;    // module's in byte offset (x4 ICs)
    uint8_t data[DFII_CMDINJECTOR_DATA_BYTES];
#ifdef SDRAM_PHY_SUBCHANNELS
    if (channel) {
        csr_rd_buf_uint8(CSR_SDRAM_DFII_B_CMDINJECTOR_RESULT_ARRAY_ADDR, data, DFII_CMDINJECTOR_DATA_BYTES);
    } else {
        csr_rd_buf_uint8(CSR_SDRAM_DFII_A_CMDINJECTOR_RESULT_ARRAY_ADDR, data, DFII_CMDINJECTOR_DATA_BYTES);
    }
#else
    csr_rd_buf_uint8(CSR_SDRAM_DFII_CMDINJECTOR_RESULT_ARRAY_ADDR, data, DFII_CMDINJECTOR_DATA_BYTES);
#endif
    ret_value = 0;
    // CSR are read as BIG Endian
    nebo = ((DFII_CMDINJECTOR_DATA_BYTES / BYTES_PER_MODULE) - 1 - module) * BYTES_PER_MODULE;
    pebo = ((DFII_CMDINJECTOR_DATA_BYTES / BYTES_PER_MODULE) - 1 - module) * BYTES_PER_MODULE + BYTES_PER_MODULE/2;

    ibo = 0; // Non zero only if x4 ICs are used
    ret_value |= (data[pebo] >> ibo) & MODULE_BITMASK;
    ibo = (0x4*BYTES_PER_MODULE) % 8;
    ret_value |= ((data[nebo] >> ibo) & MODULE_BITMASK) << SDRAM_PHY_DQ_DQS_RATIO;
    if(operation) {
        ret_value &= ret_value >> (8 * (BYTES_PER_MODULE/2));
        ret_value &= ret_value >> 4;
        ret_value &= ret_value >> 2;
        ret_value &= ret_value >> 1;
    } else {
        ret_value |= ret_value >> (8 * (BYTES_PER_MODULE/2));
        ret_value |= ret_value >> 4;
        ret_value |= ret_value >> 2;
        ret_value |= ret_value >> 1;
    }
    return ret_value&1;
}

int or_sample(int channel) {
    setup_capture(channel, 0);
    cdelay(50);
    start_capture(channel);
    cdelay(1000);
    stop_capture(channel);
    return !capture_and_reduce_result(channel, 0);
}

int and_sample(int channel) {
    setup_capture(channel, 3);
    cdelay(50);
    start_capture(channel);
    cdelay(1000);
    stop_capture(channel);
    return !!capture_and_reduce_result(channel, 1);
}

int wleveling_sample(int channel, int module) {
    setup_capture(channel, 3);
    cdelay(50);
    start_capture(channel);
    cdelay(50);
    stop_capture(channel);
    return !!capture_and_reduce_module(channel, module, 1);
}

void disable_dfi_2n_mode(void) {
    int value = sdram_dfii_control_read();
    value &= ~DFII_CONTROL_2N_MODE;
    sdram_dfii_control_write(value);
    printf("Switching DFI to 1N mode\n");
    N2_mode = 0;
}

void enable_dfi_2n_mode(void) {
    int value = sdram_dfii_control_read();
    value |= DFII_CONTROL_2N_MODE;
    sdram_dfii_control_write(value);
    printf("Switching DFI to 2N mode\n");
    N2_mode = 1;
}

void disable_dram_2n_mode(int channel, int rank) {
    cmd_injector(channel, 1, 1<<rank, 0xf | 0b1001<<5, 0, 0, 0, 1);
    issue_single(channel);
    printf("Switching DRAM on channel:%c rank:%d to 1N mode\n", 'A'+channel, rank);
}

static void phy_select(int channel, int select) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_dly_sel_write(1<<select);
    } else {
        ddrphy_A_dly_sel_write(1<<select);
    }
#else
    ddrphy_dly_sel_write(1<<select);
#endif
}

static void phy_deselect(int channel, int select) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_dly_sel_write(0);
    } else {
        ddrphy_A_dly_sel_write(0);
    }
#else
    ddrphy_dly_sel_write(0);
#endif
}

static void phy_dq_select(int channel, int select) {
#ifdef SDRAM_DELAY_PER_DQ
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_dq_dly_sel_write(1<<select);
    } else {
        ddrphy_A_dq_dly_sel_write(1<<select);
    }
#else
    ddrphy_dq_dly_sel_write(1<<select);
#endif
#endif // SDRAM_DELAY_PER_DQ
}

static void phy_dq_deselect(int channel, int select) {
#ifdef SDRAM_DELAY_PER_DQ
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_dq_B_dly_sel_write(0);
    } else {
        ddrphy_dq_A_dly_sel_write(0);
    }
#else
    ddrphy_dq_dly_sel_write(0);
#endif
#endif // SDRAM_DELAY_PER_DQ
}

static void idly_rst_internal(int channel) {
#ifdef SDRAM_INPUT_DELAY_CAPABLE
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_rdly_dqs_rst_write(1);
    } else {
        ddrphy_A_rdly_dqs_rst_write(1);
    }
#else
    ddrphy_rdly_dqs_rst_write(1);
#endif
#endif // SDRAM_INPUT_DELAY_CAPABLE
}

static void idly_inc_internal(int channel) {
#ifdef SDRAM_INPUT_DELAY_CAPABLE
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_rdly_dqs_inc_write(1);
    } else {
        ddrphy_A_rdly_dqs_inc_write(1);
    }
#else
    ddrphy_rdly_dqs_inc_write(1);
#endif
#endif // SDRAM_INPUT_DELAY_CAPABLE
}

static void idly_dq_rst_internal(int channel) {
#ifdef SDRAM_INPUT_DELAY_CAPABLE
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_rdly_dq_rst_write(1);
    } else {
        ddrphy_A_rdly_dq_rst_write(1);
    }
#else
    ddrphy_rdly_dq_rst_write(1);
#endif
#endif // SDRAM_INPUT_DELAY_CAPABLE
}

static void idly_dq_inc_internal(int channel) {
#ifdef SDRAM_INPUT_DELAY_CAPABLE
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_rdly_dq_inc_write(1);
    } else {
        ddrphy_A_rdly_dq_inc_write(1);
    }
#else
    ddrphy_rdly_dq_inc_write(1);
#endif
#endif // SDRAM_INPUT_DELAY_CAPABLE
}

static void rd_rst_internal(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_ck_rdly_rst_write(1);
    } else {
        ddrphy_A_ck_rdly_rst_write(1);
    }
#else
    ddrphy_ck_rdly_rst_write(1);
#endif
}

static void rd_inc_internal(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_ck_rdly_inc_write(1);
    } else {
        ddrphy_A_ck_rdly_inc_write(1);
    }
#else
    ddrphy_ck_rdly_inc_write(1);
#endif
}

static void odly_dqs_rst_internal(int channel) {
#ifdef SDRAM_OUTPUT_DELAY_CAPABLE
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_wdly_dqs_rst_write(1);
    } else {
        ddrphy_A_wdly_dqs_rst_write(1);
    }
#else
    ddrphy_wdly_dqs_rst_write(1);
#endif
#endif // SDRAM_OUTPUT_DELAY_CAPABLE
}

static void odly_dqs_inc_internal(int channel) {
#ifdef SDRAM_OUTPUT_DELAY_CAPABLE
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_wdly_dqs_inc_write(1);
    } else {
        ddrphy_A_wdly_dqs_inc_write(1);
    }
#else
    ddrphy_wdly_dqs_inc_write(1);
#endif
#endif // SDRAM_OUTPUT_DELAY_CAPABLE
}

static void odly_dm_rst_internal(int channel) {
#ifdef SDRAM_OUTPUT_DELAY_CAPABLE
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_wdly_dm_rst_write(1);
    } else {
        ddrphy_A_wdly_dm_rst_write(1);
    }
#else
    ddrphy_wdly_dm_rst_write(1);
#endif
#endif // SDRAM_OUTPUT_DELAY_CAPABLE
}

static void odly_dm_inc_internal(int channel) {
#ifdef SDRAM_OUTPUT_DELAY_CAPABLE
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_wdly_dm_inc_write(1);
    } else {
        ddrphy_A_wdly_dm_inc_write(1);
    }
#else
    ddrphy_wdly_dm_inc_write(1);
#endif
#endif // SDRAM_OUTPUT_DELAY_CAPABLE
}

static void odly_dq_rst_internal(int channel) {
#ifdef SDRAM_OUTPUT_DELAY_CAPABLE
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_wdly_dq_rst_write(1);
    } else {
        ddrphy_A_wdly_dq_rst_write(1);
    }
#else
    ddrphy_wdly_dq_rst_write(1);
#endif
#endif // SDRAM_OUTPUT_DELAY_CAPABLE
}

static void odly_dq_inc_internal(int channel) {
#ifdef SDRAM_OUTPUT_DELAY_CAPABLE
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_wdly_dq_inc_write(1);
    } else {
        ddrphy_A_wdly_dq_inc_write(1);
    }
#else
    ddrphy_wdly_dq_inc_write(1);
#endif
#endif // SDRAM_OUTPUT_DELAY_CAPABLE
}

static uint16_t get_ca_dly_internal(int channel) {
#ifdef SDRAM_PHY_ADDRESS_DELAY_CAPABLE
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        return ddrphy_B_cadly_read();
    } else {
        return ddrphy_A_cadly_read();
    }
#else
    return ddrphy_cadly_read();
#endif
#else
    return 0;
#endif // SDRAM_PHY_ADDRESS_DELAY_CAPABLE
}

static uint16_t get_rd_dq_dly_internal(int channel) {
#ifdef SDRAM_INPUT_DELAY_CAPABLE
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        return ddrphy_B_rdly_dq_read();
    } else {
        return ddrphy_A_rdly_dq_read();
    }
#else
    return ddrphy_rdly_dq_read();
#endif
#else
    return 0;
#endif // SDRAM_INPUT_DELAY_CAPABLE
}

static uint16_t get_rd_dqs_dly_internal(int channel) {
#ifdef SDRAM_INPUT_DELAY_CAPABLE
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        return ddrphy_B_rdly_dqs_read();
    } else {
        return ddrphy_A_rdly_dqs_read();
    }
#else
    return ddrphy_rdly_dqs_read();
#endif
#else
    return 0;
#endif // SDRAM_INPUT_DELAY_CAPABLE
}

static uint16_t get_wr_dm_dly_internal(int channel) {
#ifdef SDRAM_OUTPUT_DELAY_CAPABLE
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        return ddrphy_B_wdly_dm_read();
    } else {
        return ddrphy_A_wdly_dm_read();
    }
#else
    return ddrphy_wdly_dm_read();
#endif
#else
    return 0;
#endif // SDRAM_OUTPUT_DELAY_CAPABLE
}

static uint16_t get_wr_dq_dly_internal(int channel) {
#ifdef SDRAM_OUTPUT_DELAY_CAPABLE
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        return ddrphy_B_wdly_dq_read();
    } else {
        return ddrphy_A_wdly_dq_read();
    }
#else
    return ddrphy_wdly_dq_read();
#endif
#else
    return 0;
#endif // SDRAM_OUTPUT_DELAY_CAPABLE
}

static uint16_t get_wr_dqs_dly_internal(int channel) {
#ifdef SDRAM_OUTPUT_DELAY_CAPABLE
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        return ddrphy_B_wdly_dqs_read();
    } else {
        return ddrphy_A_wdly_dqs_read();
    }
#else
    return ddrphy_wdly_dqs_read();
#endif
#else
    return 0;
#endif // SDRAM_OUTPUT_DELAY_CAPABLE
}

static void wr_rst_internal(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_ck_wdly_rst_write(1);
    } else {
        ddrphy_A_ck_wdly_rst_write(1);
    }
#else
    ddrphy_ck_wdly_rst_write(1);
#endif
}

static void wr_inc_internal(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_ck_wdly_inc_write(1);
    } else {
        ddrphy_A_ck_wdly_inc_write(1);
    }
#else
    ddrphy_ck_wdly_inc_write(1);
#endif
}

static void wr_dq_rst_internal(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_ck_wddly_rst_write(1);
    } else {
        ddrphy_A_ck_wddly_rst_write(1);
    }
#else
    ddrphy_ck_wddly_rst_write(1);
#endif
}

static void wr_dq_inc_internal(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        ddrphy_B_ck_wddly_inc_write(1);
    } else {
        ddrphy_A_ck_wddly_inc_write(1);
    }
#else
    ddrphy_ck_wddly_inc_write(1);
#endif
}

static int read_captured_preamble_internal(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        return ddrphy_B_preamble_read();
    } else {
        return ddrphy_A_preamble_read();
    }
#else
    return ddrphy_preamble_read();
#endif
}

uint8_t lfsr_next(uint8_t input) {
    uint8_t temp = 0;
    temp |= ((input)    &1) << 7;
    temp |= ((input>>7) &1) << 6;
    temp |= (((input>>6)&1) ^ (input&1)) << 5;
    temp |= (((input>>5)&1) ^ (input&1)) << 4;
    temp |= (((input>>4)&1) ^ (input&1)) << 3;
    temp |= ((input>>3) &1) << 2;
    temp |= ((input>>2) &1) << 1;
    temp |= ((input>>1) &1) << 0;
    return temp;
}

int compare_serial(int channel, int module, uint16_t data, int inv, int select) {
    uint16_t module_data[8];
    int phase;
    int bit, it, _bit;
    for (phase = 0; phase < 8; ++phase) {
        module_data[phase] = get_data_module_phase(channel, module, phase);
#ifdef DEBUG_DDR5
        printf("%d:%x,", phase, module_data[phase]);
#endif
    }
#ifdef DEBUG_DDR5
    printf("\n");
#endif
    for (bit = 0; bit < SDRAM_PHY_DQ_DQS_RATIO; ++bit) {
        for (it = 0; it < 16; ++it) {
            _bit = (module_data[it>>1] >> (bit+((it&1)*SDRAM_PHY_DQ_DQS_RATIO))) & 1;
            if (inv & (1<<bit))
                _bit = !_bit;
            if (_bit != ((data>>it)&1)) {
#ifdef DEBUG_DDR5
                printf("Failed for line:%d bit:%d, expected %d got %d\n", bit, it, (data>>it)&1, _bit);
#endif
                return 0;
            }
        }
    }
    return 1;
}

int compare(int channel, int module, int data0, int data1, int inv, int select) {
    uint16_t module_data[8];
    uint8_t lfsr;
    int phase;
    int bit, it, _bit;
    for (phase = 0; phase < 8; ++phase) {
        module_data[phase] = get_data_module_phase(channel, module, phase);
#ifdef DEBUG_DDR5
        printf("%d:%x,", phase, module_data[phase]);
#endif
    }
#ifdef DEBUG_DDR5
    printf("\n");
#endif
    for (bit = 0; bit < SDRAM_PHY_DQ_DQS_RATIO; ++bit) {
        lfsr = (select & 1<<bit) ? data1 : data0;
        for (it = 0; it < 16; ++it) {
            _bit = (module_data[it>>1] >> (bit+((it&1)*SDRAM_PHY_DQ_DQS_RATIO))) & 1;
            if (inv & (1<<bit))
                _bit = !_bit;
            if (_bit != (lfsr&1)) {
#ifdef DEBUG_DDR5
                printf("Failed for line:%d bit:%d, expected %d got %d\n", bit, it, (lfsr&1), _bit);
#endif
                return 0;
            }
            lfsr = lfsr_next(lfsr);
        }
    }
    return 1;
}

void cs_rst(int channel, int rank, int address) {
#ifdef SDRAM_PHY_ADDRESS_DELAY_CAPABLE
    phy_select(channel, rank);
    /* Reset CS delay */
#ifdef SDRAM_PHY_SUBCHANNELS
    if (channel)
        ddrphy_B_csdly_rst_write(1);
    else
        ddrphy_A_csdly_rst_write(1);
#else
        ddrphy_csdly_rst_write(1);
#endif //SDRAM_PHY_SUBCHANNELS
    phy_deselect(channel, rank);
#endif // SDRAM_PHY_ADDRESS_DELAY_CAPABLE
}

void cs_inc(int channel, int rank, int address) {
#ifdef SDRAM_PHY_ADDRESS_DELAY_CAPABLE
    phy_select(channel, rank);
    /* Increment CS delay */
#ifdef SDRAM_PHY_SUBCHANNELS
    if (channel)
        ddrphy_B_csdly_inc_write(1);
    else
        ddrphy_A_csdly_inc_write(1);
#else
    ddrphy_csdly_inc_write(1);
#endif //SDRAM_PHY_SUBCHANNELS
    phy_deselect(channel, rank);
#endif // SDRAM_PHY_ADDRESS_DELAY_CAPABLE
}

void ca_rst(int channel, int rank, int address) {
#ifdef SDRAM_PHY_ADDRESS_DELAY_CAPABLE
    phy_select(channel, address);
    /* Reset CA delay */
#ifdef SDRAM_PHY_SUBCHANNELS
    if (channel)
        ddrphy_B_cadly_rst_write(1);
    else
        ddrphy_A_cadly_rst_write(1);
#else
    ddrphy_cadly_rst_write(1);
#endif //SDRAM_PHY_SUBCHANNELS
    phy_deselect(channel, address);
#endif // SDRAM_PHY_ADDRESS_DELAY_CAPABLE
}

void ca_inc(int channel, int rank, int address) {
#ifdef SDRAM_PHY_ADDRESS_DELAY_CAPABLE
    phy_select(channel, address);
    /* Increment CA delay */
#ifdef SDRAM_PHY_SUBCHANNELS
    if (channel)
        ddrphy_B_cadly_inc_write(1);
    else
        ddrphy_A_cadly_inc_write(1);
#else
    ddrphy_cadly_inc_write(1);
#endif //SDRAM_PHY_SUBCHANNELS
    phy_deselect(channel, address);
#endif // SDRAM_PHY_ADDRESS_DELAY_CAPABLE
}

uint16_t get_ca_dly(int channel, int rank, int address) {
    uint16_t temp;
    phy_select(channel, address);
    temp = get_ca_dly_internal(channel);
    phy_deselect(channel, address);
    return temp;
}

void ck_rst(int channel, int rank, int address) {
#ifdef SDRAM_PHY_ADDRESS_DELAY_CAPABLE
    phy_select(channel, address);
    /* Reset CK delay */
#ifdef SDRAM_PHY_SUBCHANNELS
    if (channel)
        ddrphy_B_ckdly_rst_write(1);
    else
        ddrphy_A_ckdly_rst_write(1);
#else
    ddrphy_ckdly_rst_write(1);
#endif //SDRAM_PHY_SUBCHANNELS
    phy_deselect(channel, address);
#endif // SDRAM_PHY_ADDRESS_DELAY_CAPABLE
}

void ck_inc(int channel, int rank, int address) {
#ifdef SDRAM_PHY_ADDRESS_DELAY_CAPABLE
    phy_select(channel, address);
    /* Increment CK delay */
#ifdef SDRAM_PHY_SUBCHANNELS
    if (channel)
        ddrphy_B_ckdly_inc_write(1);
    else
        ddrphy_A_ckdly_inc_write(1);
#else
    ddrphy_ckdly_inc_write(1);
#endif //SDRAM_PHY_SUBCHANNELS
    phy_deselect(channel, address);
#endif // SDRAM_PHY_ADDRESS_DELAY_CAPABLE
}

void rd_rst(int channel, int module) {
    phy_select(channel, module);
    rd_rst_internal(channel);
    phy_deselect(channel, module);
}

void rd_inc(int channel, int module) {
    phy_select(channel, module);
    rd_inc_internal(channel);
    phy_deselect(channel, module);
}

void idly_rst(int channel, int module) {
    phy_select(channel, module);
    idly_rst_internal(channel);
    for(int i=0; i < SDRAM_PHY_DQ_DQS_RATIO; ++i) {
        phy_dq_select(channel, i);
        idly_dq_rst_internal(channel);
        phy_dq_deselect(channel, i);
    }
    phy_deselect(channel, module);
}

void idly_inc(int channel, int module) {
    phy_select(channel, module);
    idly_inc_internal(channel);
    for(int i=0; i < SDRAM_PHY_DQ_DQS_RATIO; ++i) {
        phy_dq_select(channel, i);
        idly_dq_inc_internal(channel);
        phy_dq_deselect(channel, i);
    }
    phy_deselect(channel, module);
}

void idly_dq_rst(int channel, int module, int dq_line) {
    phy_dq_select(channel, dq_line);
    phy_select(channel, module);
    idly_dq_rst_internal(channel);
    phy_deselect(channel, module);
    phy_dq_deselect(channel, dq_line);
}

void idly_dq_inc(int channel, int module, int dq_line) {
    phy_dq_select(channel, dq_line);
    phy_select(channel, module);
    idly_dq_inc_internal(channel);
    phy_deselect(channel, module);
    phy_dq_deselect(channel, dq_line);
}

uint16_t get_rd_dq_dly(int channel, int module) {
    uint16_t temp;
    phy_select(channel, module);
    temp = get_rd_dq_dly_internal(channel);
    phy_deselect(channel, module);
    return temp;
}

uint16_t get_rd_dqs_dly(int channel, int module) {
    uint16_t temp;
    phy_select(channel, module);
    temp = get_rd_dqs_dly_internal(channel);
    phy_deselect(channel, module);
    return temp;
}

void wr_dqs_rst(int channel, int module) {
    phy_select(channel, module);
    wr_rst_internal(channel);
    phy_deselect(channel, module);
}

void wr_dqs_inc(int channel, int module) {
    phy_select(channel, module);
    wr_inc_internal(channel);
    phy_deselect(channel, module);
}

void odly_dqs_rst(int channel, int module) {
    phy_select(channel, module);
    odly_dqs_rst_internal(channel);
    phy_deselect(channel, module);
}

void odly_dqs_inc(int channel, int module) {
    phy_select(channel, module);
    odly_dqs_inc_internal(channel);
    phy_deselect(channel, module);
}

uint16_t get_wr_dqs_dly(int channel, int module) {
    uint16_t temp;
    phy_select(channel, module);
    temp = get_wr_dqs_dly_internal(channel);
    phy_deselect(channel, module);
    return temp;
}

void wr_dq_rst(int channel, int module) {
    phy_select(channel, module);
    wr_dq_rst_internal(channel);
    phy_deselect(channel, module);
}

void wr_dq_inc(int channel, int module) {
    phy_select(channel, module);
    wr_dq_inc_internal(channel);
    phy_deselect(channel, module);
}

void odly_dm_rst(int channel, int module) {
    phy_select(channel, module);
    odly_dm_rst_internal(channel);
    phy_deselect(channel, module);
}

void odly_dm_inc(int channel, int module) {
    phy_select(channel, module);
    odly_dm_inc_internal(channel);
    phy_deselect(channel, module);
}

void odly_dq_rst(int channel, int module) {
    phy_select(channel, module);
    for(int i=0; i < SDRAM_PHY_DQ_DQS_RATIO; ++i) {
        phy_dq_select(channel, i);
        odly_dq_rst_internal(channel);
        phy_dq_deselect(channel, i);
    }
    phy_deselect(channel, module);
}

void odly_dq_inc(int channel, int module) {
    phy_select(channel, module);
    for(int i=0; i < SDRAM_PHY_DQ_DQS_RATIO; ++i) {
        phy_dq_select(channel, i);
        odly_dq_inc_internal(channel);
        phy_dq_deselect(channel, i);
    }
    phy_deselect(channel, module);
}

void odly_per_dq_rst(int channel, int module, int dq) {
    phy_select(channel, module);
    phy_dq_select(channel, dq);
    odly_dq_rst_internal(channel);
    phy_dq_deselect(channel, dq);
    phy_deselect(channel, module);
}

void odly_per_dq_inc(int channel, int module, int dq) {
    phy_select(channel, module);
    phy_dq_select(channel, dq);
    odly_dq_inc_internal(channel);
    phy_dq_deselect(channel, dq);
    phy_deselect(channel, module);
}

uint16_t get_wr_dq_dly(int channel, int module) {
    uint16_t temp;
    phy_select(channel, module);
    temp = get_wr_dq_dly_internal(channel);
    phy_deselect(channel, module);
    return temp;
}

uint16_t get_wr_dm_dly(int channel, int module) {
    uint16_t temp;
    phy_select(channel, module);
    temp = get_wr_dm_dly_internal(channel);
    phy_deselect(channel, module);
    return temp;
}

int captured_preamble(int channel, int module) {
    int temp;
    phy_select(channel, module);
    temp = read_captured_preamble_internal(channel);
    phy_deselect(channel, module);
    return temp;
}

uint8_t recover_mrr_value(int channel, int module) {
    uint16_t temp;
    uint8_t ret, i;
    ret = 0;
    for (i = 4; i < 8; ++i){
        temp = get_data_module_phase(channel, module, i);
        ret |= ((temp&1) << ((i-4)*2));
        ret |= (((temp >> SDRAM_PHY_DQ_DQS_RATIO) & 1) << ((i-4)*2 + 1));
    }
    return ret;
}

void setup_enumerate(int channel, int rank, int module) {
    int module_, i;
#ifdef SDRAM_PHY_SUBCHANNELS
    for (module_ = 0; module_ < SDRAM_PHY_MODULES/2; module_++) {
#else
    for (module_ = 0; module_ < SDRAM_PHY_MODULES; module_++) {
#endif // SDRAM_PHY_SUBCHANNELS
        for (i = 0; i < 4; ++i)
            set_data_module_phase(channel, module_, i, 0xffff);
    }
    for (i = 0; i < 4; ++i)
        set_data_module_phase(channel, module, i, 0);
    cmd_injector(channel, 0xf, 0, 0, 1, 0, 0, 0);
    cmd_injector(channel, 0xf, 0, 0xf | ((0x60|(module&0xf))<<5), 1, 0, 0, 0);
    cmd_injector(channel, 0xf, 1<<rank, 0xf | ((0x60|(module&0xf))<<5), 1, 0, 0, 0);
    cmd_injector(channel, 0xf, 0, 0xf | ((0x60|(module&0xf))<<5), 1, 0, 0, 0);
    cdelay(50);
}

void send_mpc(int channel, int rank, int cmd) {
    cmd_injector(channel, 0xf, 0, 0xf | (cmd<<5), 0, 0, 0, 0);
    cmd_injector(channel, 0xf, 1<<rank, 0xf | (cmd<<5), 0, 0, 0, 0);
    cmd_injector(channel, 0xf, 0, 0xf | (cmd<<5), 0, 0, 0, 0);
    cdelay(50);
}

void send_mrw(int channel, int rank, int module, int reg, int value) {
    if (enumerated)
        send_mpc(channel, rank, 0x7<<4|(module&0xF));

    cmd_injector(channel, 1<<0, 1<<rank, 0x5 | (reg<<5), 0, 0, 0, 1);
    if (N2_mode)
        cmd_injector(channel, 1<<1, 0, 0x5 | (reg<<5), 0, 0, 0, 1);
    else
        cmd_injector(channel, 1<<1, 0, value, 0, 0, 0, 1);
    cmd_injector(channel, 1<<2, 0, value, 0, 0, 0, 1);
    cmd_injector(channel, 1<<3, 0, value, 0, 0, 0, 1);
    cmd_injector(channel, 1<<4, 0, 0, 0, 0, 0, 1);
    cmd_injector(channel, 1<<5, 0, 0, 0, 0, 0, 1);
    cmd_injector(channel, 1<<6, 0, 0, 0, 0, 0, 1);
    cmd_injector(channel, 1<<7, 0, 0, 0, 0, 0, 1);
    issue_single(channel);
    cdelay(50);
    send_mpc(channel, rank, 0x7f);
}

void send_mrr(int channel, int rank, int reg) {
    setup_rddata_cnt(channel, 0);
    setup_rddata_cnt(channel, 8);
    cmd_injector(channel, 1<<0, 1<<rank, 0x15 | (reg<<5), 0, 0, 1, 1);
    if (N2_mode)
        cmd_injector(channel, 1<<1, 0, 0x15 | (reg<<5), 0, 0, 1, 1);
    else
        cmd_injector(channel, 1<<1, 0, 0, 0, 0, 1, 1);
    cmd_injector(channel, 1<<2, 0, 0, 0, 0, 1, 1);
    cmd_injector(channel, 1<<3, 0, 0, 0, 0, 1, 1);
    cmd_injector(channel, 1<<4, 0, 0, 0, 0, 1, 1);
    cmd_injector(channel, 1<<5, 0, 0, 0, 0, 1, 1);
    cmd_injector(channel, 1<<6, 0, 0, 0, 0, 1, 1);
    cmd_injector(channel, 1<<7, 0, 0, 0, 0, 1, 1);
    issue_single(channel);
    cdelay(50);
    setup_rddata_cnt(channel, 0);
}

void send_wleveling_write(int channel, int rank) {
    int wr_2 = 0 | (1<<10) | (1<<11); // Second beat of write, no auto precharge, no wr_partial
    cmd_injector(channel, 1<<0, 1<<rank, 0x0D | (1<<5), 1, 0, 0, 1);
    if (N2_mode)
        cmd_injector(channel, 1<<1, 0, 0x0D | (1<<5), 0, 0, 0, 1);
    else
        cmd_injector(channel, 1<<1, 0, wr_2, 0, 0, 0, 1);
    cmd_injector(channel, 1<<2, 0, wr_2, 0, 0, 0, 1);
    cmd_injector(channel, 1<<3, 0, wr_2, 0, 0, 0, 1);
    cmd_injector(channel, 1<<4, 0, 0, 0, 0, 0, 1);
    cmd_injector(channel, 1<<5, 0, 0, 0, 0, 0, 1);
    cmd_injector(channel, 1<<6, 0, 0, 0, 0, 0, 1);
    cmd_injector(channel, 1<<7, 0, 0, 0, 0, 0, 1);
    issue_single(channel);
    cdelay(50);
}

void send_precharge(int channel, int rank) {
    int pre = 0xB; // Precharge all

    cmd_injector(channel, 1<<0, 1<<rank, pre, 0, 0, 0, 1);
    cmd_injector(channel, 0xfe, 0, 0, 0, 0, 0, 1);
    if (N2_mode)
        cmd_injector(channel, 1<<1, 0, pre, 0, 0, 0, 1);
    issue_single(channel);
    cdelay(500);
}

void send_activate(int channel, int rank) {
    int bg  = 0 << 8;
    int ba  = 0 << 6;

    int act_1 = 0x0 | ba | bg;              // Activate bank group 0, bank 0, row 0
    int act_2 = 0;

    cmd_injector(channel, 1<<0, 1<<rank, act_1, 0, 0, 0, 1);
    cmd_injector(channel, 0xfe, 0, 0, 0, 0, 0, 1);
    if (N2_mode)
        cmd_injector(channel, 1<<1, 0, act_1, 0, 0, 0, 1);
    else
        cmd_injector(channel, 1<<1, 0, act_2, 0, 0, 0, 1);
    cmd_injector(channel, 1<<2, 0, act_2, 0, 0, 0, 1);
    cmd_injector(channel, 1<<3, 0, act_2, 0, 0, 0, 1);
    issue_single(channel);
    cdelay(500);
}

void send_write(int channel, int rank) {
    int bg  = 0 << 8;
    int ba  = 0 << 6;
    int col = 0;

    int wr_1 = 0xD | (1 << 5) | ba | bg;    // Write to bank group 0, bank 0
    int wr_2 = col | (1 << 11);             // Second beat of write, with auto precharge, no wr_partial

    send_activate(channel, rank);
    cmd_injector(channel, 1<<0, 1<<rank, wr_1, 1, 0, 0, 1);
    if (N2_mode)
        cmd_injector(channel, 1<<1, 0, wr_1, 1, 0, 0, 1);
    else
        cmd_injector(channel, 1<<1, 0, wr_2, 1, 0, 0, 1);
    cmd_injector(channel, 1<<2, 0, wr_2, 1, 0, 0, 1);
    cmd_injector(channel, 1<<3, 0, wr_2, 1, 0, 0, 1);
    cmd_injector(channel, 1<<4, 0, 0, 1, 0, 0, 1);
    cmd_injector(channel, 1<<5, 0, 0, 1, 0, 0, 1);
    cmd_injector(channel, 1<<6, 0, 0, 1, 0, 0, 1);
    cmd_injector(channel, 1<<7, 0, 0, 1, 0, 0, 1);
    issue_single(channel);
    cdelay(500);
}

void send_write_byte(int channel, int rank, int module, int byte) {
    int bg  = 0 << 8;
    int ba  = 0 << 6;
    int col = 0;

    int wr_1 = 0xD | (1 << 5) | ba | bg;    // Write to bank group 0, bank 0
    int wr_2 = col;                         // Second beat of write, with auto precharge, with wr_partial

    uint32_t mask     = ~((1 << (byte&1)) << (2*module));
    uint8_t  transfer = byte >> 1;
    uint8_t  cmd_r = 0;
    int      cmd = 0;

    send_activate(channel, rank);
    cmd_injector(channel, 1<<0, 1<<rank, wr_1, 1, WRDATA_BITMASK, 0, 1);
    if (N2_mode)
        cmd_injector(channel, 1<<1, 0, wr_1, 1, WRDATA_BITMASK, 0, 1);
    else
        cmd_injector(channel, 1<<1, 0, wr_2, 1, WRDATA_BITMASK, 0, 1);
    cmd_injector(channel, 1<<2, 0, wr_2, 1, WRDATA_BITMASK, 0, 1);
    cmd_injector(channel, 1<<3, 0, wr_2, 1, WRDATA_BITMASK, 0, 1);
    cmd_injector(channel, 1<<4, 0, 0, 1, WRDATA_BITMASK, 0, 1);
    cmd_injector(channel, 1<<5, 0, 0, 1, WRDATA_BITMASK, 0, 1);
    cmd_injector(channel, 1<<6, 0, 0, 1, WRDATA_BITMASK, 0, 1);
    cmd_injector(channel, 1<<7, 0, 0, 1, WRDATA_BITMASK, 0, 1);
    switch(transfer) {
    case 0:
        cmd = wr_1;
        cmd_r = 1<<rank;
        break;
    case 1:
        if (N2_mode) {
            cmd = wr_1;
            break;
        }
    case 2:
    case 3:
        cmd = wr_2;
        break;
    default:
        break;
    }
    cmd_injector(channel, 1<<transfer, cmd_r, cmd, 1, mask, 0, 1);
    issue_single(channel);
    cdelay(500);
}

void send_read(int channel, int rank) {
    int bg  = 0 << 8;
    int ba  = 0 << 6;
    int col = 0;

    int rd_1 = 0x1D | (1 << 5) | ba |bg;    // Read from bank group 0, bank 0
    int rd_2 = col;                         // Second beat of read, with auto precharge

    setup_rddata_cnt(channel, 0);
    setup_rddata_cnt(channel, 8);

    send_activate(channel, rank);
    cmd_injector(channel, 1<<0, 1<<rank, rd_1, 0, 0, 1, 1);
    if (N2_mode)
        cmd_injector(channel, 1<<1, 0, rd_1, 0, 0, 1, 1);
    else
        cmd_injector(channel, 1<<1, 0, rd_2, 0, 0, 1, 1);
    cmd_injector(channel, 1<<2, 0, rd_2, 0, 0, 1, 1);
    cmd_injector(channel, 1<<3, 0, rd_2, 0, 0, 1, 1);
    cmd_injector(channel, 1<<4, 0, 0, 0, 0, 1, 1);
    cmd_injector(channel, 1<<5, 0, 0, 0, 0, 1, 1);
    cmd_injector(channel, 1<<6, 0, 0, 0, 0, 1, 1);
    cmd_injector(channel, 1<<7, 0, 0, 0, 0, 1, 1);
    issue_single(channel);
    cdelay(500);
    setup_rddata_cnt(channel, 0);
}

void enter_cs(int channel, int rank) {
    send_mpc(channel, rank, 1);
}

void exit_cs(int channel, int rank) {
    send_mpc(channel, rank, 0);
}

void cs_sample_prep(int channel, int rank, int address, int l2h) {
    cmd_injector(channel, 0xf, 0, 0x1f, 0, 0, 1, 0);
    cmd_injector(channel, 0xa>>l2h, 1<<rank, 0x1f, 0, 0, 1, 0);
    cdelay(50);
}

void enter_ca(int channel, int rank) {
    send_mpc(channel, rank, 3);
}

void exit_ca(int channel, int rank) {
    cmd_injector(channel, 0xff, 1<<rank, 0x1f, 0, 0, 0, 1);
    issue_single(channel);
    cdelay(50);
}

void ca_sample_prep_current_period(int channel, int rank, int address, int l2h) {
    cmd_injector(channel, 0xf, 0, (!l2h)<<address, 0, 0, 1, 0);
    cmd_injector(channel, 0x1, 1<<rank, l2h<<address, 0, 0, 1, 0);
    cdelay(50);
}

void enter_write_leveling(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        return ddrphy_B_wlevel_en_write(1);
    } else {
        return ddrphy_A_wlevel_en_write(1);
    }
#else
    return ddrphy_wlevel_en_write(1);
#endif
}

void wleveling_scan(int *cycle, int *got, int *start_cycle, int *start_delay,
                    int channel, int rank, int module) {
    int sample, delay, it;
    printf("%2d|", *cycle);
    odly_dqs_rst(channel, module);
    for(delay = 0; delay < SDRAM_PHY_DELAYS; ++delay) {
        sample = 1;
        // Check multiple times, as we can be on the edge of transition
        // Make sure we aren't in meta stable delay
        for (it = 0; it<16; it++) {
             send_wleveling_write(channel, rank);
             sample &= wleveling_sample(channel, module);
         }
         printf("%d", sample);
         if (sample && (*got) == 0) {
             *start_cycle = *cycle;
             *start_delay = delay;
             *got = 1;
         }
         odly_dqs_inc(channel, module);
     }
     printf("|\n");
     ++(*cycle);
     wr_dqs_inc(channel, module);
}

void exit_write_leveling(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        return ddrphy_B_wlevel_en_write(0);
    } else {
        return ddrphy_A_wlevel_en_write(0);
    }
#else
    return ddrphy_wlevel_en_write(0);
#endif
}
#endif // MEMORY_TYPE_DDR5
#endif // LIBLITEDRAM_DDR5_HELPERS_H
