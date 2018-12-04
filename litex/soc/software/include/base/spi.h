#ifndef __SPI_H
#define __SPI_H

#ifdef __cplusplus
extern "C" {
#endif

void spi_init(void);
unsigned int spi_xfer(unsigned int);
void spi_ss(unsigned short);

#ifdef __cplusplus
}
#endif

#endif
