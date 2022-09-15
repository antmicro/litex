// This file is Copyright (c) 2013-2014 Sebastien Bourdeauducq <sb@m-labs.hk>
// This file is Copyright (c) 2019 Gabriel L. Somlo <gsomlo@gmail.com>
// This file is Copyright (c) 2020 Raptor Engineering, LLC <sales@raptorengineering.com>
// License: BSD


#include <generated/csr.h>
#include <generated/soc.h>
#include <irq.h>
#include <libbase/uart.h>
#include <stdio.h>
#include <inttypes.h>

#if defined(__microwatt__)
void isr(uint64_t vec);
void isr_dec(void);
#elif defined(__blackparrot__)
void isr(uint64_t*);
#else
void isr(void);
#endif

#ifdef CONFIG_CPU_HAS_INTERRUPT

#if defined(__blackparrot__) /*TODO: Update this function for BP*/ //
void isr(void)
{
  static int onetime = 0;
  if ( onetime == 0){
    printf("ISR blackparrot\n");
    printf("TRAP!!\n");
    onetime++;
  }
}
#elif defined(__rocket__) || defined(__openc906__)
#if defined(__openc906__)
#define PLIC_EXT_IRQ_BASE 16
#else
#define PLIC_EXT_IRQ_BASE 1
#endif
void plic_init(void);
void plic_init(void)
{
	int i;

	// priorities for first 8 external interrupts
	for (i = 0; i < 8; i++)
		*((unsigned int *)PLIC_BASE + PLIC_EXT_IRQ_BASE + i) = 1;
	// enable first 8 external interrupts
	*((unsigned int *)PLIC_ENABLED) = 0xff << PLIC_EXT_IRQ_BASE;
	// set priority threshold to 0 (any priority > 0 triggers interrupt)
	*((unsigned int *)PLIC_THRSHLD) = 0;
}

void isr(void)
{
	unsigned int claim;

	while ((claim = *((unsigned int *)PLIC_CLAIM))) {
		switch (claim - PLIC_EXT_IRQ_BASE) {
		case UART_INTERRUPT:
			uart_isr();
			break;
		default:
			printf("## PLIC: Unhandled claim: %d\n", claim);
			printf("# plic_enabled:    %08x\n", irq_getmask());
			printf("# plic_pending:    %08x\n", irq_pending());
			printf("# mepc:    %016lx\n", csrr(mepc));
			printf("# mcause:  %016lx\n", csrr(mcause));
			printf("# mtval:   %016lx\n", csrr(mtval));
			printf("# mie:     %016lx\n", csrr(mie));
			printf("# mip:     %016lx\n", csrr(mip));
			printf("###########################\n\n");
			break;
		}
		*((unsigned int *)PLIC_CLAIM) = claim;
	}
}
#elif defined(__cv32e40p__)

#define FIRQ_OFFSET 16
#define IRQ_MASK 0x7FFFFFFF
#define INVINST 2
#define ECALL 11
#define RISCV_TEST

void isr(void)
{
    unsigned int cause = csrr(mcause) & IRQ_MASK;

    if (csrr(mcause) & 0x80000000) {
#ifndef UART_POLLING
        if (cause == (UART_INTERRUPT+FIRQ_OFFSET)){
            uart_isr();
        }
#endif
    } else {
#ifdef RISCV_TEST
        int gp;
        asm volatile ("mv %0, gp" : "=r"(gp));
        printf("E %d\n", cause);
        if (cause == INVINST) {
            printf("Inv Instr\n");
            for(;;);
        }
        if (cause == ECALL) {
            printf("Ecall (gp: %d)\n", gp);
            csrw(mepc, csrr(mepc)+4);
        }
#endif
    }
}
#elif defined(__cv32e41p__)

#define FIRQ_OFFSET 16
#define IRQ_MASK 0x7FFFFFFF
#define INVINST 2
#define ECALL 11
#define RISCV_TEST

void isr(void)
{
    unsigned int cause = csrr(mcause) & IRQ_MASK;

    if (csrr(mcause) & 0x80000000) {
#ifndef UART_POLLING
        if (cause == (UART_INTERRUPT+FIRQ_OFFSET)){
            uart_isr();
        }
#endif
    } else {
#ifdef RISCV_TEST
        int gp;
        asm volatile ("mv %0, gp" : "=r"(gp));
        printf("E %d\n", cause);
        if (cause == INVINST) {
            printf("Inv Instr\n");
            for(;;);
        }
        if (cause == ECALL) {
            printf("Ecall (gp: %d)\n", gp);
            csrw(mepc, csrr(mepc)+4);
        }
#endif
    }
}
#elif defined(__microwatt__)

void isr(uint64_t vec)
{
	if (vec == 0x900)
		return isr_dec();

	if (vec == 0x500) {
		// Read interrupt source
		uint32_t xirr = xics_icp_readw(PPC_XICS_XIRR);
		uint32_t irq_source = xirr & 0x00ffffff;

		__attribute__((unused)) unsigned int irqs;

		// Handle IPI interrupts separately
		if (irq_source == 2) {
			// IPI interrupt
			xics_icp_writeb(PPC_XICS_MFRR, 0xff);
		}
		else {
			// External interrupt
			irqs = irq_pending() & irq_getmask();

#ifndef UART_POLLING
			if(irqs & (1 << UART_INTERRUPT))
				uart_isr();
#endif
		}

		// Clear interrupt
		xics_icp_writew(PPC_XICS_XIRR, xirr);

		return;
	}
}

void isr_dec(void)
{
	//  For now, just set DEC back to a large enough value to slow the flood of DEC-initiated timer interrupts
	mtdec(0x000000000ffffff);
}

#elif defined(__cva6__)
void plic_init(void);
void plic_init(void)
{
	int i;

	// priorities for interrupt pins 0...7
	for (i = 0; i < 8; i++)
		*((unsigned int *)PLIC_SOURCE_0 + i) = 1;
	// enable interrupt pins 0...7 (M-mode)
	*((unsigned int *)PLIC_M_ENABLE) = 0xff;
	// set priority threshold to 0 (any priority > 0 triggers interrupt)
	*((unsigned int *)PLIC_M_THRESHOLD) = 0;
}

void isr(void)
{
	unsigned int claim;

	while ((claim = *((unsigned int *)PLIC_M_CLAIM))) {
		switch (claim - 1) {
		case UART_INTERRUPT:
			uart_isr();
			break;
		default:
			printf("## PLIC: Unhandled claim: %d\n", claim);
			printf("# plic_enabled:    %08x\n", irq_getmask());
			printf("# plic_pending:    %08x\n", irq_pending());
			printf("# mepc:    %016lx\n", csrr(mepc));
			printf("# mcause:  %016lx\n", csrr(mcause));
			printf("# mtval:   %016lx\n", csrr(mtval));
			printf("# mie:     %016lx\n", csrr(mie));
			printf("# mip:     %016lx\n", csrr(mip));
			printf("###########################\n\n");
			break;
		}
		*((unsigned int *)PLIC_M_CLAIM) = claim;
	}
}

#else
void isr(void)
{
	__attribute__((unused)) unsigned int irqs;

	irqs = irq_pending() & irq_getmask();

#ifdef CSR_UART_BASE
#ifndef UART_POLLING
	if(irqs & (1 << UART_INTERRUPT))
		uart_isr();
#endif
#endif
}
#endif

#else

#if defined(__microwatt__)
void isr(uint64_t vec){};
#elif defined(__blackparrot__)

#define OPCODE(x)  ((x >> 0) & 0x7F)
#define RD(x)      ((x >> 7) & 0x1F)
#define FUNCT3(x)  ((x >> 12) & 0x7)
#define RS1(x)     ((x >> 15) & 0x1F)
#define RS2(x)     ((x >> 20) & 0x1F)

#define RISCV_INVINSTR 0x2
#define RISCV_OPCODE_MULDIV 0b0110011
#define RISCV_FUNCT3_MULH 	0b001
#define RISCV_FUNCT3_MULHU 	0b011
#define RISCV_FUNCT3_MULHSU 0b010

extern uint64_t mul_mulh(uint64_t, uint64_t);
extern uint64_t mul_mulhu(uint64_t, uint64_t);
extern uint64_t mul_mulhsu(uint64_t, uint64_t);

void isr(uint64_t* regs)
{ 
  uint64_t _mcause = csrr(mcause);
  uint32_t* _mepc  = csrr(mepc);

  uint8_t rs2_addr = RS2(*_mepc);
  uint8_t rs1_addr = RS1(*_mepc);
  uint8_t funct3 = FUNCT3(*_mepc);
  uint8_t rd_addr = RD(*_mepc);
  uint8_t opcode = OPCODE(*_mepc);
  
  uint64_t rs1_data = regs[rs1_addr];
  uint64_t rs2_data = regs[rs2_addr];

  if (_mcause == RISCV_INVINSTR && opcode == RISCV_OPCODE_MULDIV) {
	switch(funct3) {
		case RISCV_FUNCT3_MULH:
			regs[rd_addr] = mul_mulh(rs1_data, rs2_data);
			break;
		case RISCV_FUNCT3_MULHU:
			regs[rd_addr] = mul_mulhu(rs1_data, rs2_data);
			break;
		case RISCV_FUNCT3_MULHSU:
			regs[rd_addr] = mul_mulhsu(rs1_data, rs2_data);
			break;
		default:
			return;
	}
	csrw(mepc, csrr(mepc)+4);
  }
}
#else
void isr(void){};
#endif

#endif
