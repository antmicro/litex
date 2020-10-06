// This file is Copyright (c) 2013-2014 Sebastien Bourdeauducq <sb@m-labs.hk>
// This file is Copyright (c) 2014-2019 Florent Kermarrec <florent@enjoy-digital.fr>
// This file is Copyright (c) 2015 Yann Sionneau <ys@m-labs.hk>
// This file is Copyright (c) 2015 whitequark <whitequark@whitequark.org>
// This file is Copyright (c) 2019 Ambroz Bizjak <ambrop7@gmail.com>
// This file is Copyright (c) 2019 Caleb Jamison <cbjamo@gmail.com>
// This file is Copyright (c) 2018 Dolu1990 <charles.papon.90@gmail.com>
// This file is Copyright (c) 2018 Felix Held <felix-github@felixheld.de>
// This file is Copyright (c) 2019 Gabriel L. Somlo <gsomlo@gmail.com>
// This file is Copyright (c) 2018 Jean-François Nguyen <jf@lambdaconcept.fr>
// This file is Copyright (c) 2018 Sergiusz Bazanski <q3k@q3k.org>
// This file is Copyright (c) 2016 Tim 'mithro' Ansell <mithro@mithis.com>
// This file is Copyright (c) 2020 Franck Jullien <franck.jullien@gmail.com>
// This file is Copyright (c) 2020 Antmicro <www.antmicro.com>

// License: BSD

#include <stdio.h>
#include <stdlib.h>
#include <console.h>
#include <string.h>
#include <uart.h>
#include <system.h>
#include <id.h>
#include <irq.h>
#include <crc.h>

#include "boot.h"
#include "readline.h"
#include "helpers.h"
#include "command.h"

#include <generated/csr.h>
#include <generated/soc.h>
#include <generated/mem.h>
#include <generated/git.h>

#include <spiflash.h>

#include <liblitedram/sdram.h>

#include <libliteeth/udp.h>
#include <libliteeth/mdio.h>

#include <liblitespi/spiflash.h>

#include <liblitesdcard/sdcard.h>

static void boot_sequence(void)
{
	if(serialboot()) {
#ifdef FLASH_BOOT_ADDRESS
		flashboot();
#endif
#ifdef ROM_BOOT_ADDRESS
		romboot();
#endif
#if defined(CSR_SPISDCARD_BASE) || defined(CSR_SDCORE_BASE)
		sdcardboot();
#endif
#ifdef CSR_ETHMAC_BASE
#ifdef CSR_ETHPHY_MODE_DETECTION_MODE_ADDR
		eth_mode();
#endif
		netboot();
#endif
		printf("No boot medium found\n");
	}
}

int main(int i, char **c)
{
	char buffer[CMD_LINE_BUFFER_SIZE];
	char *params[MAX_PARAM];
	char *command;
	struct command_struct *cmd;
	int nb_params;
	int sdr_ok;
	int sys_clk_freq_mhz, sys_clk_freq_mhz_frac;

#ifdef CONFIG_CPU_HAS_INTERRUPT
	irq_setmask(0);
	irq_setie(1);
#endif
	uart_init();

	// printf("\n");
	// printf("\e[1m        __   _ __      _  __\e[0m\n");
	// printf("\e[1m       / /  (_) /____ | |/_/\e[0m\n");
	// printf("\e[1m      / /__/ / __/ -_)>  <\e[0m\n");
	// printf("\e[1m     /____/_/\\__/\\__/_/|_|\e[0m\n");
	// printf("\e[1m   Build your hardware, easily!\e[0m\n");
	// printf("\n");
	// printf(" (c) Copyright 2012-2020 Enjoy-Digital\n");
	// printf(" (c) Copyright 2007-2015 M-Labs\n");
	// printf("\n");
	// printf(" BIOS built on "__DATE__" "__TIME__"\n");
	// crcbios();
	// printf("\n");
	// printf(" Migen git sha1: "MIGEN_GIT_SHA1"\n");
	// printf(" LiteX git sha1: "LITEX_GIT_SHA1"\n");
	// printf("\n");
	// printf("--=============== \e[1mSoC\e[0m ==================--\n");
	printf("\n\n\e[1mCPU\e[0m:\t\t%s @ %dMHz\n",
		CONFIG_CPU_HUMAN_NAME,
		CONFIG_CLOCK_FREQUENCY/1000000);
#ifdef CSR_CRG_COUNTERS_RUN_ADDR
	{
		unsigned int ref_start, sys_start, ref_end, sys_end, i;
		crg_counters_run_write(0);
		ref_start = crg_ref_clk_counter_read();
		sys_start = crg_sys_clk_counter_read();
		crg_counters_run_write(1);
		for (i = 0; i < 1000; ++i) {
			__asm__ volatile(CONFIG_CPU_NOP);
		}
		crg_counters_run_write(0);
		ref_end = crg_ref_clk_counter_read();
		sys_end = crg_sys_clk_counter_read();
		// 200MHz ref clk
		sys_clk_freq_mhz = (sys_end - sys_start) * 2000ul / (ref_end - ref_start);
		sys_clk_freq_mhz_frac = sys_clk_freq_mhz%10;
		sys_clk_freq_mhz /= 10;
	}
#endif
	printf("measured:\t\t %d.%dMHz\n", sys_clk_freq_mhz, sys_clk_freq_mhz_frac);
	// printf("\e[1mBUS\e[0m:\t\t%s %d-bit @ %dGiB\n",
	// 	CONFIG_BUS_STANDARD,
	// 	CONFIG_BUS_DATA_WIDTH,
	// 	(1 << (CONFIG_BUS_ADDRESS_WIDTH - 30)));
	// printf("\e[1mCSR\e[0m:\t\t%d-bit data\n",
	// 	CONFIG_CSR_DATA_WIDTH);
	// printf("\e[1mROM\e[0m:\t\t%dKiB\n", ROM_SIZE/1024);
	// printf("\e[1mSRAM\e[0m:\t\t%dKiB\n", SRAM_SIZE/1024);
#ifdef CONFIG_L2_SIZE
	// printf("\e[1mL2\e[0m:\t\t%dKiB\n", CONFIG_L2_SIZE/1024);
#endif
#ifdef MAIN_RAM_SIZE
#ifdef CSR_SDRAM_BASE
	// printf("\e[1mSDRAM\e[0m:\t\t%dKiB %d-bit @ %dMbps/pin\n",
		// MAIN_RAM_SIZE/1024,
		// sdrdatabits(),
		// sdrfreq()/1000000);
#else
	// printf("\e[1mMAIN-RAM\e[0m:\t%dKiB \n", MAIN_RAM_SIZE/1024);
#endif
#endif
	printf("\n");

        sdr_ok = 1;

    // FIXME: this way we prevent the long waiting inside a simulation
#if defined(CSR_SDRAM_BASE) && defined(SET_DDR_VCC_15)
	// printf("--=========== \e[1mDDR voltage\e[0m ==============--\n");
	// configure 1.5V DDRVCC for RPC DRAM chip
	unsigned int j;
	const char *enable = "1", *disable = "0";

	command_dispatcher("ddrvcc_en", 1, &disable);

	printf("DDRVCC = 1.5V\n");
	command_dispatcher("ddrvcc_15", 0, NULL);
	for (j = 0; j < CONFIG_CLOCK_FREQUENCY/2; ++j) {
		__asm__ volatile(CONFIG_CPU_NOP);
	}

	command_dispatcher("ddrvcc_en", 1, &enable);
	printf("\n");
#endif

#if defined(CSR_ETHMAC_BASE) || defined(CSR_SDRAM_BASE)
    // printf("--========== \e[1mInitialization\e[0m ============--\n");
#ifdef CSR_ETHMAC_BASE
	eth_init();
#endif
#if defined(CSR_SDRAM_BASE) && !defined(SDRAM_INIT_DISABLE)
	sdr_ok = sdrinit();
#else
#endif
	if (sdr_ok !=1)
		printf("init failed\n");
	printf("\n");
#endif
#ifdef CSR_SPIFLASH_MMAP_BASE
	spiflash_init();
#endif

	// if(sdr_ok) {
	//   printf("--============== \e[1mBoot\e[0m ==================--\n");
	//   boot_sequence();
	//   printf("\n");
	// }

	// printf("--============= \e[1mConsole\e[0m ================--\n");
#if !defined(TERM_MINI) && !defined(TERM_NO_HIST)
	hist_init();
#endif
	printf("\n%s", PROMPT);
	while(1) {
		readline(buffer, CMD_LINE_BUFFER_SIZE);
		if (buffer[0] != 0) {
			printf("\n");
			nb_params = get_param(buffer, &command, params);
			cmd = command_dispatcher(command, nb_params, params);
			if (!cmd)
				printf("Command not found");
		}
		printf("\n%s", PROMPT);
	}
	return 0;
}
