#ifndef LIBLITEDRAM_DDR5_HELPERS_H
#define LIBLITEDRAM_DDR5_HELPERS_H

#include <generated/csr.h>
#ifdef CSR_SDRAM_BASE
#include <generated/sdram_phy.h>

#ifdef SDRAM_PHY_DDR5

#ifdef SDRAM_PHY_SUBCHANNELS
#define CHANNELS 2
#else
#define CHANNELS 1
#endif

int prep_payload (int cs, int command, int wrdata_en,
                  uint32_t wrdata_mask, int rddata_en);
void upload_payload(int channel, int phases, int payload);
void store_payload(int channel, int single);
void cmd_injector(int channel, int phases, int cs, int command,
                  int wrdata_en, uint32_t wrdata_mask, int rddata_en, int single);
void setup_rddata_cnt(int channel, int value);
void store_continuous(int channel);
void issue_single(int channel);
uint16_t get_data_module_phase(int channel, int module, int phase);
void set_data_module_phase(int channel, int module, int phase, uint16_t wrdata);

void setup_capture(int channel, int setup);
void start_capture(int channel);
void stop_capture(int channel);
uint32_t capture_and_reduce_result(int channel, int operation);
uint32_t capture_and_reduce_module(int channel, int module, int operation);
int or_sample(int channel);
int and_sample(int channel);
int wleveling_sample(int channel, int module);

void read_registers(int channel, int rank, int module);

void enable_dfi_2n_mode(void);
void disable_dfi_2n_mode(void);
void disable_dram_2n_mode(int, int);
int in_2n_mode(void);

#define WRDATA_BITMASK ((1<<(2*SDRAM_PHY_MODULES/CHANNELS))-1)

// Use max int16_t, all Fs could be interpreted as -1
#define UNSET_DELAY 0xefff

typedef void (*inc_func)(int, int, int);

void ck_rst(int channel, int rank, int address);
void ck_inc(int channel, int rank, int address);

void cs_rst(int channel, int rank, int address);
void cs_inc(int channel, int rank, int address);

void ca_rst(int channel, int rank, int address);
void ca_inc(int channel, int rank, int address);
uint16_t get_ca_dly(int channel, int rank, int address);

uint8_t lfsr_next(uint8_t input);
int compare_serial(int channel, int module,
                   uint16_t data,
                   int inv, int select);
int compare(int channel, int module,
            int data0, int data1,
            int inv, int select);

void rd_rst(int channel, int module);
void rd_inc(int channel, int module);
void idly_rst(int channel, int module);
void idly_inc(int channel, int module);
void idly_dq_rst(int channel, int module, int dq_line);
void idly_dq_inc(int channel, int module, int dq_line);

uint16_t get_rd_dq_dly(int channel, int module);
uint16_t get_rd_dqs_dly(int channel, int module);

void wr_dqs_rst(int channel, int module);
void wr_dqs_inc(int channel, int module);
void odly_dqs_rst(int channel, int module);
void odly_dqs_inc(int channel, int module);

uint16_t get_wr_dqs_dly(int channel, int module);

void wr_dq_rst(int channel, int module);
void wr_dq_inc(int channel, int module);
void odly_dm_rst(int channel, int module);
void odly_dm_inc(int channel, int module);
void odly_dq_rst(int channel, int module);
void odly_dq_inc(int channel, int module);
void odly_per_dq_rst(int channel, int module, int dq);
void odly_per_dq_inc(int channel, int module, int dq);

uint16_t get_wr_dq_dly(int channel, int module);
uint16_t get_wr_dm_dly(int channel, int module);

int captured_preamble(int channel, int module);
uint8_t recover_mrr_value(int channel, int module);
void setup_enumerate(int channel, int rank, int module);
void send_mpc(int channel, int rank, int cmd);
void send_mrw(int channel, int rank, int module, int reg, int value);
void send_mrr(int channel, int rank, int reg);
void send_wleveling_write(int channel, int rank);
void send_activate(int channel, int rank);
void send_precharge(int channel, int rank);
void send_write(int channel, int rank);
void send_write_byte(int channel, int rank, int module, int byte);
void send_read(int channel, int rank);

void enter_cs(int channel, int rank);
void exit_cs(int channel, int rank);
void cs_sample_prep(int channel, int rank, int address, int l2h);

void enter_ca(int channel, int rank);
void exit_ca(int channel, int rank);
void ca_sample_prep_current_period(int channel, int rank, int address, int l2h, int cs_dly);

void enter_write_leveling(int channel);
void wleveling_scan(int *cycle, int *got, int *start_cycle, int *start_delay,
                    int channel, int rank, int module);
void exit_write_leveling(int channel);
#endif // SDRAM_PHY_DDR5

#endif // CSR_SDRAM_BASE

#endif // LIBLITEDRAM_DDR5_HELPERS_H
