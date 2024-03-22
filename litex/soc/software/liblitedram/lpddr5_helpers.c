#include <liblitedram/lpddr5_helpers.h>

#if defined(CSR_SDRAM_BASE) && defined(SDRAM_PHY_LPDDR5)

#include <stdio.h>
#define DFII_PIX_DATA_BYTES SDRAM_PHY_DFI_DATABITS/8

void enter_CK2WCK_leveling(void) {
    ddrphy_wlevel_en_write(1);
    busy_wait_us(1);
    send_mrw(18, DDRX_MR_WRLVL_RESET | 1<<6);
}

bool sample_CK2WCK_shift(void) {
    uint8_t _buff[DFII_PIX_DATA_BYTES];
    uint8_t ans;
    int i;
    ddrphy_wlevel_strobe_write(1);
    sdram_dfii_pi0_address_write(0);
    sdram_dfii_pi0_baddress_write(0);
    sdram_dfii_pi0_command_write(DFII_COMMAND_RDDATA);
    sdram_dfii_pi0_command_issue_write(1);
    busy_wait_ck(20);
    csr_rd_buf_uint32(CSR_SDRAM_DFII_PI0_RDDATA_ADDR, (uint32_t *)_buff, DFII_PIX_DATA_BYTES/4);
    ans = _buff[0];
    for (i=1; i<DFII_PIX_DATA_BYTES; ++i) {
        ans &= _buff[i];
    }
    ans = ans & (ans >> 4);
    ans = ans & (ans >> 2);
    ans = ans & (ans >> 1);
    return ans;
}

void exit_CK2WCK_leveling(void) {
    send_mrw(18, DDRX_MR_WRLVL_RESET);
    busy_wait_us(1);
    ddrphy_wlevel_en_write(0);
}

void send_mrw(uint8_t reg, uint8_t val) {
    sdram_dfii_pi0_address_write(val);
    sdram_dfii_pi0_baddress_write(reg);
    sdram_dfii_pi0_command_write(DFII_COMMAND_RAS|DFII_COMMAND_CAS|DFII_COMMAND_WE|DFII_COMMAND_CS);
    sdram_dfii_pi0_command_issue_write(1);
    busy_wait_us(4);
}

void sdram_read(uint8_t bank, uint16_t row, uint8_t column) {
    sdram_dfii_pi0_address_write(row);
    sdram_dfii_pi0_baddress_write(bank);
    sdram_dfii_pi0_command_write(DFII_COMMAND_RAS|DFII_COMMAND_CS);
    sdram_dfii_pi0_command_issue_write(1);
    busy_wait_us(1);

    sdram_dfii_pi0_address_write(column);
    sdram_dfii_pi0_baddress_write(bank);
    sdram_dfii_pi0_command_write(DFII_COMMAND_CAS|DFII_COMMAND_CS|DFII_COMMAND_RDDATA);
    sdram_dfii_pi0_command_issue_write(1);
    busy_wait_us(1);
}

void sdram_write(uint8_t bank, uint16_t row, uint8_t column, uint8_t value) {
    uint8_t data[DFII_PIX_DATA_BYTES];
    int p, i;
    for(i=0;i<DFII_PIX_DATA_BYTES;i++) {
        if (i&2) {
            data[i] = value;
        } else {
            data[i] = ~value;
        }
        printf("%"PRIx8":", data[i]);
    }
    for(p=0;p<SDRAM_PHY_PHASES;p++) {
            csr_wr_buf_uint8(sdram_dfii_pix_wrdata_addr(p),
                             data,
                             DFII_PIX_DATA_BYTES);
    }

    sdram_dfii_pi0_address_write(row);
    sdram_dfii_pi0_baddress_write(bank);
    sdram_dfii_pi0_command_write(DFII_COMMAND_RAS|DFII_COMMAND_CS);
    sdram_dfii_pi0_command_issue_write(1);
    busy_wait_us(1);

    sdram_dfii_pi0_address_write(column);
    sdram_dfii_pi0_baddress_write(bank);
    sdram_dfii_pi0_command_write(DFII_COMMAND_CAS|DFII_COMMAND_CS|DFII_COMMAND_WRDATA);
    sdram_dfii_pi0_command_issue_write(1);
    busy_wait_us(1);
}

#endif // defined(CSR_SDRAM_BASE) && defined(SDRAM_PHY_DDR5)
