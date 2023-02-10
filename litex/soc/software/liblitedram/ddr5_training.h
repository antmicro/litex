#ifndef LIBLITEDRAM_DDR5_TRAINING_H
#define LIBLITEDRAM_DDR5_TRAINING_H

#include <generated/csr.h>
#ifdef CSR_SDRAM_BASE
#include <generated/sdram_phy.h>

#ifdef SDRAM_PHY_DDR5
#include <liblitedram/ddr5_helpers.h>

// DRAM Mode Registers Definitions
#define DRAM_SCRATCH_PAD 63

// Use max int16_t, all Fs could be interpreted as -1
#define UNSET_DELAY 0xefff

// max CL is 66 (JESD79-5A 3.5.2)
// if in 2N Mode, 1 more cycle is used for the command
#define MAX_READ_CYCLE_DELAY (66 + 1)

typedef void (*action_callback_t)(int channel, int rank, int address);
typedef void (*training_mode_callback_t)(int channel, int rank);

typedef int (*delay_checker_t)(int channel, int rank, int address, int offset);

typedef struct {
    struct {
        action_callback_t rst_dly;
        action_callback_t inc_dly;
    } ck;
    struct {
        int delays[CHANNELS][SDRAM_PHY_RANKS][2];
        int coarse_delays[CHANNELS][SDRAM_PHY_RANKS];
        int final_delays[CHANNELS][SDRAM_PHY_RANKS];

        training_mode_callback_t enter_training_mode;
        training_mode_callback_t exit_training_mode;

        action_callback_t rst_dly;
        action_callback_t inc_dly;

        delay_checker_t check;
    } cs;
    struct {
        int line_count;

        int delays[CHANNELS][14][2];
        int final_delays[CHANNELS][14];
        // If per-rank timings are available, the arrays above should be [CHANNELS][SDRAM_PHY_RANKS][14][2]
        // to cover clock/ca delays per rank

        training_mode_callback_t enter_training_mode;
        training_mode_callback_t exit_training_mode;

        action_callback_t rst_dly;
        action_callback_t inc_dly;

        delay_checker_t check;

        int (*has_line13)(int32_t channel);
    } ca;
    struct {
        int delays[CHANNELS][2];
        int final_delays[CHANNELS];

        action_callback_t rst_dly;
        action_callback_t inc_dly;
    } par;
    enum {
        HOST_DRAM,
        HOST_RCD,
        RCD_DRAM,
        TRAINING_TYPE_COUNT,
    } training_type;
    int ranks;
} training_ctx_t;

void sdram_ddr5_module_enumerate(training_ctx_t *ctx);
void sdram_ddr5_cs_ca_training(training_ctx_t *ctx);
void sdram_ddr5_read_training(training_ctx_t *ctx);
void sdram_ddr5_write_training(training_ctx_t *ctx);

extern training_ctx_t host_dram_ctx;
#if defined(CONFIG_HAS_I2C)
extern training_ctx_t host_rcd_ctx;
#endif // defined(CONFIG_HAS_I2C)

void sdram_ddr5_flow(void);

#endif // SDRAM_PHY_DDR5

#endif // CSR_SDRAM_BASE

#endif // LIBLITEDRAM_DDR5_TRAINING_H
