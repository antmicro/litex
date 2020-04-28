#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <generated/csr.h>
#include <generated/mem.h>
#include <hw/flags.h>
#include <system.h>

#include "spi.h"

spi_mode spi_get_mode(void)
{
	return (spi_mode)spi_cfg_read();
}

void spi_set_mode(spi_mode mode)
{
	spi_cfg_write((unsigned char)mode);
}

void spi_cs_trigger(spi_cs_status st)
{
        spi_master_cs_write((unsigned char)st);
}

void spi_configure(spi_phy_config cfg)
{
	uint32_t cfg_word = cfg.len | (cfg.width << 8) | (cfg.mask << 16);
	spi_master_phyconfig_write(cfg_word);
}

void spi_transfer(spi_transfer_config *cfg)
{
	spi_configure(cfg->phy_cfg);
	spi_cs_trigger(SPI_CS_LOW);

	spi_master_rxtx_write(cfg->cmd);
	while(!(spi_master_status_read() & 0x2));
	cfg->rdata[0] = (uint8_t)spi_master_rxtx_read();

	for (uint32_t recv = 1; recv < cfg->rbytes; recv++) {
		spi_master_rxtx_write(0xFF);
		while(!(spi_master_status_read() & 0x2));
		cfg->rdata[recv] = (uint8_t)spi_master_rxtx_read();
	}

	spi_cs_trigger(SPI_CS_HIGH);
}

void spi_test(void)
{
}
