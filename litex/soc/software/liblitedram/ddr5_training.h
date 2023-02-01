#ifndef LIBLITEDRAM_DDR5_TRAINING_H
#define LIBLITEDRAM_DDR5_TRAINING_H

#include <generated/csr.h>
#ifdef CSR_SDRAM_BASE
#include <generated/sdram_phy.h>

#ifdef MEMORY_TYPE_DDR5
void sdram_ddr5_module_enumerate(void);
void sdram_ddr5_cs_ca_training(void);
void sdram_ddr5_read_training(void);
void sdram_ddr5_write_training(void);
#endif // MEMORY_TYPE_DDR5

#endif // CSR_SDRAM_BASE

#endif // LIBLITEDRAM_DDR5_TRAINING_H
