#include <liblitedram/accessors.h>
#include <liblitedram/ddr5_helpers.h>
#ifdef LIBLITEDRAM_DDR5_HELPERS_H
#include <stdio.h>

#ifdef MEMORY_TYPE_DDR5
int prep_payload (int cs, int command, int wrdata_en,
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
                  int wrdata_en, int wrdata_mask, int rddata_en, int single) {
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

uint32_t capture_result(int channel) {
#ifdef SDRAM_PHY_SUBCHANNELS
    if(channel) {
        return sdram_dfii_b_cmdinjector_result_read();
    } else {
        return sdram_dfii_a_cmdinjector_result_read();
    }
#else
    return sdram_dfii_cmdinjector_result_read();
#endif
}

int or_sample(int channel) {
    setup_capture(channel, 0);
    cdelay(100);
    start_capture(channel);
    cdelay(2000);
    stop_capture(channel);
    return !capture_result(channel);
}

int and_sample(int channel) {
    setup_capture(channel, 3);
    cdelay(100);
    start_capture(channel);
    cdelay(2000);
    stop_capture(channel);
    return !!capture_result(channel);
}

void disable_dfi_2n_mode(void) {
    int value = sdram_dfii_control_read();
    value &= ~DFII_CONTROL_2N_MODE;
    sdram_dfii_control_write(value);
    printf("Switching DFI to 1N mode\n");
}

void enable_dfi_2n_mode(void) {
    int value = sdram_dfii_control_read();
    value |= DFII_CONTROL_2N_MODE;
    sdram_dfii_control_write(value);
    printf("Switching DFI to 2N mode\n");
}

void disable_dram_2n_mode(int channel, int rank) {
    cmd_injector(channel, 1, 1<<rank, 0xf | 0b1001<<5, 0, 0, 0, 1);
    issue_single(channel);
    printf("Switching DRAM on channel:%c rank:%d to 1N mode\n", 'A'+channel, rank);
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

void enter_cs(int channel, int rank) {
    cmd_injector(channel, 0xf, 0, 0xf | (1<<5), 0, 0, 0, 0);
    cmd_injector(channel, 0xf, 1<<rank, 0xf | (1<<5), 0, 0, 0, 0);
    cmd_injector(channel, 0xf, 0, 0xf | (1<<5), 0, 0, 0, 0);
    cdelay(100);
}

void exit_cs(int channel, int rank) {
    cmd_injector(channel, 0xf, 0, 0xf, 0, 0, 0, 0);
    cmd_injector(channel, 0xf, 1<<rank, 0xf, 0, 0, 0, 0);
    cmd_injector(channel, 0xf, 0, 0xf, 0, 0, 0, 0);
    cdelay(100);
}

void cs_sample_prep(int channel, int rank, int address, int l2h) {
    cmd_injector(channel, 0xf, 0, 0x1f, 0, 0, 1, 0);
    cmd_injector(channel, 0xa>>l2h, 1<<rank, 0x1f, 0, 0, 1, 0);
    cdelay(100);
}

void enter_ca(int channel, int rank) {
    cmd_injector(channel, 0xf, 0, 0xf | (3<<5), 0, 0, 0, 0);
    cmd_injector(channel, 0xf, 1<<rank, 0xf | (3<<5), 0, 0, 0, 0);
    cmd_injector(channel, 0xf, 0, 0xf | (3<<5), 0, 0, 0, 0);
    cdelay(100);
}

void exit_ca(int channel, int rank) {
    cmd_injector(channel, 0xff, 1<<rank, 0x1f, 0, 0, 0, 1);
    issue_single(channel);
    cdelay(100);
}

void ca_sample_prep_current_period(int channel, int rank, int address, int l2h) {
    cmd_injector(channel, 0xf, 0, (!l2h)<<address, 0, 0, 1, 0);
    cmd_injector(channel, 0x1, 1<<rank, l2h<<address, 0, 0, 1, 0);
    cdelay(100);
}

void ca_sample_prep_previous_period(int channel, int rank, int address, int l2h) {
    cmd_injector(channel, 0xf, 0, (!l2h)<<address, 0, 0, 1, 0);
    cmd_injector(channel, 0x1, 0, l2h<<address, 0, 0, 1, 0);
    cmd_injector(channel, 0x2, 1<<rank, (!l2h)<<address, 0, 0, 1, 0);
    cdelay(100);
}

int32_t _ca_results[14][2];

void setup_ca_results(void) {
    int address;
    for (address = 0; address < 14; address++) {
        _ca_results[address][0] = -SDRAM_PHY_DELAYS; // right
        _ca_results[address][1] = SDRAM_PHY_DELAYS;  // left
    }
}

void mid_point_calc_and_set(uint8_t* success, const char* format_str, int channel,
                            int rank, int address, int32_t index, int32_t left,
                            int32_t right, inc_func inc, int cs) {
    int32_t mid_point, delay;
    printf(format_str, index, left - right);
    mid_point = (left + right) / 2;
    if (mid_point < 0 && cs)
        mid_point += SDRAM_PHY_DELAYS;
    else if (mid_point < 0)
        mid_point = 0;

    printf("Mid point:%"PRId32"\n", mid_point);
    for (delay = 0; delay < mid_point; delay++) {
        inc(channel, rank, address);
    }
    *success &= (left != UNSET_DELAY) && (right != UNSET_DELAY);
}
#endif // MEMORY_TYPE_DDR5
#endif // LIBLITEDRAM_DDR5_HELPERS_H
