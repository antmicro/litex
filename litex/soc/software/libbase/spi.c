#include <generated/csr.h>
#include <spi.h>

void spi_init(void)
{
	spi_config_write(0 | 100 << 16 | 100 << 24);
	spi_xfer_write(0 | 8 << 16 | 8 << 24);
}

unsigned int spi_xfer(unsigned int mosi)
{
	spi_mosi_data_write(mosi);
	spi_start_write(1);
	while(spi_active_read());
	return spi_miso_data_read();
}

void spi_ss(unsigned short ss)
{
	unsigned int reg = spi_xfer_read();
	reg &= 0xFFFF0000;
	reg |= ss;
	spi_xfer_write(reg);
}
