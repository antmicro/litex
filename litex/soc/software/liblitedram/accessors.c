#include <liblitedram/accessors.h>
#include <stdio.h>

int _sdram_write_leveling_bitslips[16];
int _sdram_write_leveling_dat_delays[16];

#if defined(SDRAM_PHY_READ_LEVELING_CAPABLE) || defined(SDRAM_INPUT_DELAY_CAPABLE)

void read_inc_dq_delay(int module) {
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

void read_rst_dq_delay(int module) {
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

void read_inc_dq_bitslip(int module) {
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

void read_rst_dq_bitslip(int module) {
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

#endif // defined(SDRAM_PHY_READ_LEVELING_CAPABLE) || defined(SDRAM_INPUT_DELAY_CAPABLE)

#if defined(SDRAM_PHY_WRITE_LEVELING_CAPABLE) || defined(SDRAM_FULL_OUTPUT_DELAY_CAPABLE)

void write_inc_dq_delay(int module) {
	/* Increment DQ delay */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_wdly_dq_inc_write(1);
	else
		ddrphy_B_wdly_dq_inc_write(1);
#else
	ddrphy_wdly_dq_inc_write(1);
#endif // SDRAM_PHY_SUBCHANNELS
	cdelay(100);
}

void write_rst_dq_delay(int module) {
#if defined(SDRAM_PHY_USDDRPHY) || defined(SDRAM_PHY_USPDDRPHY)
	/* Reset DQ delay */
	while (ddrphy_wdly_dqs_inc_count_read() != 0) {
		ddrphy_wdly_dq_inc_write(1);
		cdelay(100);
	}
#else
	/* Reset DQ delay */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_wdly_dq_rst_write(1);
	else
		ddrphy_B_wdly_dq_rst_write(1);
#else
	ddrphy_wdly_dq_rst_write(1);
#endif // SDRAM_PHY_SUBCHANNELS
	cdelay(100);
#endif //defined(SDRAM_PHY_USDDRPHY) || defined(SDRAM_PHY_USPDDRPHY)
}

void write_inc_dqs_delay(int module) {
	/* Increment DQS delay */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_wdly_dqs_inc_write(1);
	else
		ddrphy_B_wdly_dqs_inc_write(1);
#else
	ddrphy_wdly_dqs_inc_write(1);
#endif // SDRAM_PHY_SUBCHANNELS
	cdelay(100);
}

void write_rst_dqs_delay(int module) {
#if defined(SDRAM_PHY_USDDRPHY) || defined(SDRAM_PHY_USPDDRPHY)
	/* Reset DQS delay */
	while (ddrphy_wdly_dqs_inc_count_read() != 0) {
		ddrphy_wdly_dqs_inc_write(1);
		cdelay(100);
	}
#else
	/* Reset DQS delay */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_wdly_dqs_rst_write(1);
	else
		ddrphy_B_wdly_dqs_rst_write(1);
#else
	ddrphy_wdly_dqs_rst_write(1);
#endif // SDRAM_PHY_SUBCHANNELS
	cdelay(100);
#endif //defined(SDRAM_PHY_USDDRPHY) || defined(SDRAM_PHY_USPDDRPHY)
}


void write_inc_delay(int module) {
	/* Increment DQ/DQS delay */
	write_inc_dq_delay(module);
	write_inc_dqs_delay(module);
}

void write_rst_delay(int module) {
	write_rst_dq_delay(module);
	write_rst_dqs_delay(module);
}

#endif // defined(SDRAM_PHY_WRITE_LEVELING_CAPABLE) || defined(SDRAM_FULL_OUTPUT_DELAY_CAPABLE)

#if defined(SDRAM_PHY_WRITE_LATENCY_CALIBRATION_CAPABLE) || defined(SDRAM_BITSLIP_CAPABLE)

void write_inc_dq_bitslip(int module) {
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

void write_rst_dq_bitslip(int module) {
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

void write_inc_dqs_bitslip(int module) {
	/* Increment bitslip */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_wdly_dqs_bitslip_write(1);
	else
		ddrphy_B_wdly_dqs_bitslip_write(1);
#else
	ddrphy_wdly_dqs_bitslip_write(1);
#endif //SDRAM_PHY_SUBCHANNELS
}

void write_rst_dqs_bitslip(int module) {
	/* Increment bitslip */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (module < SDRAM_PHY_MODULES/2)
		ddrphy_A_wdly_dqs_bitslip_rst_write(1);
	else
		ddrphy_B_wdly_dqs_bitslip_rst_write(1);
#else
	ddrphy_wdly_dqs_bitslip_rst_write(1);
#endif //SDRAM_PHY_SUBCHANNELS
}

#endif // defined(SDRAM_PHY_WRITE_LATENCY_CALIBRATION_CAPABLE) || defined(SDRAM_BITSLIP_CAPABLE)

#if defined(SDRAM_FULL_OUTPUT_DELAY_CAPABLE) || defined(SDRAM_PHY_ADDRESS_DELAY_CAPABLE)

void cs_inc_delay(int channel) {
	/* Increment CS delay */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (channel)
		ddrphy_B_csdly_inc_write(1);
	else
		ddrphy_A_csdly_inc_write(1);
#else
	ddrphy_csdly_inc_write(1);
#endif //SDRAM_PHY_SUBCHANNELS

}

void cs_rst_delay(int channel) {
	/* Reset CS delay */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (channel)
		ddrphy_B_csdly_rst_write(1);
	else
		ddrphy_A_csdly_rst_write(1);
#else
	ddrphy_csdly_rst_write(1);
#endif //SDRAM_PHY_SUBCHANNELS

}

void ca_inc_delay(int channel) {
	/* Increment CA delay */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (channel)
		ddrphy_B_cadly_inc_write(1);
	else
		ddrphy_A_cadly_inc_write(1);
#else
	ddrphy_cadly_inc_write(1);
#endif //SDRAM_PHY_SUBCHANNELS

}

void ca_rst_delay(int channel) {
	/* Reset CA delay */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (channel)
		ddrphy_B_cadly_rst_write(1);
	else
		ddrphy_A_cadly_rst_write(1);
#else
	ddrphy_cadly_rst_write(1);
#endif //SDRAM_PHY_SUBCHANNELS
}

void par_inc_delay(int channel) {
	/* Increment PAR delay */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (channel)
		ddrphy_B_pardly_inc_write(1);
	else
		ddrphy_A_pardly_inc_write(1);
#else
	ddrphy_pardly_inc_write(1);
#endif //SDRAM_PHY_SUBCHANNELS

}

void par_rst_delay(int channel) {
	/* Reset PAR delay */
#ifdef SDRAM_PHY_SUBCHANNELS
	if (channel)
		ddrphy_B_pardly_rst_write(1);
	else
		ddrphy_A_pardly_rst_write(1);
#else
	ddrphy_pardly_rst_write(1);
#endif //SDRAM_PHY_SUBCHANNELS
}

void cs_ca_select(int channel, int ca_cs_line) {
#ifdef SDRAM_PHY_SUBCHANNELS
	if (channel)
		ddrphy_B_dly_sel_write(1 << ca_cs_line);
	else
		ddrphy_A_dly_sel_write(1 << ca_cs_line);
#else
	ddrphy_dly_sel_write(1 << ca_cs_line);
#endif //SDRAM_PHY_SUBCHANNELS
}

void cs_ca_deselect(int channel, int ca_cs_line) {
#ifdef SDRAM_PHY_SUBCHANNELS
	if (channel)
		ddrphy_B_dly_sel_write(0);
	else
		ddrphy_A_dly_sel_write(0);
#else
	ddrphy_dly_sel_write(0);
#endif //SDRAM_PHY_SUBCHANNELS

#if defined(SDRAM_PHY_ECP5DDRPHY) || defined(SDRAM_PHY_GW2DDRPHY)
	/* Sync all DQSBUFM's, By toggling all dly_sel (DQSBUFM.PAUSE) lines. */
	ddrphy_dly_sel_write(0xff);
	ddrphy_dly_sel_write(0);
#endif //SDRAM_PHY_ECP5DDRPHY
}

#endif // defined(SDRAM_FULL_OUTPUT_DELAY_CAPABLE) || defined(SDRAM_PHY_ADDRESS_DELAY_CAPABLE)

#if defined(SDRAM_PHY_WRITE_LEVELING_CAPABLE) || defined(SDRAM_PHY_WRITE_LATENCY_CALIBRATION_CAPABLE) || defined(SDRAM_PHY_READ_LEVELING_CAPABLE) || \
defined(SDRAM_FULL_OUTPUT_DELAY_CAPABLE) || defined(SDRAM_INPUT_DELAY_CAPABLE) || defined(SDRAM_BITSLIP_CAPABLE)

void sdram_select(int module, int dq_line) {
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

void sdram_deselect(int module, int dq_line) {
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

void sdram_leveling_action(int module, int dq_line, action_callback action) {
	/* Select module */
	sdram_select(module, dq_line);

	/* Action */
	action(module);

	/* Un-select module */
	sdram_deselect(module, dq_line);
}

#endif // defined(SDRAM_PHY_WRITE_LEVELING_CAPABLE) || defined(SDRAM_PHY_WRITE_LATENCY_CALIBRATION_CAPABLE) || defined(SDRAM_PHY_READ_LEVELING_CAPABLE) ...

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
