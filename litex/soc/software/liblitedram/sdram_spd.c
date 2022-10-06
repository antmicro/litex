#include <generated/csr.h>
#include <generated/mem.h>

#include <stdlib.h>
#include <string.h>

#include <libbase/memtest.h>
#include <libbase/lfsr.h>
#include <libbase/i2c.h>

#ifdef CSR_SDRAM_BASE
#include <generated/sdram_phy.h>
#include <generated/sdram_timings.h>
#endif
#include <generated/soc.h>
#include <generated/mem.h>
#include <system.h>

#include <liblitedram/sdram.h>
#include <liblitedram/sdram_dbg.h>
#include <liblitedram/sdram_spd.h>


/*-----------------------------------------------------------------------*/
/* SPD parsing                                                           */
/*-----------------------------------------------------------------------*/
int sdram_spd_parse_geometry_ddr4(struct sdram_spd_ctx_s *ctx, uint8_t *spd)
{
    if ((ctx == NULL) || (spd == NULL)) {
        return 0;
    }

    int bankgroupbits_opts[] = {0, 1, 2};
    int bankbits_opts[] = {2, 3};
    int rowbits_opts[] = {12, 13, 14, 15, 16, 17, 18};
    int colbits_opts[] = {9, 10, 11, 12};

    int groupbits = bankgroupbits_opts[FIELD(spd[4], 2, 6)];
    int groupbankbits = bankbits_opts[FIELD(spd[4], 2, 4)];
    int rowbits = rowbits_opts[FIELD(spd[5], 3, 3)];
    int colbits = colbits_opts[FIELD(spd[5], 3, 0)];

    struct sdram_spd_geometry_s geometry = {
        .bankbits = groupbits + groupbankbits,
        .rowbits = rowbits,
        .colbits = colbits
    };

    memcpy(&(ctx->geometry), &geometry, sizeof(geometry));

    return 1;
}

int sdram_spd_parse_timebase_ddr4(struct sdram_spd_ctx_s *ctx, uint8_t *spd)
{
    if ((ctx == NULL) || (spd == NULL)) {
        return 0;
    }

    int medium_timebases_ps[] = {125};
    int fine_timebases_ps[] = {1};

    ctx->medium_timebase_ps = medium_timebases_ps[FIELD(spd[17], 2, 2)];
    ctx->fine_timebase_ps = fine_timebases_ps[FIELD(spd[17], 2, 0)];

    return 1;
}

int sdram_spd_txx_ps(struct sdram_spd_ctx_s *ctx, uint16_t mtb, int8_t ftb)
{
    int medium = ctx->medium_timebase_ps * mtb;
    int fine = ctx->fine_timebase_ps * ftb;
    return medium + fine;
}

int sdram_spd_parse_timings_ddr4(struct sdram_spd_ctx_s *ctx, uint8_t *spd)
{
    if ((ctx == NULL) || (spd == NULL)) {
        return 0;
    }

    /* Read raw values from SPD */
    int trefi_1x = 7812500; // 64e9/8192
    int tckavg_min = sdram_spd_txx_ps(ctx, spd[18], spd[125]);
    //int tckavg_max = sdram_spd_txx_ps(ctx, spd[19], spd[124]);
    //int taa_min = sdram_spd_txx_ps(ctx, spd[24], spd[123]);
    int trcd_min = sdram_spd_txx_ps(ctx, spd[25], spd[122]);
    int trp_min = sdram_spd_txx_ps(ctx, spd[26], spd[121]);
    int tras_min = sdram_spd_txx_ps(ctx, WORD(LSN(spd[27]), spd[28]), 0);
    //int trc_min = sdram_spd_txx_ps(ctx, WORD(MSN(spd[27]), spd[29]), spd[120]);
    int trfc1_min = sdram_spd_txx_ps(ctx, WORD(spd[31], spd[30]), 0);
    int trfc2_min = sdram_spd_txx_ps(ctx, WORD(spd[33], spd[32]), 0);
    int trfc4_min = sdram_spd_txx_ps(ctx, WORD(spd[35], spd[34]), 0);
    int tfaw_min = sdram_spd_txx_ps(ctx, WORD(LSN(spd[36]), spd[37]), 0);
    //int trrd_s_min = sdram_spd_txx_ps(ctx, spd[38], spd[119]);
    int trrd_l_min = sdram_spd_txx_ps(ctx, spd[39], spd[118]);
    int tccd_l_min = sdram_spd_txx_ps(ctx, spd[40], spd[117]);
    int twr_min = sdram_spd_txx_ps(ctx, WORD(LSN(spd[41]), spd[42]), 0);
    //int twtr_s_min = sdram_spd_txx_ps(ctx, WORD(LSN(spd[43]), spd[44]), 0);
    int twtr_l_min = sdram_spd_txx_ps(ctx, WORD(MSN(spd[43]), spd[45]), 0);

    int sdram_device_widths[] = {4, 8, 16, 32};
    int sdram_device_width = sdram_device_widths[FIELD(spd[12], 3, 0)];

    /* Calculate minimal tFAW length depending on page size */
    int page_size_bytes = (1 << ctx->geometry.colbits) * sdram_device_width / 8;
    int tfaw_min_ck = 0;
    switch (page_size_bytes) {
        case 512:
            tfaw_min_ck = 16;
            break;
        case 1024:
            tfaw_min_ck = 20;
            break;
        case 2048:
            tfaw_min_ck = 28;
            break;
        default:
            break;
    };

    struct sdram_spd_timings_s min_timings = {
        /* technology timings */
        .tck = tckavg_min,
        .trefi = {trefi_1x, trefi_1x<<1, trefi_1x<<2},
        .twtr = {4, twtr_l_min},
        .tccd = {4, tccd_l_min},
        .trrd = {4, trrd_l_min},
        .tzqcs = {128, 80},
        /* speedgrade timings */
        .trp = trp_min,
        .trcd = trcd_min,
        .twr = twr_min,
        .trfc = {trfc1_min, trfc2_min, trfc4_min},
        .tfaw = {tfaw_min_ck, tfaw_min},
        .tras = tras_min
    };

    memcpy(&(ctx->min_timings), &min_timings, sizeof(min_timings));

    return 1;
}

int sdram_spd_parse(struct sdram_spd_ctx_s *ctx, uint8_t *spd, unsigned int fine_refresh_mode) {
    if ((ctx == NULL) || (spd == NULL)) {
        return 0;
    }

    ctx->clk_period_ps = 1000000000000/CONFIG_CLOCK_FREQUENCY; /* hertz to picoseconds */
    ctx->rate_frac_num = 1;
    ctx->rate_frac_denom = SDRAM_PHY_PHASES;
    ctx->margin = ctx->clk_period_ps * (ctx->rate_frac_denom - ctx->rate_frac_num) / ctx->rate_frac_denom;
    if (fine_refresh_mode > SDRAM_SPD_FINE_REFRESH_MODE_MAX) ctx->fine_refresh_mode = SDRAM_SPD_FINE_REFRESH_MODE_MAX;
    else ctx->fine_refresh_mode = fine_refresh_mode;

    sdram_spd_parse_geometry_ddr4(ctx, spd);
    sdram_spd_parse_timebase_ddr4(ctx, spd);
    sdram_spd_parse_timings_ddr4(ctx, spd);

    return 1;
}

int sdram_spd_read_i2c(struct sdram_spd_ctx_s *ctx, unsigned int fine_refresh_mode, uint8_t spdaddr, bool send_stop)
{
    unsigned char buf[256];
	int len = sizeof(buf);

	if (spdaddr > 0b111) {
		/* SPD EEPROM max address is 0b111 (defined by A0, A1, A2 pins) */
		return 0;
	}

	if (!i2c_read(SPD_RW_ADDR(spdaddr), 0, buf, len, send_stop)) {
		/* Error when reading SPD EEPROM */
		return 0;
	}

    return sdram_spd_parse(ctx, buf, fine_refresh_mode);
}

/*-----------------------------------------------------------------------*/
/* SPD to controller timings                                             */
/*-----------------------------------------------------------------------*/

int sdram_spd_ps_to_cycles(struct sdram_spd_ctx_s *ctx, int time, bool use_margin)
{
    if (ctx == NULL) {
        return -1;
    }

    if (use_margin == true) {
        time += ctx->margin;
    }

    int cycles = time / ctx->clk_period_ps;
    int cycles_mod = time % ctx->clk_period_ps;

    if (cycles_mod > 0) cycles++;

    return cycles;
}

int sdram_spd_ck_to_cycles(struct sdram_spd_ctx_s *ctx, int ck)
{
    if (ctx == NULL) {
        return -1;
    }

    int cycles = ck / ctx->rate_frac_denom;
    int cycles_mod = ck % ctx->rate_frac_denom;

    if (cycles_mod > 0) cycles++;

    return cycles;
}

int sdram_spd_ck_ps_to_cycles(struct sdram_spd_ctx_s *ctx, int ck_ps[2], bool use_margin)
{
    if (ctx == NULL) {
        return -1;
    }

    return max(sdram_spd_ck_to_cycles(ctx, ck_ps[0]), sdram_spd_ps_to_cycles(ctx, ck_ps[1], use_margin));
}

int sdram_timings_spd(struct sdram_spd_ctx_s *ctx)
{
    if (ctx == NULL) {
        return 0;
    }

    struct sdram_spd_timings_s *min_timings = &(ctx->min_timings);

    /* Convert SPD timings to controller timings */
    struct sdram_timings_s timings = {
        .trp = sdram_spd_ps_to_cycles(ctx, min_timings->trp, true),
        .trcd = sdram_spd_ps_to_cycles(ctx, min_timings->trcd, true),
        .twr = sdram_spd_ps_to_cycles(ctx, min_timings->twr, true),
        .trefi = sdram_spd_ps_to_cycles(ctx, min_timings->trefi[ctx->fine_refresh_mode], false),
        .trfc = sdram_spd_ps_to_cycles(ctx, min_timings->trfc[ctx->fine_refresh_mode], true),
        .twtr = sdram_spd_ck_ps_to_cycles(ctx, min_timings->twtr, true),
        /* optional timings below (default to 0) */
        .tfaw = sdram_spd_ck_ps_to_cycles(ctx, min_timings->tfaw, true),
        .tccd = sdram_spd_ck_ps_to_cycles(ctx, min_timings->tccd, true),
        .trrd = sdram_spd_ck_ps_to_cycles(ctx, min_timings->trrd, true),
        .trc = sdram_spd_ps_to_cycles(ctx, (min_timings->trp) + (min_timings->tras), true),
        .tras = sdram_spd_ps_to_cycles(ctx, min_timings->tras, true),
        .tzqcs = sdram_spd_ck_ps_to_cycles(ctx, min_timings->tzqcs, true)
    };

    sdram_set_timings(&timings);

    return 1;
}
