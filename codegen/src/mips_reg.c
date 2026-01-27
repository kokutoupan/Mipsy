#include "mips_reg.h"

const char *reg_to_string(MipsReg r) {
  switch (r) {
  case R_ZERO:
    return "$zero";
  case R_AT:
    return "$at";
  case R_V0:
    return "$v0";
  case R_V1:
    return "$v1";

  case R_A0:
    return "$a0";
  case R_A1:
    return "$a1";
  case R_A2:
    return "$a2";
  case R_A3:
    return "$a3";

  case R_T0:
    return "$t0";
  case R_T1:
    return "$t1";
  case R_T2:
    return "$t2";
  case R_T3:
    return "$t3";
  case R_T4:
    return "$t4";
  case R_T5:
    return "$t5";
  case R_T6:
    return "$t6";
  case R_T7:
    return "$t7";

  case R_S0:
    return "$s0";
  case R_S1:
    return "$s1";
  case R_S2:
    return "$s2";
  case R_S3:
    return "$s3";
  case R_S4:
    return "$s4";
  case R_S5:
    return "$s5";
  case R_S6:
    return "$s6";
  case R_S7:
    return "$s7";

  case R_T8:
    return "$t8";
  case R_T9:
    return "$t9";

  case R_K0:
    return "$k0";
  case R_K1:
    return "$k1";

  case R_GP:
    return "$gp";
  case R_SP:
    return "$sp";
  case R_FP:
    return "$fp";
  case R_RA:
    return "$ra";

  default:
    return "<invalid-reg>";
  }
}
