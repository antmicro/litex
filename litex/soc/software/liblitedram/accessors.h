#include <generated/csr.h>
#ifdef CSR_SDRAM_BASE
#include <generated/sdram_phy.h>
#ifndef LIBLITEDRAM_ACCESSORS_H
#define LIBLITEDRAM_ACCESSORS_H

extern int _sdram_write_leveling_bitslips[16];
extern int _sdram_write_leveling_dat_delays[16];

typedef void (*action_callback)(int module);
typedef void (*delay_callback)(int module, int dq_line, action_callback action);

#if defined(SDRAM_PHY_READ_LEVELING_CAPABLE) || defined(SDRAM_INPUT_DELAY_CAPABLE)

void read_inc_dq_delay(int module);
void read_rst_dq_delay(int module);
void read_inc_dq_bitslip(int module);
void read_rst_dq_bitslip(int module);

#endif // defined(SDRAM_PHY_READ_LEVELING_CAPABLE) || defined(SDRAM_INPUT_DELAY_CAPABLE)

#if defined(SDRAM_PHY_WRITE_LEVELING_CAPABLE) || defined(SDRAM_FULL_OUTPUT_DELAY_CAPABLE)

void write_inc_dq_delay(int module);
void write_rst_dq_delay(int module);
void write_inc_dqs_delay(int module);
void write_rst_dqs_delay(int module);

void write_inc_delay(int module);
void write_rst_delay(int module);

#endif // defined(SDRAM_PHY_WRITE_LEVELING_CAPABLE) || defined(SDRAM_FULL_OUTPUT_DELAY_CAPABLE)

#if defined(SDRAM_PHY_WRITE_LATENCY_CALIBRATION_CAPABLE) || defined(SDRAM_BITSLIP_CAPABLE)

void write_inc_dq_bitslip(int module);
void write_rst_dq_bitslip(int module);
void write_inc_dqs_bitslip(int module);
void write_rst_dqs_bitslip(int module);

#endif // defined(SDRAM_PHY_WRITE_LATENCY_CALIBRATION_CAPABLE) || defined(SDRAM_BITSLIP_CAPABLE)

#if defined(SDRAM_FULL_OUTPUT_DELAY_CAPABLE) || defined(SDRAM_PHY_ADDRESS_DELAY_CAPABLE)

void cs_inc_delay(int channel);
void cs_rst_delay(int channel);
void ca_inc_delay(int channel);
void ca_rst_delay(int channel);
void par_inc_delay(int channel);
void par_rst_delay(int channel);
void cs_ca_select(int channel, int ca_cs_line);
void cs_ca_deselect(int channel, int ca_cs_line);

#endif // defined(SDRAM_FULL_OUTPUT_DELAY_CAPABLE) || defined(SDRAM_PHY_ADDRESS_DELAY_CAPABLE)

#if defined(SDRAM_PHY_WRITE_LEVELING_CAPABLE) || defined(SDRAM_PHY_WRITE_LATENCY_CALIBRATION_CAPABLE) || defined(SDRAM_PHY_READ_LEVELING_CAPABLE) || \
defined(SDRAM_FULL_OUTPUT_DELAY_CAPABLE) || defined(SDRAM_INPUT_DELAY_CAPABLE) || defined(SDRAM_BITSLIP_CAPABLE)

void sdram_select(int module, int dq_line);
void sdram_deselect(int module, int dq_line);
void sdram_leveling_action(int module, int dq_line, action_callback action);

#endif // defined(SDRAM_PHY_WRITE_LEVELING_CAPABLE) || defined(SDRAM_PHY_WRITE_LATENCY_CALIBRATION_CAPABLE) || defined(SDRAM_PHY_READ_LEVELING_CAPABLE) ...

void sdram_write_leveling_rst_dat_delay(int module, int show);
void sdram_write_leveling_force_dat_delay(int module, int taps, int show);
void sdram_write_leveling_rst_bitslip(int module, int show);
void sdram_write_leveling_force_bitslip(int module, int bitslip, int show);
#endif
#endif
