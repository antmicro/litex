#ifndef __SDRAM_SPD_H
#define __SDRAM_SPD_H

#include <stdint.h>

#define MASK(nbits) ((1 << nbits) - 1)
#define FIELD(byte, nbits, shift) ( (uint8_t) (((byte & MASK(nbits)) << shift) >> shift) )
#define LSN(b) FIELD(b, 4, 0)
#define MSN(b) FIELD(b, 4, 4)
#define WORD(msb, lsb) ( (uint16_t) ((msb << 8) | lsb) )

struct sdram_spd_timings_s {
    int tck;
    int trefi[3];
    int twtr[2];
    int tccd[2];
    int trrd[2];
    int tzqcs[2];
    int trp;
    int trcd;
    int twr;
    int trfc[3];
    int tfaw[2];
    int tras;
};

struct sdram_spd_geometry_s {
    int bankbits;
    int rowbits;
    int colbits;
};

struct sdram_spd_ctx_s {
    int rate_frac_num;
    int rate_frac_denom;
    int margin;
    int speedgrade;
    int fine_refresh_mode;
    int medium_timebase_ps;
    int fine_timebase_ps;
    struct sdram_spd_geometry_s geometry;
    struct sdram_spd_timings_s min_timings;
};

int sdram_spd_parse_geometry_ddr4(struct sdram_spd_ctx_s *ctx, uint8_t *spd);
int sdram_spd_parse_timebase_ddr4(struct sdram_spd_ctx_s *ctx, uint8_t *spd);
int sdram_spd_txx_ps(struct sdram_spd_ctx_s *ctx, uint16_t mtb, int8_t ftb);
int sdram_spd_parse_timings_ddr4(struct sdram_spd_ctx_s *ctx, uint8_t *spd);
int sdram_set_spd_timings(struct sdram_spd_ctx_s *ctx);


#endif /* __SDRAM_SPD_H */
