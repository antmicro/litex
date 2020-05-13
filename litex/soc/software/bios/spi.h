#ifndef __SPI_H
#define __SPI_H

#include <generated/csr.h>

#define SPI_FLASH_BLOCK_SIZE	256
#define CRC32_ERASED_FLASH	0xFEA8A821

typedef enum {
	SPI_MODE_MMAP = 0,
	SPI_MODE_MASTER = 1,
} spi_mode;

unsigned int spi_frequency_test(void);
void spi_autoconfig(void);

#endif /* __SPI_H */
