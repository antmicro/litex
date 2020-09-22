// SPDX-License-Identifier: BSD-Source-Code

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <generated/csr.h>
#include <i2c.h>

#include "../command.h"
#include "../helpers.h"


/**
 * Command "i2creset"
 *
 * Reset I2C line state in case a slave locks the line.
 *
 */
#ifdef CSR_I2C_BASE
define_command(i2creset, i2c_reset, "Reset I2C line state", I2C_CMDS);
#endif

/**
 * Command "i2cwr"
 *
 * Write I2C slave memory using 7-bit slave address and 8-bit memory address.
 *
 */
#ifdef CSR_I2C_BASE
static void i2cwr_handler(int nb_params, char **params)
{
	int i;
	char *c;
	unsigned char write_params[32];  // also indirectly limited by CMD_LINE_BUFFER_SIZE

	if (nb_params < 2) {
		printf("i2cwr <slaveaddr7bit> <addr> [<data>, ...]");
		return;
	}

	if (nb_params - 1 > sizeof(write_params)) {
		printf("Max data length is %d", sizeof(write_params));
		return;
	}

	for (i = 0; i < nb_params; ++i) {
		write_params[i] = strtoul(params[i], &c, 0);
		if (*c != 0) {
			printf("Incorrect value of parameter %d", i);
			return;
		}
	}

	if (!i2c_write(write_params[0], write_params[1], &write_params[2], nb_params - 2)) {
		printf("Error during I2C write");
		return;
	}
}
define_command(i2cwr, i2cwr_handler, "Write over I2C", I2C_CMDS);
#endif

/**
 * Command "i2crd"
 *
 * Read I2C slave memory using 7-bit slave address and 8-bit memory address.
 *
 */
#ifdef CSR_I2C_BASE
static void i2crd_handler(int nb_params, char **params)
{
	char *c;
	int len;
	unsigned char slave_addr, addr;
	unsigned char buf[256];
	bool send_stop = true;

	if (nb_params < 3) {
		printf("i2crd <slaveaddr7bit> <addr> <len> [<send_stop>]");
		return;
	}

	slave_addr = strtoul(params[0], &c, 0);
	if (*c != 0) {
		printf("Incorrect slave address");
		return;
	}

	addr = strtoul(params[1], &c, 0);
	if (*c != 0) {
		printf("Incorrect memory address");
		return;
	}

	len = strtoul(params[2], &c, 0);
	if (*c != 0) {
		printf("Incorrect data length");
		return;
	}
	if (len > sizeof(buf)) {
		printf("Max data count is %d", sizeof(buf));
		return;
	}

	if (nb_params > 3) {
		send_stop = strtoul(params[3], &c, 0) != 0;
		if (*c != 0) {
			printf("Incorrect send_stop value");
			return;
		}
	}

	if (!i2c_read(slave_addr, addr, buf, len, send_stop)) {
		printf("Error during I2C read");
		return;
	}

	dump_bytes((unsigned int *) buf, len, addr);
}
define_command(i2crd, i2crd_handler, "Read over I2C", I2C_CMDS);
#endif

/**
 * Command "i2cscan"
 *
 * Scan for available I2C devices
 *
 */
#ifdef CSR_I2C_BASE
static void i2cscan_handler(int nb_params, char **params)
{
	int slave_addr;

	printf("\n      0 1 2 3 4 5 6 7 8 9 a b c d e f");
	for (slave_addr = 0; slave_addr < 0x80; slave_addr++) {
		if (slave_addr % 0x10 == 0) {
			printf("\n0x%02x  ", (slave_addr/0x10) * 0x10);
		}
		if (i2c_poll(slave_addr)) {
			printf("+ ");
		} else {
			printf(". ");
		}
	}
	printf("\n");
}
define_command(i2cscan, i2cscan_handler, "Scan for I2C slaves", I2C_CMDS);
#endif

#ifdef CSR_I2C_BASE
static void rpc_ddrvcc_read(int nb_params, char **params)
{
	unsigned char vbuck2_a, vbuck2_b;

	i2c_read(0x58, 0xa3, &vbuck2_a, 1, true);
	i2c_read(0x58, 0xb4, &vbuck2_b, 1, true);

#define BUCK_mV(val) (300 + 10 * (val))
	printf("DDRVCC settings:\n");
	printf("  buck2[A] = %d.%d\n", BUCK_mV(vbuck2_a) / 1000, BUCK_mV(vbuck2_a) % 1000);
	printf("  buck2[B] = %d.%d\n", BUCK_mV(vbuck2_b) / 1000, BUCK_mV(vbuck2_b) % 1000);
#undef BUCK_mV
}
define_command(ddrvcc_rd, rpc_ddrvcc_read, "DDRVCC read A/B voltages", I2C_CMDS);
#endif

/*
 * Originally there are 2 voltages controller by R261/R266: A = 1.5V, B = 1.35V,
 * B is used by default on Arty.
 */
#ifdef CSR_I2C_BASE
static void rpc_ddrvcc_swap(int nb_params, char **params)
{
	unsigned int vbuck2_a, vbuck2_b;
	i2c_read(0x58, 0xa3, &vbuck2_a, 1, true);
	i2c_read(0x58, 0xb4, &vbuck2_b, 1, true);
	i2c_write(0x58, 0xa3, &vbuck2_b, 1);
	i2c_write(0x58, 0xb4, &vbuck2_a, 1);
}
define_command(ddrvcc_swp, rpc_ddrvcc_swap, "DDRVCC swap A/B voltages", I2C_CMDS);
#endif

/*
 * Set both voltages to 1.5V (independent of R261/R266 resistors)
 */
#ifdef CSR_I2C_BASE
static void rpc_ddrvcc_15(int nb_params, char **params)
{
	unsigned int vbuck2_15 = 0x78;
	i2c_write(0x58, 0xa3, &vbuck2_15, 1);
	i2c_write(0x58, 0xb4, &vbuck2_15, 1);
}
define_command(ddrvcc_15, rpc_ddrvcc_15, "DDRVCC 1.5V", I2C_CMDS);
#endif

/*
 * Set both voltages to 1.35V (independent of R261/R266 resistors)
 */
#ifdef CSR_I2C_BASE
static void rpc_ddrvcc_135(int nb_params, char **params)
{
	unsigned int vbuck2_135 = 0x69;
	i2c_write(0x58, 0xa3, &vbuck2_135, 1);
	i2c_write(0x58, 0xb4, &vbuck2_135, 1);
}
define_command(ddrvcc_135, rpc_ddrvcc_135, "DDRVCC 1.35V", I2C_CMDS);
#endif

/*
 * DA9062 configuration is not persistent (?) so in order to reset the RPC DRAM chip,
 * we can disable and enable buck 2 voltage.
 */
#ifdef CSR_I2C_BASE
static void rpc_ddrvcc_enable(int nb_params, char **params)
{
	char *c;
	unsigned int en;
	unsigned int buck2_cont;

	if (nb_params < 1) {
		printf("ddrvcc_en <en>");
		return;
	}

	en = strtoul(params[0], &c, 0);
	if (*c != 0 || en > 1) {
		printf("Incorrect value");
		return;
	}

	i2c_read(0x58, 0x20, &buck2_cont, 1, true);
	if (en) {
		buck2_cont |= 1u;
	} else {
		buck2_cont &= ~1u;
	}
	i2c_write(0x58, 0x20, &buck2_cont, 1);
}
define_command(ddrvcc_en, rpc_ddrvcc_enable, "DDRVCC enable/disable", I2C_CMDS);
#endif
