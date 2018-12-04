#include <generated/csr.h>
#include <gpio.h>

void gpio_init(void)
{
	gpio_outputs_out_write(0);
}

void gpio_write(unsigned int val)
{
	gpio_outputs_out_write(val);
}
