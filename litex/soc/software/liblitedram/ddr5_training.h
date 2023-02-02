#ifndef LIBLITEDRAM_DDR5_TRAINING_H
#define LIBLITEDRAM_DDR5_TRAINING_H

#include <generated/csr.h>
#ifdef CSR_SDRAM_BASE
#include <generated/sdram_phy.h>

#ifdef SDRAM_PHY_DDR5
#include <liblitedram/ddr5_helpers.h>

// Use max int16_t, all Fs could be interpreted as -1
#define UNSET_DELAY 0xefff

typedef void (*action_callback_t)(int channel, int rank, int address);
typedef void (*training_mode_callback_t)(int channel, int rank);

typedef int (*delay_checker_t)(int channel, int rank, int address, int offset);

typedef struct {
    struct {
        action_callback_t rst_dly;
        action_callback_t inc_dly;
    } ck;
    struct {
        training_mode_callback_t enter_training_mode;
        training_mode_callback_t exit_training_mode;
        action_callback_t rst_dly;
        action_callback_t inc_dly;
        delay_checker_t check;
    } cs;
    struct {
        training_mode_callback_t enter_training_mode;
        training_mode_callback_t exit_training_mode;
        action_callback_t rst_dly;
        action_callback_t inc_dly;
    } ca;
    struct {
        action_callback_t rst_dly;
        action_callback_t inc_dly;
    } par;
    enum {
        HOST_DRAM,
        HOST_RCD,
        RCD_DRAM,
        TRAINING_TYPE_COUNT,
    } training_type;
} training_ctx_t;

void sdram_ddr5_module_enumerate(void);
void sdram_ddr5_cs_ca_training(training_ctx_t *ctx);
void sdram_ddr5_read_training(void);
void sdram_ddr5_write_training(void);

extern training_ctx_t host_dram_ctx;
#if defined(CONFIG_HAS_I2C)
extern training_ctx_t host_rcd_ctx;
#endif // defined(CONFIG_HAS_I2C)

#endif // SDRAM_PHY_DDR5

#endif // CSR_SDRAM_BASE

#endif // LIBLITEDRAM_DDR5_TRAINING_H
