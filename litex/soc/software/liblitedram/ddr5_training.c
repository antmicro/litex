#include <liblitedram/ddr5_training.h>

#if defined(CSR_SDRAM_BASE) && defined(SDRAM_PHY_DDR5)
#include <liblitedram/ddr5_helpers.h>

#include <liblitedram/sdram_rcd.h>
#include <liblitedram/sdram_spd.h>

#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>

//#define INFO_DDR5
//#define DEBUG_DDR5
//#define CA_INFO_DDR5
//#define CA_DEBUG_DDR5
//#define READ_INFO_DDR5
//#define READ_DEBUG_DDR5
//#define READ_DEEP_DEBUG_DDR5
//#define WRITE_INFO_DDR5
//#define WRITE_DEBUG_DDR5
//#define WRITE_DEEP_DEBUG_DDR5
#if defined(DEBUG_DDR5) && !defined(CA_DEBUG_DDR5)
    #define CA_DEBUG_DDR5
#endif
#if defined(INFO_DDR5) && !defined(READ_INFO_DDR5)
    #define READ_INFO_DDR5
#endif
#if defined(DEBUG_DDR5) && !defined(WRITE_INFO_DDR5)
    #define WRITE_INFO_DDR5
#endif
#if defined(DEBUG_DDR5) && !defined(READ_DEBUG_DDR5)
    #define READ_DEBUG_DDR5
#endif
#if defined(DEBUG_DDR5) && !defined(WRITE_DEBUG_DDR5)
    #define WRITE_DEBUG_DDR5
#endif

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)

// Addressing: channel, pin, 0-right eye closing, 1-left eye closing
//      \______________/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
//      --------<============>-------------
//              | valid data |
//most left point            most right point
//
// Delay clock has effects of moving signal to "the left"
//      ‾‾‾‾‾\______________/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
//      --------<============>-------------
// while delaying signal itself to "the right"
//      \______________/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
//      --------------<============>-------------

static int32_t helper_arr[2*MAX(64, SDRAM_PHY_DELAYS)];
static int32_t helper_arr_it;

static void clear_helper_arr(void) {
    helper_arr_it = 0;
    for (int i = 0; i < sizeof(helper_arr)/sizeof(int); ++i)
        helper_arr[i] = 0;
}

static int reduce_cs(uint32_t cs, int modules) {
    uint32_t ok, module;
    ok = 1;
    for (module = 0; module < modules; ++module)
        ok &= (cs >> module)&1;
    return !!ok;
}

static void find_eye_in_helper_arr(int *left, int *right, int mid) {
    int it = 0;
    while (helper_arr[mid - it]) ++it;
    if (helper_arr[mid]) *right = - (it - 1);
    for (it = 0; it < mid; ++it) {
        if (helper_arr[mid + it] && *right == UNSET_DELAY)
            *right = it;
        if (!helper_arr[mid + it] && *right != UNSET_DELAY) {
            *left = it;
            return;
        }
    }
    if (helper_arr[2*mid - 1])
        *left = it;
}

static void CS_scan_single(const training_ctx_t *const ctx, int32_t channel, int32_t rank,
    int shift_0101) {
    int csdly;
    uint32_t works;

    for (csdly = 0; csdly < ctx->max_delay_taps; csdly++) {
        works  = ctx->cs.check(channel, rank, 0, shift_0101, ctx->modules, ctx->die_width);
        works |= ctx->cs.check(channel, rank, 0, !shift_0101, ctx->modules, ctx->die_width);
        printf("%d", reduce_cs(works, ctx->modules));
        helper_arr[helper_arr_it++] = reduce_cs(works, ctx->modules);
        ctx->cs.inc_dly(channel, rank, 0);
    }
    ctx->cs.rst_dly(channel, rank, 0);
}

static void CS_scan(const training_ctx_t *const ctx, int32_t channel, int32_t rank) {
    clear_helper_arr();
    ctx->cs.rst_dly(channel, rank, 0);

    // Enter CS training
    printf("Rank: %2"PRId32"\t|", rank);
    ctx->cs.enter_training_mode(channel, rank);
    CS_scan_single(ctx, channel, rank, 1);
    // Reset CS_n training states
    printf("|");
    CS_scan_single(ctx, channel, rank, 0);
    printf("|\n");
    // Exit CS training
    ctx->cs.exit_training_mode(channel, rank);
}

static void CS_training(training_ctx_t *const ctx, int32_t channel, uint8_t *success) {
    int left_side, right_side;
    int32_t csdly, coarse;

    for (int _rank = 0; _rank < ctx->ranks; ++_rank) {
        left_side = UNSET_DELAY;
        right_side = UNSET_DELAY;
        CS_scan(ctx, channel, _rank);
        find_eye_in_helper_arr(&left_side, &right_side, ctx->max_delay_taps);

        if (left_side == UNSET_DELAY || right_side == UNSET_DELAY) {
            printf("CS:%2d Eye width:0 Failed\n", _rank);
            *success = 0;
            return;
        }

        // Set up coarse delay adjustment until we get CA results
        printf("Rank delays: %2d:%2d\n", right_side, left_side);
        coarse = (right_side + left_side) / 2;
        coarse = MAX(0, coarse);
        printf("Coarse adjustment:%"PRId32"\n", coarse);
        ctx->cs.coarse_delays[channel][_rank] = coarse;

        ctx->cs.rst_dly(channel, _rank, 0);
        for (csdly = 0; csdly < coarse; ++csdly)
            ctx->cs.inc_dly(channel, _rank, 0);

        ctx->cs.delays[channel][_rank][0] = right_side;
        ctx->cs.delays[channel][_rank][1] = left_side;
    }
}

/**
 * CA_setup_array
 *
 * Fills CA delays array in the training_ctx_t with initial values
 */
static void CA_setup_array(training_ctx_t *const ctx) {
    int channel, address;
    for (channel = 0; channel < ctx->channels; ++channel) {
        for (address = 0; address < 14; ++address) {
           ctx->ca.delays[channel][address][0] = -ctx->max_delay_taps;
           ctx->ca.delays[channel][address][1] = ctx->max_delay_taps;
        }
    }
}

/**
 * CA_check_lines
 *
 * Detect and assign CA lines count.
 * Depending on the die density and usage of die stacking,
 * CA13 may be used or not.
 */
static void CA_check_lines(training_ctx_t *const ctx, int32_t channel) {
    if (ctx->training_type == HOST_DRAM) {
        ctx->ca.enter_training_mode(channel, 0);
        if (ctx->ca.has_line13(channel))
            ctx->ca.line_count = 14;
        else
            ctx->ca.line_count = 13;
        ctx->ca.exit_training_mode(channel, 0);
    }
    printf("DDR5 module has %d address lines\n", ctx->ca.line_count);
}

static void CA_scan_single(training_ctx_t *const ctx, int32_t channel, int32_t rank, int32_t address, int shift_back) {
    int works, cadly;

    ctx->ca.rst_dly(channel, rank, address);
    for (cadly = 0; cadly < ctx->max_delay_taps; cadly++) {
        works = ctx->ca.check(channel, rank, address, shift_back);
        printf("%d", !!works);
        helper_arr[helper_arr_it++] = works;
        ctx->ca.inc_dly(channel, rank, address);
    }
    ctx->ca.rst_dly(channel, rank, address);
}

static void CA_scan(training_ctx_t *const ctx, int32_t channel, int32_t rank, int32_t address) {
    clear_helper_arr();
    ctx->ca.rst_dly(channel, rank, address);

    // Enter CA training
    ctx->ca.enter_training_mode(channel, rank);
    printf("CA line:%2"PRId32"\t|", address);
    CA_scan_single(ctx, channel, rank, address, 1);
    printf("|");
    CA_scan_single(ctx, channel, rank, address, 0);
    printf("|\n");
    // Exit CA training early
    ctx->ca.exit_training_mode(channel, rank);
}

static void CA_training(training_ctx_t *const ctx , int32_t channel, uint8_t *success) {
    int left_side, right_side;
    int32_t address;
    int32_t _rank, _max_rank;

    _rank = 0;
    _max_rank = ctx->ranks;

    if (ctx->training_type == HOST_RCD)
        _max_rank = 1;

    for (; _rank < _max_rank; ++_rank) {
        printf("Rank:%2"PRId32"\n", _rank);
        for (address = 0; address < ctx->ca.line_count; address++) {
            left_side = UNSET_DELAY;
            right_side = UNSET_DELAY;
            CA_scan(ctx, channel, _rank, address);
            find_eye_in_helper_arr(&left_side, &right_side, ctx->max_delay_taps);

            // Check if we found the eye
            if (left_side == UNSET_DELAY || right_side == UNSET_DELAY) {
                // If not, then exit CA training
                printf("CA line:%2"PRId32" Eye width:0 Failed\n", address);
                *success = 0;
                return;
            }

            if (right_side > ctx->ca.delays[channel][address][0])
                ctx->ca.delays[channel][address][0] = right_side;

            if (left_side < ctx->ca.delays[channel][address][1])
                ctx->ca.delays[channel][address][1] = left_side;
        }
    }
}

/**
 * CS_CA_rescan
 *
 * Performs scan of CS and CA delays.
 * It could be just running `ctx->cs.check` and `ctx->ca.check`
 * on currently selected delays, but by using functions from
 * training procedure, we get output in the same format, which
 * can be used to compare training results.
 */
static void CS_CA_rescan(training_ctx_t *const ctx , int ckdly, int channel) {
    int _channel, _max_channel;
    int32_t address;
    int cntdly;

    _channel = channel;
    _max_channel = channel + 1;
    if (channel == -1) {
        _channel = 0;
        _max_channel = ctx->channels;
    }

    printf("Re-scan CS/CA\n");
    for (; _channel < _max_channel; ++_channel) {
        printf("Subchannel:%c\n", 'A'+_channel);

        //                    CS rescan                    //
        for (int _rank = 0; _rank < ctx->ranks; ++_rank) {
            CS_scan(ctx, _channel, _rank);

            // Restore CS delay
            ctx->cs.rst_dly(_channel, _rank, 0);
            for (cntdly = 0; cntdly < ctx->cs.final_delays[_channel][_rank]; ++cntdly)
                ctx->cs.inc_dly(_channel, _rank, 0);
        }

        for (int _rank = 0; _rank < ctx->ranks; ++_rank) {
            if (ctx->training_type == HOST_RCD && _rank == 1)
                continue;
            //                    CA rescan                    //
            for (address = 0; address < ctx->ca.line_count; address++) {
                CA_scan(ctx, _channel, _rank, address);

                // Restore CA delay
                ctx->ca.rst_dly(_channel, _rank, address);
                for (cntdly = 0; cntdly < ctx->ca.final_delays[_channel][address]; ++cntdly)
                    ctx->ca.inc_dly(_channel, _rank, address);
            }
        }
    }
}

/**
 * CS_CA_calculate_midpoints
 *
 * Calculate eye midpoints for all trained signals
 * and save them in their `final_delays`.
 *
 * Also find minimal and maximal used delays to use
 * them later to shift the clock.
 */
static void CS_CA_calculate_midpoints(training_ctx_t *const ctx , int *min, int *max, int channel) {
    int _channel, _max_channel;
    int address;

    _channel = channel;
    _max_channel = channel + 1;
    if (channel == -1) {
        _channel = 0;
        _max_channel = ctx->channels;
    }

    int temp;
    for (; _channel < _max_channel; ++_channel) {
        printf("Subchannel:%c Timings\n", 'A'+_channel);

        for (int _rank = 0; _rank < ctx->ranks; ++_rank) {
            temp = (ctx->cs.delays[_channel][_rank][0] + ctx->cs.delays[_channel][_rank][1])/2;
            printf("Rank:\t\t%2d: min delay %2d, max delay %2d, center %2d\n",
                _rank, ctx->cs.delays[_channel][_rank][0], ctx->cs.delays[_channel][_rank][1], temp);

            ctx->cs.final_delays[_channel][_rank] = temp;
            *min = MIN(*min, temp);
            *max = MAX(*max, temp);
        }

        for (address = 0; address < ctx->ca.line_count; address++) {
            temp = (ctx->ca.delays[_channel][address][0] + ctx->ca.delays[_channel][address][1])/2;
            printf("CA line:\t%2d: min delay %2d, max delay %2d, center %2d\n",
                address, ctx->ca.delays[_channel][address][0], ctx->ca.delays[_channel][address][1], temp);

            ctx->ca.final_delays[_channel][address] = temp;
            *min = MIN(*min, temp);
            *max = MAX(*max, temp);
        }

        // FIXME: add parity training
        //if (ctx->training_type == HOST_RCD) {
        //    temp = (ctx->par.delays[_channel][0] + ctx->par.delays[_channel][1])/2;
        //    printf("PAR: min delay %2d, max delay %2d, center %2d\n",
        //        ctx->par.delays[_channel][0], ctx->par.delays[_channel][1], temp);

        //    ctx->par.final_delays[_channel] = temp;
        //    *min = MIN(*min, temp);
        //    *max = MAX(*max, temp);
        //}
    }
}

/**
 * CS_CA_set_adjusted_delays
 *
 * Set signal delays to ones stored in the `final_delays`.
 * Delays are decreased by `ck_offset` to account for the
 * clock delay.
 */
static void CS_CA_set_adjusted_delays(training_ctx_t *const ctx , int ck_offset, int channel) {
    int _channel, _max_channel;
    int address, cntdly;

    _channel = channel;
    _max_channel = channel + 1;
    if (channel == -1) {
        _channel = 0;
        _max_channel = ctx->channels;
    }

    for (; _channel < _max_channel; ++_channel) {
        printf("Subchannel:%c Adjusted Tick_offsetgs\n", 'A'+_channel);

        for (int _rank = 0; _rank < ctx->ranks; ++_rank) {
            ctx->cs.final_delays[_channel][_rank] -= ck_offset;
            printf("Rank:\t%2d center point delay:%2d\n", _rank, ctx->cs.final_delays[_channel][_rank]);
            ctx->cs.rst_dly(_channel, _rank, 0);
            for (cntdly = 0; cntdly < ctx->cs.final_delays[_channel][_rank]; ++cntdly)
                ctx->cs.inc_dly(_channel, _rank, 0);
        }

        for (address = 0; address < ctx->ca.line_count; address++) {
            ctx->ca.final_delays[_channel][address] -= ck_offset;
            printf("CA:\t%2d center point delay:%2d\n", address, ctx->ca.final_delays[_channel][address]);
            ctx->ca.rst_dly(_channel, 0, address);
            for (cntdly = 0; cntdly < ctx->ca.final_delays[_channel][address]; ++cntdly)
                ctx->ca.inc_dly(_channel, 0, address);
        }

        // FIXME: add parity training
        //if (ctx->training_type == HOST_RCD) {
        //    ctx->par.final_delays[_channel] -= ck_offset;
        //    printf("PAR center point delay:%2d\n", ctx->par.final_delays[_channel]);
        //    ctx->par.rst_dly(_channel, 0, 0);
        //    for (cntdly = 0; cntdly < ctx->par.final_delays[_channel]; ++cntdly)
        //        ctx->par.inc_dly(_channel, 0, 0);
        //}
    }
}

/**
 * CK_CS_CA_finalize_timings
 *
 * CS and CA trainings were successful and we found an eye for
 * all trained signals. Now we need to calculate the midpoints
 * of such eyes.
 *
 * Some eyes could start on negative offset relative to the
 * clock, so we need to fix that by delaying the clock.
 * This way, all midpoints are in the [0, ctx->max_delay_taps)
 * range.
 */
static void CK_CS_CA_finalize_timings(training_ctx_t *const ctx , int channel) {
    int new_ckdly, cntdly;
    int min, max;
    min = ctx->max_delay_taps;
    max = -ctx->max_delay_taps;

    // Calculate eye midpoints for all trained signals and save them in final_delays.
    // Also find minimal and maximal used delays to use them later to shift the clock
    CS_CA_calculate_midpoints(ctx, &min, &max, channel);

    printf("Max center point delay:%2d, min center point delay:%2d, spread:%2d\n", max, min, max-min);

    // Calculate new clock delay. It is the smallest of used delays
    printf("Adjusting clock delay, so min center point is at delay 0\n");
    new_ckdly = (ctx->max_delay_taps - min) % ctx->max_delay_taps;

    // Set new clock delay
    printf("New clock delay:%2d\n", new_ckdly);

    ctx->ck.rst_dly(channel, 0, 0);
    for (cntdly = 0; cntdly < new_ckdly; ++cntdly) {
        ctx->ck.inc_dly(channel, 0, 0);
    }
    busy_wait(10);

    // Now that CK is shifted, we can set new delays calculated
    // in `CS_CA_calculate_midpoints` adjusted by the clock offset,
    // which is equal to the minimal midpoint.
    CS_CA_set_adjusted_delays(ctx, min, channel);

    // Make sure that selected delays still work
    CS_CA_rescan(ctx, new_ckdly, channel);
}

#ifdef SKIP_NO_DELAYS
void sdram_ddr5_cs_ca_training(training_ctx_t *const ctx , int channel) {
    printf("CS/CA training impossible\n"
           "Keeping DRAM in 2N mode\n");
}
#else
void sdram_ddr5_cs_ca_training(training_ctx_t *const ctx , int channel) {
#ifndef SDRAM_PHY_ADDRESS_DELAY_CAPABLE
    printf("WARNING:\n"
           "PHY does not have IO delays on address lines!!!\n"
           "BIOS will try to check if 1N mode is possible, but it may be unstable.\n"
           "Build BIOS with -DSKIP_NO_DELAYS, to skip CS/CA training and force 2N mode.\n");
#endif // SDRAM_PHY_ADDRESS_DELAY_CAPABLE
    int32_t _channel, _max_channel;
    uint8_t CS_success, CA_success;
    if (ctx->rate == DDR && ctx->training_type != RCD_DRAM)
        disable_dfi_2n_mode();

    _channel = channel;
    _max_channel = channel + 1;
    if (channel == -1) {
        _channel = 0;
        _max_channel = ctx->channels;
    }

    CS_success = 1;
    CA_success = 1;
    CA_setup_array(ctx);
    for (; _channel < _max_channel; ++_channel) {
        ctx->ck.rst_dly(_channel, 0, 0);

        printf("Subchannel:%c CS training\n", (char)('A'+_channel));
        CS_training(ctx, _channel, &CS_success);
        ctx->CS_CA_successful &= CS_success;
#ifndef KEEP_GOING_ON_DRAM_ERROR
        if (!ctx->CS_CA_successful)
            return;
#endif // KEEP_GOING_ON_DRAM_ERROR
        printf("CA training\n");
        CA_check_lines(ctx, _channel);
        CA_training(ctx, _channel, &CA_success);
        ctx->CS_CA_successful &= CA_success;
#ifndef KEEP_GOING_ON_DRAM_ERROR
        if (!ctx->CS_CA_successful)
            return;
#endif // KEEP_GOING_ON_DRAM_ERROR
    }

    ctx->CS_CA_successful &= (CS_success & CA_success);
    if (ctx->CS_CA_successful) {
        CK_CS_CA_finalize_timings(ctx, channel);
    }
}
#endif // SKIP_NO_DELAYS

// MR2:OP[7] value to use, whenever MR2 is being modified
static int use_internal_write_timing = 0;

// MR2:OP[4] indicates that MPCs are single cycle
int single_cycle_MPC = 0;

int enumerated = 0;

static void sdram_ddr5_module_enumerate(int rank, int width, int channels, int modules) {
    int channel, module;
    if (modules > 15) {
        printf("Too many modules on single rank to enumerate,\n"
               "maximum is 15 but this design has %2d\n", modules);
        enumerated = 0;
        return;
    }
    printf("Enumerating rank:%2d\n", rank);
    for (channel = 0; channel < channels; channel++) {
        printf("\tEnumerating subchannel:%c\n", (char)('A'+channel));
        // Enter PDA Enumerate Programming Mode
        send_mpc(channel, rank, 0xB, 1);
        for (module = 0; module < modules; module++) {
            printf("\t\tmodule:%2d\n", module);
#ifndef CA_INFO_DDR5
            setup_enumerate(channel, rank, module, width, 0);
#else
            setup_enumerate(channel, rank, module, width, 1);
#endif // CA_INFO_DDR5
        }
        // Exit PDA Enumerate Programming Mode
        send_mpc(channel, rank, 0xA, 1);
        busy_wait_us(1);
        send_mpc(channel, rank, 0xA, 0);
        busy_wait_us(1);
    }
    enumerated = 1;
}

static bool sdram_ddr5_check_enumerate(int rank, int width, int channels, int modules) {
    int channel, module;
    bool ok = true;
    if (!enumerated)
        return false;
    printf("Checking rank:%2d\n", rank);
    for (channel = 0; channel < channels; channel++) {
        printf("\tChecking subchannel:%c\n", (char)('A'+channel));
        send_mrw(channel, rank, MODULE_BROADCAST, 2, 1|use_internal_write_timing|single_cycle_MPC);
        printf("\tBase line:");
#ifndef CA_INFO_DDR5
        ok &= check_enumerate(channel, rank, -1, width, 0);
#else
        ok &= check_enumerate(channel, rank, -1, width, 1);
#endif // CA_INFO_DDR5
        for (module = 0; module < modules; module++) {
            printf("\t\tmodule:%2d", module);
#ifndef CA_INFO_DDR5
            ok &= check_enumerate(channel, rank, module, width, 0);
#else
            ok &= check_enumerate(channel, rank, module, width, 1);
#endif // CA_INFO_DDR5
        }
        send_mrw(channel, rank, MODULE_BROADCAST, 2, 0|use_internal_write_timing|single_cycle_MPC);
        busy_wait_us(1);
    }
    return ok;
}

static bool dram_enumerate(training_ctx_t *const ctx , int rank) {
    sdram_ddr5_module_enumerate(rank, ctx->die_width, ctx->channels, ctx->modules);
    return sdram_ddr5_check_enumerate(rank, ctx->die_width, ctx->channels, ctx->modules);
}

static const uint8_t seeds0[] = {
    0x1c, 0x5a, 0x24, 0x11,
#ifndef DDR5_TRAINING_SIM
    0x36, 0xaa, 0xc1, 0xee,
#endif
};

static const uint8_t seeds1[] = {
    0x72, 0x55, 0x95, 0x3e,
#ifndef DDR5_TRAINING_SIM
    0x59, 0x3c, 0x48, 0xfd,
#endif
};

static const int seeds_count = sizeof(seeds0) / sizeof(seeds0[0]);

static const uint16_t serial[] = {
    0x0000, 0xffff,
    0xfffe, 0xfffd, 0xfffb, 0xfff7, 0xffef, 0xffdf, 0xffbf, 0xff7f,
    0xfeff, 0xfdff, 0xfbff, 0xf7ff, 0xefff, 0xdfff, 0xbfff, 0x7fff,
    0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000};
static const int serial_count = sizeof(serial) / sizeof(serial[0]);

#ifdef READ_DEEP_DEBUG_DDR5
static int _read_verbosity = 3;
#elif defined(READ_DEBUG_DDR5)
static int _read_verbosity = 2;
#elif defined(READ_INFO_DDR5)
static int _read_verbosity = 1;
#else
static int _read_verbosity = 0;
#endif

/**
 * read_serial_number
 *
 * Reads serial number from the mode registers.
 * It is a 5 byte value stored in registers
 * MR65-MR69.
 * JESD79-5A 3.5.66-70
 */
static uint64_t read_serial_number(int channel, int rank, int module, int width) {
    int i;
    uint64_t serial_number = 0;

    // Serial number is a 5 byte value
    for (i = 0; i < 5; i++) {
        // Base register 65
        send_mrr(channel, rank, 65+i);
        serial_number = (serial_number << 8) | recover_mrr_value(channel, module, width);
    }

    return serial_number;
}

/**
 * enter_rptm
 *
 * Enters Read Preamble Training Mode.
 * Sets up Mode Registers to be used during the training.
 * JESD79-5A 4.18.2
 */
static void enter_rptm(int channel, int rank) {
    // Setup MRs
    send_mrw(channel, rank, MODULE_BROADCAST, 28, 0xA5); // select DQL to invert
    send_mrw(channel, rank, MODULE_BROADCAST, 29, 0xA5); // select DQU to invert
    send_mrw(channel, rank, MODULE_BROADCAST, 30, 0x33); // select data sources for DQ lines

    // Actual write to enter Read Preamble Training Mode
    send_mrw(channel, rank, MODULE_BROADCAST, 2, 1|use_internal_write_timing|single_cycle_MPC);
}

/**
 * exit_rptm
 *
 * Exits Read Preamble Training Mode.
 * Clears Mode Registers set up in enter_rptm to default values.
 * JESD79-5A 4.18.2
 */
static void exit_rptm(int channel, int rank) {
    // Setup MRs
    send_mrw(channel, rank, MODULE_BROADCAST, 25, 0); // restore Serial mode
    send_mrw(channel, rank, MODULE_BROADCAST, 26, 0x5a); // restore default data
    send_mrw(channel, rank, MODULE_BROADCAST, 27, 0x3c); // restore default data
    send_mrw(channel, rank, MODULE_BROADCAST, 28, 0); // don't invert DQL[7:0]
    send_mrw(channel, rank, MODULE_BROADCAST, 29, 0); // don't invert DQU[7:0]

    // Actual write to exit Read Preamble Training Mode
    send_mrw(channel, rank, MODULE_BROADCAST, 2, 0|use_internal_write_timing|single_cycle_MPC);
}

/**
 * rd_cycle_dly_idly_check_if_works
 *
 * Checks if for selected read cycle delay and
 * input DQ delay Mode Register readout is returning
 * correct data.
 *
 * Two tests are being performed:
 *  - Serial - we get data we wrote before
 *  - LFSR   - we get subsequent values of LFSR we seeded
 *
 * JESD79-5A 4.18.2
 */
static int rd_cycle_dly_idly_check_if_works(int channel, int rank, int module, int width) {
    int works = 1;
    int seed;

#ifndef DDR5_TRAINING_SIM
    // Check if Serial readout works
    for (seed = 0; seed < serial_count && works; seed++) {
        /* Setup MRs */
        send_mrw(channel, rank, module, 25, 0); // select Serial mode
        send_mrw(channel, rank, module, 26, serial[seed]&0xff);
        send_mrw(channel, rank, module, 27, serial[seed]>>8);
        for (int i = 0 ; i < 16 && works; ++i) {
            send_mrr(channel, rank, 31);
            works &= compare_serial(channel, rank, module, width, serial[seed], 0xA5, 0);
            if (!works && _read_verbosity > 1) {
                compare_serial(channel, rank, module, width, serial[seed], 0xA5, 1);
            }
        }
    }
#endif // DDR5_TRAINING_SIM
    if (!works)
        return works;

    // Check if LFSR readout works
    for (seed = 0; seed < seeds_count && works; ++seed) {
        for (int i = 0 ; i < 16 && works; ++i) {
            /* Setup MRs */
            send_mrw(channel, rank, module, 25, 1); // select LFSR mode
            send_mrw(channel, rank, module, 26, seeds0[seed]);
            send_mrw(channel, rank, module, 27, seeds1[seed]);
            send_mrr(channel, rank, 31);
            works &= compare(channel, rank, module, width, seeds0[seed], seeds1[seed], 0xA5, 0x33, 0);
            if (!works && _read_verbosity > 1)
                compare(channel, rank, module, width, seeds0[seed], seeds1[seed], 0xA5, 0x33, 1);
        }
    }
    if (!works)
        return 1;
    return 3;
}

/**
 * find_read_preamble_cycle
 *
 * Finds the first cycle in which we detect the read preamble.
 * It will be used to configure the read cycle delay in the basephy.
 *
 * This delay depends on the CL set in the MR0 of the DRAM.
 * `read_training_data_scan` performs a more extensive check to find
 * the read DQ delay.
 */
static int find_read_preamble_cycle(int channel, int rank, int module, int width, int max_delay_taps) {
    int rd_cycle_dly, idly, preamble;

    // in this stage we don't care about eye end
    eye_t eye = DEFAULT_EYE;

    if (_read_verbosity)
        printf("Finding read preamble\n");

    /* Coarse alignment */
    rd_rst(channel, module, width);
    for (rd_cycle_dly = 0; rd_cycle_dly < MAX_READ_CYCLE_DELAY && eye.state != AFTER; rd_cycle_dly ++) {
        if (_read_verbosity)
            printf("%2d|", rd_cycle_dly);
        if (_read_verbosity > 2)
            printf("\nPreamble CK dly:%"PRIu16, get_rd_preamble_ck_dly(channel, module, width));

        idly_rst(channel, module, width);
        for (idly = 0; idly < max_delay_taps; idly++) {
            send_mrr(channel, rank, 31);
            preamble = captured_preamble(channel, module, width);

            if (_read_verbosity > 2)
                printf("\nDQS dly:%"PRIu16"|", get_rd_dqs_dly(channel, module, width));
            if (_read_verbosity)
                printf("%01x", preamble);
            if (_read_verbosity > 2)
                printf("\n");

            // Should be 1tCK preamble 0b10 (JESD79-5A 4.18.3),
            // but due to the way basephy.py works we sample 2 cycles,
            // so we get 4 bits 0b0010, which gets reversed to 0b0100.
            if (preamble == 4 && eye.state == BEFORE) {
                eye.start = rd_cycle_dly;
                eye.state = INSIDE;
            } else if (preamble != 4 && eye.state == INSIDE) {
                eye.state = AFTER;
            }
            idly_inc(channel, module, width);
        }

        if (_read_verbosity)
            printf("\n");

        rd_inc(channel, module, width);
    }

    return eye.start;
}

/**
 * read_training_data_scan
 *
 * Performs a search for working pair of read cycle and DQ delays.
 * It finds the first eye of working delays and selects its center
 * to configure the read cycle and DQ delays.
 */
static bool read_training_data_scan(int channel, int rank, int module, int width, int max_delay_taps, int preamble_cycle) {
    eye_t eye = DEFAULT_EYE;

    int rd_cycle_dly, idly;
    int works;

    // Pull back 1 cycle as DQ and DQS can be misaligned
    preamble_cycle -= 1;

    printf("Data scan:\n");

    // Set read cycle delay
    rd_rst(channel, module, width);
    for (rd_cycle_dly = 0; rd_cycle_dly < preamble_cycle; rd_cycle_dly++) {
        rd_inc(channel, module, width);
    }

    for (rd_cycle_dly = preamble_cycle; rd_cycle_dly < MAX_READ_CYCLE_DELAY && eye.state != AFTER; rd_cycle_dly++) {
        printf("%2d|", rd_cycle_dly);
        if (_read_verbosity > 2)
            printf("\nDQ CK dly:%"PRIu16, get_rd_dq_ck_dly(channel, module, width));

        idly_rst(channel, module, width);
        for(idly = 0; idly < max_delay_taps; idly++){

            if (_read_verbosity > 2)
                printf("\nDQ dly:%"PRIu16"|", get_rd_dq_dly(channel, module, width));

            works = rd_cycle_dly_idly_check_if_works(channel, rank, module, width);
            printf("%d", works);

            if (works == 3 && eye.state == BEFORE) {
                eye.start = rd_cycle_dly * max_delay_taps + idly;
                eye.state = INSIDE;
            } else if (!works && eye.state == INSIDE) {
                eye.end = rd_cycle_dly * max_delay_taps + idly;
                eye.state = AFTER;
            }

            if (_read_verbosity > 2)
                printf("\n");

            idly_inc(channel, module, width);
        }

        printf("|\n");
        rd_inc(channel, module, width);
    }
    if (eye.state != AFTER) {
        printf("Read training data scan failed for: "
               "channel:%c rank:%d module:%d\n", 'A'+channel, rank, module);
#ifndef KEEP_GOING_ON_DRAM_ERROR
        return false;
#endif // KEEP_GOING_ON_DRAM_ERROR
        return true;
    }

    int eye_width = eye.end - eye.start;
    eye.center = eye.start + (eye_width / 2);
    int eye_center_cycle = eye.center / max_delay_taps;
    int eye_center_delay = eye.center % max_delay_taps;

    printf("eye_width:%2d; eye center: cycle:%2d,delay:%2d\n",
        eye_width, eye_center_cycle, eye_center_delay);

    // Setting read delay to eye center
    rd_rst(channel, module, width);
    for (rd_cycle_dly = 0; rd_cycle_dly < eye_center_cycle; rd_cycle_dly++) {
        rd_inc(channel, module, width);
    }
    if (_read_verbosity)
        printf("Final DQ CK dly:%"PRIu16"\n", get_rd_dq_ck_dly(channel, module, width));

    idly_rst(channel, module, width);
    for (idly = 0; idly < eye_center_delay; idly++) {
        idly_inc(channel, module, width);
    }

    if (_read_verbosity)
        printf("Final DQ dly:%"PRIu16"\n", get_rd_dq_dly(channel, module, width));
    return true;
}

/**
 * simple_read_check
 *
 * Performs a simple read check, in which a 0xDEADBEEF
 * is being written to the scratch pad register of the DRAM
 * one byte at a time. After each write, a read is being
 * performed and the read value is being compared with the one
 * written before.
 */
static int simple_read_check(int channel, int rank, int module, int width) {
    int works = 1;

    printf("Simple read check: ");

    // Check if data is read correctly
    uint8_t test_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    for (int i = 0; i < 4; i++) {
        send_mrw(channel, rank, module, DRAM_SCRATCH_PAD, test_data[i]);
        send_mrr(channel, rank, DRAM_SCRATCH_PAD);

        uint8_t read_back = recover_mrr_value(channel, module, width);
        works &= read_back == test_data[i];

        printf("%"PRIX8, read_back);
    }
    printf("\n");

    return works;
}

static bool rank_read_training(int channel, int rank, int modules, int die_width, int max_taps) {
    int module;
    bool good = true;
    // Enter Read Preamble Training Mode
    enter_rptm(channel, rank);

    for (module = 0; module < modules; module++) {
        printf("Training module%2d\n", module);

        // Find cycle in which read preamble starts
        int preamble_cycle = find_read_preamble_cycle(
            channel, rank, module, die_width, max_taps);

        if (preamble_cycle == -1) {
            printf("Failed to find read preamble for module %2d\n", module);
            good &= false;
            continue;;
        }
        printf("Read preamble starts in cycle:%2d\n", preamble_cycle);

        if (!read_training_data_scan(
            channel, rank, module, die_width, max_taps, preamble_cycle)) {
            good &= false;
        }
    }

    // Exit Read Preamble Training Mode
    exit_rptm(channel, rank);
    return good;
}

static bool rank_read_check(
    int channel, int rank, int modules, int die_width, bool read_back_check) {

    int module;
    bool good = true;
    for (module = 0; module < modules; module++) {
        // Read the serial number
        printf(
            "Channel:%c rank:%2d module:%2d serial number: 0x%010"PRIX64"\n",
            (char)('A'+channel),
            rank,
            module,
            read_serial_number(channel, rank, module, die_width)
        );
    }

    for (module = 0; module < modules; module++) {
        if (read_back_check) {
            if (!simple_read_check(channel, rank, module, die_width)) {
                if (_read_verbosity)
                    printf("Simple read check failure!\n");
                good &= false;
                continue;
            }
        }
    }

    if (_read_verbosity > 1) {
        for (module = 0; module < modules; module++) {
            printf("Channel:%c rank:%d module:%d\n", (char)('A'+channel), rank, module);
            read_registers(channel, rank, module, die_width);
        }
    }
    return good;
}

/**
 * sdram_ddr5_read_training
 *
 * Performs read preamble training for each module.
 *
 * It consists of 3 major steps:
 * 1. Find read preamble cycle
 * 2. With the preamble cycle, find the best read DQ delay
 * 3. Perform a simple read check
 */
bool sdram_ddr5_read_training(training_ctx_t *const ctx) {
    int channel, rank;
    bool good = true;
    for (channel = 0; channel < ctx->channels; channel++) {
        get_dimm_dq_remapping(channel, ctx->modules, ctx->die_width);
        printf("Subchannel:%c Read training\n", (char)('A'+channel));
        for (rank = 0; rank < ctx->ranks; rank++) {
            printf("Training rank%2d\n", rank);
            good &= rank_read_training(channel, rank,
                ctx->modules, ctx->die_width, ctx->max_delay_taps);
#ifndef KEEP_GOING_ON_DRAM_ERROR
            if (!good)
                return good;
#endif // KEEP_GOING_ON_DRAM_ERROR
            // We must perform read checks below after exiting RPTM
            good &= rank_read_check(channel, rank,
                ctx->modules,ctx->die_width,
                ctx->training_type == HOST_DRAM && !ctx->RDIMM);
#ifndef KEEP_GOING_ON_DRAM_ERROR
            if (!good)
                return good;
#endif // KEEP_GOING_ON_DRAM_ERROR
        }
    }
#ifndef KEEP_GOING_ON_DRAM_ERROR
    return good;
#endif // KEEP_GOING_ON_DRAM_ERROR
    return true;
}

#ifdef WRITE_DEEP_DEBUG_DDR5
static int _write_verbosity = 3;
#elif defined(WRITE_DEBUG_DDR5)
static int _write_verbosity = 2;
#elif defined(WRITE_INFO_DDR5)
static int _write_verbosity = 1;
#else
static int _write_verbosity = 0;
#endif

/**
 * enter_wltm
 *
 * Enters Write Leveling Training Mode.
 * JESD79-5A 4.21.2
 */
static void enter_wltm(int channel, int rank) {
    enter_write_leveling(channel);

    // Set MR2:OP[1]
    send_mrw(channel, rank, MODULE_BROADCAST, 2, 2|single_cycle_MPC);
}

/**
 * exit_wltm
 *
 * Exits Write Leveling Training Mode.
 * It keeps the setting of MR2:OP[7] so the results
 * of Internal Write Leveling are actually used.
 * JESD79-5A 4.21.2
 */
static void exit_wltm(int channel, int rank) {
    // Unset MR2:OP[1] while keeping MR2:OP[7]
    send_mrw(channel, rank, MODULE_BROADCAST, 2, 0|use_internal_write_timing|single_cycle_MPC);

    exit_write_leveling(channel);
    clear_phy_fifos(channel);
}

/**
 * wltm_align_external_cycle
 *
 * Finds the first cycle in which we get response that DQS delay is correct.
 * It's a part of the External Write Leveling procedure.
 *
 * Starting from `minimal_wr_dqs_cycle_dly`, it checks each cycle delay and
 * stops at the first one with response indicating it works.
 * JESD79-5A 4.21.3
 */
static int wltm_align_external_cycle(int channel, int rank, int module, int width) {
    int works, wr_dqs_cycle_dly;
    eye_t eye = DEFAULT_EYE;

    // As per JESD79-5A 4.21.3, strobe pulses are sent no earlier than
    // CWL/2 after the WR command.
    // We need to offset that by the basephy's internal minimal WR command latency.
    const int minimal_wr_dqs_cycle_dly = SDRAM_PHY_CWL/2 - SDRAM_PHY_MIN_WR_LATENCY;

    // Set starting write DQS cycle delay
    wr_dqs_rst(channel, module, width);
    for (wr_dqs_cycle_dly = 0; wr_dqs_cycle_dly < minimal_wr_dqs_cycle_dly; wr_dqs_cycle_dly++)
        wr_dqs_inc(channel, module, width);

    // Now find the first working cycle delay
    for (; wr_dqs_cycle_dly < MAX_WRITE_CYCLE_DELAY && eye.state != INSIDE; wr_dqs_cycle_dly++) {
        works = 1;
        printf("%2d|", wr_dqs_cycle_dly);

        // Check multiple times, as we can be on the edge of transition
        // Make sure we aren't in meta stable delay
        for (int i = 0; i < 16; i++) {
            int temp = wr_dqs_check_if_works(channel, rank, module, width);
            printf("%d", temp);
            works &= temp;
        }

        printf("|%d\n", works);

        if (works && eye.state == BEFORE) {
            eye.start = wr_dqs_cycle_dly;
            eye.state = INSIDE;
        }

        wr_dqs_inc(channel, module, width);
    }

    return eye.start;
}

/**
 * wltm_align_to_eye_edge
 *
 * Scans output delays of the DQS signals to find the eye's edge.
 * It is used in both, External and Internal Write Leveling procedures.
 *
 * Pulls transition cycle back as we could
 * Sets write DQS cycle delay to the value of transition cycle and scans
 * output delays until it finds the first one that works.
 */
static int wltm_align_to_eye_edge(int channel, int rank, int module, int width, int max_delay_taps, int *transition_cycle) {
    eye_t eye = DEFAULT_EYE;

    // Passed transition_cycle could have been working with output delay 0.
    // We found the cycle using output delay of 0, so we need to go one
    // cycle back first.
    *transition_cycle -= 1;

    wr_dqs_rst(channel, module, width);
    for (int wr_dqs_cycle_dly = 0; wr_dqs_cycle_dly < *transition_cycle; wr_dqs_cycle_dly++)
        wr_dqs_inc(channel, module, width);

    printf("DQS edge scan:\n");

    printf("%2d|", *transition_cycle);
    wleveling_scan(channel, rank, module, width, max_delay_taps, &eye);
    printf("|\n");
    while (*transition_cycle < MAX_WRITE_CYCLE_DELAY && eye.state != INSIDE) {
        wr_dqs_inc(channel, module, width);
        (*transition_cycle)++;

        printf("%2d|", *transition_cycle);
        wleveling_scan(channel, rank, module, width, max_delay_taps, &eye);
        printf("|\n");
    }

    return eye.start;
}

/**
 * wltm_align_internal_cycle
 *
 * Performs scan of Write Leveling Internal Cycle Alignment values.
 * First it enables the usage of Internal Write Timings in MR2:OP[7].
 * And then it searches WICA values from [0, 7) range until it finds
 * the first one that works.
 * JESD79-5A 4.21.4
 */
static void wltm_align_internal_cycle(int channel, int rank, int module, int width) {
    int works;
    int wica = 0;
    eye_t eye = DEFAULT_EYE;

    printf("DQS internal cycle alignment\n|");

    // Enable Internal Write Timing (stored in MR3)
    // JESD79-5A 3.5.4 and 3.5.5
    use_internal_write_timing = 1 << 7;
    send_mrw(channel, rank, module, 2, 2|use_internal_write_timing|single_cycle_MPC);

    do {
        // Set WICA value (MR3:OP[3:0] = WICA)
        send_mrw(channel, rank, module, 3, wica);

        works = 1;
        // Check multiple times, as we can be on the edge of transition
        // Make sure we aren't in meta stable delay
        for (int i = 0; i < 16 && works; i++) {
            works &= wr_dqs_check_if_works(channel, rank, module, width);
        }

        printf("WICA:%d,%d|", wica, works);
        wica++;

        if (works && eye.state == BEFORE) {
            eye.state = INSIDE;
        }

        // JEDEC defines delays from 0 to -6 tCK, their operand values are [0, 7)
        // support for operands [7, 15] is optional, that's why we limit wica < 7
    } while (eye.state != INSIDE && wica < 7);

    printf("\n");
}

/**
 * write_leveling
 *
 * This function wraps together External and Internal Write Leveling.
 *
 * It's purpose is to find the best DQS delay (a combination of full
 * cycle delays (1 DFI phase) and partial, phase delays).
 *
 * It consists of following major steps:
 * 1. External Write Leveling
 *   - align external cycle
 *   - align to the eye's edge
 * 2. Internal Write Leveling
 *   - align internal cycle
 *   - align to the eye's edge
 *
 * In each of the steps above, we receive a response from the DRAM
 * indicating if selected delay combination is working or not.
 *
 * JESD79-5A 4.21
 */
static int write_leveling(training_ctx_t *const ctx , int channel, int rank, int module) {
    enter_wltm(channel, rank);

    printf("WL m:%2d\n", module);

    // ==================== External Write Leveling ====================

    // Find the first cycle in which we get response that DQS delay is correct.
    // As the eye width is 2 tCK (JESD79-5A Table 113, tWL_Pulse_Width) we can first
    // find the cycle and later align to the eye's edge with output delays.
    // That's why we search for the cycle after resetting output delays.
    odly_dqs_rst(channel, module, ctx->die_width);
    int transition_cycle = wltm_align_external_cycle(channel, rank, module, ctx->die_width);

    if (transition_cycle == -1) {
        printf("Failed to find a transition cycle for module %2d\n", module);
        return transition_cycle;
    }

    printf("DQS write leveling response transition starts in cycle:%2d (adjusted %2d)\n",
        transition_cycle, transition_cycle + SDRAM_PHY_MIN_WR_LATENCY);

    // After finding the transition cycle, we search for the eye's edge.
    int transition_delay = wltm_align_to_eye_edge(
        channel, rank, module, ctx->die_width, ctx->max_delay_taps, &transition_cycle);

#ifdef WRITE_INFO_DDR5
    printf("cycle:%2d delay:%2d\n", transition_cycle, transition_delay);
#endif // WRITE_INFO_DDR5

    // ==================== Internal Write Leveling ====================

    // JEDEC specifies that we need to adjust DQS delay before and after
    // Internal Write Leveling, based on write preamble length.
    // We use 2 tCK write preamble, so first we adjust by -0.75 tCK and
    // after finishing Internal Write Leveling, we adjust by +1.25 tCK.
    // JESD79-5A 4.21.4, Table 110
    transition_cycle -= 1;
    transition_delay += ctx->max_delay_taps/4;
    if (transition_delay >= ctx->max_delay_taps) {
        transition_cycle += 1;
        transition_delay -= ctx->max_delay_taps;
    }

#ifdef WRITE_INFO_DDR5
    printf("After adjusting by WL_ADJ_start (-0.75 tCK); cycle:%2d delay:%2d\n",
        transition_cycle, transition_delay);
#endif // WRITE_INFO_DDR5

    // Set new cycle delay
    wr_dqs_rst(channel, module, ctx->die_width);
    for (int i = 0; i < transition_cycle; i++)
        wr_dqs_inc(channel, module, ctx->die_width);

    // Set new output delay
    odly_dqs_rst(channel, module, ctx->die_width);
    for (int i = 0; i < transition_delay; i++)
        odly_dqs_inc(channel, module, ctx->die_width);

    // Perform search for working Write Leveling Internal Cycle Alignment (WICA).
    // This is the lower part of the first column of the Internal Write Leveling
    // flowchart (JESD79-5A Figure 92).
    wltm_align_internal_cycle(channel, rank, module, ctx->die_width);

    // After finding a correct WICA setting, we need to once again find the eye's edge.
    // This is the upper part of the third column of the Internal Write Leveling
    // flowchart (JESD79-5A Figure 92).
    transition_delay = wltm_align_to_eye_edge(
        channel, rank, module, ctx->die_width, ctx->max_delay_taps, &transition_cycle);

    // Just like at the beginning of the Internal Write Leveling,
    // we need to adjust the DQS delay based on write preamble length.
    // We use 2 tCK write preamble, so we adjust by +1.25 tCK.
    // JESD79-5A 4.21.4, Table 110
    transition_cycle += 1;
    transition_delay += ctx->max_delay_taps/4;
    if (transition_delay >= ctx->max_delay_taps) {
        transition_cycle += 1;
        transition_delay -= ctx->max_delay_taps;
    }

    printf("Final timing values: cycles:%2d(adjusted %2d) delay:%2d\n",
        transition_cycle, transition_cycle + SDRAM_PHY_MIN_WR_LATENCY, transition_delay);

    // Set new cycle delay
    wr_dqs_rst(channel, module, ctx->die_width);
    for (int i = 0; i < transition_cycle; i++) {
        wr_dqs_inc(channel, module, ctx->die_width);
    }

    // Set new output delay
    odly_dqs_rst(channel, module, ctx->die_width);
    for (int i = 0; i < transition_delay; i++) {
        odly_dqs_inc(channel, module, ctx->die_width);
    }
    return transition_cycle;
}

static void setup_serial_write_data(training_ctx_t *const ctx , int cnt_seed, int channel, int module, int print) {
    int it;
    uint8_t temp;
    uint16_t wrdata;
    if(print)
        printf("wrdata:");
    for (it =0; it <8; ++it) {
        wrdata = 0;
        for (temp = 0; temp < ctx->die_width; ++temp) {
            wrdata |= ((serial[cnt_seed]>>(2*it))&1) << temp;
        }
        for (temp = 0; temp < ctx->die_width; ++temp) {
            wrdata |= ((serial[cnt_seed]>>(2*it+1))&1) << (temp + ctx->die_width);
        }
        if(print)
            printf("%04"PRIx16"|", wrdata);
        set_data_module_phase(channel, module, ctx->die_width, it, wrdata);
    }
    if(print)
        printf("\n");
}

static int compare_serial_write_data(training_ctx_t *const ctx , int cnt_seed, int channel, int module, int print) {
    int phase;
    uint8_t temp;
    uint16_t rddata;
    int works = 1;
    if(print)
        printf("rddata:");
    for (phase = 0; phase < 8 && works; ++phase) {
        rddata = get_data_module_phase(channel, module, ctx->die_width, phase);
        if(print)
            printf("%04"PRIx16"|", rddata);
        for (temp = 0; temp < ctx->die_width; ++temp)
            works &= !!(((rddata>>temp)&1) == ((serial[cnt_seed]>>(2*phase))&1));
        for (temp = 0; temp < ctx->die_width; ++temp)
            works &= !!(((rddata>>(temp + ctx->die_width)) & 1) == ((serial[cnt_seed]>>(2*phase+1)) & 1));
    }
    if(print)
        printf("\n");
    return works;
}

static int write_serial_check(training_ctx_t *const ctx , int channel, int rank, int module) {
    int cnt_seed, it;
    int works = 1;
    for (cnt_seed = 0; cnt_seed < serial_count; ++cnt_seed) {
        setup_serial_write_data(ctx, cnt_seed, channel, module, 0);
        for (int i=0; i < 8; ++i) {
            send_write(channel, rank);
            send_read(channel, rank);
            works &= compare_serial_write_data(ctx, cnt_seed, channel, module, 0);
            if (!works && _write_verbosity > 1) {
                setup_serial_write_data(ctx, cnt_seed, channel, module, 1);
                compare_serial_write_data(ctx, cnt_seed, channel, module, 1);
            }
            if (!works)
                return works;
        }
        // Set all 0's
        for (it =0; it <8; ++it) {
            set_data_module_phase(channel, module, ctx->die_width, it, 0);
        }
        send_write(channel, rank);
    }
    return works;
}

static void setup_lfsr_write_data(training_ctx_t *const ctx , int seed, int channel, int module, int print) {
    int it;
    uint8_t lfsr;
    uint16_t wrdata;

    lfsr = seed;
    for (it =0; it <8; ++it) {
        wrdata = (lfsr^0x55);
        lfsr = lfsr_next(lfsr);
        if (ctx->die_width > 4) {
            wrdata |= (lfsr^0x55) << 8;
            lfsr = lfsr_next(lfsr);
        }
        if(print)
            printf("wrdata:%04"PRIx16"|", wrdata);
        set_data_module_phase(channel, module, ctx->die_width, it, wrdata);
    }
    if(print)
        printf("\n");
}

static int compare_lfsr_write_data(training_ctx_t *const ctx , int seed, int channel, int module, int print) {
    int it;
    int works = 1;
    uint8_t lfsr;
    uint16_t rddata;
    lfsr = seed;
    for (it =0; it < 8 && works; ++it) {
        rddata = get_data_module_phase(channel, module, ctx->die_width, it);
        if(print)
            printf("rddata:%04"PRIx16"|", rddata);
        works &= ((rddata&0xff) == (lfsr^0x55));
        lfsr = lfsr_next(lfsr);
        if (ctx->die_width > 4) {
            rddata >>= 8;
            works &= ((rddata&0xff) == (lfsr^0x55));
            lfsr = lfsr_next(lfsr);
        }
    }
    if(print)
        printf("\n");
    return works;
}

static int write_lfsr_check(training_ctx_t *const ctx , int channel, int rank, int module) {
    int cnt_seed, it;
    int works = 1;
    int seed;
    for (cnt_seed = 0; cnt_seed < seeds_count * 2 && works; ++cnt_seed) {
        if(cnt_seed < seeds_count)
            seed = seeds0[cnt_seed];
        else
            seed = seeds1[cnt_seed - seeds_count];

        for (int i = 0; i < 8; ++i) {
            setup_lfsr_write_data(ctx, seed, channel, module, 0);
            send_write(channel, rank);
            send_read(channel, rank);
            works &= compare_lfsr_write_data(ctx, seed, channel, module, 0);
            if (!works && _write_verbosity > 1) {
                setup_lfsr_write_data(ctx, seed, channel, module, 1);
                compare_lfsr_write_data(ctx, seed, channel, module, 1);
            }
            if (!works)
                return works;
        }
        for (it =0; it <8; ++it) {
            set_data_module_phase(channel, module, ctx->die_width, it, 0);
        }
        send_write(channel, rank);
    }
    return works;
}

static int compare_dm_lfsr_write_data(training_ctx_t *const ctx , int seed, int channel, int module, int byte) {
    int it;
    int works = 1;
    uint8_t lfsr;
    uint16_t rddata;
    lfsr = seed;
    for (it = 0; it < 16; ++it) {
        rddata = get_data_module_phase(channel, module, ctx->die_width, it/2);
        if (_write_verbosity > 1)
            printf("rddata:%04"PRIx16"|", rddata);

        if (it & 1)
            rddata >>= 8;
        if (byte == it)
            works &= ((rddata&0xff) == lfsr);
        lfsr = lfsr_next(lfsr);
    }
    if (_write_verbosity > 1)
        printf("\n");

    return works;
}

static int write_dm_lfsr_check(training_ctx_t *const ctx , int channel, int rank, int module, int byte, int mr5) {
    int cnt_seed, it;
    int works = 1;
    int seed;
#ifndef DDR5_TRAINING_SIM
    for (cnt_seed = 0; cnt_seed < seeds_count * 2 && works; ++cnt_seed) {
#else
    {cnt_seed = 0;
#endif // DDR5_TRAINING_SIM
        if(cnt_seed < seeds_count)
            seed = seeds0[cnt_seed];
        else
            seed = seeds1[cnt_seed - seeds_count];

        send_mrw(channel, rank, module, 5, mr5 & 0xDF); // Disable DM
        for (it =0; it <8; ++it) {
            set_data_module_phase(channel, module, ctx->die_width, it, 0);
        }
        send_write(channel, rank);
        send_mrw(channel, rank, module, 5, mr5); // Enable DM

        setup_lfsr_write_data(ctx, seed, channel, module, 0);
        send_write_byte(channel, rank, module, byte);
        send_read(channel, rank);

        works &= compare_dm_lfsr_write_data(ctx, seed, channel, module, byte);
        for (it =0; it <8; ++it) {
            set_data_module_phase(channel, module, ctx->die_width, it, 0);
        }
        send_write(channel, rank);
        if (!works)
            return works;
    }
    return works;
}

static eye_t write_data_scan(training_ctx_t *const ctx , int channel, int rank, int module, int write_strobe_cycle, int print) {
    eye_t eye = DEFAULT_EYE;
    eye_t serial_only_eye = DEFAULT_EYE;
    int works = 1, p_works;

    wr_dq_rst(channel, module, ctx->die_width);
    for (int cycle = 0; cycle < write_strobe_cycle - 3; ++cycle) {
        wr_dq_inc(channel, module, ctx->die_width);
    }
    if (print)
        printf("Data scan:\n");
    for (int cycle = write_strobe_cycle - 3; eye.state != AFTER && serial_only_eye.state != AFTER && cycle < 65 && cycle < write_strobe_cycle + 5; ++cycle) {
        if (print) {
            printf("%2d|", cycle);
            if (_write_verbosity > 2)
                printf("\n");
        }

        odly_dq_rst(channel, module, ctx->die_width);
        for(int delay = 0; delay < ctx->max_delay_taps; ++delay){
            if (_write_verbosity > 2)
                printf("DQ dly:%"PRIu16"\n", get_wr_dq_dly(channel, module, ctx->die_width));

            works = 1;
            p_works = 0;
#ifndef DDR5_TRAINING_SIM
            works &= write_serial_check(ctx, channel, rank, module);
#endif // DDR5_TRAINING_SIM
            if (works) {
                p_works = 1;
                works &= write_lfsr_check(ctx, channel, rank, module);
                if (works)
                    p_works = 3;
            }

            if (print)
                printf("%d", p_works);
            if (_write_verbosity > 1)
                printf("\n");

            if (works && eye.state == BEFORE) {
                eye.start = cycle * ctx->max_delay_taps + delay;
                eye.state  = INSIDE;
            } else if (!works && eye.state == INSIDE) {
                eye.end = cycle * ctx->max_delay_taps + delay;
                eye.state  = AFTER;
            }

            if ((p_works & 1) && serial_only_eye.state == BEFORE) {
                serial_only_eye.state  = INSIDE;
            } else if (!(p_works & 1) && serial_only_eye.state == INSIDE) {
                serial_only_eye.state  = AFTER;
            }
            odly_dq_inc(channel, module, ctx->die_width);
        }
        if (print)
            printf("|\n");
        wr_dq_inc(channel, module, ctx->die_width);
    }
    return eye;
}

static int moduel_dq_vref_scan(training_ctx_t *const ctx, int channel, int rank, int module, int wl_cycle) {
    int eye_width_range [2][SDRAM_PHY_DELAYS];
    int vref, _width;
    int best_vref = -1;

    for(_width = 0; _width < SDRAM_PHY_DELAYS; ++_width) {
        eye_width_range[0][_width] = -1;
        eye_width_range[1][_width] = -1;
    }

    for(vref = 0x32; vref < 0x46; ++vref) { // FIXME: check over whole DQ VREF space, but keep performance
        if (_write_verbosity)
            printf("Vref:%2X", vref);
        send_mrw(channel, rank, module, 10, vref);
        busy_wait_us(1);
        if (_write_verbosity)
            printf("\n");
        eye_t eye = write_data_scan(ctx, channel, rank, module, wl_cycle, _write_verbosity);
        if (_write_verbosity)
            printf("|start cycle:%2d, delay:%2d; end cycle:%2d, delay:%2d|",
                eye.start/ctx->max_delay_taps, eye.start%ctx->max_delay_taps,
                eye.end/ctx->max_delay_taps, eye.end%ctx->max_delay_taps);
        eye.center = eye.end - eye.start;

        if (_write_verbosity)
            printf("eye_width:%2d; eye center: cycle:%2d,delay:%2d\n", eye.center,
                ((eye.start + eye.end)/2)/ctx->max_delay_taps,
                ((eye.start + eye.end)/2)%ctx->max_delay_taps);

        for(_width = 0; _width < eye.center; ++_width) {
            if (eye_width_range[0][_width] == -1)
                eye_width_range[0][_width] = vref;
            eye_width_range[1][_width] = vref + 1;
        }
    }

    for (_width = 0; _width < SDRAM_PHY_DELAYS; ++_width) {
        if (eye_width_range[0][_width] != -1)
            best_vref = (eye_width_range[0][_width] + eye_width_range[1][_width]) / 2;
    }
    printf("m%2d|Best Vref:%2x\n", module, best_vref);
    if (best_vref > -1) {
        send_mrw(channel, rank, module, 10, best_vref);
        busy_wait_us(1);
    }
    send_mrr(channel, rank, 10);
    if (_write_verbosity)
        printf("MR10:%02"PRIx8"\n", recover_mrr_value(channel, module, ctx->die_width));
    return best_vref;
}

static bool moduel_dm_scan(training_ctx_t *const ctx, int channel, int rank, int module, int mr5) {
    int delay, works, byte, middle_delay;
    bool good = true;
    printf("DM scan\nm:%2d DM|", module);
    odly_dm_rst(channel, module, ctx->die_width);
    eye_t eye_dm = DEFAULT_EYE;
    for(delay = 0; delay < ctx->max_delay_taps && eye_dm.state != AFTER; ++delay) {
        if (_write_verbosity > 2)
            printf("DM dly:%"PRIu16"\n", get_wr_dm_dly(channel, module, ctx->die_width));

        works = 1;
        for (byte = 0; byte < 16 && works; ++byte) {
            write_dm_lfsr_check(ctx, channel, rank, module, byte, mr5);
        }
        printf("%d", works);
        if (_write_verbosity > 2)
            printf("\n");

        if (works && eye_dm.state == BEFORE) {
            eye_dm.start = delay;
            eye_dm.state  = INSIDE;
        }
        if (!works && eye_dm.state == INSIDE) {
            eye_dm.end = delay;
            eye_dm.state  = AFTER;
        } else if (delay == ctx->max_delay_taps-1 && eye_dm.state == INSIDE) {
            eye_dm.end = delay + 1;
            eye_dm.state = AFTER;
        }
        odly_dm_inc(channel, module, ctx->die_width);
    }
    good &= eye_dm.state == AFTER;

    printf("|\n");
    printf("m%2d|DM start delay:%2d; delay:%2d|",
        module, eye_dm.start, eye_dm.start);
    eye_dm.center = eye_dm.end - eye_dm.start;
    middle_delay = (eye_dm.start + eye_dm.end)/2;
    printf("eye_width:%2d; eye center: delay:%2d\n",
            eye_dm.center, middle_delay);

    // Setting read delay to eye center
    odly_dm_rst(channel, module, ctx->die_width);
    for (delay = 0; delay < middle_delay; ++delay) {
        odly_dm_inc(channel, module, ctx->die_width);
    }
    return good;
}

static int module_vref_scan(training_ctx_t *const ctx, int channel, int rank, int module, int wl_cycle) {
    int best_vref, middle_cycle, middle_delay, it;
    uint8_t mr5 = 0;
    send_mrr(channel, rank, 5);
    mr5 = recover_mrr_value(channel, module, ctx->die_width);

    if (_write_verbosity) {
        printf("m%2d|\n", module);
        printf("MR5:%02"PRIx8"\n", mr5);
    }

    send_mrw(channel, rank, module, 5, mr5 & 0xDF); // Disable DM

    wr_dq_rst(channel, module, ctx->die_width);
    odly_dq_rst(channel, module, ctx->die_width);
    best_vref = moduel_dq_vref_scan(ctx, channel, rank, module, wl_cycle);

#ifndef KEEP_GOING_ON_DRAM_ERROR
    if (best_vref == -1)
        return best_vref;
#endif // KEEP_GOING_ON_DRAM_ERROR

    // Setting read delay to eye center
    wr_dq_rst(channel, module, ctx->die_width);
    odly_dq_rst(channel, module, ctx->die_width);
    eye_t eye = write_data_scan(ctx, channel, rank, module, wl_cycle, 1);
    middle_cycle = ((eye.start + eye.end)/2)/ctx->max_delay_taps;
    middle_delay = ((eye.start + eye.end)/2)%ctx->max_delay_taps;
    eye.center = eye.end - eye.start;
    printf("m%2d|start cycle:%2d, delay:%2d; end cycle:%2d, delay:%2d|",
        module,
        eye.start/ctx->max_delay_taps, eye.start%ctx->max_delay_taps,
        eye.end/ctx->max_delay_taps, eye.end%ctx->max_delay_taps);
    printf("eye_width:%2d; eye center: cycle:%2d,delay:%2d\n",
        eye.center, middle_cycle, middle_delay);

    wr_dq_rst(channel, module, ctx->die_width);
    odly_dq_rst(channel, module, ctx->die_width);
    if (eye.state == AFTER)
        for (it = 0; it < middle_cycle; ++it) {
            wr_dq_inc(channel, module, ctx->die_width);
        }
    if (eye.state == AFTER)
    for (it = 0; it < middle_delay; ++it) {
        odly_dq_inc(channel, module, ctx->die_width);
    }
    // DM training
    if ((mr5 & 0x20) && ctx->die_width > 4) // DM was enabled
        if (!moduel_dm_scan(ctx, channel, rank, module, mr5))
            return -1;

    return best_vref;
}

bool sdram_ddr5_write_training(training_ctx_t *const ctx ) {
    int channel, rank, module;
    int write_strobe_cycle[16];
    bool good = true;

    for (channel = 0; channel < ctx->channels; channel++) {
        printf("Subchannel:%c Write leveling\n", (char)('A'+channel));
        /* Coarse alignment */
        for (rank = 0; rank < ctx->ranks; rank++) {
            // Perform Write Leveling (both External and Internal)
            enter_wltm(channel, rank);
            for (module = 0; module < ctx->modules; module++) {
                write_strobe_cycle[module] = write_leveling(ctx, channel, rank, module);
                good &= write_strobe_cycle[module] != -1;
#ifndef KEEP_GOING_ON_DRAM_ERROR
                if (!good)
                    break;
#endif // KEEP_GOING_ON_DRAM_ERROR
            }
            exit_wltm(channel, rank);
#ifndef KEEP_GOING_ON_DRAM_ERROR
            if (!good)
                return good;
#endif // KEEP_GOING_ON_DRAM_ERROR

            if (_write_verbosity > 1)
                for (module = 0; module < ctx->modules; module++) {
                    read_registers(channel, rank, module, ctx->die_width);
                }

            printf("DQ write training\n");
            for (module = 0; module < ctx->modules; module++) {
                good &= (module_vref_scan(ctx, channel, rank, module, write_strobe_cycle[module]) != -1);
#ifndef KEEP_GOING_ON_DRAM_ERROR
                if (!good)
                    return good;
#endif // KEEP_GOING_ON_DRAM_ERROR
            }
        }
    }
#ifndef KEEP_GOING_ON_DRAM_ERROR
    return good;
#endif // KEEP_GOING_ON_DRAM_ERROR
    return true;
}

/**
 * ca_check_if_has_line13
 *
 * Detect if CA13 is present.
 * Requires to already be in the CATM.
 */
static int ca_check_if_has_line13(int32_t channel) {
    cmd_injector(channel, 0xf, 0, 1<<13, 0, 0, 1, 0);
    cmd_injector(channel, 0x1, 1, 1<<13, 0, 0, 1, 0);
    store_continuous(channel);

    return and_sample(channel);
}

#if defined(CONFIG_HAS_I2C)
enum module_type {
    RDIMM       = 0b0001,
    UDIMM       = 0b0010,
    SODIMM      = 0b0011,
    LRDIMM      = 0b0100,
    DDIM        = 0b1010,
    SOLDER_DOWN = 0b1011,
};

/**
 * read_module_type
 *
 * Reads the 3rd byte of the SPD and extracts the module type.
 * If the SPD cannot be read, it defaults to the UDIMM.
 */
static enum module_type read_module_type(uint8_t spd) {
    uint8_t module_type;
    if (!sdram_read_spd(spd, 3, &module_type, 1, false)) {
        printf("Couldn't read the SPD and check the module type. Defaulting to UDIMM.\n");
        return UDIMM;
    }

    // Module type is in the lower nibble
    return module_type & 0x0f;
}

//static uint8_t read_module_width(uint8_t spd) {
//    uint8_t buf;
//
//    // Module width is stored in SPD[6][7:5]
//    //     000: x4
//    //     001: x8
//    //     010: x16
//    //     011: x32
//
//    if (!sdram_read_spd(spd, 6, &buf, 1, false)) {
//        printf("Couldn't read module width from the SPD, defaulting to x%d.\n", SDRAM_PHY_DQ_DQS_RATIO);
//        return SDRAM_PHY_DQ_DQS_RATIO;
//    }
//
//    // minimal supported is x4
//    uint8_t shift = (buf & 0xe0) >> 5;
//    uint8_t module_width = 4 << shift;
//
//    return module_width;
//}

static uint8_t read_module_ranks(uint8_t spd) {
    uint8_t buf;

    // Module ranks count is stored in SPD[234][5:3]
    //     000: 1
    //     001: 2
    //     010: 3
    //     .
    //     .
    //     .
    //     111: 8

    if (!sdram_read_spd(spd, 234, &buf, 1, false)) {
        printf("Couldn't read module ranks from the SPD, defaulting to x%d.\n", 1);
        return 1;
    }

    // minimal supported is x4
    uint8_t shift = (buf & 0x38) >> 3;
    uint8_t module_ranks = shift + 1;

    return module_ranks;
}

static uint8_t read_module_channels(uint8_t spd) {
    uint8_t buf;

    // Module channels count is stored in SPD[235][6:5]
    //     00: 1
    //     01: 2

    if (!sdram_read_spd(spd, 235, &buf, 1, false)) {
        printf("Couldn't read module channels from the SPD, defaulting to x%d.\n", CHANNELS);
        return CHANNELS;
    }

    // minimal supported is x4
    uint8_t shift = (buf & 0x60) >> 5;
    uint8_t module_channels = shift + 1;

    return module_channels;
}

static uint8_t read_module_enabled_clock(uint8_t spd) {
    uint8_t buf;

    // Module channels count is stored in SPD[248]
    //     [0]: QACK: 0 enable/1 disable
    //     [1]: QBCK: 0 enable/1 disable
    //     [2]: QCCK: 0 enable/1 disable
    //     [3]: QDCK: 0 enable/1 disable
    //     [5]:  BCK: 0 enable/1 disable (LRDIMM)

    if (!sdram_read_spd(spd, 248, &buf, 1, false)) {
        printf("Couldn't read module clock enables from the SPD, defaulting to x%d.\n", 0);
        return 0;
    }

    return buf & 0x2f;
}

static uint8_t read_module_enabled_ca(uint8_t spd) {
    uint8_t buf;

    // Module channels count is stored in SPD[249]
    //     [0]:    QACA: 0 enable/1 disable
    //     [1]:    QBCA: 0 enable/1 disable
    //     [2]:  DCS1_n: 0 enable/1 disable
    //     [3]:   BCS_n: 0 enable/1 disable
    //     [4]:  QxCA13: 0 enable/1 disable
    //     [5]: QACSx_n: 0 enable/1 disable
    //     [6]: QBCSx_n: 0 enable/1 disable

    if (!sdram_read_spd(spd, 249, &buf, 1, false)) {
        printf("Couldn't read module CA enables from the SPD, defaulting to x%d.\n", 0);
        return 0;
    }

    return buf & 0x7f;
}

static uint8_t read_module_qck_setup(uint8_t spd) {
    uint8_t buf;

    // Module channels count is stored in SPD[250]
    // [1:0]: QACK: 00 20Ohm/ 01 14Ohm /10 10Ohm /11 RES
    // [3:2]: QBCK: 00 20Ohm/ 01 14Ohm /10 10Ohm /11 RES
    // [5:4]: QCCK: 00 20Ohm/ 01 14Ohm /10 10Ohm /11 RES
    // [7:6]: QDCK: 00 20Ohm/ 01 14Ohm /10 10Ohm /11 RES

    if (!sdram_read_spd(spd, 250, &buf, 1, false)) {
        printf("Couldn't read module QCK setup from the SPD, defaulting to x%d.\n", 0);
        return 0;
    }

    return buf & 0xff;
}

static uint8_t read_module_qca_qcs_setup(uint8_t spd) {
    uint8_t buf;

    // Module channels count is stored in SPD[252]
    // [1:0]: QxCA: 00 20Ohm/ 01 14Ohm /10 10Ohm /11 RES
    // [5:4]: QxCS: 00 20Ohm/ 01 14Ohm /10 10Ohm /11 RES

    if (!sdram_read_spd(spd, 252, &buf, 1, false)) {
        printf("Couldn't read module QCA/QCS setup from the SPD, defaulting to x%d.\n", 0);
        return 0;
    }

    return buf & 0x33;
}

static uint8_t read_module_slew_rates(uint8_t spd) {
    uint8_t buf;

    // Module channels count is stored in SPD[252]
    // [1:0]: QxCK: 00 12-20 V/ns/ 01 14-27 V/ns /10 RES /11 RES
    // [3:2]: QxCA: 00   4-7 V/ns/ 01  6-10 V/ns /10 2.7-4.5 V/ns /11 RES
    // [5:4]: QxCS: 00   4-7 V/ns/ 01  6-10 V/ns /10 2.7-4.5 V/ns /11 RES

    if (!sdram_read_spd(spd, 254, &buf, 1, false)) {
        printf("Couldn't read module slew rates from the SPD, defaulting to x%d.\n", 0);
        return 0;
    }

    return buf & 0x3f;
}
#endif // defined(CONFIG_HAS_I2C)

training_ctx_t host_dram_ctx;
#if defined(CONFIG_HAS_I2C)
training_ctx_t host_rcd_ctx;
training_ctx_t rcd_dram_ctx;
#endif // defined(CONFIG_HAS_I2C)

static void init_structs(void) {
    host_dram_ctx = (training_ctx_t) DEFAULT_HOST_DRAM;
#if defined(CONFIG_HAS_I2C)
    host_rcd_ctx  = (training_ctx_t) DEFAULT_HOST_RCD;
    rcd_dram_ctx  = (training_ctx_t) DEFAULT_RCD_DRAM;
#endif // defined(CONFIG_HAS_I2C)
}

#if defined(CONFIG_HAS_I2C)
static void rcd_init(training_ctx_t *const ctx ) {
    // Issue a VR_ENABLE command to the PMIC
    uint8_t cmd = 0xa0;
    i2c_write(0x48, 0x32, &cmd, 1, 1); // FIXME: this should be sent to all PMICs
    busy_wait(50);

    rcd_set_enables_and_slew_rates(
        0, 0, 0, 0, 0, 0);
    reset_sequence(ctx->ranks);

    rcd_set_dca_rate(0, 0, ctx->rate);
    if (ctx->rate != DDR)
        ctx->ca.check = dca_check_if_works_sdr;
    rcd_set_dimm_operating_speed(0, 0, 2000);
    rcd_set_termination_and_vref(0);
    reset_sequence(ctx->ranks);
    rcd_set_dimm_operating_speed_band(0, 0, 2000);
    busy_wait_us(50);
    rcd_forward_all_dram_cmds(0, 0, false); // FIXME: this should forward for all RCDs

    sdram_ddr5_cs_ca_training(ctx, -1);
#ifndef KEEP_GOING_ON_DRAM_ERROR
    if(!ctx->CS_CA_successful)
        return;
#endif // KEEP_GOING_ON_DRAM_ERROR
    busy_wait(6);

    // FIXME: this function should initialize all RCDs
    rcd_set_enables_and_slew_rates(
        0,
        read_module_enabled_clock(0),
        read_module_enabled_ca(0),
        read_module_qck_setup(0),
        read_module_qca_qcs_setup(0),
        read_module_slew_rates(0)
    );
    busy_wait(6);

    for (int channel = 0; channel < ctx->channels; channel++) {
        rcd_set_qrst(channel, 0); // FIXME: this should set QRST for all RCDs
    }
    busy_wait(1);

    for (int channel = 0; channel < ctx->channels; channel++)
        rcd_clear_qrst(channel, 0); // FIXME: this should clear QRST for all RCDs

    rcd_forward_all_dram_cmds(0, 0, true); // FIXME: this should forward for all RCDs
    busy_wait(6);

    for (int channel = 0; channel < ctx->channels; channel++) {
        rcd_release_qcs(channel, 0, true); // FIXME: this should set QRST for all RCDs
    }
    busy_wait(6);

    for (int channel = 0; channel < ctx->channels; ++channel)
        prep_nop(channel, 0);

    force_issue_single();
    busy_wait_us(500);
}
#endif // defined(CONFIG_HAS_I2C)

/**
 * sdram_ddr5_flow
 *
 * Performs the entire initialization and training
 * procedure for DDR5 memory. In runtime finds out
 * if connected memory is RDIMM and selects proper
 * training context.
 */
void sdram_ddr5_flow(void) {
    single_cycle_MPC = 0;
    use_internal_write_timing = 0;
    enumerated = 0;
    clear_helper_arr();
    init_structs();
    enable_phy();

    training_ctx_t *base_ctx = &host_dram_ctx;

    bool is_rdimm = false;
#if defined(CONFIG_HAS_I2C)
#ifdef DDR5_RDIMM_SIM
    is_rdimm = true;
#else
    is_rdimm = read_module_type(0) == RDIMM;
#endif // DDR5_RDIMM_SIM
    // FIXME: handle multiple sticks and SPDs
    int die_width = SDRAM_PHY_DQ_DQS_RATIO; //FIXME: change to SPD value when PHY works `read_module_width(0);`
    if (is_rdimm) {
        die_width = 4;
        base_ctx = &host_rcd_ctx;
    }
    host_dram_ctx.die_width = die_width;
    host_rcd_ctx.die_width = die_width;
    rcd_dram_ctx.die_width = die_width;
    rcd_dram_ctx.ranks     = read_module_ranks(0); // FIXME: handle multiple sticks and SPDs
    rcd_dram_ctx.channels  = read_module_channels(0); // FIXME: handle multiple sticks and SPDs
#endif // defined(CONFIG_HAS_I2C)

    reset_all_phy_regs(host_dram_ctx.channels, host_dram_ctx.ranks,
        host_dram_ctx.all_ca_count, host_dram_ctx.modules, host_dram_ctx.die_width);

#if defined(CONFIG_HAS_I2C)
    if (is_rdimm) {
        printf("Detected RDIMM. Initializing RCD and running Host->RCD training\n");
        ddrphy_CSRModule_rdimm_mode_write(1);
        rcd_init(base_ctx);
        base_ctx = &rcd_dram_ctx;
        base_ctx->rate = host_rcd_ctx.rate;
        base_ctx->CS_CA_successful &= host_rcd_ctx.CS_CA_successful;
#ifndef KEEP_GOING_ON_DRAM_ERROR
        if(!base_ctx->CS_CA_successful)
            return;
#endif // KEEP_GOING_ON_DRAM_ERROR
    } else {
        reset_sequence(base_ctx->ranks);
    }
#else
    reset_sequence(base_ctx->ranks);
#endif // defined(CONFIG_HAS_I2C)

    dram_start_sequence(base_ctx->ranks);

    if (is_rdimm) {
        enter_ca_pass(0); // FIXME: handle multiple RCDs
        for (int rank = 0; rank < base_ctx->ranks; ++rank) {
            select_ca_pass(rank);
            setup_dram_mrs_sequence(rank);
        }
        exit_ca_pass(0); // FIXME: handle multiple RCDs
        for (int channel = 0; channel < base_ctx->channels; ++channel) {
            sdram_ddr5_cs_ca_training(base_ctx, channel);
        }
    } else {
        for (int rank = 0; rank < base_ctx->ranks; ++rank)
            setup_dram_mrs_sequence(rank);
        sdram_ddr5_cs_ca_training(base_ctx, -1);
    }
#ifndef KEEP_GOING_ON_DRAM_ERROR
    if(!base_ctx->CS_CA_successful)
        return;
#endif // KEEP_GOING_ON_DRAM_ERROR

    for (int channel = 0; channel < base_ctx->channels; channel++) {
        for (int rank = 0; rank < base_ctx->ranks; rank++) {
            if (base_ctx->CS_CA_successful && base_ctx->rate == DDR) {
                disable_dram_2n_mode(channel, rank);
            }
        }
    }

    if (in_2n_mode()) {
        printf("2N mode setup\n");
        init_sequence_2n(base_ctx->ranks);
    } else {
        printf("1N mode setup\n");
        init_sequence_1n(base_ctx->ranks);
    }

    single_cycle_MPC = 1<<4;
    for (int channel = 0; channel < base_ctx->channels; channel++)
        for (int rank = 0; rank < base_ctx->ranks; rank++)
            send_mrw(channel, rank, MODULE_BROADCAST, 2, 0|use_internal_write_timing|single_cycle_MPC);

    for (int rank = 0; rank < base_ctx->ranks; ++rank) {
        if(dram_enumerate(base_ctx, rank))
            continue;
#ifndef KEEP_GOING_ON_DRAM_ERROR
        return;
#endif // KEEP_GOING_ON_DRAM_ERROR
    }

#if defined(CONFIG_HAS_I2C)
    if (is_rdimm) {
        base_ctx = &host_dram_ctx;
        host_dram_ctx.ranks = rcd_dram_ctx.ranks;
        host_dram_ctx.RDIMM = rcd_dram_ctx.RDIMM;
    }
#endif // defined(CONFIG_HAS_I2C)

    if (!sdram_ddr5_read_training(base_ctx)) {
#ifndef KEEP_GOING_ON_DRAM_ERROR
        return;
#endif // KEEP_GOING_ON_DRAM_ERROR
    }
    if (!sdram_ddr5_write_training(base_ctx)) {
#ifndef KEEP_GOING_ON_DRAM_ERROR
        return;
#endif // KEEP_GOING_ON_DRAM_ERROR
    }
}

#endif // defined(CSR_SDRAM_BASE) && defined(SDRAM_PHY_DDR5)
