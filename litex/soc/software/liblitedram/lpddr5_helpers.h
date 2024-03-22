#ifndef LIBLITEDRAM_DDR5_HELPERS_H
#define LIBLITEDRAM_DDR5_HELPERS_H

#include <stdbool.h>

#include <generated/csr.h>

#ifdef CSR_SDRAM_BASE
#include <generated/sdram_phy.h>

#ifdef SDRAM_PHY_LPDDR5

typedef struct {
    enum {
        BEFORE,
        INSIDE,
        AFTER,
    } state;
    int start;
    int center;
    int end;
} eye_t;

#define DEFAULT_EYE   { \
    .state  = BEFORE,   \
    .start  = -1,       \
    .center = -1,       \
    .end    = -1,       \
}

void enter_CK2WCK_leveling(void);
bool sample_CK2WCK_shift(void);
void exit_CK2WCK_leveling(void);

void send_mrw(uint8_t reg, uint8_t val);

void sdram_read(uint8_t bank, uint16_t row, uint8_t column);
void sdram_write(uint8_t bank, uint16_t row, uint8_t column, uint8_t value);

#endif // SDRAM_PHY_LPDDR5

#endif // CSR_SDRAM_BASE

#endif // LIBLITEDRAM_DDR5_HELPERS_H
