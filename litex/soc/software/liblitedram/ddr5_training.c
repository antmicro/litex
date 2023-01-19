#include <liblitedram/ddr5_training.h>
#include <liblitedram/ddr5_helpers.h>
#include <stdio.h>
#include <inttypes.h>

#ifdef MEMORY_TYPE_DDR5
#define DEBUG_DDR5

static void cs_non_negative_tap(int channel, int rank, int offset, int32_t *left, int32_t *right, const char* format) {
    int _result, delay;
    cs_rst(channel, rank, 0);
    cs_sample_prep(channel, rank, 0, offset);
#ifdef DEBUG_DDR5
    printf(format, 0xa>>offset);
#endif // DEBUG_DDR5
    _result = 1;
    for (delay = 0; delay < SDRAM_PHY_DELAYS && (_result || *right == UNSET_DELAY); delay++) {
        _result =  or_sample(channel);
        if (_result && *right == UNSET_DELAY)
            *right = delay;
        else if (!_result && *right != UNSET_DELAY && *left == UNSET_DELAY)
            *left = delay;
        else if (_result && *left == UNSET_DELAY && delay+1 == SDRAM_PHY_DELAYS)
            *left = delay + 1;
#ifdef DEBUG_DDR5
        printf("%d", _result);
#endif // DEBUG_DDR5
        cs_inc(channel, rank, 0);
    }
}

static void cs_on_edge(int channel, int rank, int32_t *left, int32_t *right, int on_edge) {
    int _result, delay, offset;
    offset = on_edge >> 1;
    cs_rst(channel, rank, 0);
    cs_sample_prep(channel, rank, 0, (offset+1)%2);
#ifdef DEBUG_DDR5
    printf("CS right_edge|");
#endif // DEBUG_DDR5
    _result = 0;
    for (delay = 0; delay < SDRAM_PHY_DELAYS; delay++) {
        _result =  or_sample(channel);
        if (_result && *right == UNSET_DELAY)
            *right = delay + 1 - SDRAM_PHY_DELAYS;
#ifdef DEBUG_DDR5
        printf("%d", _result);
#endif // DEBUG_DDR5
        cs_inc(channel, rank, 0);
    }
    cs_non_negative_tap(channel, rank, offset, left, right, "\n left_edge|");
#ifdef DEBUG_DDR5
    printf(";%"PRId32":%"PRId32"\n", *right, *left);
#endif // DEBUG_DDR5
}

static void cs_in_middle(int channel, int rank, int32_t *left, int32_t *right) {
    int32_t temp_l, temp_r, offset;
    for (offset = 0; offset < 2; offset++) {
        temp_l = UNSET_DELAY; temp_r = UNSET_DELAY;
        cs_non_negative_tap(channel, rank, offset, &temp_l, &temp_r, "CS_pattern:0x%x|");
        if (temp_l != UNSET_DELAY && temp_r != UNSET_DELAY) {
            if (*left == UNSET_DELAY) {
                *left = temp_l; *right = temp_r;
            } else if (*left - *right < temp_l - temp_r) {
                *left = temp_l; *right = temp_r;
            }
        }
        printf("\n");
    }
#ifdef DEBUG_DDR5
    printf(";%" PRId32 ":%" PRId32 "\n", *right, *left);
#endif // DEBUG_DDR5
}

static void ca_on_edge(int channel, int rank, int address, int32_t *right) {
    int _result, delay;

    ca_rst(channel, rank, address);
#ifdef DEBUG_DDR5
    printf("CA line:%02d\nright_edge|", address);
#endif // DEBUG_DDR5
    _result = 0;
    for (delay = 0; delay < SDRAM_PHY_DELAYS; delay++) {
        ca_sample_prep_previous_period(channel, rank, address, 1);
        _result = and_sample(channel);
        ca_sample_prep_previous_period(channel, rank, address, 0);
        _result &= or_sample(channel);
        if (_result && *right == UNSET_DELAY)
            *right = delay + 1 - SDRAM_PHY_DELAYS;
#ifdef DEBUG_DDR5
        printf("%d", _result);
#endif // DEBUG_DDR5
        ca_inc(channel, rank, address);
    }
    if (*right == UNSET_DELAY)
        *right = 0;
}

static void ca_non_negative_taps(int channel, int rank, int address, int32_t *left, int32_t *right, const char* format) {
    int _result, delay;
    ca_rst(channel, rank, address);
#ifdef DEBUG_DDR5
    printf(format, address);
#endif // DEBUG_DDR5
    _result = 1;
    for (delay = 0; delay < SDRAM_PHY_DELAYS && (_result || *right == UNSET_DELAY); delay++) {
        ca_sample_prep_current_period(channel, rank, address, 1);
        _result = and_sample(channel);
        ca_sample_prep_current_period(channel, rank, address, 0);
        _result &= or_sample(channel);
        if (_result && *right == UNSET_DELAY)
            *right = delay;
        else if (!_result && *right != UNSET_DELAY && *left == UNSET_DELAY)
            *left = delay;
        else if (_result && *left == UNSET_DELAY&& delay+1 == SDRAM_PHY_DELAYS)
            *left = delay + 1;
#ifdef DEBUG_DDR5
        printf("%d", _result);
#endif // DEBUG_DDR5
        ca_inc(channel, rank, address);
    }
    printf("\n");
#ifdef DEBUG_DDR5
    printf(";%"PRId32":%"PRId32"\n", *right, *left);
#endif // DEBUG_DDR5
}

static void CS_training(int32_t channel, uint8_t *success, int debug) {
    int32_t left_side, right_side;
    int32_t rank;
    int32_t on_edge, _result, offset;
    for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
        if (debug)
            printf("Rank: %"PRId32"\n", rank);
        // Enter CS training MPC
        enter_cs(channel, rank);

        on_edge = 0;
        for (offset = 0; offset < 2; offset++) {
            cs_sample_prep(channel, rank, 0, offset);
            _result =  or_sample(channel);
            if (_result)
                on_edge = offset<<1|1;
        }

        left_side = UNSET_DELAY; right_side = UNSET_DELAY;
        if (on_edge)
            cs_on_edge(channel, rank, &left_side, &right_side, on_edge);
        else
            cs_in_middle(channel, rank, &left_side, &right_side);

        cs_rst(channel, rank, 0);

        // Exit CS training MPC
        exit_cs(channel, rank);

        mid_point_calc_and_set(success, "Rank:%d Eye width:%d ", channel, rank,
                               -1, rank, left_side, right_side, cs_inc, 1);
    }
}

static void CA_training(int32_t channel, int debug) {
    int32_t left_side, right_side;
    int32_t rank, address;
    int32_t on_edge;

    for (rank = 0; rank < SDRAM_PHY_RANKS; rank++) {
        if (debug)
            printf("Rank:%02"PRId32"\n", rank);
        // Enter CA training MPC
        enter_ca(channel, rank);

        for (address = 0; address < SDRAM_PHY_ADDRESS_LINES; address++) {
            // Reset CA delay
            ca_rst(channel, rank, address);

            // Check if address line is correct with 0 tap.
            on_edge = 0;
            ca_sample_prep_current_period(channel, rank, address, 1);
            if (and_sample(channel))
                on_edge = 1;
            ca_sample_prep_current_period(channel, rank, address, 0);
            if (or_sample(channel))
                on_edge &= 1;

            left_side = UNSET_DELAY; right_side = UNSET_DELAY;
            if (on_edge) {
                ca_on_edge(channel, rank, address, &right_side);
                ca_non_negative_taps(channel, rank, address, &left_side, &right_side, "\n left_edge|");
            } else {
                ca_non_negative_taps(channel, rank, address, &left_side, &right_side, "CA line:%02d|");
            }


            if(right_side == UNSET_DELAY) {
                _ca_results[address][0] = UNSET_DELAY;
                break;
            } else if (right_side > _ca_results[address][0]) {
                _ca_results[address][0] = right_side;
            }

            if(left_side == UNSET_DELAY) {
                _ca_results[address][1] = UNSET_DELAY;
                break;
            } else if (left_side < _ca_results[address][1]) {
                _ca_results[address][1] = left_side;
            }
        }
        // Exit CA training multiple NOPs
        exit_ca(channel, rank);
    }
}

static void CA_setup_values(int32_t channel, uint8_t *success, int debug) {
    int32_t left_side, right_side;
    int32_t address;
    for (address = 0; address < SDRAM_PHY_ADDRESS_LINES; address++) {
        right_side = _ca_results[address][0];
        left_side  = _ca_results[address][1];

        if (left_side == UNSET_DELAY || right_side == UNSET_DELAY) {
            printf("CA:%02"PRId32" Eye width:0 Failed\n", address);
            continue;
        }
        ca_rst(channel, -1, address);
        mid_point_calc_and_set(success, "CA:%02d Eye width:%d ", channel, -1,
                               address, address, left_side, right_side, ca_inc, 0);
    }
}

#if defined(SDRAM_PHY_ADDRESS_DELAY_CAPABLE)
void sdram_ddr5_cs_ca_training(void) {
    int32_t channel;
    uint8_t CS_success, CA_success;
    int debug;
    disable_2n_mode();

#ifdef DEBUG_DDR5
    debug = 1;
#else
    debug = 0;
#endif // DEBUG_DDR5

    CS_success = 1;
    CA_success = 1;
#ifdef SDRAM_PHY_SUBCHANNELS
    for (channel = 0; channel < 2; channel++) {
        printf("Subchannel:%c CS training\n", (char)('A'+channel));
#else
    {channel = 0;
        printf("CS training\n");
#endif // SDRAM_PHY_SUBCHANNELS
        CS_training(channel, &CS_success, debug);
        printf("CA training\n");
        setup_ca_results();
        CA_training(channel, debug);
        CA_setup_values(channel, &CA_success, debug);
    }
    if (!(CS_success & CA_success))
        enable_2n_mode();
    return;
}
#else
void sdram_ddr5_cs_ca_training(void) {
    printf("CS/CA training impossible\n"
           "Keeping DRAM in 2N mode\n");
}
#endif // defined(SDRAM_PHY_ADDRESS_DELAY_CAPABLE)
#endif // MEMORY_TYPE_DDR5
