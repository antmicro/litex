.global mul_mulh
.global mul_mulhu
.global mul_mulhsu

mul_mulh:
  slli a2, a0, 32
  srli a2, a2, 32
  slli a3, a1, 32
  srli a3, a3, 32
  mul a4, a2, a3
  srai a5, a4, 32
  srai t0, a0, 32
  mul a4, t0, a3
  add a4, a4, a5
  slli a5, a4, 32
  srli a5, a5, 32
  srai a6, a4, 32
  srai t1, a1, 32
  mul a4, a2, t1
  add a4, a4, a5
  srai a5, a4, 32
  mul t0, t0, t1
  add t0, t0, a6
  add a0, t0, a5
  jalr x0, ra

mul_mulhu:
  slli a2, a0, 32
  srli a2, a2, 32
  srli t0, a0, 32
  slli a3, a1, 32
  srli a3, a3, 32
  srli t1, a1, 32
  mul a4, a2, a3
  srli a5, a4, 32
  mul a4, t0, a3
  add a4, a4, a5
  slli a5, a4, 32
  srli a5, a5, 32
  srli a6, a4, 32
  mul a4, a2, t1
  add a4, a4, a5
  srli a5, a4, 32
  mul t0, t0, t1
  add t0, t0, a6
  add a0, t0, a5
  jalr x0, ra

mul_mulhsu:
  slli a2, a0, 32
  srli a2, a2, 32
  slli a3, a1, 32
  srli a3, a3, 32
  mul a4, a2, a3
  srai a5, a4, 32
  srai t0, a0, 32
  mul a4, t0, a3
  add a4, a4, a5
  slli a5, a4, 32
  srli a5, a5, 32
  srai a6, a4, 32
  srli t1, a1, 32
  mul a4, a2, t1
  add a4, a4, a5
  srli a5, a4, 32
  mul t0, t0, t1
  add t0, t0, a6
  add a0, t0, a5
  jalr x0, ra