#include <generated/csr.h>
#ifdef CSR_SDRAM_BASE
#include <generated/sdram_phy.h>
#ifndef LIBLITEDRAM_DDR5_TRAINING_H
#define LIBLITEDRAM_DDR5_TRAINING_H

#ifdef MEMORY_TYPE_DDR5
void sdram_ddr5_cs_ca_training(void);
#endif // MEMORY_TYPE_DDR5

#endif // LIBLITEDRAM_DDR5_TRAINING_H
#endif // CSR_SDRAM_BASE
