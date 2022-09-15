.global main
.global isr
.global _start

_start:
  j crt_init
  nop
  nop
  nop
  nop
  nop
  nop
  nop

trap_entry:
  sd x31, -8(sp)
  sd x30, -16(sp)
  sd x29, -24(sp)
  sd x28, -32(sp)
  sd x27, -40(sp)
  sd x26, -48(sp)
  sd x25, -56(sp)
  sd x24, -64(sp)
  sd x23, -72(sp)
  sd x22, -80(sp)
  sd x21, -88(sp)
  sd x20, -96(sp)
  sd x19, -104(sp)
  sd x18, -112(sp)
  sd x17, -120(sp)
  sd x16, -128(sp)
  sd x15, -136(sp)
  sd x14, -144(sp)
  sd x13, -152(sp)
  sd x12, -160(sp)
  sd x11, -168(sp)
  sd x10, -176(sp)
  sd x9, -184(sp)
  sd x8, -192(sp)
  sd x7, -200(sp)
  sd x6, -208(sp)
  sd x5, -216(sp)
  sd x4, -224(sp)
  sd x3, -232(sp)
  sd x1, -248(sp)
  sd x0, -256(sp) 
  addi sp,sp,-256
  
  addi a0,sp,0
  
  call isr

  ld x0, 0(sp)
  ld x1, 8(sp)
  ld x3, 24(sp)
  ld x4, 32(sp)
  ld x5, 40(sp)
  ld x6, 48(sp)
  ld x7, 56(sp)
  ld x8, 64(sp)
  ld x9, 72(sp)
  ld x10, 80(sp)
  ld x11, 88(sp)
  ld x12, 96(sp)
  ld x13, 104(sp)
  ld x14, 112(sp)
  ld x15, 120(sp)
  ld x16, 128(sp)
  ld x17, 136(sp)
  ld x18, 144(sp)
  ld x19, 152(sp)
  ld x20, 160(sp)
  ld x21, 168(sp)
  ld x22, 176(sp)
  ld x23, 184(sp)
  ld x24, 192(sp)
  ld x25, 200(sp)
  ld x26, 208(sp)
  ld x27, 216(sp)
  ld x28, 224(sp)
  ld x29, 232(sp)
  ld x30, 240(sp)
  ld x31, 248(sp)
  addi sp,sp,256
  
  mret
  .text


crt_init:
  la sp, _fstack
  la a0, trap_entry
  csrw mtvec, a0

data_init:
  la a0, _fdata
  la a1, _edata
  la a2, _fdata_rom
data_loop:
  beq a0,a1,data_done
  ld a3,0(a2)
  sd a3,0(a0)
  add a0,a0,8
  add a2,a2,8
  j data_loop
data_done:

bss_init:
  la a0, _fbss
  la a1, _ebss
bss_loop:
  beq a0,a1,bss_done
  sd zero,0(a0)
  add a0,a0,8
  j bss_loop
bss_done:

//  call plic_init // initialize external interrupt controller
  li a0, 0x800   // external interrupt sources only (using LiteX timer);
                 // NOTE: must still enable mstatus.MIE!
  csrw mie,a0
  
  // enable FPU
  li t0, 0x2000
  csrs mstatus, t0

  call main
inf_loop:
  j inf_loop