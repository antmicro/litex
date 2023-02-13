#include <liblitedram/ddr5_training.h>

#if defined(CSR_SDRAM_BASE) && defined(SDRAM_PHY_DDR5)
#include <liblitedram/ddr5_helpers.h>

#include <liblitedram/sdram_spd.h>

#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>

//#define INFO_DDR5
//#define DEBUG_CA_DDR5
//#define DEBUG_DDR5

#define BYTES_PER_MODULE (SDRAM_PHY_DQ_DQS_RATIO/4)

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

/**
 * CS_in_eye
 *
 * Checks if selected CS (rank) signal delay is in
 * the eye of working delays.
 *
 * As we don't know when the DRAM/RCD started sampling,
 * we check 0101 pattern and if it doesn't work, we shift
 * it by one and check again.
 */
static int CS_in_eye(training_ctx_t *ctx, int32_t channel, int32_t rank, int *shift_0101) {
    int _shift_0101;
    for (_shift_0101 = 0; _shift_0101 < 2; _shift_0101++) {
        if (ctx->cs.check(channel, rank, 0, _shift_0101)) {
            *shift_0101 = _shift_0101;
            return 1;
        }
    }
    return 0;
}

static int CS_ck_scan(training_ctx_t *ctx, int32_t channel, int32_t rank, int shift_0101) {
    int works, last_good, _result, ckdly;
    works = 1;
    printf("|");
    last_good = 0;
    ctx->cs.rst_dly(channel, rank, 0);
    for(ckdly = 0; ckdly < SDRAM_PHY_DELAYS && works; ckdly++) {
        _result = ctx->cs.check(channel, rank, 0, shift_0101);
        printf("%d", !!_result);
        if (!_result && works) {
            works = 0;
            last_good = -(ckdly - 1);
        } else if (_result && works && ckdly == SDRAM_PHY_DELAYS - 1) {
            last_good = -ckdly;
        }
        ctx->ck.inc_dly(channel, rank, 0);
    }
    ctx->ck.rst_dly(channel, rank, 0);
    return last_good;
}

/**
 * CS_should_shift_pattern
 *
 * During the CS training we get responses based
 * on detection of `0101` CS pattern.
 *
 * As we only care if the pattern is being detected
 * (CK and CS are aligned) and not about the cycle
 * in which sampling began, we can change the pattern
 * to `1010` to solve that.
 */
static int CS_should_shift_pattern(training_ctx_t *ctx, int32_t channel, int32_t rank) {
    int csdly, shift_0101;

    ctx->cs.rst_dly(channel, rank, 0);
    for (csdly = 0; csdly < SDRAM_PHY_DELAYS; csdly++) {
        if (CS_in_eye(ctx, channel, rank, &shift_0101))
            return shift_0101;

        ctx->cs.inc_dly(channel, rank, 0);
    }

    return 0;
}

static void CS_scan(training_ctx_t *ctx, int32_t channel, int32_t rank, int* left, int* right, int shift_0101) {
    int works, csdly;
    eye_t eye = DEFAULT_EYE;

    ctx->cs.rst_dly(channel, rank, 0);
    for (csdly = 0; csdly < SDRAM_PHY_DELAYS; csdly++) {
        works = ctx->cs.check(channel, rank, 0, shift_0101);
        printf("%d", !!works);
        if (works && eye.state == BEFORE) {
            eye.start = csdly;
            eye.state = INSIDE;
        } else if ((!works || csdly == SDRAM_PHY_DELAYS - 1) && eye.state == INSIDE) {
            eye.end = csdly;
            eye.state = AFTER;
        }
        ctx->cs.inc_dly(channel, rank, 0);
    }
    ctx->cs.rst_dly(channel, rank, 0);

    *right = eye.start;
    *left = eye.end;
}

static void CS_training(training_ctx_t *ctx, int32_t channel, uint8_t *success) {
    int left_side, right_side;
    int32_t csdly, coarse;
    int32_t rank;
    int shift_0101 = 0;
    for (rank = 0; rank < ctx->ranks; rank++) {
        printf("Rank: %2"PRId32"", rank);

        // Enter CS training
        ctx->cs.enter_training_mode(channel, rank);

        left_side = UNSET_DELAY;
        right_side = UNSET_DELAY;

        // Scan clock delays only if one of patterns work (0x55 or 0xAA)
        // If neither works then we are in meta state, both clock and CS change
        // too close to each other. In such situations we only check CS delays
        if (CS_in_eye(ctx, channel, rank, &shift_0101)) {
            // We are already in the eye and can look for the eye start by changing CK delay
            right_side = CS_ck_scan(ctx, channel, rank, shift_0101);

            // After delaying clock, pattern could have changes, as we missed one clock cycle
            shift_0101 = CS_should_shift_pattern(ctx, channel, rank);
        }

        printf("|");
        CS_scan(ctx, channel, rank, &left_side, &right_side, shift_0101);
        printf("|\n");

        ctx->cs.rst_dly(channel, rank, 0);

        // Set up coarse delay adjustment until we get CA results
        printf("Rank delays: %2d:%2d\n", right_side, left_side);
        coarse = (right_side + left_side) / 2;
        coarse = MAX(0, coarse);
        printf("Coarse adjustment:%"PRId32"\n", coarse);
        ctx->cs.coarse_delays[channel][rank] = coarse;

        for (csdly = 0; csdly < coarse; ++csdly)
            ctx->cs.inc_dly(channel, rank, 0);

        // Exit CS training
        ctx->cs.exit_training_mode(channel, rank);

        if (left_side == UNSET_DELAY || right_side == UNSET_DELAY) {
            printf("CS:%2"PRId32" Eye width:0 Failed\n", rank);
            *success = 0;
            return;
        }

        ctx->cs.delays[channel][rank][0] = right_side;
        ctx->cs.delays[channel][rank][1] = left_side;
    }
}

/**
 * CA_setup_array
 *
 * Fills CA delays array in the training_ctx_t with initial values
 */
static void CA_setup_array(training_ctx_t *ctx) {
    int channel, address;
    for (channel = 0; channel < CHANNELS; ++channel) {
        for (address = 0; address < 14; ++address) {
           ctx->ca.delays[channel][address][0] = -SDRAM_PHY_DELAYS;
           ctx->ca.delays[channel][address][1] = SDRAM_PHY_DELAYS;
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
static void CA_check_lines(training_ctx_t *ctx, int32_t channel) {
    ctx->ca.enter_training_mode(channel, 0);
    if (ctx->ca.has_line13(channel))
        ctx->ca.line_count = 14;
    else
        ctx->ca.line_count = 13;
    ctx->ca.exit_training_mode(channel, 0);
    printf("DDR5 module has %d address lines\n", ctx->ca.line_count);
}

static int CA_ck_scan(training_ctx_t *ctx, int32_t channel, int32_t rank, int32_t address, int32_t csdly_base) {
    int works, last_good, _result, ckdly, csdly;
    printf("|");
    works = 1;
    last_good = 0;
    csdly = csdly_base;
    ctx->ca.rst_dly(channel, rank, address);
    for(ckdly = 0; ckdly < SDRAM_PHY_DELAYS && works; ++ckdly, ++csdly) {
        _result = ctx->ca.check(channel, rank, address, csdly/SDRAM_PHY_DELAYS);
        printf("%d", !!_result);
        if (!_result && works) {
            works = 0;
            last_good = -(ckdly - 1);
        } else if(_result && works && ckdly == SDRAM_PHY_DELAYS - 1) {
            last_good = -ckdly;
        }
        ctx->ck.inc_dly(channel, rank, 0);
        ctx->cs.inc_dly(channel, rank, 0);
    }
    ctx->ck.rst_dly(channel, rank, 0);

    ctx->cs.rst_dly(channel, rank, 0);
    for (csdly = 0; csdly < csdly_base; ++csdly)
        ctx->cs.inc_dly(channel, rank, 0);

    return last_good;
}

static void CA_scan(training_ctx_t *ctx, int32_t channel, int32_t rank, int32_t address, int* left, int* right) {
    int works, cadly;
    eye_t eye = DEFAULT_EYE;

    ctx->ca.rst_dly(channel, rank, address);
    for (cadly = 0; cadly < SDRAM_PHY_DELAYS; cadly++) {

#ifdef DEBUG_CA_DDR5
        printf("CA%2"PRIu32" dly:%"PRIu16"\n", address, get_ca_dly(channel, rank, address));
#endif // DEBUG_CA_DDR5

        works = ctx->ca.check(channel, rank, address, 0);
        printf("%d", !!works);

#ifdef DEBUG_CA_DDR5
        printf("\n");
#endif // DEBUG_CA_DDR5

        if (works && eye.state == BEFORE) {
            eye.start = cadly;
            eye.state = INSIDE;
        } else if ((!works || cadly == SDRAM_PHY_DELAYS - 1) && eye.state == INSIDE) {
            eye.end = cadly;
            eye.state = AFTER;
        }

        ctx->ca.inc_dly(channel, rank, address);
    }
    ctx->ca.rst_dly(channel, rank, address);

    *right = eye.start;
    *left = eye.end;
}

static void CA_training(training_ctx_t *ctx, int32_t channel, uint8_t *success) {
    int left_side, right_side;
    int32_t rank, address;

    for (rank = 0; rank < ctx->ranks; rank++) {
        printf("Rank:%2"PRId32"\n", rank);
        // Enter CA training
        ctx->ca.enter_training_mode(channel, rank);

        for (address = 0; address < ctx->ca.line_count; address++) {
            printf("CA line:%2"PRId32"", address);

            left_side = UNSET_DELAY;
            right_side = UNSET_DELAY;

            // Reset CA delay
            ctx->ca.rst_dly(channel, rank, address);

            // Check if we are already in the eye
            if (ctx->ca.check(channel, rank, address, 0)) {
                // If we are, then find how much we can delay both CK and CS, until we leave the eye
                right_side = CA_ck_scan(ctx, channel, rank, address, ctx->cs.coarse_delays[channel][rank]);
            }

            printf("|");
            CA_scan(ctx, channel, rank, address, &left_side, &right_side);
            printf("|\n");

            // Check if we found the eye
            if (left_side == UNSET_DELAY || right_side == UNSET_DELAY) {
                // If not, then exit CA training
                printf("CA:%2"PRId32" Eye width:0 Failed\n", address);
                *success = 0;

                // Exit CA training early
                ctx->ca.exit_training_mode(channel, rank);
                return;
            }

            if (right_side > ctx->ca.delays[channel][address][0])
                ctx->ca.delays[channel][address][0] = right_side;

            if (left_side < ctx->ca.delays[channel][address][1])
                ctx->ca.delays[channel][address][1] = left_side;
        }

        // Exit CA training
        ctx->ca.exit_training_mode(channel, rank);
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
static void CS_CA_rescan(training_ctx_t *ctx, int ckdly) {
    int channel, rank, address;
    int shift_0101, cntdly, discard;
    printf("Re-scan CS/CA\n");
    for (channel = 0; channel < CHANNELS; channel++) {
        printf("Subchannel:%c\n", 'A'+channel);
        for (rank = 0; rank < ctx->ranks; rank++) {
            printf("Rank:%d\n", rank);

            //                    CS rescan                    //
            ctx->cs.enter_training_mode(channel, rank);

            // Get shift_0101 value
            shift_0101 = CS_should_shift_pattern(ctx, channel, rank);

            CS_ck_scan(ctx, channel, rank, shift_0101);
            // Restore CK delay after CS_ck_scan
            ctx->ck.rst_dly(0, 0, 0);
            for (cntdly = 0; cntdly < ckdly; ++cntdly)
                ctx->ck.inc_dly(0, 0, 0);

            printf("|");
            CS_scan(ctx, channel, rank, &discard, &discard, shift_0101);
            printf("\n");

            // Restore CS delay
            ctx->cs.rst_dly(channel, rank, 0);
            for (cntdly = 0; cntdly < ctx->cs.final_delays[channel][rank]; ++cntdly)
                ctx->cs.inc_dly(channel, rank, 0);

            ctx->cs.exit_training_mode(channel, rank);

            //                    CA rescan                    //
            ctx->ca.enter_training_mode(channel, rank);

            for (address = 0; address < ctx->ca.line_count; address++) {
                printf("Address:%2d\n", address);
                CA_ck_scan(ctx, channel, rank, address, ctx->cs.final_delays[channel][rank]);
                // Restore CK delay after CA_ck_scan
                ctx->ck.rst_dly(0, 0, 0);
                for (cntdly = 0; cntdly < ckdly; ++cntdly)
                    ctx->ck.inc_dly(0, 0, 0);

                printf("|");
                CA_scan(ctx, channel, rank, address, &discard, &discard);
                printf("\n");

                // Restore CA delay
                ctx->ca.rst_dly(channel, rank, address);
                for (cntdly = 0; cntdly < ctx->ca.final_delays[channel][address]; ++cntdly)
                    ctx->ca.inc_dly(channel, rank, address);
            }

            ctx->ca.exit_training_mode(channel, rank);
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
static void CS_CA_calculate_midpoints(training_ctx_t *ctx, int *min, int *max) {
    int channel, rank, address;
    int temp;
    for (channel = 0; channel < CHANNELS; channel++) {
        printf("Subchannel:%c Timings\n", 'A'+channel);
        for (rank = 0; rank < ctx->ranks; rank++) {
            temp = (ctx->cs.delays[channel][rank][0] + ctx->cs.delays[channel][rank][1])/2;
            printf("Rank:%2d: min delay %2d, max delay %2d, center %2d\n",
                rank, ctx->cs.delays[channel][rank][0], ctx->cs.delays[channel][rank][1], temp);

            ctx->cs.final_delays[channel][rank] = temp;
            *min = MIN(*min, temp);
            *max = MAX(*max, temp);
        }

        for (address = 0; address < ctx->ca.line_count; address++) {
            temp = (ctx->ca.delays[channel][address][0] + ctx->ca.delays[channel][address][1])/2;
            printf("CA:%2d: min delay %2d, max delay %2d, center %2d\n",
                address, ctx->ca.delays[channel][address][0], ctx->ca.delays[channel][address][1], temp);

            ctx->ca.final_delays[channel][address] = temp;
            *min = MIN(*min, temp);
            *max = MAX(*max, temp);
        }

        if (ctx->training_type == HOST_RCD) {
            temp = (ctx->par.delays[channel][0] + ctx->par.delays[channel][1])/2;
            printf("PAR: min delay %2d, max delay %2d, center %2d\n",
                ctx->par.delays[channel][0], ctx->par.delays[channel][1], temp);

            ctx->par.final_delays[channel] = temp;
            *min = MIN(*min, temp);
            *max = MAX(*max, temp);
        }

    }
}

/**
 * CS_CA_set_adjusted_delays
 *
 * Set signal delays to ones stored in the `final_delays`.
 * Delays are decreased by `ck_offset` to account for the
 * clock delay.
 */
static void CS_CA_set_adjusted_delays(training_ctx_t *ctx, int ck_offset) {
    int channel, rank, address, cntdly;
    for (channel = 0; channel < CHANNELS; channel++) {
        printf("Subchannel:%c Adjusted Tick_offsetgs\n", 'A'+channel);

        for (rank = 0; rank < ctx->ranks; rank++) {
            ctx->cs.final_delays[channel][rank] -= ck_offset;
            printf("Rank:%2d center point delay:%2d\n", rank, ctx->cs.final_delays[channel][rank]);
            ctx->cs.rst_dly(channel, rank, 0);
            for (cntdly = 0; cntdly < ctx->cs.final_delays[channel][rank]; ++cntdly)
                ctx->cs.inc_dly(channel, rank, 0);
        }

        for (address = 0; address < ctx->ca.line_count; address++) {
            ctx->ca.final_delays[channel][address] -= ck_offset;
            printf("CA:%2d center point delay:%2d\n", address, ctx->ca.final_delays[channel][address]);
            ctx->ca.rst_dly(channel, 0, address);
            for (cntdly = 0; cntdly < ctx->ca.final_delays[channel][address]; ++cntdly)
                ctx->ca.inc_dly(channel, 0, address);
        }

        if (ctx->training_type == HOST_RCD) {
            ctx->par.final_delays[channel] -= ck_offset;
            printf("PAR center point delay:%2d\n", ctx->par.final_delays[channel]);
            ctx->par.rst_dly(channel, 0, 0);
            for (cntdly = 0; cntdly < ctx->par.final_delays[channel]; ++cntdly)
                ctx->par.inc_dly(channel, 0, 0);
        }
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
 * This way, all midpoints are in the [0, SDRAM_PHY_DELAYS)
 * range.
 */
static void CK_CS_CA_finalize_timings(training_ctx_t *ctx) {
    int new_ckdly, cntdly;
    int min, max;
    min = SDRAM_PHY_DELAYS;
    max = -SDRAM_PHY_DELAYS;

    // Calculate eye midpoints for all trained signals and save them in final_delays.
    // Also find minimal and maximal used delays to use them later to shift the clock
    CS_CA_calculate_midpoints(ctx, &min, &max);

    printf("Max center point delay:%2d, min center point delay:%2d, spread:%2d\n", max, min, max-min);

    // Calculate new clock delay. It is the smallest of used delays
    printf("Adjusting clock delay, so min center point is at delay 0\n");
    new_ckdly = (SDRAM_PHY_DELAYS - min) % SDRAM_PHY_DELAYS;

    // Set new clock delay
    printf("New clock delay:%2d\n", new_ckdly);

    ctx->ck.rst_dly(0, 0, 0);
    for (cntdly = 0; cntdly < new_ckdly; ++cntdly)
        ctx->ck.inc_dly(0, 0, 0);

    // Now that CK is shifted, we can set new delays calculated
    // in `CS_CA_calculate_midpoints` adjusted by the clock offset,
    // which is equal to the minimal midpoint.
    CS_CA_set_adjusted_delays(ctx, min);

    // Make sure that selected delays still work
    CS_CA_rescan(ctx, new_ckdly);
}

#ifdef SKIP_NO_DELAYS
void sdram_ddr5_cs_ca_training(training_ctx_t *ctx) {
    printf("CS/CA training impossible\n"
           "Keeping DRAM in 2N mode\n");
}
#else
void sdram_ddr5_cs_ca_training(training_ctx_t *ctx) {
#ifndef SDRAM_PHY_ADDRESS_DELAY_CAPABLE
    printf("WARNING:\n"
           "PHY does not have IO delays on address lines!!!\n"
           "BIOS will try to check if 1N mode is possible, but it may be unstable.\n"
           "Build BIOS with -DSKIP_NO_DELAYS, to skip CS/CA training and force 2N mode.\n");
#endif // SDRAM_PHY_ADDRESS_DELAY_CAPABLE
    int32_t channel, rank;
    uint8_t CS_success, CA_success;
    disable_dfi_2n_mode();

    CS_success = 1;
    CA_success = 1;
    CA_setup_array(ctx);
    for (channel = 0; channel < CHANNELS; channel++) {
        printf("Subchannel:%c CS training\n", (char)('A'+channel));
        CS_training(ctx, channel, &CS_success);
        printf("CA training\n");
        CA_check_lines(ctx, channel);
        CA_training(ctx, channel, &CA_success);
    }
    if (!(CS_success & CA_success)) {
        enable_dfi_2n_mode();
    } else {
        CK_CS_CA_finalize_timings(ctx);
        for (channel = 0; channel < CHANNELS; channel++) {
            for (rank = 0; rank < ctx->ranks; rank++) {
                disable_dram_2n_mode(channel, rank);
            }
        }
    }
}
#endif // SKIP_NO_DELAYS

int enumerated = 0;

void sdram_ddr5_module_enumerate(training_ctx_t *ctx) {
    int channel, rank, module;
    if (SDRAM_PHY_MODULES/CHANNELS > 15) {
        printf("Too many modules on single rank to enumerate,\n"
               "maximum is 15 but this design has %2d\n", SDRAM_PHY_MODULES);
        enumerated = 0;
        return;
    }
    for (channel = 0; channel < CHANNELS; channel++) {
        printf("Enumerating subchannel:%c\n", (char)('A'+channel));
        for (rank = 0; rank < ctx->ranks; rank++) {
            printf("\tEnumerating rank:%2d\n", rank);
            // Enter PDA Enumerate Programming Mode
            send_mpc(channel, rank, 0xB);
            for (module = 0; module < SDRAM_PHY_MODULES/CHANNELS; module++) {
                setup_enumerate(channel, rank, module);
            }
            // Exit PDA Enumerate Programming Mode
            send_mpc(channel, rank, 0xA);
        }
    }
    enumerated = 1;
}

static const uint8_t seeds0[] = {
    0x1c, 0x5a, 0x24,
#ifndef DDR5_TRAINING_SIM
    0x36, 0xaa, 0xc1,
#endif
};

static const uint8_t seeds1[] = {
    0x72, 0x55, 0x95,
#ifndef DDR5_TRAINING_SIM
    0x59, 0x3c, 0x48,
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

// MR2:OP[7] value to use, whenever MR2 is being modified
static int use_internal_write_timing = 0;

/**
 * read_serial_number
 *
 * Reads serial number from the mode registers.
 * It is a 5 byte value stored in registers
 * MR65-MR69.
 * JESD79-5A 3.5.66-70
 */
static uint64_t read_serial_number(int channel, int rank, int module) {
    int i;
    uint64_t serial_number = 0;

    // Serial number is a 5 byte value
    for (i = 0; i < 5; i++) {
        // Base register 65
        send_mrr(channel, rank, 65+i);
        serial_number = (serial_number << 8) | recover_mrr_value(channel, module);
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
    send_mrw(channel, rank, MODULE_BROADCAST, 2, 1|use_internal_write_timing);
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
    send_mrw(channel, rank, MODULE_BROADCAST, 2, 0|use_internal_write_timing);
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
static int rd_cycle_dly_idly_check_if_works(int channel, int rank, int module) {
    int works = 1;
    int seed;

#ifndef DDR5_TRAINING_SIM
    // Check if Serial readout works
    for (seed = 0; seed < serial_count && works; seed++) {
        /* Setup MRs */
        send_mrw(channel, rank, module, 25, 0); // select Serial mode
        send_mrw(channel, rank, module, 26, serial[seed]&0xff);
        send_mrw(channel, rank, module, 27, serial[seed]>>8);
        send_mrr(channel, rank, 31);
        works &= compare_serial(channel, module, serial[seed], 0xA5, 0x33);
    }
#endif // DDR5_TRAINING_SIM

    // Check if LFSR readout works
    for (seed = 0; seed < seeds_count && works; ++seed) {
        /* Setup MRs */
        send_mrw(channel, rank, module, 25, 1); // select LFSR mode
        send_mrw(channel, rank, module, 26, seeds0[seed]);
        send_mrw(channel, rank, module, 27, seeds1[seed]);
        send_mrr(channel, rank, 31);
        works &= compare(channel, module, seeds0[seed], seeds1[seed], 0xA5, 0x33);
    }

    return works;
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
static int find_read_preamble_cycle(int channel, int rank, int module) {
    int rd_cycle_dly, idly, preamble;

    // in this stage we don't care about eye end
    eye_t eye = DEFAULT_EYE;

#ifdef INFO_DDR5
    printf("Finding read preamble\n");
#endif // INFO_DDR5

    /* Coarse alignment */
    rd_rst(channel, module);
    for (rd_cycle_dly = 0; rd_cycle_dly < MAX_READ_CYCLE_DELAY && eye.state != AFTER; rd_cycle_dly ++) {
#ifdef INFO_DDR5
        printf("%2d|", rd_cycle_dly);
#endif // INFO_DDR5

        idly_rst(channel, module);
        for (idly = 0; idly < SDRAM_PHY_DELAYS; idly++) {
            send_mrr(channel, rank, 31);
            preamble = captured_preamble(channel, module);

#ifdef INFO_DDR5
            printf("%01x", preamble);
#endif // INFO_DDR5

            // Should be 1tCK preamble 0b10 (JESD79-5A 4.18.3),
            // but due to the way basephy.py works we sample 2 cycles,
            // so we get 4 bits 0b0010, which gets reversed to 0b0100.
            if (preamble == 4 && eye.state == BEFORE) {
                eye.start = rd_cycle_dly;
                eye.state = INSIDE;
            } else if (preamble != 4 && eye.state == INSIDE) {
                eye.state = AFTER;
            }
            idly_inc(channel, module);
        }

#ifdef INFO_DDR5
        printf("\n");
#endif // INFO_DDR5

        rd_inc(channel, module);
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
static void read_training_data_scan(int channel, int rank, int module, int preamble_cycle) {
    eye_t eye = DEFAULT_EYE;

    int rd_cycle_dly, idly;
    int works;

    // Pull back 1 cycle as DQ and DQS can be misaligned
    preamble_cycle -= 1;

    printf("Data scan:\n");

    // Set read cycle delay
    rd_rst(channel, module);
    for (rd_cycle_dly = 0; rd_cycle_dly < preamble_cycle; rd_cycle_dly++) {
        rd_inc(channel, module);
    }

    for (rd_cycle_dly = preamble_cycle; rd_cycle_dly < MAX_READ_CYCLE_DELAY && eye.state != AFTER; rd_cycle_dly++) {
        printf("%2d|", rd_cycle_dly);

#ifdef DEBUG_DDR5
        printf("\n");
#endif // DEBUG_DDR5

        idly_rst(channel, module);
        for(idly = 0; idly < SDRAM_PHY_DELAYS; idly++){

#ifdef DEBUG_DDR5
            printf("DQ dly:%"PRIu16"\n", get_rd_dq_dly(channel, module));
#endif // DEBUG_DDR5

            works = rd_cycle_dly_idly_check_if_works(channel, rank, module);
            printf("%d", works);

            if (works && eye.state == BEFORE) {
                eye.start = rd_cycle_dly * SDRAM_PHY_DELAYS + idly;
                eye.state = INSIDE;
            } else if (!works && eye.state == INSIDE) {
                eye.end = rd_cycle_dly * SDRAM_PHY_DELAYS + idly;
                eye.state = AFTER;
            }

#ifdef DEBUG_DDR5
            printf("\n");
#endif // DEBUG_DDR5

            idly_inc(channel, module);
        }

        printf("|\n");
        rd_inc(channel, module);
    }

    int eye_width = eye.end - eye.start;
    eye.center = eye.start + (eye_width / 2);
    int eye_center_cycle = eye.center / SDRAM_PHY_DELAYS;
    int eye_center_delay = eye.center % SDRAM_PHY_DELAYS;

    printf("eye_width:%2d; eye center: cycle:%2d,delay:%2d\n",
        eye_width, eye_center_cycle, eye_center_delay);

    // Setting read delay to eye center
    rd_rst(channel, module);
    for (rd_cycle_dly = 0; rd_cycle_dly < eye_center_cycle; rd_cycle_dly++) {
        rd_inc(channel, module);
    }

    idly_rst(channel, module);
    for (idly = 0; idly < eye_center_delay; idly++) {
        idly_inc(channel, module);
    }
}

#ifdef INFO_DDR5
/**
 * simple_read_check
 *
 * Performs a simple read check, in which a 0xDEADBEEF
 * is being written to the scratch pad register of the DRAM
 * one byte at a time. After each write, a read is being
 * performed and the read value is being compared with the one
 * written before.
 */
static int simple_read_check(int channel, int rank, int module) {
    int works = 1;

    printf("Simple read check: ");

    // Check if data is read correctly
    uint8_t test_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    for (int i = 0; i < 4; i++) {
        send_mrw(channel, rank, module, DRAM_SCRATCH_PAD, test_data[i]);
        send_mrr(channel, rank, DRAM_SCRATCH_PAD);

        uint8_t read_back = recover_mrr_value(channel, module);
        works &= read_back == test_data[i];

        printf("%"PRIX8, read_back);
    }
    printf("\n");

    return works;
}
#endif // INFO_DDR5

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
void sdram_ddr5_read_training(training_ctx_t *ctx) {
    int channel, rank, module;
    for (channel = 0; channel < CHANNELS; channel++) {
        printf("Subchannel:%c Read training\n", (char)('A'+channel));
        for (rank = 0; rank < ctx->ranks; rank++) {
            printf("Training rank%2d\n", rank);

            // Enter Read Preamble Training Mode
            enter_rptm(channel, rank);

            for (module = 0; module < SDRAM_PHY_MODULES/CHANNELS; module++) {
                printf("Training module%2d\n", module);

                // Find cycle in which read preamble starts
                int preamble_cycle = find_read_preamble_cycle(channel, rank, module);

                if (preamble_cycle == -1) {
                    printf("Failed to find read preamble for module %2d\n", module);
                    continue;
                }
                printf("Read preamble starts in cycle:%2d\n", preamble_cycle);

                read_training_data_scan(channel, rank, module, preamble_cycle);
            }

            // Exit Read Preamble Training Mode
            exit_rptm(channel, rank);

            // We must perform read checks below after exiting RPTM
            for (module = 0; module < SDRAM_PHY_MODULES/CHANNELS; module++) {
                // Read the serial number
                printf(
                    "Channel:%c rank:%2d module:%2d serial number: 0x%010"PRIX64"\n",
                    (char)('A'+channel),
                    rank,
                    module,
                    read_serial_number(channel, rank, module)
                );

#ifdef INFO_DDR5
                if (!simple_read_check(channel, rank, module)) {
                    printf("Simple read check failure!\n");
                    continue;
                }

                printf("Channel:%c rank:%d module:%d\n", (char)('A'+channel), rank, module);
                read_registers(channel, rank, module);
#endif // INFO_DDR5
            }
        }
    }
}

void sdram_ddr5_write_training(training_ctx_t *ctx) {
    int channel, rank, module, seed, cnt_seed, byte;
    int cycle, delay, got, sample, it, works;
    int start_cycle, start_delay,   // First working cycle delay pair
        middle_cycle, middle_delay, // Middle between first and last working
        end_cycle, end_delay;       // First cycle delay pair that does not work after working
    uint8_t lfsr, temp;
    uint8_t mr5;
    uint16_t wrdata, rddata;
    uint32_t eye_width;             // In taps
    use_internal_write_timing = 1<<7;
    for (channel = 0; channel < CHANNELS; channel++) {
        printf("Subchannel:%c Write leveling\n", (char)('A'+channel));
        /* Coarse alignment */
        for (rank = 0; rank < ctx->ranks; rank++) {
            enter_write_leveling(channel);
            /* Setup MRs */
            send_mrw(channel, rank, 0xf, 2, 2);
            for (module = 0; module < SDRAM_PHY_MODULES/CHANNELS; module++) {
                printf("WL m:%2d\n", module);
                start_cycle = -1;
                wr_dqs_rst(channel, module);
                cycle = 0;
                for(delay = SDRAM_PHY_MIN_WR_LATENCY; delay < SDRAM_PHY_CWL/2; ++delay, ++cycle) {
                    wr_dqs_inc(channel, module);
                }
                got = 0;
                for (; cycle < 63 && got < 1; ++cycle) {
                    sample = 1;
                    // Check multiple times, as we can be on the edge of transition
                    // Make sure we aren't in meta stable delay
                    printf("%2d|", cycle);
                    for (it = 0; it<16; it++) {
                        send_wleveling_write(channel, rank);
                        temp = wleveling_sample(channel, module);
                        sample &= temp;
#ifdef DEBUG_DDR5
                        printf("%d:%d|", sample, temp);
#else
                        printf("%d", sample);
#endif // DEBUG_DDR5
                    }
                    printf("|\n");
                    if (sample && got == 0) {
                        start_cycle = cycle;
                        got = 1;
                    }
                    wr_dqs_inc(channel, module);
                }
                if (start_cycle == -1) {
                    printf("Failed to find result for %2d\n", module);
                    continue;
                }
                wr_dqs_rst(channel, module);
                printf("DQS write leveling starts in cycle:%2d (adjusted %2d)\n",
                    start_cycle, start_cycle + SDRAM_PHY_MIN_WR_LATENCY);
                /* Pull back 1 cycle */
                start_cycle -= 1;
                wr_dqs_rst(channel, module);
                odly_dqs_rst(channel, module);
                for (it = 0; it < start_cycle; ++it) {
                    wr_dqs_inc(channel, module);
                }

                got = 0;
                cycle = start_cycle;
                start_cycle = -1; start_delay = 1;
                printf("DQS edge scan:\n");
                // Break out when 0 to 1 transition was found
                do {
                    wleveling_scan(&cycle, &got, &start_cycle, &start_delay, channel, rank, module);
                } while (got != 1);

#ifdef INFO_DDR5
                printf("cycle:%2d delay:%2d\n", start_cycle, start_delay);
#endif // INFO_DDR5

                // Pull back 0.75 clock as specified by JEDEC
                // to train WICA
                start_cycle -= 1;
                start_delay += SDRAM_PHY_DELAYS/4;
                if (start_delay >= SDRAM_PHY_DELAYS) {
                    start_cycle += 1;
                    start_delay -= SDRAM_PHY_DELAYS;
                }

#ifdef INFO_DDR5
                printf("After -0.75; cycle:%2d delay:%2d\n", start_cycle, start_delay);
#endif // INFO_DDR5

                wr_dqs_rst(channel, module);
                odly_dqs_rst(channel, module);
                for (it = 0; it < start_cycle; ++it) {
                    wr_dqs_inc(channel, module);
                }
                for (it = 0; it < start_delay; ++it) {
                    odly_dqs_inc(channel, module);
                }

                printf("DQS internal cycle alignment\n|");
                send_mrw(channel, rank, module, 2, 2|use_internal_write_timing);
                got = 0;
                cycle = 0;
                do {
                    send_mrw(channel, rank, module, 3, cycle);
                    sample = 1;
                    // Check multiple times, as we can be on the edge of transition
                    // Make sure we aren't in meta stable delay
                    for (it = 0; it<16; it++) {
                        send_wleveling_write(channel, rank);
                        sample &= wleveling_sample(channel, module);
                    }
                    printf("WICA:%d,%d|", cycle, sample);
                    ++cycle;
                    if (sample && got == 0) {
                        got = 1;
                    }
                } while (got != 1 && cycle < 7 );
                printf("\n");

                start_cycle -= 1;

                wr_dqs_rst(channel, module);
                odly_dqs_rst(channel, module);
                for (it = 0; it < start_cycle; ++it) {
                    wr_dqs_inc(channel, module);
                }

                got = 0;
                cycle = start_cycle;
                start_cycle = -1; start_delay = -1;
                printf("Scan for internal edge:\n");
                // Break out when 0 to 1 transition was found
                do {
                    wleveling_scan(&cycle, &got, &start_cycle, &start_delay, channel, rank, module);
                } while (got != 1);

                // Push forward 1.25 clock as specified by JEDEC
                // after training WICA

                start_cycle += 1;
                start_delay += SDRAM_PHY_DELAYS/4;
                if (start_delay >= SDRAM_PHY_DELAYS) {
                    start_cycle += 1;
                    start_delay -= SDRAM_PHY_DELAYS;
                }

                printf("Final timing values: cycles:%2d(adjusted %2d) delay:%2d\n",
                    start_cycle, start_cycle + SDRAM_PHY_MIN_WR_LATENCY, start_delay);

                wr_dqs_rst(channel, module);
                odly_dqs_rst(channel, module);
                for (it = 0; it < start_cycle; ++it) {
                    wr_dqs_inc(channel, module);
                }
                for (it = 0; it < start_delay; ++it) {
                    odly_dqs_inc(channel, module);
                }
            }
            send_mrw(channel, rank, 0xf, 2, 0|use_internal_write_timing);
            exit_write_leveling(channel);
#ifdef DEBUG_DDR5
            for (module = 0; module < SDRAM_PHY_MODULES/CHANNELS; module++) {
                read_registers(channel, rank, module);
            }
#endif // DEBUG_DDR5
            printf("DQ write training\n");
            mr5 = 0;
            for (module = 0; module < SDRAM_PHY_MODULES/CHANNELS; module++) {
                send_mrr(channel, rank, 5);
                mr5 = recover_mrr_value(channel, module);
                printf("mr5:%02"PRIx8"\n", mr5);
                send_mrw(channel, rank, module, 5, mr5 & 0xDF); // Disable DM

                wr_dq_rst(channel, module);
                printf("m%2d|\n", module);
                cycle = 0;
                got = 0;
                start_cycle = -1; start_delay = 1;
                end_cycle = -1; end_delay = -1;
                printf("Data scan:\n");
                while (got != 2 && cycle < 65) {
                    printf("%2d|", cycle);
#ifdef DEBUG_DDR5
                    printf("\n");
#endif // DEBUG_DDR5
                    odly_dq_rst(channel, module);
                    for(delay = 0; delay < SDRAM_PHY_DELAYS; ++delay){
#ifdef DEBUG_DDR5
                        printf("DQ dly:%"PRIu16"\n", get_wr_dq_dly(channel, module));
#endif // DEBUG_DDR5
                        works = 1;
#ifndef DDR5_TRAINING_SIM
                        for (cnt_seed = 0; cnt_seed < serial_count && works; ++cnt_seed) {
                            for (it =0; it <8; ++it) {
                                wrdata = 0;
                                for (temp = 0; temp < SDRAM_PHY_DQ_DQS_RATIO; ++temp) {
                                    wrdata |= ((serial[cnt_seed]>>(2*it))&1) << temp;
                                }
                                for (temp = 0; temp < SDRAM_PHY_DQ_DQS_RATIO; ++temp) {
                                    wrdata |= ((serial[cnt_seed]>>(2*it+1))&1) << (temp + SDRAM_PHY_DQ_DQS_RATIO);
                                }
#ifdef DEBUG_DDR5
                                printf("wrdata:%04"PRIx16"|", wrdata);
#endif // DEBUG_DDR5
                                set_data_module_phase(channel, module, it, wrdata);
                            }
#ifdef DEBUG_DDR5
                            printf("\n");
#endif // DEBUG_DDR5
                            send_write(channel, rank);
                            send_read(channel, rank);

                            for (it =0; it <8; ++it) {
                                rddata = get_data_module_phase(channel, module, it);
#ifdef DEBUG_DDR5
                                printf("rddata:%04"PRIx16"|", rddata);
#endif // DEBUG_DDR5
                                for (temp = 0; temp < SDRAM_PHY_DQ_DQS_RATIO; ++temp) {
                                    works &= !!(((rddata>>temp)&1) == ((serial[cnt_seed]>>(2*it))&1));
                                }
                                for (temp = 0; temp < SDRAM_PHY_DQ_DQS_RATIO; ++temp) {
                                    works &= !!(((rddata>>(temp + SDRAM_PHY_DQ_DQS_RATIO))&1) == ((serial[cnt_seed]>>(2*it+1))&1));
                                }
                            }
                            for (it =0; it <8; ++it) {
                                set_data_module_phase(channel, module, it, 0);
                            }
                            send_write(channel, rank);
#ifdef DEBUG_DDR5
                            printf("\n");
#endif // DEBUG_DDR5
                        }
#endif // DDR5_TRAINING_SIM
                        for (cnt_seed = 0; cnt_seed < seeds_count * 2 && works; ++cnt_seed) {
                            if(cnt_seed < seeds_count)
                                seed = seeds0[cnt_seed];
                            else
                                seed = seeds1[cnt_seed - seeds_count];

                            lfsr = seed;
                            for (it =0; it <8; ++it) {
                                wrdata = lfsr;
                                lfsr = lfsr_next(lfsr);
                                if (BYTES_PER_MODULE > 1) {
                                    wrdata |= lfsr << 8;
                                    lfsr = lfsr_next(lfsr);
                                }
#ifdef DEBUG_DDR5
                                printf("wrdata:%04"PRIx16"|", wrdata);
#endif // DEBUG_DDR5
                                set_data_module_phase(channel, module, it, wrdata);
                            }
#ifdef DEBUG_DDR5
                            printf("\n");
#endif // DEBUG_DDR5
                            send_write(channel, rank);
                            send_read(channel, rank);

                            lfsr = seed;
                            for (it =0; it <8; ++it) {
                                rddata = get_data_module_phase(channel, module, it);
#ifdef DEBUG_DDR5
                                printf("rddata:%04"PRIx16"|", rddata);
#endif // DEBUG_DDR5
                                works &= ((rddata&0xff) == lfsr);
                                lfsr = lfsr_next(lfsr);
                                if (BYTES_PER_MODULE > 1) {
                                    rddata >>= 8;
                                    works &= (rddata == lfsr);
                                    lfsr = lfsr_next(lfsr);
                                }
                            }
                            for (it =0; it <8; ++it) {
                                set_data_module_phase(channel, module, it, 0);
                            }
                            send_write(channel, rank);
#ifdef DEBUG_DDR5
                            printf("\n");
#endif // DEBUG_DDR5
                        }
                        printf("%d", works);
#ifdef DEBUG_DDR5
                        printf("\n");
#endif // DEBUG_DDR5
                        if (works && got == 0) {
                            start_cycle = cycle;
                            start_delay = delay;
                            got = 1;
                        } else if (!works && got == 1) {
                            end_cycle = cycle;
                            end_delay = delay;
                            got = 2;
                        }
                        odly_dq_inc(channel, module);
                    }
                    printf("|\n");
                    ++cycle;
                    wr_dq_inc(channel, module);
                }
                printf("m%2d|start cycle:%2d, delay:%2d; end cycle:%2d, delay:%2d|",
                    module, start_cycle, start_delay, end_cycle, end_delay);
                eye_width = (end_cycle-start_cycle)*SDRAM_PHY_DELAYS + end_delay - start_delay;
                middle_cycle = start_cycle + (start_delay + eye_width/2)/SDRAM_PHY_DELAYS;
                middle_delay = (start_delay + eye_width/2)%SDRAM_PHY_DELAYS;
                printf("eye_width:%2"PRIu32"; eye center: cycle:%2d,delay:%2d\n",
                    eye_width, middle_cycle, middle_delay);

                // Setting read delay to eye center
                wr_dq_rst(channel, module);
                odly_dq_rst(channel, module);
                for (it = 0; it < middle_cycle; ++it) {
                    wr_dq_inc(channel, module);
                }
                for (it = 0; it < middle_delay; ++it) {
                    odly_dq_inc(channel, module);
                }

                // DM training
                printf("%x\n", mr5);
                if (mr5 & 0x20) { // DM was enabled
                    printf("DM scan\nm:%2d DM|", module);
                    odly_dm_rst(channel, module);
                    got = 0;
                    start_delay = 1; end_delay = -1;
                    for(delay = 0; delay < SDRAM_PHY_DELAYS; ++delay){
#ifdef DEBUG_DDR5
                        printf("DM dly:%"PRIu16"\n", get_wr_dm_dly(channel, module));
#endif // DEBUG_DDR5
                        works = 1;
                        for (byte = 0; byte < 16 && works; ++byte) {
#ifndef DDR5_TRAINING_SIM
                            for (cnt_seed = 0; cnt_seed < seeds_count * 2 && works; ++cnt_seed) {
#else
                            {cnt_seed = 0;
#endif // DDR5_TRAINING_SIM
                                send_mrw(channel, rank, module, 5, mr5 & 0xDF); // Disable DM
                                if(cnt_seed < seeds_count)
                                    seed = seeds0[cnt_seed];
                                else
                                    seed = seeds1[cnt_seed - seeds_count];

                                for (it =0; it <8; ++it) {
                                    set_data_module_phase(channel, module, it, 0);
                                }
                                send_write(channel, rank);

                                send_mrw(channel, rank, module, 5, mr5); // Enable DM
                                lfsr = seed;
                                for (it =0; it <8; ++it) {
                                    wrdata = lfsr;
                                    lfsr = lfsr_next(lfsr);
                                    if (BYTES_PER_MODULE > 1) {
                                        wrdata |= lfsr << 8;
                                        lfsr = lfsr_next(lfsr);
                                    }
#ifdef DEBUG_DDR5
                                    printf("wrdata:%04"PRIx16"|", wrdata);
#endif // DEBUG_DDR5
                                    set_data_module_phase(channel, module, it, wrdata);
                                }
#ifdef DEBUG_DDR5
                                printf("\n");
#endif // DEBUG_DDR5
                                send_write_byte(channel, rank, module, byte);
                                send_read(channel, rank);

                                lfsr = seed;
                                for (it =0; it <8; ++it) {
                                    rddata = get_data_module_phase(channel, module, it);
#ifdef DEBUG_DDR5
                                    printf("rddata:%04"PRIx16"|", rddata);
#endif // DEBUG_DDR5
                                    if ((byte >> 1) == it) {
                                        if(!(byte & 1)) {
                                            works &= ((rddata&0xff) == lfsr);
                                        }
                                        lfsr = lfsr_next(lfsr);
                                        rddata >>= 8;
                                        if(byte & 1) {
                                            works &= ((rddata&0xff) == lfsr);
                                        }
                                        lfsr = lfsr_next(lfsr);
                                    } else {
                                        lfsr = lfsr_next(lfsr);
                                        lfsr = lfsr_next(lfsr);
                                    }
                                }
                                for (it =0; it <8; ++it) {
                                    set_data_module_phase(channel, module, it, 0);
                                }
                                send_write(channel, rank);
#ifdef DEBUG_DDR5
                                printf("\n");
#endif // DEBUG_DDR5
                            }
                        }
                        printf("%d", works);
#ifdef DEBUG_DDR5
                        printf("\n");
#endif // DEBUG_DDR5
                        if (works && got == 0) {
                            start_delay = delay;
                            got = 1;
                        }
                        if (!works && got == 1) {
                            end_delay = delay;
                            got = 2;
                        } else if (delay == SDRAM_PHY_DELAYS-1 && got == 1) {
                            end_delay = delay + 1;
                            got = 2;
                        }
                        odly_dm_inc(channel, module);
                    }
                    printf("|\n");
                }
                printf("m%2d|DM start delay:%2d; delay:%2d|",
                    module, start_delay, end_delay);
                eye_width = end_delay - start_delay;
                middle_delay = (start_delay + eye_width/2)%SDRAM_PHY_DELAYS;
                printf("eye_width:%2"PRIu32"; eye center: delay:%2d\n",
                    eye_width, middle_delay);

                // Setting read delay to eye center
                odly_dm_rst(channel, module);
                for (it = 0; it < middle_delay; ++it) {
                    odly_dm_inc(channel, module);
                }
            }
        }
    }
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

training_ctx_t host_dram_ctx = {
    .ck = {
        .rst_dly = ck_rst,
        .inc_dly = ck_inc,
    },
    .cs = {
        .enter_training_mode = enter_cstm,
        .exit_training_mode  = exit_cstm,
        .rst_dly = cs_rst,
        .inc_dly = cs_inc,
        .check = cs_check_if_works,
    },
    .ca = {
        .line_count = 13,
        .enter_training_mode = enter_catm,
        .exit_training_mode  = exit_catm,
        .inc_dly = ca_inc,
        .rst_dly = ca_rst,
        .check = ca_check_if_works,
        .has_line13 = ca_check_if_has_line13,
    },
    .par = {
        .rst_dly = par_rst,
        .inc_dly = par_inc,
    },
    .training_type = HOST_DRAM,

    // So far, all PHYs have support for single delay far all ranks,
    // use only first rank, leave all other as inactive.
    // Use SDRAM_PHY_RANKS if we ever support multiple ranks
    // and independent timing for them.
    .ranks = 1,
};

#if defined(CONFIG_HAS_I2C)
/**
 * dca_check_if_has_line13
 *
 * Detect if DCA13 is present
 * Requires to already be in the DCATM.
 */
static int dca_check_if_has_line13(int32_t channel) {
    ddrphy_alert_reduce_write(1); // write 1 to reduce with AND
    cmd_injector(channel, 0xf, 0, 1<<13, 0, 0, 1, 0);
    cmd_injector(channel, 0x1, 1, 1<<13, 0, 0, 1, 0);
    store_continuous(channel);

    return ddrphy_alert_read();
}

training_ctx_t host_rcd_ctx = {
    .ck = {
        .rst_dly = ck_rst,
        .inc_dly = ck_inc,
    },
    .cs = {
        .enter_training_mode = enter_dcstm,
        .exit_training_mode  = exit_dcstm,
        .rst_dly = cs_rst,
        .inc_dly = cs_inc,
        .check = dcs_check_if_works,
    },
    .ca = {
        .line_count = 14,
        .enter_training_mode = enter_dcatm,
        .exit_training_mode  = exit_dcatm,
        .inc_dly = ca_inc,
        .rst_dly = ca_rst,
        .check = dca_check_if_works,
        .has_line13 = dca_check_if_has_line13,
    },
    .par = {
        .rst_dly = par_rst,
        .inc_dly = par_inc,
    },
    .training_type = HOST_RCD,

    // So far, all PHYs have support for single delay far all ranks,
    // use only first rank, leave all other as inactive.
    // Use SDRAM_PHY_RANKS if we ever support multiple ranks
    // and independent timing for them.
    .ranks = 1,
};

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
    if (!sdram_read_spd(spd, 3, &module_type, 1, true)) {
        printf("Couldn't read the SPD and check the module type. Defaulting to UDIMM.");
        return UDIMM;
    }

    // Module type is in the lower nibble
    return module_type & 0x0f;
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
    training_ctx_t *base_ctx = &host_dram_ctx;

#if defined(CONFIG_HAS_I2C)
    bool is_rdimm = read_module_type(0) == RDIMM;

    if (is_rdimm)
        base_ctx = &host_rcd_ctx;
#endif // defined(CONFIG_HAS_I2C)

    sdram_ddr5_module_enumerate(base_ctx);
    sdram_ddr5_cs_ca_training(base_ctx);

    if (in_2n_mode()) {
        printf("2N mode setup\n");
        init_sequence_2n();
    } else {
        printf("1N mode setup\n");
        init_sequence_1n();
    }

    sdram_ddr5_read_training(base_ctx);
    sdram_ddr5_write_training(base_ctx);
}

#endif // defined(CSR_SDRAM_BASE) && defined(SDRAM_PHY_DDR5)
