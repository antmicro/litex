#ifndef __SPI_H
#define __SPI_H

#include <generated/csr.h>

typedef enum {
	SPI_XFER_1 = 1,
	SPI_XFER_2 = 2,
	SPI_XFER_4 = 4,
	SPI_XFER_8 = 8,
} spi_xfer_width;

typedef enum {
	SPI_MASK_1 = 1,
	SPI_MASK_2 = 2,
	SPI_MASK_4 = 4,
	SPI_MASK_8 = 8,
} spi_mask;

typedef enum {
	SPI_MODE_MMAP = 0,
	SPI_MODE_MASTER = 1,
} spi_mode;

typedef enum {
	SPI_CS_HIGH = 0,
	SPI_CS_LOW = 1,
} spi_cs_status;

typedef struct {
	uint8_t len;
	spi_xfer_width width;
	spi_mask mask;
} spi_phy_config;

typedef struct {
	uint8_t *rdata;
	uint8_t cmd;
	uint32_t rbytes;
	spi_phy_config phy_cfg;
} spi_transfer_config;

spi_mode spi_get_mode(void);
void spi_set_mode(spi_mode mode);
void spi_cs_trigger(spi_cs_status st);
void spi_configure(spi_phy_config cfg);
void spi_transfer(spi_transfer_config *cfg);
void spi_test(void);

#endif /* __SPI_H */
