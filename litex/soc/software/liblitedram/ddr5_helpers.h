#include <generated/csr.h>
#ifdef CSR_SDRAM_BASE
#include <generated/sdram_phy.h>
#ifndef LIBLITEDRAM_DDR5_HELPERS_H
#define LIBLITEDRAM_DDR5_HELPERS_H

#ifdef MEMORY_TYPE_DDR5
int prep_payload (int cs, int command, int wrdata_en,
                  int wrdata_mask, int rddata_en);
void upload_payload(int channel, int phases, int payload);
void store_payload(int channel, int single);
void cmd_injector(int channel, int phases, int cs, int command,
                  int wrdata_en, int wrdata_mask, int rddata_en, int single);
void setup_rddata_cnt(int channel, int value);
void issue_single(int channel);
uint16_t get_data_module_phase(int channel, int module, int phase);
void set_data_module_phase(int channel, int module, int phase, uint16_t wrdata);

void setup_capture(int channel, int setup);
void start_capture(int channel);
void stop_capture(int channel);
uint32_t capture_result(int channel);
int or_sample(int channel);
int and_sample(int channel);

void enable_dfi_2n_mode(void);
void disable_dfi_2n_mode(void);
void disable_dram_2n_mode(int, int);

#define UNSET_DELAY 0xffff

typedef void (*inc_func)(int, int, int);

void cs_rst(int channel, int rank, int address);
void cs_inc(int channel, int rank, int address);

void ca_rst(int channel, int rank, int address);
void ca_inc(int channel, int rank, int address);

int compare(int channel, int module,
            int data0, int data1,
            int inv, int select);

void rd_rst(int channel, int module);
void rd_inc(int channel, int module);
void idly_rst(int channel, int module);
void idly_inc(int channel, int module);

void wr_rst(int channel, int module);
void wr_inc(int channel, int module);

int captured_preamble(int channel, int module);
uint8_t recover_mrr_value(int channel, int module);
void setup_enumerate(int channel, int rank, int module);
void send_mpc(int channel, int rank, int cmd);
void send_mrw(int channel, int rank, int module, int reg, int value);
void send_mrr(int channel, int rank, int reg);

void enter_cs(int channel, int rank);
void exit_cs(int channel, int rank);
void cs_sample_prep(int channel, int rank, int address, int l2h);

void enter_ca(int channel, int rank);
void exit_ca(int channel, int rank);
void ca_sample_prep_current_period(int channel, int rank, int address, int l2h);
void ca_sample_prep_previous_period(int channel, int rank, int address, int l2h);

extern int32_t _ca_results[14][2];
void setup_ca_results(void);

void mid_point_calc_and_set(uint8_t* success, const char* format_str, int channel,
                            int rank, int address, int32_t index, int32_t left,
                            int32_t right, inc_func inc, int cs);
#endif // MEMORY_TYPE_DDR5
#endif // LIBLITEDRAM_DDR5_HELPERS_H
#endif // CSR_SDRAM_BASE
