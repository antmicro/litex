// SPDX-License-Identifier: BSD-Source-Code

#include <stdio.h>
#include <stdlib.h>
#include <id.h>
#include <crc.h>
#include <system.h>
#include <sim_debug.h>

#include <generated/csr.h>
#include <generated/mem.h>

#include "../command.h"
#include "../helpers.h"

/**
 * Command "help"
 *
 * Print a list of available commands with their help text
 *
 */
static void help_handler(int nb_params, char **params)
{
	struct command_struct * const *cmd;
	int i, not_empty;

	puts("\nLiteX BIOS, available commands:\n");

	for (i = 0; i < NB_OF_GROUPS; i++) {
		not_empty = 0;
		for (cmd = __bios_cmd_start; cmd != __bios_cmd_end; cmd++) {
			if ((*cmd)->group == i) {
				printf("%-16s - %s\n", (*cmd)->name, (*cmd)->help ? (*cmd)->help : "-");
				not_empty = 1;
			}
		}
		if (not_empty)
			printf("\n");
	}
}

define_command(help, help_handler, "Print this help", MISC_CMDS);

/**
 * Command "ident"
 *
 * Identifier of the system
 *
 */
static void ident_helper(int nb_params, char **params)
{
	char buffer[IDENT_SIZE];

	get_ident(buffer);
	printf("Ident: %s", *buffer ? buffer : "-");
}

define_command(ident, ident_helper, "Identifier of the system", SYSTEM_CMDS);

/**
 * Command "reboot"
 *
 * Reboot the system
 *
 */
#ifdef CSR_CTRL_RESET_ADDR
static void reboot(int nb_params, char **params)
{
	ctrl_reset_write(1);
}

define_command(reboot, reboot, "Reboot the system", SYSTEM_CMDS);
#endif

/**
 * Command "uptime"
 *
 * Uptime of the system
 *
 */
#ifdef CSR_TIMER0_UPTIME_CYCLES_ADDR
static void uptime(int nb_params, char **params)
{
	unsigned long uptime;

	timer0_uptime_latch_write(1);
	uptime = timer0_uptime_cycles_read();
	printf("Uptime: %ld sys_clk cycles / %ld seconds",
		uptime,
		uptime/CONFIG_CLOCK_FREQUENCY
	);
}

define_command(uptime, uptime, "Uptime of the system since power-up", SYSTEM_CMDS);
#endif

/**
 * Command "crc"
 *
 * Compute CRC32 over an address range
 *
 */
static void crc(int nb_params, char **params)
{
	char *c;
	unsigned int addr;
	unsigned int length;

	if (nb_params < 2) {
		printf("crc <address> <length>");
		return;
	}

	addr = strtoul(params[0], &c, 0);
	if (*c != 0) {
		printf("Incorrect address");
		return;
	}

	length = strtoul(params[1], &c, 0);
	if (*c != 0) {
		printf("Incorrect length");
		return;
	}

	printf("CRC32: %08x", crc32((unsigned char *)addr, length));
}

define_command(crc, crc, "Compute CRC32 of a part of the address space", MISC_CMDS);

/**
 * Command "flush_cpu_dcache"
 *
 * Flush CPU data cache
 *
 */

define_command(flush_cpu_dcache, flush_cpu_dcache, "Flush CPU data cache", CACHE_CMDS);

/**
 * Command "flush_l2_cache"
 *
 * Flush L2 cache
 *
 */
#ifdef CONFIG_L2_SIZE
define_command(flush_l2_cache, flush_l2_cache, "Flush L2 cache", CACHE_CMDS);
#endif


/**
 * Command "trace"
 *
 * Start/stop simulation trace dump.
 *
 */
#ifdef CSR_SIM_TRACE_BASE
static void cmd_sim_trace(int nb_params, char **params)
{
  sim_trace(!sim_trace_enable_read());
}
define_command(trace, cmd_sim_trace, "Toggle simulation tracing", MISC_CMDS);
#endif

/**
 * Command "finish"
 *
 * Finish simulation.
 *
 */
#ifdef CSR_SIM_FINISH_BASE
static void cmd_sim_finish(int nb_params, char **params)
{
  sim_finish();
}
define_command(finish, cmd_sim_finish, "Finish simulation", MISC_CMDS);
#endif

/**
 * Command "mark"
 *
 * Set a debug marker value
 *
 */
#ifdef CSR_SIM_MARKER_BASE
static void cmd_sim_mark(int nb_params, char **params)
{
  // cannot use param[1] as it is not a const string
  sim_mark(NULL);
}
define_command(mark, cmd_sim_mark, "Set a debug simulation marker", MISC_CMDS);
#endif

// module: MT41K64M16, 8 banks, 1024 columns
#define COLBITS 10
#define BANKBITS 3
#define ROW_ADDR(x) (MAIN_RAM_BASE + ((x) << (COLBITS + BANKBITS + 2)))

/**
 * Command "rowhammer"
 */
static void rowhammer(int nb_params, char **params)
{
	char *c;
	unsigned int length, i;

	if (nb_params < 1) {
		printf("rowhammer <length>");
		return;
	}
	length = strtoul(params[0], &c, 0);
	if (*c != 0) {
		printf("Incorrect length");
		return;
	}

	sim_mark_func();
	sim_trace(1);

	for (i = 0; i < length; ++i) {
		*((volatile unsigned int *) ROW_ADDR(1));
		*((volatile unsigned int *) ROW_ADDR(2));
// 		flush_cpu_dcache();
// #ifdef CONFIG_L2_SIZE
// 		flush_l2_cache();
// #endif
	}

	sim_trace(0);
}
define_command(rowhammer, rowhammer, "Row Hammer software version", MISC_CMDS);

static void rowhammer_dma(int nb_params, char **params)
{
	char *c;
	unsigned int delay;

	if (nb_params < 1) {
		printf("rowhammer_dma <delay>");
		return;
	}
	delay = strtoul(params[0], &c, 0);
	if (*c != 0) {
		printf("Incorrect delay");
		return;
	}

	sim_mark_func();
	sim_trace(1);

	rowhammer_enabled_write(1);
	volatile unsigned int i = 0;
	while (i++ < delay);
	rowhammer_enabled_write(0);

	sim_trace(0);
}
define_command(rowhammer_dma, rowhammer_dma, "Row Hammer DMA version", MISC_CMDS);
