#ifndef LIBLITEDRAM_ACCESSORS_H
#define LIBLITEDRAM_ACCESSORS_H

#include <generated/sdram_phy.h>

typedef void (*action_callback)(int module);
typedef void (*delay_callback)(int module, int dq_line, action_callback action);

#ifdef SDRAM_PHY_READ_LEVELING_CAPABLE

static void read_inc_dq_delay(int module) {
	/* Increment delay */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_rdly_dq_inc_write(1);
	else
		ddrphy_B_rdly_dq_inc_write(1);
#else
	ddrphy_rdly_dq_inc_write(1);
#endif //SDRAM_PHY_SUBCHANNELS
}

static void read_rst_dq_delay(int module) {
	/* Reset delay */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_rdly_dq_rst_write(1);
	else
		ddrphy_B_rdly_dq_rst_write(1);
#else
	ddrphy_rdly_dq_rst_write(1);
#endif //SDRAM_PHY_SUBCHANNELS
}

static void read_inc_dq_bitslip(int module) {
	/* Increment bitslip */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_rdly_dq_bitslip_write(1);
	else
		ddrphy_B_rdly_dq_bitslip_write(1);
#else
	ddrphy_rdly_dq_bitslip_write(1);
#endif //SDRAM_PHY_SUBCHANNELS
}

static void read_rst_dq_bitslip(int module) {
	/* Reset bitslip */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_rdly_dq_bitslip_rst_write(1);
	else
		ddrphy_B_rdly_dq_bitslip_rst_write(1);
#else
	ddrphy_rdly_dq_bitslip_rst_write(1);
#endif //SDRAM_PHY_SUBCHANNELS
}

#endif // SDRAM_PHY_READ_LEVELING_CAPABLE

#ifdef SDRAM_PHY_WRITE_LEVELING_CAPABLE

static void write_inc_dq_delay(int module) {
	/* Increment DQ delay */
	ddrphy_wdly_dq_inc_write(1);
	cdelay(100);
}

static void write_rst_dq_delay(int module) {
#if defined(SDRAM_PHY_USDDRPHY) || defined(SDRAM_PHY_USPDDRPHY)
	/* Reset DQ delay */
	while (ddrphy_wdly_dqs_inc_count_read() != 0) {
		ddrphy_wdly_dq_inc_write(1);
		cdelay(100);
	}
#else
	/* Reset DQ delay */
	ddrphy_wdly_dq_rst_write(1);
	cdelay(100);
#endif //defined(SDRAM_PHY_USDDRPHY) || defined(SDRAM_PHY_USPDDRPHY)
}

static void write_inc_dqs_delay(int module) {
	/* Increment DQS delay */
	ddrphy_wdly_dqs_inc_write(1);
	cdelay(100);
}

static void write_rst_dqs_delay(int module) {
#if defined(SDRAM_PHY_USDDRPHY) || defined(SDRAM_PHY_USPDDRPHY)
	/* Reset DQS delay */
	while (ddrphy_wdly_dqs_inc_count_read() != 0) {
		ddrphy_wdly_dqs_inc_write(1);
		cdelay(100);
	}
#else
	/* Reset DQS delay */
	ddrphy_wdly_dqs_rst_write(1);
	cdelay(100);
#endif //defined(SDRAM_PHY_USDDRPHY) || defined(SDRAM_PHY_USPDDRPHY)
}


static void write_inc_delay(int module) {
	/* Increment DQ/DQS delay */
	write_inc_dq_delay(module);
	write_inc_dqs_delay(module);
}

static void write_rst_delay(int module) {
	write_rst_dq_delay(module);
	write_rst_dqs_delay(module);
}

#endif //SDRAM_PHY_WRITE_LEVELING_CAPABLE

#ifdef SDRAM_PHY_WRITE_LATENCY_CALIBRATION_CAPABLE

static void write_inc_dq_bitslip(int module) {
	/* Increment bitslip */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_wdly_dq_bitslip_write(1);
	else
		ddrphy_B_wdly_dq_bitslip_write(1);
#else
	ddrphy_wdly_dq_bitslip_write(1);
#endif //SDRAM_PHY_SUBCHANNELS
}

static void write_rst_dq_bitslip(int module) {
	/* Increment bitslip */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_wdly_dq_bitslip_rst_write(1);
	else
		ddrphy_B_wdly_dq_bitslip_rst_write(1);
#else
	ddrphy_wdly_dq_bitslip_rst_write(1);
#endif //SDRAM_PHY_SUBCHANNELS
}

#endif // SDRAM_PHY_WRITE_LATENCY_CALIBRATION_CAPABLE

#if defined(SDRAM_PHY_WRITE_LEVELING_CAPABLE) || defined(SDRAM_PHY_WRITE_LATENCY_CALIBRATION_CAPABLE) || defined(SDRAM_PHY_READ_LEVELING_CAPABLE)

static void sdram_select(int module, int dq_line) {
#ifdef SDRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dly_sel_write(1 << module);
	else
		ddrphy_B_dly_sel_write(1 << (module/2));
#else
	ddrphy_dly_sel_write(1 << module);
#endif //SDRAM_PHY_SUBCHANNELS

#ifdef SDRAM_DELAY_PER_DQ
	/* Select DQ line */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dq_dly_sel_write(1 << dq_line);
	else
		ddrphy_B_dq_dly_sel_write(1 << dq_line);
#else
	ddrphy_dq_dly_sel_write(1 << dq_line);
#endif //SDRAM_PHY_SUBCHANNELS
#endif //SDRAM_DELAY_PER_DQ
}

static void sdram_deselect(int module, int dq_line) {
#ifdef SDRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dly_sel_write(0);
	else
		ddrphy_B_dly_sel_write(0);
#else
	ddrphy_dly_sel_write(0);
#endif //SDRAM_PHY_SUBCHANNELS

#ifdef SDRAM_DELAY_PER_DQ
	/* Un-select DQ line */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_dq_dly_sel_write(0);
	else
		ddrphy_B_dq_dly_sel_write(0);
#else
	ddrphy_dq_dly_sel_write(0);
#endif //SRAM_PHY_SUBCHANNELS
#endif //SDRAM_DELAY_PER_DQ

#if defined(SDRAM_PHY_ECP5DDRPHY) || defined(SDRAM_PHY_GW2DDRPHY)
	/* Sync all DQSBUFM's, By toggling all dly_sel (DQSBUFM.PAUSE) lines. */
	ddrphy_dly_sel_write(0xff);
	ddrphy_dly_sel_write(0);
#endif //SDRAM_PHY_ECP5DDRPHY
}

static void sdram_leveling_action(int module, int dq_line, action_callback action) {
	/* Select module */
	sdram_select(module, dq_line);

	/* Action */
	action(module);

	/* Un-select module */
	sdram_deselect(module, dq_line);
}

#endif // defined(SDRAM_PHY_WRITE_LEVELING_CAPABLE) || defined(SDRAM_PHY_WRITE_LATENCY_CALIBRATION_CAPABLE) || defined(SDRAM_PHY_READ_LEVELING_CAPABLE)

#endif
