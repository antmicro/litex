// This file is Copyright (c) 2013-2020 Florent Kermarrec <florent@enjoy-digital.fr>
// This file is Copyright (c) 2013-2014 Sebastien Bourdeauducq <sb@m-labs.hk>
// This file is Copyright (c) 2018 Chris Ballance <chris.ballance@physics.ox.ac.uk>
// This file is Copyright (c) 2018 Dolu1990 <charles.papon.90@gmail.com>
// This file is Copyright (c) 2019 Gabriel L. Somlo <gsomlo@gmail.com>
// This file is Copyright (c) 2018 Jean-François Nguyen <jf@lambdaconcept.fr>
// This file is Copyright (c) 2018 Sergiusz Bazanski <q3k@q3k.org>
// This file is Copyright (c) 2018 Tim 'mithro' Ansell <me@mith.ro>
// This file is Copyright (c) 2021 Antmicro <www.antmicro.com>
// License: BSD

#include <generated/csr.h>
#include <generated/mem.h>

#include <stdio.h>
#include <stdlib.h>

#include <libbase/memtest.h>
#include <libbase/lfsr.h>

#ifdef CSR_SDRAM_BASE
#include <generated/sdram_phy.h>
#endif
#include <generated/mem.h>
#include <system.h>

#include <liblitedram/sdram.h>
#include <liblitedram/sdram_dbg.h>

//#define SDRAM_TEST_DISABLE
//#define SDRAM_WRITE_LEVELING_CMD_DELAY_DEBUG
//#define SDRAM_WRITE_LATENCY_CALIBRATION_DEBUG
//#define SDRAM_LEVELING_SCAN_DISPLAY_HEX_DIV 10

#ifdef CSR_SDRAM_BASE

/*-----------------------------------------------------------------------*/
/* Helpers                                                               */
/*-----------------------------------------------------------------------*/

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

__attribute__((unused)) void cdelay(int i)
{
#ifndef CONFIG_BIOS_NO_DELAYS
	while(i > 0) {
		__asm__ volatile(CONFIG_CPU_NOP);
		i--;
	}
#endif
}

/*-----------------------------------------------------------------------*/
/* Constants                                                             */
/*-----------------------------------------------------------------------*/

#define DFII_PIX_DATA_BYTES SDRAM_PHY_DFI_DATABITS/8

int sdram_get_databits(void) {
	return SDRAM_PHY_DATABITS;
}

int sdram_get_freq(void) {
	return SDRAM_PHY_XDR*SDRAM_PHY_PHASES*CONFIG_CLOCK_FREQUENCY;
}

int sdram_get_cl(void) {
#ifdef SDRAM_PHY_CL
	return SDRAM_PHY_CL;
#else
	return -1;
#endif
}

int sdram_get_cwl(void) {
#ifdef SDRAM_PHY_CWL
	return SDRAM_PHY_CWL;
#else
	return -1;
#endif
}

/*-----------------------------------------------------------------------*/
/* DFII                                                                  */
/*-----------------------------------------------------------------------*/

#ifdef CSR_DDRPHY_BASE

static unsigned char sdram_dfii_get_rdphase(void) {
#ifdef CSR_DDRPHY_RDPHASE_ADDR
	return ddrphy_rdphase_read();
#else
	return SDRAM_PHY_RDPHASE;
#endif
}

static unsigned char sdram_dfii_get_wrphase(void) {
#ifdef CSR_DDRPHY_WRPHASE_ADDR
	return ddrphy_wrphase_read();
#else
	return SDRAM_PHY_WRPHASE;
#endif
}

static void sdram_dfii_pix_address_write(unsigned char phase, unsigned int value) {
#if (SDRAM_PHY_PHASES > 8)
	#error "More than 8 DFI phases not supported"
#endif
	switch (phase) {
#if (SDRAM_PHY_PHASES > 4)
	case 7: sdram_dfii_pi7_address_write(value); break;
	case 6: sdram_dfii_pi6_address_write(value); break;
	case 5: sdram_dfii_pi5_address_write(value); break;
	case 4: sdram_dfii_pi4_address_write(value); break;
#endif
#if (SDRAM_PHY_PHASES > 2)
	case 3: sdram_dfii_pi3_address_write(value); break;
	case 2: sdram_dfii_pi2_address_write(value); break;
#endif
#if (SDRAM_PHY_PHASES > 1)
	case 1: sdram_dfii_pi1_address_write(value); break;
#endif
	default: sdram_dfii_pi0_address_write(value);
	}
}

static void sdram_dfii_pird_address_write(unsigned int value) {
	unsigned char rdphase = sdram_dfii_get_rdphase();
	sdram_dfii_pix_address_write(rdphase, value);
}

static void sdram_dfii_piwr_address_write(unsigned int value) {
	unsigned char wrphase = sdram_dfii_get_wrphase();
	sdram_dfii_pix_address_write(wrphase, value);
}

static void sdram_dfii_pix_baddress_write(unsigned char phase, unsigned int value) {
#if (SDRAM_PHY_PHASES > 8)
	#error "More than 8 DFI phases not supported"
#endif
	switch (phase) {
#if (SDRAM_PHY_PHASES > 4)
	case 7: sdram_dfii_pi7_baddress_write(value); break;
	case 6: sdram_dfii_pi6_baddress_write(value); break;
	case 5: sdram_dfii_pi5_baddress_write(value); break;
	case 4: sdram_dfii_pi4_baddress_write(value); break;
#endif
#if (SDRAM_PHY_PHASES > 2)
	case 3: sdram_dfii_pi3_baddress_write(value); break;
	case 2: sdram_dfii_pi2_baddress_write(value); break;
#endif
#if (SDRAM_PHY_PHASES > 1)
	case 1: sdram_dfii_pi1_baddress_write(value); break;
#endif
	default: sdram_dfii_pi0_baddress_write(value);
	}
}

static void sdram_dfii_pird_baddress_write(unsigned int value) {
	unsigned char rdphase = sdram_dfii_get_rdphase();
	sdram_dfii_pix_baddress_write(rdphase, value);
}

static void sdram_dfii_piwr_baddress_write(unsigned int value) {
	unsigned char wrphase = sdram_dfii_get_wrphase();
	sdram_dfii_pix_baddress_write(wrphase, value);
}

static void command_px(unsigned char phase, unsigned int value) {
#if (SDRAM_PHY_PHASES > 8)
	#error "More than 8 DFI phases not supported"
#endif
	switch (phase) {
#if (SDRAM_PHY_PHASES > 4)
	case 7: command_p7(value); break;
	case 6: command_p6(value); break;
	case 5: command_p5(value); break;
	case 4: command_p4(value); break;
#endif
#if (SDRAM_PHY_PHASES > 2)
	case 3: command_p3(value); break;
	case 2: command_p2(value); break;
#endif
#if (SDRAM_PHY_PHASES > 1)
	case 1: command_p1(value); break;
#endif
	default: command_p0(value);
	}
}

static void command_prd(unsigned int value) {
	unsigned char rdphase = sdram_dfii_get_rdphase();
	command_px(rdphase, value);
}

static void command_pwr(unsigned int value) {
	unsigned char wrphase = sdram_dfii_get_wrphase();
	command_px(wrphase, value);
}

#endif

/*-----------------------------------------------------------------------*/
/* Software/Hardware Control                                             */
/*-----------------------------------------------------------------------*/

#define DFII_CONTROL_SOFTWARE (DFII_CONTROL_CKE|DFII_CONTROL_ODT|DFII_CONTROL_RESET_N)
#define DFII_CONTROL_HARDWARE (DFII_CONTROL_SEL)

void sdram_software_control_on(void)
{
	unsigned int previous;
	previous = sdram_dfii_control_read();
	/* Switch DFII to software control */
	if (previous != DFII_CONTROL_SOFTWARE) {
		sdram_dfii_control_write(DFII_CONTROL_SOFTWARE);
		printf("Switching SDRAM to software control.\n");
	}

#if CSR_DDRPHY_EN_VTC_ADDR
	/* Disable Voltage/Temperature compensation */
	ddrphy_en_vtc_write(0);
#endif
}

void sdram_software_control_off(void)
{
	unsigned int previous;
	previous = sdram_dfii_control_read();
	/* Switch DFII to hardware control */
	if (previous != DFII_CONTROL_HARDWARE) {
		sdram_dfii_control_write(DFII_CONTROL_HARDWARE);
		printf("Switching SDRAM to hardware control.\n");
	}
#if CSR_DDRPHY_EN_VTC_ADDR
	/* Enable Voltage/Temperature compensation */
	ddrphy_en_vtc_write(1);
#endif
}

/*-----------------------------------------------------------------------*/
/*  Mode Register                                                        */
/*-----------------------------------------------------------------------*/

void sdram_mode_register_write(char reg, int value) {
	sdram_dfii_pi0_address_write(value);
	sdram_dfii_pi0_baddress_write(reg);
	command_p0(DFII_COMMAND_RAS|DFII_COMMAND_CAS|DFII_COMMAND_WE|DFII_COMMAND_CS);
}

#if 0 && defined(CSR_DDRPHY_BASE)

/*-----------------------------------------------------------------------*/
/* Leveling Centering (Common for Read/Write Leveling)                   */
/*-----------------------------------------------------------------------*/

typedef void (*delay_callback)(int module, int dq_line);

static void sdram_activate_test_row(void) {
	sdram_dfii_pi0_address_write(0);
	sdram_dfii_pi0_baddress_write(0);
	command_p0(DFII_COMMAND_RAS|DFII_COMMAND_CS);
	cdelay(15);
}

static void sdram_precharge_test_row(void) {
	sdram_dfii_pi0_address_write(0);
	sdram_dfii_pi0_baddress_write(0);
	command_p0(DFII_COMMAND_RAS|DFII_COMMAND_WE|DFII_COMMAND_CS);
	cdelay(15);
}

// Count number of bits in a 32-bit word, faster version than a while loop
// see: https://www.johndcook.com/blog/2020/02/21/popcount/
static unsigned int popcount(unsigned int x) {
	x -= ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	x = (x + (x >> 4)) & 0x0F0F0F0F;
	x += (x >> 8);
	x += (x >> 16);
	return x & 0x0000003F;
}

static void print_scan_errors(unsigned int errors) {
#ifdef SDRAM_LEVELING_SCAN_DISPLAY_HEX_DIV
	// Display '.' for no errors, errors/div in hex if it is a single char, else show 'X'
	errors = errors / SDRAM_LEVELING_SCAN_DISPLAY_HEX_DIV;
	if (errors == 0)
		printf(".");
	else if (errors > 0xf)
		printf("X");
	else
		printf("%x", errors);
#else
		printf("%d", errors == 0);
#endif
}

#define READ_CHECK_TEST_PATTERN_MAX_ERRORS (8*SDRAM_PHY_PHASES*DFII_PIX_DATA_BYTES/SDRAM_PHY_MODULES)
#define MODULE_BITMASK ((1<<SDRAM_PHY_DQ_DQS_RATIO)-1)

static unsigned int sdram_write_read_check_test_pattern(int module, unsigned int seed, int dq_line) {
	int p, i, bit;
	unsigned int errors;
	unsigned int prv;
	unsigned char value;
	unsigned char tst[DFII_PIX_DATA_BYTES];
	unsigned char prs[SDRAM_PHY_PHASES][DFII_PIX_DATA_BYTES];

	/* Generate pseudo-random sequence */
	prv = seed;
	for(p=0;p<SDRAM_PHY_PHASES;p++) {
		for(i=0;i<DFII_PIX_DATA_BYTES;i++) {
			value = 0;
			for (bit=0;bit<8;bit++) {
				prv = lfsr(32, prv);
				value |= (prv&1) << bit;
			}
			prs[p][i] = value;
		}
	}

	/* Activate */
	sdram_activate_test_row();

	/* Write pseudo-random sequence */
	for(p=0;p<SDRAM_PHY_PHASES;p++) {
		csr_wr_buf_uint8(sdram_dfii_pix_wrdata_addr(p), prs[p], DFII_PIX_DATA_BYTES);
	}
	sdram_dfii_piwr_address_write(0);
	sdram_dfii_piwr_baddress_write(0);
	command_pwr(DFII_COMMAND_CAS|DFII_COMMAND_WE|DFII_COMMAND_CS|DFII_COMMAND_WRDATA);
	cdelay(15);

#ifdef SDRAM_PHY_ECP5DDRPHY
	ddrphy_burstdet_clr_write(1);
#endif

	/* Read/Check pseudo-random sequence */
	sdram_dfii_pird_address_write(0);
	sdram_dfii_pird_baddress_write(0);
	command_prd(DFII_COMMAND_CAS|DFII_COMMAND_CS|DFII_COMMAND_RDDATA);
	cdelay(15);

	/* Precharge */
	sdram_precharge_test_row();

	errors = 0;
	for(p=0;p<SDRAM_PHY_PHASES;p++) {
		/* Read back test pattern */
		csr_rd_buf_uint8(sdram_dfii_pix_rddata_addr(p), tst, DFII_PIX_DATA_BYTES);
		/* Verify bytes matching current 'module' */
		int positive_edge_byte_offset;
		int negative_edge_byte_offset;
		int in_byte_offset;
		int mask;

#ifdef SDRAM_DELAY_PER_DQ
		mask = 1 << dq_line;
#else
		mask = MODULE_BITMASK;
#endif

		/* Values written into CSR are Big Endian */
		positive_edge_byte_offset = (DFII_PIX_DATA_BYTES/2) - 1 - (module * SDRAM_PHY_DQ_DQS_RATIO)/8;
		negative_edge_byte_offset = positive_edge_byte_offset + DFII_PIX_DATA_BYTES / 2;
		if ((DFII_PIX_DATA_BYTES/2) == 0) {
			positive_edge_byte_offset = 0;
			negative_edge_byte_offset = 0;
		}
		in_byte_offset = (module * SDRAM_PHY_DQ_DQS_RATIO)%8;
		errors += popcount((prs[p][positive_edge_byte_offset] & (mask << in_byte_offset)) ^
		                   (tst[positive_edge_byte_offset] & (mask << in_byte_offset)));
		if (SDRAM_PHY_DQ_DQS_RATIO == 16)
			errors += popcount((prs[p][positive_edge_byte_offset+1] & (mask << in_byte_offset)) ^
			                   (tst[positive_edge_byte_offset+1] & (mask << in_byte_offset)));

		if (DFII_PIX_DATA_BYTES == 1) // Special case for x4 single IC
			in_byte_offset = 0x4;
		errors += popcount((prs[p][negative_edge_byte_offset] & (mask << in_byte_offset)) ^
		                   (tst[negative_edge_byte_offset] & (mask << in_byte_offset)));
		if (SDRAM_PHY_DQ_DQS_RATIO == 16)
			errors += popcount((prs[p][negative_edge_byte_offset+1] & (mask << in_byte_offset)) ^
			                   (tst[negative_edge_byte_offset+1] & (mask << in_byte_offset)));
	}

#ifdef SDRAM_PHY_ECP5DDRPHY
	if (((ddrphy_burstdet_seen_read() >> module) & 0x1) != 1)
		errors += 1;
#endif

	return errors;
}

static void sdram_leveling_center_module(
	int module, int show_short, int show_long, delay_callback rst_delay, delay_callback inc_delay, int dq_line)
{
	int i;
	int show;
	int working, last_working;
	unsigned int errors;
	int delay, delay_mid, delay_range;
	int delay_min = -1, delay_max = -1;

	if (show_long)
#ifdef SDRAM_DELAY_PER_DQ
		printf("m%d dq_line:%d: |", module, dq_line);
#else
		printf("m%d: |", module);
#endif

	/* Find smallest working delay */
	delay = 0;
	last_working = 0;
	rst_delay(module, dq_line);
	while(1) {
		errors  = sdram_write_read_check_test_pattern(module, 42, dq_line);
		errors += sdram_write_read_check_test_pattern(module, 84, dq_line);
		errors += sdram_write_read_check_test_pattern(module, 36, dq_line);
		errors += sdram_write_read_check_test_pattern(module, 72, dq_line);
		errors += sdram_write_read_check_test_pattern(module, 24, dq_line);
		errors += sdram_write_read_check_test_pattern(module, 48, dq_line);
		working = errors == 0;
		show = show_long;
#if SDRAM_PHY_DELAYS > 32
		show = show && (delay%16 == 0);
#endif
		if (show)
			print_scan_errors(errors);
		if(working && last_working && delay_min < 0) {
			delay_min = delay - 1; // delay on edges can be spotty
			break;
		}
		last_working = working;
		delay++;
		if(delay >= SDRAM_PHY_DELAYS)
			break;
		inc_delay(module, dq_line);
	}

	/* Get a bit further into the working zone */
/*
#if SDRAM_PHY_DELAYS > 32
	#define	SDRAM_PHY_DELAY_JUMP 16
#elif SDRAM_PHY_DELAYS > 8
	#define SDRAM_PHY_DELAY_JUMP 4
#else
	#define SDRAM_PHY_DELAY_JUMP 1
#endif
	for(i=0;i<SDRAM_PHY_DELAY_JUMP;i++) {
		delay += 1;
		inc_delay(module);
	}
*/
	/* Find largest working delay */
	while(1) {
		errors  = sdram_write_read_check_test_pattern(module, 42, dq_line);
		errors += sdram_write_read_check_test_pattern(module, 84, dq_line);
		errors += sdram_write_read_check_test_pattern(module, 36, dq_line);
		errors += sdram_write_read_check_test_pattern(module, 72, dq_line);
		errors += sdram_write_read_check_test_pattern(module, 24, dq_line);
		errors += sdram_write_read_check_test_pattern(module, 48, dq_line);
		working = errors == 0;
		show = show_long;
#if SDRAM_PHY_DELAYS > 32
		show = show && (delay%16 == 0);
#endif
		if (show)
			print_scan_errors(errors);
		if(!working && delay_max < 0) {
			delay_max = delay;
		}
		delay++;
		if(delay >= SDRAM_PHY_DELAYS)
			break;
		inc_delay(module, dq_line);
	}
	if(delay_max < 0) {
		delay_max = delay;
	}

	if (show_long)
		printf("| ");

	delay_mid   = (delay_min+delay_max)/2 % SDRAM_PHY_DELAYS;
	delay_range = (delay_max-delay_min)/2;
	if (show_short) {
		if (delay_min < 0)
			printf("delays: -");
		else
			printf("delays: %02d+-%02d", delay_mid, delay_range);
	}

	if (show_long)
		printf("\n");

	/* Set delay to the middle and check */
	if (delay_min >= 0) {
		int retries = 8; /* Do N configs/checks and give up if failing */
		while (retries > 0) {
			/* Set delay. */
			rst_delay(module, dq_line);
			cdelay(100);
			for(i = 0; i < delay_mid; i++) {
				inc_delay(module, dq_line);
				cdelay(100);
			}

			/* Check */
			errors  = sdram_write_read_check_test_pattern(module, 42, dq_line);
			errors += sdram_write_read_check_test_pattern(module, 84, dq_line);
			errors += sdram_write_read_check_test_pattern(module, 36, dq_line);
			errors += sdram_write_read_check_test_pattern(module, 72, dq_line);
			errors += sdram_write_read_check_test_pattern(module, 24, dq_line);
			errors += sdram_write_read_check_test_pattern(module, 48, dq_line);
			if (errors == 0)
				break;
			retries--;
		}
	}
}

/*-----------------------------------------------------------------------*/
/* Write Leveling                                                        */
/*-----------------------------------------------------------------------*/

int _sdram_tck_taps;
int _sdram_write_leveling_bitslips[16];

#ifdef SDRAM_PHY_WRITE_LEVELING_CAPABLE

int _sdram_write_leveling_cmd_scan  = 1;
int _sdram_write_leveling_cmd_delay = 0;
int _sdram_write_leveling_dat_delays[16];

int _sdram_write_leveling_cdly_range_start = -1;
int _sdram_write_leveling_cdly_range_end   = -1;

static void sdram_write_leveling_on(void)
{
	// Flip write leveling bit in the Mode Register, as it is disabled by default
	sdram_dfii_pi0_address_write(DDRX_MR_WRLVL_RESET ^ (1 << DDRX_MR_WRLVL_BIT));
	sdram_dfii_pi0_baddress_write(DDRX_MR_WRLVL_ADDRESS);
	command_p0(DFII_COMMAND_RAS|DFII_COMMAND_CAS|DFII_COMMAND_WE|DFII_COMMAND_CS);

#ifdef SDRAM_PHY_DDR4_RDIMM
	sdram_dfii_pi0_address_write((DDRX_MR_WRLVL_RESET ^ (1 << DDRX_MR_WRLVL_BIT)) ^ 0x2BF8) ;
	sdram_dfii_pi0_baddress_write(DDRX_MR_WRLVL_ADDRESS ^ 0xF);
	command_p0(DFII_COMMAND_RAS|DFII_COMMAND_CAS|DFII_COMMAND_WE|DFII_COMMAND_CS);
#endif

	ddrphy_wlevel_en_write(1);
}

static void sdram_write_leveling_off(void)
{
	sdram_dfii_pi0_address_write(DDRX_MR_WRLVL_RESET);
	sdram_dfii_pi0_baddress_write(DDRX_MR_WRLVL_ADDRESS);
	command_p0(DFII_COMMAND_RAS|DFII_COMMAND_CAS|DFII_COMMAND_WE|DFII_COMMAND_CS);

#ifdef SDRAM_PHY_DDR4_RDIMM
	sdram_dfii_pi0_address_write(DDRX_MR_WRLVL_RESET ^ 0x2BF8);
	sdram_dfii_pi0_baddress_write(DDRX_MR_WRLVL_ADDRESS ^ 0xF);
	command_p0(DFII_COMMAND_RAS|DFII_COMMAND_CAS|DFII_COMMAND_WE|DFII_COMMAND_CS);
#endif

	ddrphy_wlevel_en_write(0);
}

void sdram_write_leveling_rst_cmd_delay(int show) {
	_sdram_write_leveling_cmd_scan = 1;
	if (show)
		printf("Reseting Cmd delay\n");
}

void sdram_write_leveling_force_cmd_delay(int taps, int show) {
	int i;
	_sdram_write_leveling_cmd_scan  = 0;
	_sdram_write_leveling_cmd_delay = taps;
	if (show)
		printf("Forcing Cmd delay to %d taps\n", taps);
	ddrphy_cdly_rst_write(1);
	cdelay(100);
	for (i=0; i<taps; i++) {
		ddrphy_cdly_inc_write(1);
		cdelay(100);
	}
}

void sdram_write_leveling_rst_dat_delay(int module, int show) {
	_sdram_write_leveling_dat_delays[module] = -1;
	if (show)
		printf("Reseting Dat delay of module %d\n", module);
}

void sdram_write_leveling_force_dat_delay(int module, int taps, int show) {
	_sdram_write_leveling_dat_delays[module] = taps;
	if (show)
		printf("Forcing Dat delay of module %d to %d taps\n", module, taps);
}

void sdram_write_leveling_rst_bitslip(int module, int show) {
	_sdram_write_leveling_bitslips[module] = -1;
	if (show)
		printf("Reseting Bitslip of module %d\n", module);
}

void sdram_write_leveling_force_bitslip(int module, int bitslip, int show) {
	_sdram_write_leveling_bitslips[module] = bitslip;
	if (show)
		printf("Forcing Bitslip of module %d to %d\n", module, bitslip);
}

static void sdram_write_leveling_rst_delay(int module, int dq_line) {
	/* Select module */
	ddrphy_dly_sel_write(1 << module);

#ifdef SDRAM_DELAY_PER_DQ
	/* Select DQ line */
	ddrphy_dq_dly_sel_write(1 << dq_line);
#endif

	/* Reset DQ delay */
	ddrphy_wdly_dq_rst_write(1);

#if defined(SDRAM_PHY_USDDRPHY) || defined(SDRAM_PHY_USPDDRPHY)
	/* Reset DQS delay */
	while (ddrphy_wdly_dqs_inc_count_read() != 0) {
		ddrphy_wdly_dqs_inc_write(1);
		cdelay(100);
	}
#else
	/* Reset DQ/DQS delay */
	ddrphy_wdly_dq_rst_write(1);
	ddrphy_wdly_dqs_rst_write(1);
	cdelay(100);
#endif

	/* Un-select module */
	ddrphy_dly_sel_write(0);

#ifdef SDRAM_DELAY_PER_DQ
	/* Un-select DQ line */
	ddrphy_dq_dly_sel_write(0);
#endif
}

static void sdram_write_leveling_inc_delay(int module, int dq_line) {
	/* Select module */
	ddrphy_dly_sel_write(1 << module);

#ifdef SDRAM_DELAY_PER_DQ
	/* Select DQ line */
	ddrphy_dq_dly_sel_write(1 << dq_line);
#endif

	/* Increment DQ/DQS delay */
	ddrphy_wdly_dq_inc_write(1);
	ddrphy_wdly_dqs_inc_write(1);

	/* Un-select module */
	ddrphy_dly_sel_write(0);

#ifdef SDRAM_DELAY_PER_DQ
	/* Un-select DQ line */
	ddrphy_dq_dly_sel_write(0);
#endif
}

static int sdram_write_leveling_scan(int *delays, int loops, int show)
{
	int i, j, k, dq_line;

	int err_ddrphy_wdly;

	unsigned char taps_scan[SDRAM_PHY_DELAYS];

	int one_window_active;
	int one_window_start, one_window_best_start;
	int one_window_count, one_window_best_count;

	unsigned char buf[DFII_PIX_DATA_BYTES];

	int ok;

	err_ddrphy_wdly = SDRAM_PHY_DELAYS - _sdram_tck_taps/4;

	sdram_write_leveling_on();
	cdelay(100);
	for(i=0;i<SDRAM_PHY_MODULES;i++) {

#ifdef SDRAM_DELAY_PER_DQ
		for (dq_line = 0; dq_line < SDRAM_PHY_DQ_DQS_RATIO; dq_line++) {
#else
		for (dq_line = 0; dq_line < 1; dq_line++) {
#endif
			if (show)
#ifdef SDRAM_DELAY_PER_DQ
				printf("  m%d dq%d: |", i, dq_line);
#else
				printf("  m%d: |", i);
#endif

			/* Reset delay */
			sdram_write_leveling_rst_delay(i, dq_line);
			cdelay(100);

			/* Scan write delay taps */
			for(j=0;j<err_ddrphy_wdly;j++) {
				int zero_count = 0;
				int one_count = 0;
				int show_iter = show;
#if SDRAM_PHY_DELAYS > 32
				show_iter = (j%16 == 0) && show;
#endif
				for (k=0; k<loops; k++) {
					ddrphy_wlevel_strobe_write(1);
					cdelay(100);
					csr_rd_buf_uint8(sdram_dfii_pix_rddata_addr(0), buf, DFII_PIX_DATA_BYTES);
#if SDRAM_PHY_DQ_DQS_RATIO == 4
					if (buf[SDRAM_PHY_MODULES-1-(i/2)] != 0)
#else
					if (buf[SDRAM_PHY_MODULES-1-i] != 0)
#endif
						one_count++;
					else
						zero_count++;
				}
				if (one_count > zero_count)
					taps_scan[j] = 1;
				else
					taps_scan[j] = 0;
				if (show_iter)
					printf("%d", taps_scan[j]);
				sdram_write_leveling_inc_delay(i, dq_line);
				cdelay(100);
			}
			if (show)
				printf("|");

			/* Find longer 1 window and set delay at the 0/1 transition */
			one_window_active = 0;
			one_window_start = 0;
			one_window_count = 0;
			one_window_best_start = 0;
			one_window_best_count = -1;
			delays[i] = -1;
			for(j=0;j<err_ddrphy_wdly;j++) {
				if (one_window_active) {
					if ((taps_scan[j] == 0) | (j == err_ddrphy_wdly - 1)) {
						one_window_active = 0;
						one_window_count = j - one_window_start;
						if (one_window_count > one_window_best_count) {
							one_window_best_start = one_window_start;
							one_window_best_count = one_window_count;
						}
					}
				} else {
					if (taps_scan[j]) {
						one_window_active = 1;
						one_window_start = j;
					}
				}
			}

			/* Reset delay */
			sdram_write_leveling_rst_delay(i, dq_line);
			cdelay(100);

			/* Use forced delay if configured */
			if (_sdram_write_leveling_dat_delays[i] >= 0) {
				delays[i] = _sdram_write_leveling_dat_delays[i];

				/* Configure write delay */
				for(j=0; j<delays[i]; j++)  {
					sdram_write_leveling_inc_delay(i, dq_line);
					cdelay(100);
				}
			/* Succeed only if the start of a 1s window has been found: */
			} else if (
				/* Start of 1s window directly seen after 0. */
				((one_window_best_start) > 0 && (one_window_best_count > 0)) ||
				/* Start of 1s window indirectly seen before 0. */
				((one_window_best_start == 0) && (one_window_best_count > _sdram_tck_taps/4))
				){
#if SDRAM_PHY_DELAYS > 32
				/* Ensure write delay is just before transition */
				one_window_start -= min(one_window_start, 16);
#endif
				delays[i] = one_window_best_start;

				/* Configure write delay */
				for(j=0; j<delays[i]; j++) {
					sdram_write_leveling_inc_delay(i, dq_line);
					cdelay(100);
				}
			}
			if (show) {
				if (delays[i] == -1)
					printf(" delay: -\n");
				else
					printf(" delay: %02d\n", delays[i]);
			}
		}
	}

	sdram_write_leveling_off();

	ok = 1;
	for(i=SDRAM_PHY_MODULES-1;i>=0;i--) {
		if(delays[i] < 0)
			ok = 0;
	}

	return ok;
}

static void sdram_write_leveling_find_cmd_delay(unsigned int *best_error, unsigned int *best_count, int *best_cdly,
		int cdly_start, int cdly_stop, int cdly_step)
{
	int cdly;
	int cdly_actual = 0;
	int delays[SDRAM_PHY_MODULES];
#ifndef SDRAM_WRITE_LEVELING_CMD_DELAY_DEBUG
	int ok;
#endif

	/* Scan through the range */
	ddrphy_cdly_rst_write(1);
	cdelay(100);
	for (cdly = cdly_start; cdly < cdly_stop; cdly += cdly_step) {
		/* Increment cdly to current value */
		while (cdly_actual < cdly) {
			ddrphy_cdly_inc_write(1);
			cdelay(100);
			cdly_actual++;
		}

		/* Write level using this delay */
#ifdef SDRAM_WRITE_LEVELING_CMD_DELAY_DEBUG
		printf("Cmd/Clk delay: %d\n", cdly);
		sdram_write_leveling_scan(delays, 8, 1);
#else
		ok = sdram_write_leveling_scan(delays, 8, 0);
#endif
		/* Use the mean of delays for error calulation */
		int delay_mean  = 0;
		int delay_count = 0;
		for (int i=0; i < SDRAM_PHY_MODULES; ++i) {
			if (delays[i] != -1) {
				delay_mean  += delays[i] + _sdram_tck_taps/4;
				delay_count += 1;
			}
		}
		if (delay_count != 0)
			delay_mean /= delay_count;

		/* We want the higher number of valid modules and delay to be centered */
		int ideal_delay = (SDRAM_PHY_DELAYS - _sdram_tck_taps/4)/2;
		int error = ideal_delay - delay_mean;
		if (error < 0)
			error *= -1;

		if (delay_count >= *best_count) {
			if (error < *best_error) {
				*best_cdly  = cdly;
				*best_error = error;
				*best_count = delay_count;
			}
		}
#ifdef SDRAM_WRITE_LEVELING_CMD_DELAY_DEBUG
		printf("Delay mean: %d, ideal: %d\n", delay_mean, ideal_delay);
#else
		printf("%d", ok);
#endif
	}
}

int sdram_write_leveling(void)
{
	int delays[SDRAM_PHY_MODULES];
	unsigned int best_error = ~0u;
	unsigned int best_count = 0;
	int best_cdly = -1;
	int cdly_range_start;
	int cdly_range_end;
	int cdly_range_step;

	_sdram_tck_taps = ddrphy_half_sys8x_taps_read()*4;
	printf("  tCK equivalent taps: %d\n", _sdram_tck_taps);

	if (_sdram_write_leveling_cmd_scan) {
		/* Center write leveling by varying cdly. Searching through all possible
		 * values is slow, but we can use a simple optimization method of iterativly
		 * scanning smaller ranges with decreasing step */
		if (_sdram_write_leveling_cdly_range_start != -1)
			cdly_range_start = _sdram_write_leveling_cdly_range_start;
		else
			cdly_range_start = 0;
		if (_sdram_write_leveling_cdly_range_end != -1)
			cdly_range_end = _sdram_write_leveling_cdly_range_end;
		else
			cdly_range_end = _sdram_tck_taps/2; /* Limit Clk/Cmd scan to 1/2 tCK */

		printf("  Cmd/Clk scan (%d-%d)\n", cdly_range_start, cdly_range_end);
		if (SDRAM_PHY_DELAYS > 32)
			cdly_range_step = SDRAM_PHY_DELAYS/8;
		else
			cdly_range_step = 1;
		while (cdly_range_step > 0) {
			printf("  |");
			sdram_write_leveling_find_cmd_delay(&best_error, &best_count, &best_cdly,
					cdly_range_start, cdly_range_end, cdly_range_step);

			/* Small optimization - stop if we have zero error */
			if (best_error == 0)
				break;

			/* Use best result as the middle of next range */
			cdly_range_start = best_cdly - cdly_range_step;
			cdly_range_end = best_cdly + cdly_range_step + 1;
			if (cdly_range_start < 0)
				cdly_range_start = 0;
			if (cdly_range_end > 512)
				cdly_range_end = 512;

			cdly_range_step /= 4;
		}
		printf("| best: %d\n", best_cdly);
	} else {
		best_cdly = _sdram_write_leveling_cmd_delay;
	}
	printf("  Setting Cmd/Clk delay to %d taps.\n", best_cdly);
	/* Set working or forced delay */
	if (best_cdly >= 0) {
		ddrphy_cdly_rst_write(1);
		cdelay(100);
		for (int i = 0; i < best_cdly; ++i) {
			ddrphy_cdly_inc_write(1);
			cdelay(100);
		}
	}

	printf("  Data scan:\n");

	/* Re-run write leveling the final time */
	if (!sdram_write_leveling_scan(delays, 128, 1))
		return 0;

	return best_cdly >= 0;
}

#endif /*  SDRAM_PHY_WRITE_LEVELING_CAPABLE */

/*-----------------------------------------------------------------------*/
/* Read Leveling                                                         */
/*-----------------------------------------------------------------------*/

static void sdram_read_leveling_rst_delay(int module, int dq_line) {
	/* Select module */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dly_sel_write(1 << module);
	else
		ddrphy_B_dly_sel_write(1 << (module/2));
#else
	ddrphy_dly_sel_write(1 << module);
#endif

#ifdef SDRAM_DELAY_PER_DQ
	/* Select DQ line */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dq_dly_sel_write(1 << dq_line);
	else
		ddrphy_B_dq_dly_sel_write(1 << dq_line);
#else
	ddrphy_dq_dly_sel_write(1 << dq_line);
#endif
#endif

	/* Reset delay */
	ddrphy_rdly_dq_rst_write(1);

	/* Un-select module */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dly_sel_write(0);
	else
		ddrphy_B_dly_sel_write(0);
#else
	ddrphy_dly_sel_write(0);
#endif

#ifdef SDRAM_DELAY_PER_DQ
	/* Un-select DQ line */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dq_dly_sel_write(0);
	else
		ddrphy_B_dq_dly_sel_write(0);
#else
	ddrphy_dq_dly_sel_write(0);
#endif
#endif

#ifdef SDRAM_PHY_ECP5DDRPHY
	/* Sync all DQSBUFM's, By toggling all dly_sel (DQSBUFM.PAUSE) lines. */
	ddrphy_dly_sel_write(0xff);
	ddrphy_dly_sel_write(0);
#endif
}

static void sdram_read_leveling_inc_delay(int module, int dq_line) {
	/* Select module */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dly_sel_write(1 << module);
	else
		ddrphy_B_dly_sel_write(1 << (module/2));
#else
	ddrphy_dly_sel_write(1 << module);
#endif

#ifdef SDRAM_DELAY_PER_DQ
	/* Select DQ line */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dq_dly_sel_write(1 << dq_line);
	else
		ddrphy_B_dq_dly_sel_write(1 << dq_line);
#else
	ddrphy_dq_dly_sel_write(1 << dq_line);
#endif
#endif

	/* Increment delay */
	ddrphy_rdly_dq_inc_write(1);

	/* Un-select module */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dly_sel_write(0);
	else
		ddrphy_B_dly_sel_write(0);
#else
	ddrphy_dly_sel_write(0);
#endif

#ifdef SDRAM_DELAY_PER_DQ
	/* Un-select DQ line */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dq_dly_sel_write(0);
	else
		ddrphy_B_dq_dly_sel_write(0);
#else
	ddrphy_dq_dly_sel_write(0);
#endif
#endif

#ifdef SDRAM_PHY_ECP5DDRPHY
	/* Sync all DQSBUFM's, By toggling all dly_sel (DQSBUFM.PAUSE) lines. */
	ddrphy_dly_sel_write(0xff);
	ddrphy_dly_sel_write(0);
#endif
}

static void sdram_read_leveling_rst_bitslip(int module, int dq_line)
{
	/* Select module */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dly_sel_write(1 << module);
	else
		ddrphy_B_dly_sel_write(1 << (module/2));
#else
	ddrphy_dly_sel_write(1 << module);
#endif

#ifdef SDRAM_DELAY_PER_DQ
	/* Select DQ line */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dq_dly_sel_write(1 << dq_line);
	else
		ddrphy_B_dq_dly_sel_write(1 << dq_line);
#else
	ddrphy_dq_dly_sel_write(1 << dq_line);
#endif
#endif

	/* Reset delay */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_rdly_dq_bitslip_rst_write(1);
	else
		ddrphy_B_rdly_dq_bitslip_rst_write(1);
#else
	ddrphy_rdly_dq_bitslip_rst_write(1);
#endif

	/* Un-select module */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dly_sel_write(0);
	else
		ddrphy_B_dly_sel_write(0);
#else
	ddrphy_dly_sel_write(0);
#endif

#ifdef SDRAM_DELAY_PER_DQ
	/* Un-select DQ line */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dq_dly_sel_write(0);
	else
		ddrphy_B_dq_dly_sel_write(0);
#else
	ddrphy_dq_dly_sel_write(0);
#endif
#endif
}


static void sdram_read_leveling_inc_bitslip(int module, int dq_line)
{
	/* Select module */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dly_sel_write(1 << module);
	else
		ddrphy_B_dly_sel_write(1 << (module/2));
#else
	ddrphy_dly_sel_write(1 << module);
#endif

#ifdef SDRAM_DELAY_PER_DQ
	/* Select DQ line */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dq_dly_sel_write(1 << dq_line);
	else
		ddrphy_B_dq_dly_sel_write(1 << dq_line);
#else
	ddrphy_dq_dly_sel_write(1 << dq_line);
#endif
#endif

	/* Increment delay */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_rdly_dq_bitslip_write(1);
	else
		ddrphy_B_rdly_dq_bitslip_write(1);
#else
	ddrphy_rdly_dq_bitslip_write(1);
#endif

	/* Un-select module */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dly_sel_write(0);
	else
		ddrphy_B_dly_sel_write(0);
#else
	ddrphy_dly_sel_write(0);
#endif

#ifdef SDRAM_DELAY_PER_DQ
	/* Un-select DQ line */
#ifdef SRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dq_dly_sel_write(0);
	else
		ddrphy_B_dq_dly_sel_write(0);
#else
	ddrphy_dq_dly_sel_write(0);
#endif
#endif
}

static unsigned int sdram_read_leveling_scan_module(int module, int bitslip, int show, int dq_line)
{
	const unsigned int max_errors = 6*READ_CHECK_TEST_PATTERN_MAX_ERRORS;
	int i;
	unsigned int score;
	unsigned int errors;

	/* Check test pattern for each delay value */
	score = 0;
	if (show)
		printf("  m%d, b%02d: |", module, bitslip);
	sdram_read_leveling_rst_delay(module, dq_line);
	for(i=0;i<SDRAM_PHY_DELAYS;i++) {
		int working;
		int _show = show;
#if SDRAM_PHY_DELAYS > 32
		_show = (i%16 == 0) & show;
#endif
		errors  = sdram_write_read_check_test_pattern(module, 42, dq_line);
		errors += sdram_write_read_check_test_pattern(module, 84, dq_line);
		errors += sdram_write_read_check_test_pattern(module, 36, dq_line);
		errors += sdram_write_read_check_test_pattern(module, 72, dq_line);
		errors += sdram_write_read_check_test_pattern(module, 24, dq_line);
		errors += sdram_write_read_check_test_pattern(module, 48, dq_line);
		working = errors == 0;
		/* When any scan is working then the final score will always be higher then if no scan was working */
		score += (working * max_errors*SDRAM_PHY_DELAYS) + (max_errors - errors);
		if (_show) {
			print_scan_errors(errors);
		}
		sdram_read_leveling_inc_delay(module, dq_line);
	}
	if (show)
		printf("| ");

	return score;
}

#endif /* CSR_DDRPHY_BASE */

#endif /* CSR_SDRAM_BASE */

#ifdef CSR_SDRAM_BASE

#if 0 && (defined(SDRAM_PHY_WRITE_LEVELING_CAPABLE) || defined(SDRAM_PHY_READ_LEVELING_CAPABLE))

void sdram_read_leveling(void)
{
	int module;
	int bitslip;
	int dq_line;
	unsigned int score;
	unsigned int best_score;
	int best_bitslip;

	for(module=0; module<SDRAM_PHY_MODULES; module++) {
#ifdef SDRAM_DELAY_PER_DQ
		for (dq_line = 0; dq_line < SDRAM_PHY_DQ_DQS_RATIO; dq_line++) {
#else
		for (dq_line = 0; dq_line < 1; dq_line++) {
#endif
			/* Scan possible read windows */
			best_score = 0;
			best_bitslip = 0;
			sdram_read_leveling_rst_bitslip(module, dq_line);
			for(bitslip=0; bitslip<SDRAM_PHY_BITSLIPS; bitslip++) {
				/* Compute score */
				score = sdram_read_leveling_scan_module(module, bitslip, 1, dq_line);
				sdram_leveling_center_module(module, 1, 0,
					sdram_read_leveling_rst_delay, sdram_read_leveling_inc_delay, dq_line);
				printf("\n");
				if (score > best_score) {
					best_bitslip = bitslip;
					best_score = score;
				}
				/* Exit */
				if (bitslip == SDRAM_PHY_BITSLIPS-1)
					break;
				/* Increment bitslip */
				sdram_read_leveling_inc_bitslip(module, dq_line);
			}

			/* Select best read window */
#ifdef SDRAM_DELAY_PER_DQ
			printf("  best: m%d, b%02d, dq_line%d ", module, best_bitslip, dq_line);
#else
			printf("  best: m%d, b%02d ", module, best_bitslip);
#endif
			sdram_read_leveling_rst_bitslip(module, dq_line);
			for (bitslip=0; bitslip<best_bitslip; bitslip++)
				sdram_read_leveling_inc_bitslip(module, dq_line);

			/* Re-do leveling on best read window*/
			sdram_leveling_center_module(module, 1, 0,
				sdram_read_leveling_rst_delay, sdram_read_leveling_inc_delay, dq_line);
			printf("\n");
		}
	}
}

/*-----------------------------------------------------------------------*/
/* Write latency calibration                                             */
/*-----------------------------------------------------------------------*/

#ifdef SDRAM_PHY_WRITE_LATENCY_CALIBRATION_CAPABLE

static void sdram_write_latency_calibration(void) {
	int i;
	int module;
	int bitslip;
	int dq_line;
	unsigned int score;
	unsigned int subscore;
	unsigned int best_score;
	int best_bitslip;

	for(module=0; module<SDRAM_PHY_MODULES; module++) {

#ifdef SDRAM_DELAY_PER_DQ
		for (dq_line = 0; dq_line < SDRAM_PHY_DQ_DQS_RATIO; dq_line++) {
#else
		for (dq_line = 0; dq_line < 1; dq_line++) {
#endif

			/* Scan possible write windows */
			best_score   = 0;
			best_bitslip = -1;
			for(bitslip=0; bitslip<SDRAM_PHY_BITSLIPS; bitslip+=2) { /* +2 for tCK steps */

#ifdef SDRAM_WRITE_LATENCY_CALIBRATION_DEBUG
				printf("m%d wb%02d:\n", module, bitslip);
#endif
				score = 0;
				/* Select module */
				ddrphy_dly_sel_write(1 << module);
#ifdef SDRAM_DELAY_PER_DQ
				/* Select DQ line */
				ddrphy_dq_dly_sel_write(1 << dq_line);
#endif
				/* Reset bitslip */
				ddrphy_wdly_dq_bitslip_rst_write(1);
				for (i=0; i<bitslip; i++) {
					ddrphy_wdly_dq_bitslip_write(1);
				}
				/* Un-select module */
				ddrphy_dly_sel_write(0);
#ifdef SDRAM_DELAY_PER_DQ
				/* Un-select DQ line */
				ddrphy_dq_dly_sel_write(0);
#endif
				score = 0;
				sdram_read_leveling_rst_bitslip(module, dq_line);
				for(i=0; i<SDRAM_PHY_BITSLIPS; i++) {
					/* Compute score */
#ifdef SDRAM_WRITE_LATENCY_CALIBRATION_DEBUG
					subscore = sdram_read_leveling_scan_module(module, i, 1, dq_line);
					printf("\n");
#else
					subscore = sdram_read_leveling_scan_module(module, i, 0, dq_line);
#endif
					score = subscore > score ? subscore : score;
					/* Increment bitslip */
					sdram_read_leveling_inc_bitslip(module, dq_line);
				}
				if (score > best_score) {
					best_bitslip = bitslip;
					best_score = score;
				}
			}

#ifdef SDRAM_PHY_WRITE_LEVELING_CAPABLE
			if (_sdram_write_leveling_bitslips[module] < 0)
				bitslip = best_bitslip;
			else
				bitslip = _sdram_write_leveling_bitslips[module];
#else
				bitslip = best_bitslip;
#endif
			if (bitslip == -1)
				printf("m%d:- ", module);
			else
#ifdef SDRAM_DELAY_PER_DQ
				printf("m%d dq%d:%d ", module, dq_line, bitslip);
#else
				printf("m%d:%d ", module, bitslip);
#endif
#ifdef SDRAM_WRITE_LATENCY_CALIBRATION_DEBUG
			printf("\n");
#endif

			/* Select best write window */
			ddrphy_dly_sel_write(1 << module);

#ifdef SDRAM_DELAY_PER_DQ
			/* Select dq_line */
			ddrphy_dq_dly_sel_write(1 << dq_line);
#endif
			/* Reset bitslip */
			ddrphy_wdly_dq_bitslip_rst_write(1);
			for (i=0; i<bitslip; i++) {
				ddrphy_wdly_dq_bitslip_write(1);
			}
			/* Un-select module */
			ddrphy_dly_sel_write(0);
#ifdef SDRAM_DELAY_PER_DQ
			/* Un-select QD_line */
			ddrphy_dq_dly_sel_write(0);
#endif
		}
		printf("\n");
	}
}

#endif

/*-----------------------------------------------------------------------*/
/* Write DQ-DQS training                                                 */
/*-----------------------------------------------------------------------*/

#ifdef SDRAM_PHY_WRITE_DQ_DQS_TRAINING_CAPABLE

static void sdram_write_dq_dqs_training_rst_delay(int module, int dq_line) {
	/* Select module */
	ddrphy_dly_sel_write(1 << module);

#if defined(SDRAM_PHY_USDDRPHY) || defined(SDRAM_PHY_USPDDRPHY)
	/* Reset DQ delay */
	int dq_count = ddrphy_wdly_dqs_inc_count_read();
	while (dq_count != SDRAM_PHY_DELAYS) {
		ddrphy_wdly_dq_inc_write(1);
		cdelay(100);
		dq_count++;
	}
#else
	/* Reset DQ delay */
	ddrphy_wdly_dq_rst_write(1);
	cdelay(100);
#endif

	/* Un-select module */
	ddrphy_dly_sel_write(0);
}

static void sdram_write_dq_dqs_training_inc_delay(int module, int dq_line) {
	/* Select module */
	ddrphy_dly_sel_write(1 << module);
	/* Increment delay */
	ddrphy_wdly_dq_inc_write(1);
	cdelay(100);
	/* Un-select module */
	ddrphy_dly_sel_write(0);
}

static void sdram_read_leveling_best_bitslip(int module, int dq_line)
{
	unsigned int score;
	int bitslip;
	int best_bitslip = 0;
	unsigned int best_score = 0;

	sdram_read_leveling_rst_bitslip(module, dq_line);
	for(bitslip=0; bitslip<SDRAM_PHY_BITSLIPS; bitslip++) {
		score = sdram_read_leveling_scan_module(module, bitslip, 0, dq_line);
		sdram_leveling_center_module(module, 0, 0,
			sdram_read_leveling_rst_delay, sdram_read_leveling_inc_delay, dq_line);
		if (score > best_score) {
			best_bitslip = bitslip;
			best_score = score;
		}
		if (bitslip == SDRAM_PHY_BITSLIPS-1)
			break;
		sdram_read_leveling_inc_bitslip(module, dq_line);
	}

	/* Select best read window and re-center it */
	sdram_read_leveling_rst_bitslip(module, dq_line);
	for (bitslip=0; bitslip<best_bitslip; bitslip++)
		sdram_read_leveling_inc_bitslip(module, dq_line);
	sdram_leveling_center_module(module, 0, 0,
		sdram_read_leveling_rst_delay, sdram_read_leveling_inc_delay, dq_line);
}

static void sdram_write_dq_dqs_training(void)
{
	int module;
	int dq_line;

	for(module=0; module<SDRAM_PHY_MODULES; module++) {
#ifdef SDRAM_DELAY_PER_DQ
		for (dq_line = 0; dq_line < SDRAM_PHY_DQ_DQS_RATIO; dq_line++) {
#else
		for (dq_line = 0; dq_line < 1; dq_line++) {
#endif
			/* Find best bitslip */
			sdram_read_leveling_best_bitslip(module, dq_line);
			/* Center DQ-DQS window */
			sdram_leveling_center_module(module, 1, 1,
				sdram_write_dq_dqs_training_rst_delay, sdram_write_dq_dqs_training_inc_delay, dq_line);
		}
	}
}

#endif /* SDRAM_PHY_WRITE_DQ_DQS_TRAINING_CAPABLE */

#endif

#if defined(SDRAM_PHY_WRITE_LEVELING_CAPABLE) || defined(SDRAM_PHY_READ_LEVELING_CAPABLE)

/*-----------------------------------------------------------------------*/
/* DDR5 Training                                                         */
/*-----------------------------------------------------------------------*/

#define DDR5_NOP_COMMAND		(0x001F) /* CA[4:0]  = 0b11111 */

#ifdef SDRAM_PHY_SUBCHANNELS
#define SAMPLED_RDDATA_SIZE		CSR_SDRAM_DFII_A_CMDINJECTOR_SAMPLED_RDDATA_SIZE
#else
#define SAMPLED_RDDATA_SIZE		CSR_SDRAM_DFII_CMDINJECTOR_SAMPLED_RDDATA_SIZE
#endif

/* Save state of rddata and copy it to provided buffer.
 * Use this function only if DQ state will be the same over many cycles,
 * as software overhead is high compared to speed of the memory. */
static void sdram_sample_rddata(
	uint32_t out_buffer[SAMPLED_RDDATA_SIZE],
	uint32_t cmdinjector_sample_rddata_addr,
	uint32_t cmdinjector_sampled_rddata_addr)
{
	csr_wr_uint32(1, cmdinjector_sample_rddata_addr);
	csr_rd_buf_uint32(cmdinjector_sampled_rddata_addr, out_buffer, SAMPLED_RDDATA_SIZE);
}

static void sdram_increase_cs_delay(int rank)
{
	/* Select which signal we will work on.
	 * There is one CS_n signal for each rank */
	ddrphy_dly_sel_write(1 << rank);
	/* Increase CS delay for selected signal */
	ddrphy_csdly_inc_write(1);
	/* Deselect all signals */
	ddrphy_dly_sel_write(0);
}

static void sdram_reset_cs_delay(int rank)
{
	/* Select which signal we will work on.
	 * There is one CS_n signal for each rank */
	ddrphy_dly_sel_write(1 << rank);
	/* Reset CS delay for selected signal */
	ddrphy_csdly_rst_write(1);
	/* Deselect all signals */
	ddrphy_dly_sel_write(0);
}

/*-----------------------------------------------------------------------*/
/* CS Training (CSTM)                                                    */
/*-----------------------------------------------------------------------*/
#define SDRAM_CS_TRAINING_DEBUG

#ifdef SDRAM_DEBUG
#define SDRAM_CS_TRAINING_DEBUG
#endif

#define DDR5_MPC_ENTER_CS_TRAINING	(0x01)
#define DDR5_MPC_EXIT_CS_TRAINING	(0x00)

/* CS training works by toggling CS value each cycle.
 * It doesn't matter what's on CA bus when CS is low (deselect),
 * so we leave NOP command there. */
static void sdram_cstm_enable_toggling_cs(
	int rank,
	uint32_t cmdinjector_flag,
	uint32_t cmdinjector_cs0_addr,
	uint32_t cmdinjector_cs1_addr,
	uint32_t cmdinjector_cs2_addr,
	uint32_t cmdinjector_cs3_addr,
	uint32_t cmdinjector_ca0_addr,
	uint32_t cmdinjector_ca1_addr,
	uint32_t cmdinjector_ca2_addr,
	uint32_t cmdinjector_ca3_addr)
{
	csr_wr_uint32(DDR5_NOP_COMMAND, cmdinjector_ca0_addr);
	csr_wr_uint32(DDR5_NOP_COMMAND, cmdinjector_ca1_addr);
	csr_wr_uint32(DDR5_NOP_COMMAND, cmdinjector_ca2_addr);
	csr_wr_uint32(DDR5_NOP_COMMAND, cmdinjector_ca3_addr);

	csr_wr_uint32(1 << rank, cmdinjector_cs0_addr); /* CS_N LOW */
	csr_wr_uint32(0 << rank, cmdinjector_cs1_addr); /* CS_N HIGH -> deselect*/
	csr_wr_uint32(1 << rank, cmdinjector_cs2_addr); /* CS_N LOW */
	csr_wr_uint32(0 << rank, cmdinjector_cs3_addr); /* CS_N HIGH -> deselect*/

	sdram_dfii_control_write(DFII_CONTROL_SOFTWARE|DFII_CONTROL_DDR5|cmdinjector_flag);

	cdelay(200);
}

/* When clock and CS are aligned, then DDR5 die will output 0s on all DQ pins */
static bool sdram_cstm_verify_rddata(uint32_t rddata[SAMPLED_RDDATA_SIZE])
{
	/* Check if all sampled bits are 0 */
	uint32_t any_non_zero_dq = 0;
	for (int i = 0; i < SAMPLED_RDDATA_SIZE; i++) {
		any_non_zero_dq |= rddata[i];
#ifdef SDRAM_CS_TRAINING_DEBUG
		printf("rddata[%d] = %lX\n", i, rddata[i]);
#endif
	}

	return !any_non_zero_dq;
}

/* MPC Command has one cycle, but we send it every cycle as CS_n and CA lanes
 * aren't trained yet. It's a concatenation of command code (CA[4:0] = 01111)
 * and 8-bit MPC opcode. */
static void sdram_cstm_send_mpc(
	int rank,
	uint8_t command,
	uint32_t cmdinjector_flag,
	uint32_t cmdinjector_cs0_addr,
	uint32_t cmdinjector_cs1_addr,
	uint32_t cmdinjector_cs2_addr,
	uint32_t cmdinjector_cs3_addr,
	uint32_t cmdinjector_ca0_addr,
	uint32_t cmdinjector_ca1_addr,
	uint32_t cmdinjector_ca2_addr,
	uint32_t cmdinjector_ca3_addr)
{
	csr_wr_uint32(command << 5 | 0x0f, cmdinjector_ca0_addr);
	csr_wr_uint32(command << 5 | 0x0f, cmdinjector_ca1_addr);
	csr_wr_uint32(command << 5 | 0x0f, cmdinjector_ca2_addr);
	csr_wr_uint32(command << 5 | 0x0f, cmdinjector_ca3_addr);

	csr_wr_uint32(1 << rank, cmdinjector_cs0_addr);
	csr_wr_uint32(1 << rank, cmdinjector_cs1_addr);
	csr_wr_uint32(1 << rank, cmdinjector_cs2_addr);
	csr_wr_uint32(1 << rank, cmdinjector_cs3_addr);

	sdram_dfii_control_write(DFII_CONTROL_SOFTWARE|DFII_CONTROL_DDR5|cmdinjector_flag);

	cdelay(100);
}

/* Core part of CS training
 * It takes many arguments to make it usable when subchannels are enabled.
 * 
 * High level description:
 *
 * 1. Enter CS training mode
 * 2. Iterate over all possible delays and check which work
 * 3. Choose midpoint between shortest and longest working delay
 *    and use it as final delay
 * 4. Double check that selected delay works
 * 5. Exit CS training mode */
static void sdram_cstm_core(
	int rank,
	uint32_t nopinjector_flag,
	uint32_t cmdinjector_flag,
	uint32_t cmdinjector_sample_rddata_addr,
	uint32_t cmdinjector_sampled_rddata_addr,
	uint32_t cmdinjector_wrdata_addr,
	uint32_t cmdinjector_wrdata_en0_addr,
	uint32_t cmdinjector_wrdata_en1_addr,
	uint32_t cmdinjector_wrdata_en2_addr,
	uint32_t cmdinjector_wrdata_en3_addr,
	uint32_t cmdinjector_cs0_addr,
	uint32_t cmdinjector_cs1_addr,
	uint32_t cmdinjector_cs2_addr,
	uint32_t cmdinjector_cs3_addr,
	uint32_t cmdinjector_ca0_addr,
	uint32_t cmdinjector_ca1_addr,
	uint32_t cmdinjector_ca2_addr,
	uint32_t cmdinjector_ca3_addr)
{
	uint32_t sampled_rddata[SAMPLED_RDDATA_SIZE];

	int current_series_of_0s_start, current_series_of_0s_end, current_series_length;
	int longest_series_of_0s_start = 0, longest_series_of_0s_end = 0, longest_series_length = 0;
	bool in_series_of_0s = false;

	sdram_reset_cs_delay(rank);

	sdram_cstm_send_mpc(
		rank,
		DDR5_MPC_ENTER_CS_TRAINING,
		cmdinjector_flag,
		cmdinjector_cs0_addr,
		cmdinjector_cs1_addr,
		cmdinjector_cs2_addr,
		cmdinjector_cs3_addr,
		cmdinjector_ca0_addr,
		cmdinjector_ca1_addr,
		cmdinjector_ca2_addr,
		cmdinjector_ca3_addr);

	/* Normally you would expect to get following values on DQ pins
	 * for subsequent delays 1111111110000000111111111 (<- example).
	 * So first for some delays you get all 1s, then for some you get all 0s
	 * and then again you get all 1s.  But because you can start with already
	 * good delay, we need to go through the delays second time.  This way we
	 * can just search for longest series of working delays and at the end
	 * calculate midpoint delay as:
	 *	 'middle of longest series of 0s' % 'number of supported delays'. */
	for (int current_delay = 0; current_delay < SDRAM_PHY_DELAYS * 2; current_delay++) {
#ifdef SDRAM_CS_TRAINING_DEBUG
		printf("Testing delay %d\n", current_delay);
#endif
		sdram_dfii_control_write(DFII_CONTROL_SOFTWARE|DFII_CONTROL_DDR5|nopinjector_flag);

		sdram_cstm_enable_toggling_cs(
			rank,
			cmdinjector_flag,
			cmdinjector_cs0_addr,
			cmdinjector_cs1_addr,
			cmdinjector_cs2_addr,
			cmdinjector_cs3_addr,
			cmdinjector_ca0_addr,
			cmdinjector_ca1_addr,
			cmdinjector_ca2_addr,
			cmdinjector_ca3_addr);

		sdram_sample_rddata(
			sampled_rddata,
			cmdinjector_sample_rddata_addr,
			cmdinjector_sampled_rddata_addr);

		bool delay_is_ok = sdram_cstm_verify_rddata(sampled_rddata);
		if (delay_is_ok && !in_series_of_0s) {
			in_series_of_0s = true;
			current_series_of_0s_start = current_delay;
		} else if (!delay_is_ok && in_series_of_0s) {
			in_series_of_0s = false;
			current_series_of_0s_end = current_delay - 1;
			current_series_length = current_series_of_0s_end - current_series_of_0s_start;
			if (current_series_length > longest_series_length) {
				longest_series_of_0s_start = current_series_of_0s_start;
				longest_series_of_0s_end = current_series_of_0s_end;
				longest_series_length = longest_series_of_0s_end - longest_series_of_0s_start;
			}
		}

		sdram_increase_cs_delay(rank);
	}

	/* Calculate midpoint. We need to calculate remainder of division,
	 * beacuse midpoint could land in repeated part of the checked delays. */
	int midpoint_delay = ((longest_series_length / 2) + longest_series_of_0s_start) % SDRAM_PHY_DELAYS;

	/* Reset delay and set it to selected value */
	sdram_reset_cs_delay(rank);
	for (int i = 0; i < midpoint_delay; i++)
		sdram_increase_cs_delay(rank);

#ifdef SDRAM_CS_TRAINING_DEBUG
	printf("Selected CS delay: %d\n", midpoint_delay);
#endif

	/* Verify selected delay. Just in case */
	sdram_dfii_control_write(DFII_CONTROL_SOFTWARE|DFII_CONTROL_DDR5|nopinjector_flag);

	sdram_cstm_enable_toggling_cs(
		rank,
		cmdinjector_flag,
		cmdinjector_cs0_addr,
		cmdinjector_cs1_addr,
		cmdinjector_cs2_addr,
		cmdinjector_cs3_addr,
		cmdinjector_ca0_addr,
		cmdinjector_ca1_addr,
		cmdinjector_ca2_addr,
		cmdinjector_ca3_addr);

	sdram_sample_rddata(
		sampled_rddata,
		cmdinjector_sample_rddata_addr,
		cmdinjector_sampled_rddata_addr);

	if (!sdram_cstm_verify_rddata(sampled_rddata)) {
		printf("There was a problem with CS training. Selected delay of %d didn't pass verification\n", midpoint_delay);
	}

	sdram_dfii_control_write(DFII_CONTROL_SOFTWARE|DFII_CONTROL_DDR5|nopinjector_flag);

	/* Send MPC 'CS Training Mode Exit' command */
	sdram_cstm_send_mpc(
		rank,
		DDR5_MPC_EXIT_CS_TRAINING,
		cmdinjector_flag,
		cmdinjector_cs0_addr,
		cmdinjector_cs1_addr,
		cmdinjector_cs2_addr,
		cmdinjector_cs3_addr,
		cmdinjector_ca0_addr,
		cmdinjector_ca1_addr,
		cmdinjector_ca2_addr,
		cmdinjector_ca3_addr);

	sdram_dfii_control_write(DFII_CONTROL_SOFTWARE);
}

static void sdram_cs_training(void)
{
	for (int rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
#ifdef SDRAM_PHY_SUBCHANNELS
#ifdef SDRAM_CS_TRAINING_DEBUG
		printf("Training CS for subchannel A\n");
#endif
		sdram_cstm_core(
			rank,
			DFII_CONTROL_A_NOPINJECTOR,
			DFII_CONTROL_A_CMDINJECTOR,
			CSR_SDRAM_DFII_A_CMDINJECTOR_SAMPLE_RDDATA_ADDR,
			CSR_SDRAM_DFII_A_CMDINJECTOR_SAMPLED_RDDATA_ADDR,
			CSR_SDRAM_DFII_A_CMDINJECTOR_WRDATA_ADDR,
			CSR_SDRAM_DFII_A_CMDINJECTOR_WRDATA_EN0_ADDR,
			CSR_SDRAM_DFII_A_CMDINJECTOR_WRDATA_EN1_ADDR,
			CSR_SDRAM_DFII_A_CMDINJECTOR_WRDATA_EN2_ADDR,
			CSR_SDRAM_DFII_A_CMDINJECTOR_WRDATA_EN3_ADDR,
			CSR_SDRAM_DFII_A_CMDINJECTOR_CS0_ADDR,
			CSR_SDRAM_DFII_A_CMDINJECTOR_CS1_ADDR,
			CSR_SDRAM_DFII_A_CMDINJECTOR_CS2_ADDR,
			CSR_SDRAM_DFII_A_CMDINJECTOR_CS3_ADDR,
			CSR_SDRAM_DFII_A_CMDINJECTOR_CA0_ADDR,
			CSR_SDRAM_DFII_A_CMDINJECTOR_CA1_ADDR,
			CSR_SDRAM_DFII_A_CMDINJECTOR_CA2_ADDR,
			CSR_SDRAM_DFII_A_CMDINJECTOR_CA3_ADDR
		);
#ifdef SDRAM_CS_TRAINING_DEBUG
		printf("Training CS for subchannel B\n");
#endif
		sdram_cstm_core(
			rank,
			DFII_CONTROL_B_NOPINJECTOR,
			DFII_CONTROL_B_CMDINJECTOR,
			CSR_SDRAM_DFII_B_CMDINJECTOR_SAMPLE_RDDATA_ADDR,
			CSR_SDRAM_DFII_B_CMDINJECTOR_SAMPLED_RDDATA_ADDR,
			CSR_SDRAM_DFII_B_CMDINJECTOR_WRDATA_ADDR,
			CSR_SDRAM_DFII_B_CMDINJECTOR_WRDATA_EN0_ADDR,
			CSR_SDRAM_DFII_B_CMDINJECTOR_WRDATA_EN1_ADDR,
			CSR_SDRAM_DFII_B_CMDINJECTOR_WRDATA_EN2_ADDR,
			CSR_SDRAM_DFII_B_CMDINJECTOR_WRDATA_EN3_ADDR,
			CSR_SDRAM_DFII_B_CMDINJECTOR_CS0_ADDR,
			CSR_SDRAM_DFII_B_CMDINJECTOR_CS1_ADDR,
			CSR_SDRAM_DFII_B_CMDINJECTOR_CS2_ADDR,
			CSR_SDRAM_DFII_B_CMDINJECTOR_CS3_ADDR,
			CSR_SDRAM_DFII_B_CMDINJECTOR_CA0_ADDR,
			CSR_SDRAM_DFII_B_CMDINJECTOR_CA1_ADDR,
			CSR_SDRAM_DFII_B_CMDINJECTOR_CA2_ADDR,
			CSR_SDRAM_DFII_B_CMDINJECTOR_CA3_ADDR
		);
#else
		sdram_cstm_core(
			rank,
			DFII_CONTROL_NOPINJECTOR,
			DFII_CONTROL_CMDINJECTOR,
			CSR_SDRAM_DFII_CMDINJECTOR_SAMPLE_RDDATA_ADDR,
			CSR_SDRAM_DFII_CMDINJECTOR_SAMPLED_RDDATA_ADDR,
			CSR_SDRAM_DFII_CMDINJECTOR_WRDATA_ADDR,
			CSR_SDRAM_DFII_CMDINJECTOR_WRDATA_EN0_ADDR,
			CSR_SDRAM_DFII_CMDINJECTOR_WRDATA_EN1_ADDR,
			CSR_SDRAM_DFII_CMDINJECTOR_WRDATA_EN2_ADDR,
			CSR_SDRAM_DFII_CMDINJECTOR_WRDATA_EN3_ADDR,
			CSR_SDRAM_DFII_CMDINJECTOR_CS0_ADDR,
			CSR_SDRAM_DFII_CMDINJECTOR_CS1_ADDR,
			CSR_SDRAM_DFII_CMDINJECTOR_CS2_ADDR,
			CSR_SDRAM_DFII_CMDINJECTOR_CS3_ADDR,
			CSR_SDRAM_DFII_CMDINJECTOR_CA0_ADDR,
			CSR_SDRAM_DFII_CMDINJECTOR_CA1_ADDR,
			CSR_SDRAM_DFII_CMDINJECTOR_CA2_ADDR,
			CSR_SDRAM_DFII_CMDINJECTOR_CA3_ADDR
		);
#endif
	}
}

/*-----------------------------------------------------------------------*/
/* Leveling                                                              */
/*-----------------------------------------------------------------------*/

int sdram_leveling(void)
{
	int module;
	int dq_line;
	sdram_software_control_on();

#if 0
	for(module=0; module<SDRAM_PHY_MODULES; module++) {
#ifdef SDRAM_DELAY_PER_DQ
		for (dq_line = 0; dq_line < SDRAM_PHY_DQ_DQS_RATIO; dq_line++) {
#else
		for (dq_line = 0; dq_line < 1; dq_line++) {
#endif
#ifdef SDRAM_PHY_WRITE_LEVELING_CAPABLE
			sdram_write_leveling_rst_delay(module, dq_line);
#endif
			sdram_read_leveling_rst_delay(module, dq_line);
			sdram_read_leveling_rst_bitslip(module, dq_line);
		}
	}

#ifdef SDRAM_PHY_WRITE_LEVELING_CAPABLE
	printf("Write leveling:\n");
	sdram_write_leveling();
#endif

#ifdef SDRAM_PHY_WRITE_LATENCY_CALIBRATION_CAPABLE
	printf("Write latency calibration:\n");
	sdram_write_latency_calibration();
#endif

#ifdef SDRAM_PHY_WRITE_DQ_DQS_TRAINING_CAPABLE
	printf("Write DQ-DQS training:\n");
	sdram_write_dq_dqs_training();
#endif

#ifdef SDRAM_PHY_READ_LEVELING_CAPABLE
	printf("Read leveling:\n");
	sdram_read_leveling();
#endif

#endif

	printf("CS training\n");
	sdram_cs_training();

	sdram_software_control_off();

	return 1;
}
#endif

/*-----------------------------------------------------------------------*/
/* Initialization                                                        */
/*-----------------------------------------------------------------------*/

int sdram_init(void)
{
#if 0
	/* Reset Cmd/Dat delays */
#ifdef SDRAM_PHY_WRITE_LEVELING_CAPABLE
	int i;
	sdram_write_leveling_rst_cmd_delay(0);
	for (i=0; i<16; i++) sdram_write_leveling_rst_dat_delay(i, 0);
	for (i=0; i<16; i++) sdram_write_leveling_rst_bitslip(i, 0);
#endif
#endif
	/* Reset Read/Write phases */
#ifdef CSR_DDRPHY_RDPHASE_ADDR
	ddrphy_rdphase_write(SDRAM_PHY_RDPHASE);
#endif
#ifdef CSR_DDRPHY_WRPHASE_ADDR
	ddrphy_wrphase_write(SDRAM_PHY_WRPHASE);
#endif
	/* Set Cmd delay if enforced at build time */
#ifdef SDRAM_PHY_CMD_DELAY
	_sdram_write_leveling_cmd_scan  = 0;
	_sdram_write_leveling_cmd_delay = SDRAM_PHY_CMD_DELAY;
#endif
	printf("Initializing SDRAM @0x%08lx...\n", MAIN_RAM_BASE);
	sdram_software_control_on();
#if CSR_DDRPHY_RST_ADDR
	ddrphy_rst_write(1);
	cdelay(1000);
	ddrphy_rst_write(0);
	cdelay(1000);
#endif

#ifdef CSR_DDRCTRL_BASE
	ddrctrl_init_done_write(0);
	ddrctrl_init_error_write(0);
#endif
	init_sequence();
#if defined(SDRAM_PHY_WRITE_LEVELING_CAPABLE) || defined(SDRAM_PHY_READ_LEVELING_CAPABLE)
	sdram_leveling();
#endif
	sdram_software_control_off();
#ifndef SDRAM_TEST_DISABLE
	if(!memtest((unsigned int *) MAIN_RAM_BASE, MEMTEST_DATA_SIZE)) {
#ifdef CSR_DDRCTRL_BASE
		ddrctrl_init_error_write(1);
		ddrctrl_init_done_write(1);
#endif
		return 0;
	}
	memspeed((unsigned int *) MAIN_RAM_BASE, MEMTEST_DATA_SIZE, false, 0);
#endif
#ifdef CSR_DDRCTRL_BASE
	ddrctrl_init_done_write(1);
#endif

	return 1;
}

/*-----------------------------------------------------------------------*/
/* Debugging                                                             */
/*-----------------------------------------------------------------------*/

#ifdef SDRAM_DEBUG

#define SDRAM_DEBUG_STATS_NUM_RUNS 10
#define SDRAM_DEBUG_STATS_MEMTEST_SIZE MEMTEST_DATA_SIZE

#ifdef SDRAM_DEBUG_READBACK_MEM_ADDR
#ifndef SDRAM_DEBUG_READBACK_MEM_SIZE
#error "Provide readback memory size via SDRAM_DEBUG_READBACK_MEM_SIZE"
#endif
#define SDRAM_DEBUG_READBACK_VERBOSE 1

#define SDRAM_DEBUG_READBACK_COUNT 3
#define SDRAM_DEBUG_READBACK_MEMTEST_SIZE MEMTEST_DATA_SIZE

#define _SINGLE_READBACK (SDRAM_DEBUG_READBACK_MEM_SIZE/SDRAM_DEBUG_READBACK_COUNT)
#define _READBACK_ERRORS_SIZE (_SINGLE_READBACK - sizeof(struct readback))
#define SDRAM_DEBUG_READBACK_LEN (_READBACK_ERRORS_SIZE / sizeof(struct memory_error))
#endif

static int sdram_debug_error_stats_on_error(
	unsigned int addr, unsigned int rdata, unsigned int refdata, void *arg)
{
	struct error_stats *stats = (struct error_stats *) arg;
	struct memory_error error = {
		.addr = addr,
		.data = rdata,
		.ref = refdata,
	};
	error_stats_update(stats, error);
	return 0;
}

static void sdram_debug_error_stats(void) {
	printf("Running initial memtest to fill memory ...\n");
	memtest_data((unsigned int *) MAIN_RAM_BASE, SDRAM_DEBUG_STATS_MEMTEST_SIZE, 1, NULL);

	struct error_stats stats;
	error_stats_init(&stats);

	struct memtest_config config = {
		.show_progress = 0,
		.read_only = 1,
		.on_error = sdram_debug_error_stats_on_error,
		.arg = &stats,
	};

	printf("Running read-only memtests ... \n");
	for (int i = 0; i < SDRAM_DEBUG_STATS_NUM_RUNS; ++i) {
		printf("Running read-only memtest %3d/%3d ... \r", i + 1, SDRAM_DEBUG_STATS_NUM_RUNS);
		memtest_data((unsigned int *) MAIN_RAM_BASE, SDRAM_DEBUG_STATS_MEMTEST_SIZE, 1, &config);
	}

	printf("\n");
	error_stats_print(&stats);
}

#ifdef SDRAM_DEBUG_READBACK_MEM_ADDR
static int sdram_debug_readback_on_error(
	unsigned int addr, unsigned int rdata, unsigned int refdata, void *arg)
{
	struct readback *readback = (struct readback *) arg;
	struct memory_error error = {
		.addr = addr,
		.data = rdata,
		.ref = refdata,
	};
	// run only as long as we have space for new entries
	return readback_add(readback, SDRAM_DEBUG_READBACK_LEN, error) != 1;
}

static void sdram_debug_readback(void)
{
	printf("Using storage @0x%08x with size 0x%08x for %d readbacks.\n",
		SDRAM_DEBUG_READBACK_MEM_ADDR, SDRAM_DEBUG_READBACK_MEM_SIZE, SDRAM_DEBUG_READBACK_COUNT);

	printf("Running initial memtest to fill memory ...\n");
	memtest_data((unsigned int *) MAIN_RAM_BASE, SDRAM_DEBUG_READBACK_MEMTEST_SIZE, 1, NULL);

	for (int i = 0; i < SDRAM_DEBUG_READBACK_COUNT; ++i) {
		struct readback *readback = (struct readback *)
			(SDRAM_DEBUG_READBACK_MEM_ADDR + i * READBACK_SIZE(SDRAM_DEBUG_READBACK_LEN));
		readback_init(readback);

		struct memtest_config config = {
			.show_progress = 0,
			.read_only = 1,
			.on_error = sdram_debug_readback_on_error,
			.arg = readback,
		};

		printf("Running readback %3d/%3d ... \r", i + 1, SDRAM_DEBUG_READBACK_COUNT);
		memtest_data((unsigned int *) MAIN_RAM_BASE, SDRAM_DEBUG_READBACK_MEMTEST_SIZE, 1, &config);
	}
	printf("\n");


	// Iterate over all combinations
	for (int i = 0; i < SDRAM_DEBUG_READBACK_COUNT; ++i) {
		struct readback *first = (struct readback *)
			(SDRAM_DEBUG_READBACK_MEM_ADDR + i * READBACK_SIZE(SDRAM_DEBUG_READBACK_LEN));

		for (int j = i + 1; j < SDRAM_DEBUG_READBACK_COUNT; ++j) {
			int nums[] = {i, j};
			struct readback *readbacks[] = {
				(struct readback *) (SDRAM_DEBUG_READBACK_MEM_ADDR + i * READBACK_SIZE(SDRAM_DEBUG_READBACK_LEN)),
				(struct readback *) (SDRAM_DEBUG_READBACK_MEM_ADDR + j * READBACK_SIZE(SDRAM_DEBUG_READBACK_LEN)),
			};

			// Compare i vs j and j vs i
			for (int k = 0; k < 2; ++k) {
				printf("Comparing readbacks %d vs %d:\n", nums[k], nums[1 - k]);
				int missing = readback_compare(readbacks[k], readbacks[1 - k], SDRAM_DEBUG_READBACK_VERBOSE);
				if (missing == 0)
					printf("  OK\n");
				else
					printf("  N missing = %d\n", missing);
			}
		}
	}
}
#endif

void sdram_debug(void)
{
#if defined(SDRAM_DEBUG_STATS_NUM_RUNS) && SDRAM_DEBUG_STATS_NUM_RUNS > 0
	printf("\nError stats:\n");
	sdram_debug_error_stats();
#endif

#ifdef SDRAM_DEBUG_READBACK_MEM_ADDR
	printf("\nReadback:\n");
	sdram_debug_readback();
#endif
}
#endif

#endif
