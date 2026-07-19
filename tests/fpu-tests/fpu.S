.section .text
.globl main
.type main, @function

.macro CHECK reg, expected
  li t6, \expected
  bne \reg, t6, .Lfail
.endm

main:
  # The F extension is disabled after reset. Enable FS before touching either
  # FPRs or the floating CSR bank.
  li t0, 0x6000
  csrs mstatus, t0

#if __riscv_xlen == 64
  # Reset FPRs are deliberately not NaN boxed. A floating use must consume a
  # canonical NaN, then write its result back in boxed form.
  fadd.s f31, f0, f0
  fclass.s t1, f31
  CHECK t1, 0x200
#endif

  # FLW/FSW and the four binary arithmetic operations.
  li t0, 0x3f800000                 # 1.0
  li t1, 0x40000000                 # 2.0
  li t2, 0x40400000                 # 3.0
  fmv.w.x f1, t0
  fmv.w.x f2, t1
  fmv.w.x f3, t2
  addi sp, sp, -16
  fsw f3, 0(sp)
  flw f4, 0(sp)
  fmv.x.w t3, f4
  CHECK t3, 0x40400000
  fadd.s f5, f1, f2
  fsub.s f6, f3, f1
  fmul.s f7, f2, f3
  fdiv.s f8, f3, f2
  fmv.x.w t3, f5
  CHECK t3, 0x40400000
  fmv.x.w t3, f6
  CHECK t3, 0x40000000
  fmv.x.w t3, f7
  CHECK t3, 0x40c00000              # 6.0
  fmv.x.w t3, f8
  CHECK t3, 0x3fc00000              # 1.5
  fsqrt.s f9, f2
  fmv.x.w t3, f9
  CHECK t3, 0x3fb504f3              # sqrt(2), RNE

  # All four fused multiply-add encodings use rs3.
  fmadd.s f10, f1, f2, f3
  fmsub.s f11, f1, f2, f3
  fnmsub.s f12, f1, f2, f3
  fnmadd.s f13, f1, f2, f3
  fmv.x.w t3, f10
  CHECK t3, 0x40a00000              # 5.0
  fmv.x.w t3, f11
  CHECK t3, -1082130432             # -1.0
  fmv.x.w t3, f12
  CHECK t3, 0x3f800000              # 1.0
  fmv.x.w t3, f13
  CHECK t3, -1063256064             # -5.0

  # Sign injection, min/max, comparisons, FCLASS and FMV.
  fsgnjn.s f14, f1, f2
  fsgnjx.s f15, f14, f14
  fmin.s f16, f1, f2
  fmax.s f17, f1, f2
  fmv.x.w t3, f14
  CHECK t3, -1082130432
  fmv.x.w t3, f15
  CHECK t3, 0x3f800000
  fmv.x.w t3, f16
  CHECK t3, 0x3f800000
  fmv.x.w t3, f17
  CHECK t3, 0x40000000
  flt.s t3, f1, f2
  CHECK t3, 1
  fle.s t3, f2, f1
  CHECK t3, 0
  feq.s t3, f1, f1
  CHECK t3, 1
  li t0, 0x00000001                 # positive subnormal
  fmv.w.x f18, t0
  fclass.s t3, f18
  CHECK t3, 0x20

  # Integer conversions, all five rounding modes, and dynamic rounding.
  fcvt.w.s t3, f3, rne
  CHECK t3, 3
  fcvt.wu.s t3, f3, rtz
  CHECK t3, 3
  li t0, -7
  fcvt.s.w f19, t0, rne
  fcvt.w.s t3, f19, rne
  CHECK t3, -7
  li t0, 0x33800000                 # 2^-24, exactly half an ulp at 1.0
  fmv.w.x f20, t0
  fadd.s f21, f1, f20, rne
  fmv.x.w t3, f21
  CHECK t3, 0x3f800000
  fadd.s f21, f1, f20, rtz
  fmv.x.w t3, f21
  CHECK t3, 0x3f800000
  fadd.s f21, f1, f20, rdn
  fmv.x.w t3, f21
  CHECK t3, 0x3f800000
  li t0, 3                          # RUP
  csrw frm, t0
  fadd.s f22, f1, f20, dyn
  fmv.x.w t3, f22
  CHECK t3, 0x3f800001
  li t0, 4                          # RMM is also valid
  csrw frm, t0
  fadd.s f22, f1, f20, dyn
  fmv.x.w t3, f22
  CHECK t3, 0x3f800001
  li t0, 0
  csrw frm, t0

#if __riscv_xlen == 64
  li t0, 123
  fcvt.s.l f23, t0, rne
  fcvt.l.s t3, f23, rne
  CHECK t3, 123
  li t0, -1
  fcvt.s.lu f24, t0, rne
  fcvt.lu.s t3, f24, rtz
  CHECK t3, -1
#endif

  # Sticky NV/DZ/OF/UF/NX flags and FCSR reads/writes.
  csrw fflags, zero
  fdiv.s f25, f1, f0                # DZ (f0 is zero on RV32, NaN on RV64)
#if __riscv_xlen == 64
  fmv.w.x f0, zero
  fdiv.s f25, f1, f0
#endif
  li t0, 0x7f7fffff                 # max finite
  li t1, 0x40000000
  fmv.w.x f26, t0
  fmv.w.x f27, t1
  fmul.s f28, f26, f27              # OF | NX
  li t0, 0x00800000                 # min normal
  li t1, 0x40400000
  fmv.w.x f26, t0
  fmv.w.x f27, t1
  fdiv.s f28, f26, f27              # UF | NX
  li t0, 0x7fc00000
  fmv.w.x f29, t0
  flt.s t3, f29, f1                 # NV for ordered compare with NaN
  csrr t3, fflags
  CHECK t3, 0x1f
  csrr t3, fcsr
  CHECK t3, 0x1f

  addi sp, sp, 16
  li a0, 0
  ret

.Lfail:
  li a0, 1
  ret
