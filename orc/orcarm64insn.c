#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include <orc/orcarminsn.h>
#include <orc/orcarm64insn.h>
#include <orc/orcinternal.h>
#include <orc/orcutils-private.h>


#define ARM64_MAX_OP_LEN 64

/** data processing instructions: Arithmetic
 *
 * Immediate
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |b|op |1 0 0 0 1|sft|          imm12        |    Rn   |    Rd   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Shifted register
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |b|op |0 1 0 1 1|sft|0|    Rm   |   imm6    |    Rn   |    Rd   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Extended register
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |b|op |0 1 0 1 1|0 0|1|    Rm   | opt |imm3 |    Rn   |    Rd   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

#define arm64_code_arith_imm(b,op,sft,imm,Rn,Rd) (0x11000000 | \
    ((((b)==64)&0x1)<<31) | (((op)&0x3)<<29) | (((sft)&0x3)<<22) | \
    (((imm)&0xfff)<<10) | (((Rn)&0x1f)<<5)  | ((Rd)&0x1f))

#define arm64_code_arith_reg(b,op,sft,Rm,imm,Rn,Rd) (0x0b000000 | \
    ((((b)==64)&0x1)<<31) | (((op)&0x3)<<29) | (((sft)&0x3)<<22) | \
    (((Rm)&0x1f)<<16) | (((imm)&0x3f)<<10) | (((Rn)&0x1f)<<5)  | ((Rd)&0x1f))

#define arm64_code_arith_ext(b,op,Rm,opt,imm,Rn,Rd) (0x0b200000 | \
    ((((b)==64)&0x1)<<31) | (((op)&0x3)<<29) | (((Rm)&0x1f)<<16) | \
    (((opt)&0x7)<<13) | (((imm)&0x7)<<10) | (((Rn)&0x1f)<<5) | ((Rd)&0x1f))

void
orc_arm64_emit_am (OrcCompiler *p, OrcArm64RegBits bits, OrcArm64DP opcode,
    OrcArm64Type type, int opt, int Rd, int Rn, int Rm, orc_uint64 val)
{
  orc_uint32 code;
  orc_uint32 imm;

  int shift, extend;

  static const char *insn_names[] = {
    "add", "adds", "sub", "subs",
  };
  static const char *insn_alias[] = {
    "ERROR", "cmn", "ERROR", "cmp",
  };
  static const char *shift_names[] = {
    "lsl", "lsr", "asr", "ror"
  };
  static const char *extend_names[] = {
    "uxtb", "uxth", "uxtw", "uxtx",
    "sxtb", "sxth", "sxtw", "sxtx"
  };

  int alias_rd;
  char opt_rm[ARM64_MAX_OP_LEN];

  opcode -= ORC_ARM64_DP_ADD;

  if (opcode >= sizeof(insn_names)/sizeof(insn_names[0])) {
    ORC_COMPILER_ERROR(p, "unsupported opcode %d", opcode);
    return;
  }

  /** if a reg is not specified, it's regarded as alias; set it to SP (== 0b11111) */
  alias_rd = 0;
  if (Rd == 0) {
    Rd = ORC_ARM64_SP;
    alias_rd = 1;
  }

  memset (opt_rm, '\x00', ARM64_MAX_OP_LEN);

  switch (type) {
    case ORC_ARM64_TYPE_IMM:      /** immediate */
      /**, #imm */
      shift = 0;
      imm = (orc_uint32) val;

      /** Support up to 24-bit immediate value, explicitly shifted left (LSL) by 0 or 12 */
      if (val > 0xfff) {
        if (val > 0xffffff) {
          ORC_COMPILER_ERROR(p, "imm is out-of-range %llx", val);
          return;
        }
        if (val & 0xfff) {
          ORC_WARNING("offset is truncated %llx", val);
        }
        imm >>= 12;
        shift = 1;
      }

      if (shift)
        snprintf (opt_rm, ARM64_MAX_OP_LEN, ", #%u, lsl #12", imm);
      else
        snprintf (opt_rm, ARM64_MAX_OP_LEN, ", #%u", imm);

      code = arm64_code_arith_imm (bits, opcode, shift, imm, Rn, Rd);
      break;
    case ORC_ARM64_TYPE_REG:  /** shifted register */
      /**, <Rm>, shift #amount */
      shift = opt;
      imm = (orc_uint32) val;

      if (shift >= sizeof(shift_names)/sizeof(shift_names[0])) {
        ORC_COMPILER_ERROR(p, "unsupported shift %d", shift);
        return;
      }

      if (val > 0) {
        if (val > 63) {
          ORC_COMPILER_ERROR(p, "shift is out-of-range %llx", val);
          return;
        }

        snprintf (opt_rm, ARM64_MAX_OP_LEN, ", %s, %s #%u",
            orc_arm64_reg_name (Rm, bits), shift_names[shift], imm);
      } else
        snprintf (opt_rm, ARM64_MAX_OP_LEN, ", %s", orc_arm64_reg_name (Rm, bits));

      code = arm64_code_arith_reg (bits, opcode, shift, Rm, imm, Rn, Rd);
      break;
    case ORC_ARM64_TYPE_EXT:  /** extended register */
      /**, <Rm>, extend #amount */
      extend = opt;
      imm = (orc_uint32) val;

      if (extend >= sizeof(extend_names)/sizeof(extend_names[0])) {
        ORC_COMPILER_ERROR(p, "unsupported extend %d", extend);
        return;
      }

      if (val > 0) {
        if (val > 4) {
          ORC_COMPILER_ERROR(p, "shift is out-of-range %llx\n", val);
          return;
        }
        /** its width is determined by extend; '0bx11' ==> 64-bit reg */
        snprintf (opt_rm, ARM64_MAX_OP_LEN, ", %s, %s #%u",
            orc_arm64_reg_name (Rm, extend & 0x3 ? ORC_ARM64_REG_64 : ORC_ARM64_REG_32),
            extend_names[extend], imm);
      } else
        snprintf (opt_rm, ARM64_MAX_OP_LEN, ", %s", orc_arm64_reg_name (Rm, bits));

      code = arm64_code_arith_ext(bits, opcode, Rm, extend, imm, Rn, Rd);
      break;
    default:
      ORC_COMPILER_ERROR(p, "unknown data processing type %d", type);
      return;
  }

  /** it's preferred to use alias names if exists */
  if (alias_rd) {
    ORC_ASM_CODE(p, "  %s %s%s\n", insn_alias[opcode],
        orc_arm64_reg_name(Rn, bits), opt_rm);
  } else {
    ORC_ASM_CODE(p, "  %s %s, %s%s\n", insn_names[opcode],
        orc_arm64_reg_name(Rd, bits), orc_arm64_reg_name(Rn, bits), opt_rm);
  }

  orc_arm_emit (p, code);
}

/** data processing instructions: Logical
 *
 * Immediate
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |b|op |1 0 0 1 0 0|         imm13           |    Rn   |    Rd   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Shifted register
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |b|op |0 1 0 1 0|sft|0|    Rm   |   imm6    |    Rn   |    Rd   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

#define arm64_code_logical_imm(b,op,imm,Rn,Rd) (0x12000000 | \
    ((((b)==64)&0x1)<<31) | (((op)&0x3)<<29) | (((imm)&0x1fff)<<10) | \
    (((Rn)&0x1f)<<5)  | ((Rd)&0x1f))

#define arm64_code_logical_reg(b,op,sft,Rm,imm,Rn,Rd) (0x0a000000 | \
    ((((b)==64)&0x1)<<31) | (((op)&0x3)<<29) | (((sft)&0x3)<<22) | \
    (((Rm)&0x1f)<<16) | (((imm)&0x3f)<<10) | (((Rn)&0x1f)<<5)  | ((Rd)&0x1f))

/**
 * check if it's a non-empty sequence of ones starting at the LSB with the remainder zero
 * e.g., mask_ones(0x0000FFFF) == true
 */
#define mask_ones(val) ((val) && (((val) + 1) & (val)) == 0)
/**
 * check if it contains a non-empty sequence of ones with the remainder zero
 * e.g., mask_shifted_ones(0x0000FF00) == true
 */
#define mask_shifted_ones(val) ((val) && mask_ones(((val) - 1) | (val)))

#if defined(__GNUC__)
#define count_trailing_zeros(val) __builtin_ctzll(val)
#define count_leading_zeros(val) __builtin_clzll(val)
#else
static int
count_trailing_zeros (orc_uint64 val)
{
  int i, count = 0;

  for (i = 0; i < 64; i++) {
    if ((val >> i) & 1)
      break;
    count++;
  }

  return count;
}
static int
count_leading_zeros (orc_uint64 val)
{
  int i, count = 0;

  for (i = 63; i >= 0; i--) {
    if ((val >> i) & 1)
      break;
    count++;
  }

  return count;
}
#endif
#define count_trailing_ones(val) count_trailing_zeros(~val)
#define count_leading_ones(val) count_leading_zeros(~val)

/**
 * Encode a logical immediate value (code reference: LLVM)
 * 32-bit variant: immr:imms
 * 64-bit variant: N:immr:imms
 */
static int
encode_logical_imm (unsigned int size, orc_uint64 val, orc_uint32 *encoded)
{
  orc_uint64 mask;
  orc_uint32 I, CTO, CLO;
  orc_uint32 immr, imms, N;

  if (size > 64)
    return -1;

  /**
   * immediate values of all-zero and all-cones are not encoded.
   * Requires at least one non-zero bit, and one zero bit.
   */
  if (val == 0ULL || val == ~0ULL ||
      (size != 64 && (val >> size != 0 || val == (~0ULL >> (64 - size)))))
    return -2;

  /** decide the element size (i.e., 2, 4, 8, 16, 32 or 64 bits) */
  do {
    size /= 2;
    mask = (1ULL << size) - 1;
    if ((val & mask) != ((val >> size) & mask)) {
      size *= 2;
      break;
    }
  } while (size > 2);

  /** decide the rotations to make the element be: 0^m 1^n */
  mask = ((orc_uint64)~0ULL) >> (64 - size);
  val &= mask;

  if (mask_shifted_ones (val)) {
    I = count_trailing_zeros (val);
    CTO = count_trailing_ones (val >> I);
  } else {
    val |= ~mask;
    if (!mask_shifted_ones (~val))
      return 0;
    CLO = count_leading_ones (val);
    I = 64 - CLO;
    CTO = CLO + count_trailing_ones (val) - (64 - size);
  }

  /** encode N:immr:imms */
  immr = (size - I) & (size - 1);
  imms = ~(size-1) << 1;
  imms |= (CTO-1);
  N = ((imms >> 6) & 1) ^ 1;

  *encoded = (N << 12) | (immr << 6) | (imms & 0x3f);

  return 0;
}

void
orc_arm64_emit_lg (OrcCompiler *p, OrcArm64RegBits bits, OrcArm64DP opcode,
    OrcArm64Type type, int opt, int Rd, int Rn, int Rm, orc_uint64 val)
{
  orc_uint32 code;
  orc_uint32 imm = 0xffffffff;

  int shift, ret;

  static const char *insn_names[] = {
    "and", "orr", "eor", "ands"
  };
  static const char *insn_alias[] = {
    "ERROR", "mov", "ERROR", "tst"
  };
  static const char *shift_names[] = {
    "lsl", "lsr", "asr", "ror"
  };

  int alias_rd, alias_rn;
  char opt_rm[ARM64_MAX_OP_LEN];

  opcode -= ORC_ARM64_DP_AND;

  if (opcode >= sizeof(insn_names)/sizeof(insn_names[0])) {
    ORC_COMPILER_ERROR(p, "unsupported opcode %d", opcode);
    return;
  }

  /** if a reg is not specified, it's regarded as alias; set it to SP (== 0b11111) */
  alias_rd = alias_rn = 0;
  if (Rd == 0) {
    Rd = ORC_ARM64_SP;
    alias_rd = 1;
  }
  if (Rn == 0) {
    Rn = ORC_ARM64_SP;
    alias_rn = 1;
  }

  memset (opt_rm, '\x00', ARM64_MAX_OP_LEN);

  switch (type) {
    case ORC_ARM64_TYPE_IMM:      /** immediate */
      /**, #imm */
      if (val == 0) {
        ORC_COMPILER_ERROR(p, "zero imm is not supported");
        return;
      }

      ret = encode_logical_imm (bits, val, &imm);
      if (ret) {
        ORC_COMPILER_ERROR(p, "wrong immediate value %llx", val);
        return;
      }

      snprintf (opt_rm, ARM64_MAX_OP_LEN, ", #0x%08x", (orc_uint32) val);

      code = arm64_code_logical_imm (bits, opcode, imm, Rn, Rd);
      break;
    case ORC_ARM64_TYPE_REG:      /** shifted register */
      /**, <Rm>, shift #amount */
      shift = opt;
      imm = (orc_uint32) val;

      if (shift >= sizeof(shift_names)/sizeof(shift_names[0])) {
        ORC_COMPILER_ERROR(p, "unsupported shift %d", shift);
        return;
      }

      if (val > 0) {
        if (val > 63) {
          ORC_COMPILER_ERROR(p, "shift is out-of-range %llx", val);
          return;
        }

        snprintf (opt_rm, ARM64_MAX_OP_LEN, ", %s, %s #%u",
            orc_arm64_reg_name (Rm, bits), shift_names[shift], imm);
      } else
        snprintf (opt_rm, ARM64_MAX_OP_LEN, ", %s", orc_arm64_reg_name (Rm, bits));

      code = arm64_code_logical_reg (bits, opcode, shift, Rm, imm, Rn, Rd);
      break;
    default:
      ORC_COMPILER_ERROR(p, "unknown data processing type %d", type);
      return;
  }

  /** it's preferred to use alias names if exists */
  if (alias_rd) {
    ORC_ASM_CODE(p, "  %s %s%s\n", insn_alias[opcode],
        orc_arm64_reg_name(Rn, bits), opt_rm);
  } else if (alias_rn) {
    ORC_ASM_CODE(p, "  %s %s%s\n", insn_alias[opcode],
        orc_arm64_reg_name(Rd, bits), opt_rm);
  } else {
    ORC_ASM_CODE(p, "  %s %s, %s%s\n", insn_names[opcode],
        orc_arm64_reg_name(Rd, bits), orc_arm64_reg_name(Rn, bits), opt_rm);
  }

  orc_arm_emit (p, code);
}

/** data processing instructions: move wide
 *
 * General formats
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |b|op |1 0 0 1 0 1|hw |              imm16            |    Rd   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

#define arm64_code_mov_wide(b,op,hw,imm,Rd) (0x12800000 | ((((b)==64)&0x1)<<31) | \
    (((op)&0x3)<<29) | (((hw)&0x3)<<21) | (((imm)&0xffff)<<5) | ((Rd)&0x1f))

void
orc_arm64_emit_mov_wide (OrcCompiler *p, OrcArm64RegBits bits, int mov_op, int hw,
    int Rd, orc_uint64 val)
{
  orc_uint16 imm;
  orc_uint32 code;

  static const char *mov_names[] = {
    "movn", "ERROR", "movz", "movk"
  };
  char opt_lsl[ARM64_MAX_OP_LEN];

  if (mov_op >= sizeof(mov_names)/sizeof(mov_names[0])) {
    ORC_COMPILER_ERROR(p, "unsupported mov opcode %d", mov_op);
    return;
  }

  if (val > 65535) {
    ORC_COMPILER_ERROR(p, "unsupported amount of shift %llu", val);
    return;
  }

  if (bits == ORC_ARM64_REG_64) {
    if (!(hw == 0 || hw == 16 || hw == 32 || hw == 48)) {
      ORC_COMPILER_ERROR(p, "unsupported hw shift %d", hw);
      return;
    }
  } else {
    if (!(hw == 0 || hw == 16)) {
      ORC_COMPILER_ERROR(p, "unsupported hw shift %d", hw);
      return;
    }
  }

  memset (opt_lsl, '\x00', ARM64_MAX_OP_LEN);
  if (hw > 0) {
    snprintf (opt_lsl, ARM64_MAX_OP_LEN, ", lsl #%d", hw);
  }

  hw /= 16;
  imm = val;
  code = arm64_code_mov_wide(bits, mov_op, hw, imm, Rd);

  ORC_ASM_CODE(p, "  %s %s, #%u%s\n",
      /** movz has one alias condition */
      (mov_op == 2) && !(imm == 0 && hw != 0) ? "mov" : mov_names[mov_op],
      orc_arm64_reg_name(Rd, bits),
      imm, opt_lsl);

  orc_arm_emit (p, code);
}

/** data processing instructions: Shift (reg)
 *
 * General formats
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |b|0 0|1 1 0 1 0 1 1 0|    Rm   |0 0 1 0|sft|    Rn   |    Rd   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

#define arm64_code_shift(b,Rm,sft,Rn,Rd) (0x1ac02000 | ((((b)==64)&0x1)<<31) | \
    (((Rm)&0x1f)<<16) | (((sft)&0x3)<<10) | (((Rn)&0x1f)<<5)  | ((Rd)&0x1f))

void
orc_arm64_emit_sft (OrcCompiler *p, OrcArm64RegBits bits, OrcArmShift shift,
    int Rd, int Rn, int Rm)
{
  orc_uint32 code;

  static const char *shift_names[] = {
    "lsl", "lsr", "asr", "ror"
  };

  if (shift >= sizeof(shift_names)/sizeof(shift_names[0])) {
    ORC_COMPILER_ERROR(p, "unsupported shift %d", shift);
    return;
  }

  code = arm64_code_shift(bits, Rm, shift, Rn, Rd);

  ORC_ASM_CODE(p, "  %s %s, %s, %s\n",
      shift_names[shift],
      orc_arm64_reg_name(Rd, bits),
      orc_arm64_reg_name(Rn, bits),
      orc_arm64_reg_name(Rm, bits));

  orc_arm_emit (p, code);
}

/** data processing instructions: Bitfield Move
 *
 * General formats
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |b|op |1 0 0 1 1 0|N|    immr   |    imms   |    Rn   |    Rd   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Note that Bitfiled Move is usually accessed via one of its aliases
 */

#define arm64_code_bfm(b,opcode,immr,imms,Rn,Rd) (0x13000000 | ((((b)==64)&0x1)<<31) | \
    (((opcode)&0x3)<<29) | ((((b)==64)&0x1)<<22) | (((immr)&0x3f)<<16) | (((imms)&0x3f)<<10) | \
    (((Rn)&0x1f)<<5) | ((Rd)&0x1f))

/** Return 1 if ubfx or sbfx is preferred. Must exclude more specific aliases */
static int bfx_preferred (OrcArm64RegBits bits, int is_unsigned,
    orc_uint32 imms, orc_uint32 immr)
{
  if (imms < immr)
    return 0;

  /** must not match LSR/ASR/LSL alias */
  if (imms == 0x1f)
    return 0;

  /** must not match UXTx/SXTx alias */
  if (immr == 0) {
    if (bits == ORC_ARM64_REG_32) {
      if (imms == 0x7 || imms == 0xf)
        return 0;
    } else if (is_unsigned) {
      if (imms == 0x7 || imms == 0xf || imms == 0x1f)
        return 0;
    }
  }

  return 1;
}

void
orc_arm64_emit_bfm (OrcCompiler *p, OrcArm64RegBits bits, OrcArm64DP opcode,
    int Rd, int Rn, orc_uint32 immr, orc_uint32 imms)
{
  orc_uint32 code;
  int alias = -1;

  static const char *insn_names[3] = {
    "sbfm", "bfm", "ubfm"
  };
  static const char *insn_alias[3][6] = {
    {"asr","sbfiz","sbfx","sxtb","sxth","sxtw"},
    {"bfc","bfi","bfxil", "ERROR","ERROR","ERROR"},
    {"lsl","lsr","ubfiz","ubfx","uxtb","uxth"}
  };

  char opt_immr[ARM64_MAX_OP_LEN];
  char opt_imms[ARM64_MAX_OP_LEN];

  memset (opt_immr, '\x00', ARM64_MAX_OP_LEN);
  memset (opt_imms, '\x00', ARM64_MAX_OP_LEN);

  /** find its alias */
  switch (opcode) {
    case ORC_ARM64_DP_SBFM:
      if ((imms == 0x1f && bits == ORC_ARM64_REG_32) ||
          (imms == 0x3f && bits == ORC_ARM64_REG_64)) {
        snprintf (opt_immr, ARM64_MAX_OP_LEN, ", #%u", immr);
        alias = 0;
      } else if (imms < immr) {
        snprintf (opt_immr, ARM64_MAX_OP_LEN, ", #%u", immr);
        snprintf (opt_imms, ARM64_MAX_OP_LEN, ", #%u", imms);
        alias = 1;
      } else if (bfx_preferred(bits, 0, imms, immr)) {
        snprintf (opt_immr, ARM64_MAX_OP_LEN, ", #%u", immr);
        snprintf (opt_imms, ARM64_MAX_OP_LEN, ", #%u", imms);
        alias = 2;
      } else if (immr == 0) {
        if (imms == 0x7)
          alias = 3;
        else if (imms == 0xf)
          alias = 4;
        else if (imms == 0x1f)
          alias = 5;
      }
      break;
    case ORC_ARM64_DP_BFM:
      if (imms < immr) {
        if (Rn == 0x1f) {
          snprintf (opt_immr, ARM64_MAX_OP_LEN, ", #%u", immr);
          alias = 0;
        } else {
          snprintf (opt_immr, ARM64_MAX_OP_LEN, ", #%u", immr);
          snprintf (opt_imms, ARM64_MAX_OP_LEN, ", #%u", imms);
          alias = 1;
        }
      } else {
        snprintf (opt_immr, ARM64_MAX_OP_LEN, ", #%u", immr);
        snprintf (opt_imms, ARM64_MAX_OP_LEN, ", #%u", imms);
        alias = 2;
      }
      break;
    case ORC_ARM64_DP_UBFM:
      if ((imms == 0x1f && bits == ORC_ARM64_REG_32) ||
          (imms == 0x3f && bits == ORC_ARM64_REG_64)) {
        snprintf (opt_immr, ARM64_MAX_OP_LEN, ", #%u", immr);
        alias = 1;
      } else if (imms + 1 == immr) {
        snprintf (opt_immr, ARM64_MAX_OP_LEN, ", #%u", immr);
        alias = 0;
      } else if (imms < immr) {
        snprintf (opt_immr, ARM64_MAX_OP_LEN, ", #%u", immr);
        snprintf (opt_imms, ARM64_MAX_OP_LEN, ", #%u", imms);
        alias = 2;
      } else if (bfx_preferred(bits, 1, imms, immr)) {
        snprintf (opt_immr, ARM64_MAX_OP_LEN, ", #%u", immr);
        snprintf (opt_imms, ARM64_MAX_OP_LEN, ", #%u", imms);
        alias = 3;
      } else if (immr == 0) {
        if (imms == 0x7)
          alias = 4;
        else if (imms == 0xf)
          alias = 5;
      }
      break;
    default:
      ORC_COMPILER_ERROR(p, "unknown opcode %d", opcode);
      return;
  }

  opcode -= ORC_ARM64_DP_SBFM;

  code = arm64_code_bfm(bits, opcode, immr, imms, Rn, Rd);

  if (alias != -1) {
    ORC_ASM_CODE(p, "  %s %s, %s%s%s\n",
        insn_alias[opcode][alias],
        orc_arm64_reg_name(Rd, bits),
        orc_arm64_reg_name(Rn, bits),
        opt_immr, opt_imms);
  } else {
    ORC_ASM_CODE(p, "  %s %s, %s, #%u, #%u\n",
        insn_names[opcode],
        orc_arm64_reg_name(Rd, bits),
        orc_arm64_reg_name(Rn, bits),
        immr, imms);
  }

  orc_arm_emit (p, code);
}

/** data processing instructions: Extract, alias of ROR (imm)
 *
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |b|0 0|1 0 0 1 1 1|N|0|    Rm   |    imms   |    Rn   |    Rd   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

#define arm64_code_extr(b,Rm,imms,Rn,Rd) (0x13800000 | ((((b)==64)&0x1)<<31) | \
    ((((b)==64)&0x1)<<22) | (((Rm)&0x1f)<<16) | (((imms)&0x3f)<<10) | \
    (((Rn)&0x1f)<<5) | ((Rd)&0x1f))

void
orc_arm64_emit_extr (OrcCompiler *p, OrcArm64RegBits bits,
    int Rd, int Rn, int Rm, orc_uint32 imm)
{
  orc_uint32 code;

  code = arm64_code_extr (bits, Rm, imm, Rn, Rd);

  if (Rn == Rm) { /** ROR (imm) */
    if (bits == ORC_ARM64_REG_32 && (imm & 0x20)) {
      ORC_COMPILER_ERROR(p, "invalid immediate value 0x%08x", imm);
      return;
    }

    ORC_ASM_CODE(p, "  ror %s, %s, #%u\n",
      orc_arm64_reg_name(Rd, bits),
      orc_arm64_reg_name(Rn, bits),
      imm);
  } else {
    ORC_ASM_CODE(p, "  extr %s, %s, %s, #%u\n",
      orc_arm64_reg_name(Rd, bits),
      orc_arm64_reg_name(Rn, bits),
      orc_arm64_reg_name(Rm, bits),
      imm);
  }

  orc_arm_emit (p, code);
}

/** memory access instructions
 *
 * Literal (LDR only)
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |0 b|0 1 1|0|0 0|                imm19                |    Rt   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Register Signed offset (Post-/Pre-index)
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |1 b|1 1 1|0|0 0|op |0|       imm9      |P 1|    Rn   |    Rt   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Register Unsigned offset
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |1 b|1 1 1|0|0 1|op |         imm12         |   Rn    |    Rt   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Register Extended
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |1 b|1 1 1|0|0 0|op |1|    Rm   | opt |S|1 0|    Rn   |    Rt   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

#define arm64_code_mem_literal(b,imm,Rt) (0x18000000 | ((((b)==64)&0x1)<<30) | \
    (((imm)&0x7ffff)<<5) | ((Rt)&0x1f))

#define arm64_code_mem_signed(b,op,imm,P,Rn,Rt) (0xb8000400 | ((((b)==64)&0x1)<<30) | \
    (((op)&0x3)<<22) | (((imm)&0x3f)<<12) | (((P)&0x1)<<11) | (((Rn)&0x1f)<<5) | ((Rt)&0x1f))

#define arm64_code_mem_unsigned(b,op,imm,Rn,Rt) (0xb9000000 | ((((b)==64)&0x1)<<30) | \
    (((op)&0x3)<<22) | (((imm)&0xfff)<<10) | (((Rn)&0x1f)<<5) | ((Rt)&0x1f))

#define arm64_code_mem_extended(b,op,Rm,opt,S,Rn,Rt) (0xb8200800 | ((((b)==64)&0x1)<<30) | \
    (((op)&0x3)<<22) | (((Rm)&0x1f)<<16) | (((opt)&0x7)<<13) | (((S)&0x1)<<12) | \
    (((Rn)&0x1f)<<5) | ((Rt)&0x1f))

void
orc_arm64_emit_mem (OrcCompiler *p, OrcArm64RegBits bits, OrcArm64Mem opcode,
    OrcArm64Type type, int opt, int Rt, int Rn, int Rm, orc_uint32 val)
{
  orc_uint32 code;
  orc_uint32 amount;
  orc_int32 imm;

  int is_signed, extend, S;

  static const char *insn_names[3] = {
    "str", "ldr"
  };
  static const char *extend_names[] = {
    "ERROR", "ERROR", "uxtw", "lsl",
    "ERROR", "ERROR", "sxtw", "sxtx"
  };

  char opt_rn[ARM64_MAX_OP_LEN];
  char opt_rm[ARM64_MAX_OP_LEN];

  if (opcode >= sizeof(insn_names)/sizeof(insn_names[0])) {
    ORC_COMPILER_ERROR(p, "unsupported opcode %d", opcode);
    return;
  }

  memset (opt_rn, '\x00', ARM64_MAX_OP_LEN);
  memset (opt_rm, '\x00', ARM64_MAX_OP_LEN);

  switch (type) {
    case ORC_ARM64_TYPE_IMM:
      if (opcode != ORC_ARM64_MEM_LDR) {
        ORC_COMPILER_ERROR(p, "str with immediate is not permitted\n");
        return;
      }

      if (bits == ORC_ARM64_REG_64) {
        imm = val/8;
      } else {
        imm = val/4;
      }

      if (imm > 4095) {
        ORC_COMPILER_ERROR(p, "out-of-range immediate 0x%lx\n", val);
        return;
      }

      if (imm == 0) {
        int label = opt;
        /** resolve the actual address diff in fixup code */
        orc_arm_add_fixup (p, label, 2);
        snprintf (opt_rn, ARM64_MAX_OP_LEN, ", .L%d", label);
      } else {
        snprintf (opt_rn, ARM64_MAX_OP_LEN, ", 0x%08x", val);
      }

      code = arm64_code_mem_literal (bits, imm, Rt);
      break;
    case ORC_ARM64_TYPE_REG:
      imm = val;
      is_signed = opt;

      if (is_signed) { /** signed offset */
        if (imm > 0xff || imm < -0xff) {
          ORC_COMPILER_ERROR(p, "simm is out-of-range %x\n", imm);
          return;
        }

        if (is_signed == 1) { /** pre index */
          snprintf (opt_rn, ARM64_MAX_OP_LEN, ", [%s", orc_arm64_reg_name(Rn, bits));
          snprintf (opt_rm, ARM64_MAX_OP_LEN, ", #%d]!", imm);

          code = arm64_code_mem_signed (bits, opcode, imm, 1, Rn, Rt);
        } else {              /** post index */
          snprintf (opt_rn, ARM64_MAX_OP_LEN, ", [%s]", orc_arm64_reg_name(Rn, bits));
          snprintf (opt_rm, ARM64_MAX_OP_LEN, ", #%d", imm);

          code = arm64_code_mem_signed (bits, opcode, imm, 0, Rn, Rt);
        }
      } else {  /** unsigned offset */
        if (imm != 0) {
          snprintf (opt_rn, ARM64_MAX_OP_LEN, ", [%s", orc_arm64_reg_name(Rn, bits));
          snprintf (opt_rm, ARM64_MAX_OP_LEN, ", #%d]", imm);
        } else {
          snprintf (opt_rn, ARM64_MAX_OP_LEN, ", [%s]", orc_arm64_reg_name(Rn, bits));
        }

        if (bits == ORC_ARM64_REG_64) {
          if (imm < 0 || imm > 0xfff * 8)
            ORC_COMPILER_ERROR(p, "imm is Out-of-range %x\n", imm);
          imm /= 8;
        } else {
          if (imm < 0 || imm > 0xfff * 4)
            ORC_COMPILER_ERROR(p, "imm is Out-of-range %x\n", imm);
          imm /= 4;
        }

        code = arm64_code_mem_unsigned (bits, opcode, imm, Rn, Rt);
      }
      break;
    case ORC_ARM64_TYPE_EXT:
      extend = opt;

      if ((extend >= sizeof(extend_names)/sizeof(extend_names[0])) ||
          !strncmp (extend_names[extend], "ERROR", 5)) {
        ORC_COMPILER_ERROR(p, "unsupported extend %d\n", extend);
        return;
      }

      snprintf (opt_rn, ARM64_MAX_OP_LEN, ", [%s", orc_arm64_reg_name(Rn, bits));

      amount = val;
      if (amount > 0) {
        if (bits == ORC_ARM64_REG_64 && amount != 3) {
          ORC_COMPILER_ERROR(p, "unsupported amount %d\n", amount);
          return;
        }
        if (bits == ORC_ARM64_REG_32 && amount != 2) {
          ORC_COMPILER_ERROR(p, "unsupported amount %d\n", amount);
          return;
        }
        snprintf (opt_rm, ARM64_MAX_OP_LEN, ", %s, %s #%u]",
            orc_arm64_reg_name(Rm, bits), extend_names[extend], amount);

        S = 1;
      } else {
        if (extend == 3) {   /** LSL */
          snprintf (opt_rm, ARM64_MAX_OP_LEN, ", %s]", orc_arm64_reg_name(Rm, bits));
        } else {
          snprintf (opt_rm, ARM64_MAX_OP_LEN, ", %s, %s]",
              orc_arm64_reg_name(Rm, bits), extend_names[extend]);
        }

        S = 0;
      }

      code = arm64_code_mem_extended (bits, opcode, Rm, extend, S, Rn, Rt);
      break;
    default:
      ORC_COMPILER_ERROR(p, "unsupported type %d\n", type);
      return;
  }

  ORC_ASM_CODE(p, "  %s %s%s%s\n",
      insn_names[opcode],
      orc_arm64_reg_name(Rt, bits),
      opt_rn, opt_rm);

  orc_arm_emit (p, code);
}

/** memory access instructions (pair)
 *
 *    3                   2                   1
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |b 0|1 0 1|0| opt |L|     imm7    |   Rt2   |    Rn   |    Rt   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

#define arm64_code_mem_pair(b,opt,L,imm,Rt2,Rn,Rt) (0x40000000 | ((((b)==64)&0x1)<<31) | \
    (((opt)&0x7)<<23) | (((L)&0x1)<<22) | (((imm)&0x7f)<<15) | (((Rt2)&0x1f)<<10) | \
    (((Rn)&0x1f)<<5) | ((Rt)&0x1f))

void
orc_arm64_emit_mem_pair (OrcCompiler *p, OrcArm64RegBits bits, OrcArm64Mem opcode,
    int opt, int Rt, int Rt2, int Rn, orc_int32 imm)
{
  orc_uint32 code;

  static const char *insn_names[3] = {
    "stp", "ldp"
  };

  char opt_rn[ARM64_MAX_OP_LEN];

  if (opcode >= sizeof(insn_names)/sizeof(insn_names[0])) {
    ORC_COMPILER_ERROR(p, "unsupported opcode %d", opcode);
    return;
  }

  memset (opt_rn, '\x00', ARM64_MAX_OP_LEN);

  switch (opt) {
    case 1: /** post-index */
      snprintf (opt_rn, ARM64_MAX_OP_LEN, ", [%s], #%d", orc_arm64_reg_name(Rn, bits), imm);
      break;
    case 2: /** signed offset */
      if (imm) {
        snprintf (opt_rn, ARM64_MAX_OP_LEN, ", [%s, #%d]", orc_arm64_reg_name(Rn, bits), imm);
      } else {
        snprintf (opt_rn, ARM64_MAX_OP_LEN, ", [%s]", orc_arm64_reg_name(Rn, bits));
      }
      break;
    case 3: /** pre-index */
      snprintf (opt_rn, ARM64_MAX_OP_LEN, ", [%s, #%d]!", orc_arm64_reg_name(Rn, bits), imm);
      break;
    default:
      ORC_COMPILER_ERROR(p, "unsupported variant %d\n", opt);
      return;
  }

  if (bits == ORC_ARM64_REG_64) {
    imm /= 8;
  } else {
    imm /= 4;
  }

  if (imm < -64 || imm > 63) {
    ORC_COMPILER_ERROR(p, "imm is Out-of-range\n");
    return;
  }

  code = arm64_code_mem_pair (bits, opt, opcode, imm, Rt2, Rn, Rt);

  ORC_ASM_CODE(p, "  %s %s, %s%s\n",
      insn_names[opcode],
      orc_arm64_reg_name(Rt, bits),
      orc_arm64_reg_name(Rt2, bits),
      opt_rn);

  orc_arm_emit (p, code);
}

/** Return from subroutine */

void
orc_arm64_emit_ret (OrcCompiler *p, int Rn)
{
  orc_uint32 code;

  code = 0xd65f0000 | ((Rn & 0x1f) << 5);

  ORC_ASM_CODE (p, "  ret %s\n",
      Rn == ORC_ARM64_LR ? "" : orc_arm64_reg_name (Rn, ORC_ARM64_REG_64));

  orc_arm_emit (p, code);
}

void
orc_arm64_emit_push (OrcCompiler *compiler, int regs, orc_uint32 vregs)
{
  if (!regs)
    return;

  int i;
  int x = 0;
  int count, stores;
  int stack_increased = 0;
  /** a number of 1s in regs */
  count = orc_count_ones (regs);
  /** AArch64 requires a 16-byte aligned stack pointer */
  stores = (count-1)/2+1;
  x = -1;
  for(i=0;i<32;i++){
    if (stores == 0) break;
    if (regs & (1<<i)) {
      if (stack_increased == 0) {
        /** increase stack area & store registers */
        if (count % 2 == 1) {
          orc_arm64_emit_store_pre (compiler, 64, ORC_GP_REG_BASE+i,
              ORC_ARM64_SP, (stores--) * -16);
          stack_increased = 1;
          continue;
        } else if (x != -1) {
          orc_arm64_emit_store_pair_pre (compiler, 64, ORC_GP_REG_BASE+x,
              ORC_GP_REG_BASE+i, ORC_ARM64_SP, (stores--) * -16);
          stack_increased = 1;
          x = -1;
          continue;
        }
      }

      if (x != -1) {
        /** store registers */
        orc_arm64_emit_store_pair_reg (compiler, 64, ORC_GP_REG_BASE+x,
            ORC_GP_REG_BASE+i, ORC_ARM64_SP, (stores--) * 16);
        x = -1;
      } else {
        x = i;
      }
    }
  }

  // FIXME: Push vregs for AArch64 too
}

void
orc_arm64_emit_pop (OrcCompiler *compiler, int regs, orc_uint32 vregs)
{
  if (!regs)
    return;

  // FIXME: Pop vregs for AArch64 too

  int i;
  int x = 0;
  int count, loads, tmp_loads;
  /** a number of 1s in regs */
  count = orc_count_ones (regs);
  /** AArch64 requires a 16-byte aligned stack pointer */
  loads = (count-1)/2+1;
  tmp_loads = loads;
  x = -1;
  for(i=31;i>=0;i--){
    if (regs & (1<<i)) {
      if (x != -1) {
        if (tmp_loads == 1) break;
        /** load registers */
        orc_arm64_emit_load_pair_reg (compiler, 64, ORC_GP_REG_BASE+i,
            ORC_GP_REG_BASE+x, ORC_ARM64_SP, (loads - (--tmp_loads)) * 16);
        x = -1;
      } else {
        x = i;
      }
    }
  }
  /** decrease stack area & load registers */
  if (count % 2 == 1) {
    orc_arm64_emit_load_post (compiler, 64, ORC_GP_REG_BASE+x,
        ORC_ARM64_SP, loads * 16);
  } else {
    orc_arm64_emit_load_pair_post (compiler, 64, ORC_GP_REG_BASE+i,
        ORC_GP_REG_BASE+x, ORC_ARM64_SP, loads * 16);
  }
}

void
orc_arm64_emit_align (OrcCompiler *compiler, int align_shift)
{
  int diff;

  diff = (compiler->code - compiler->codeptr)&((1<<align_shift) - 1);
  while (diff) {
    orc_arm64_emit_nop (compiler);
    diff-=4;
  }
}

void
orc_arm64_emit_nop (OrcCompiler *compiler)
{
  ORC_ASM_CODE(compiler,"  nop\n");
  orc_arm_emit (compiler, 0xd503201f);
}

void
orc_arm64_emit_branch (OrcCompiler *compiler, int cond, int label)
{
  orc_uint32 code;

  /** B.cond
   *    3                   2                   1
   *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   * +---------------------------------------------------------------+
   * |0 1 0 1 0 1 0|0|                imm19                |0| cond  |
   * +---------------------------------------------------------------+
   *
   *  B
   *    3                   2                   1
   *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   * +---------------------------------------------------------------+
   * |0 0 0 1 0 1|                       imm26                       |
   * +---------------------------------------------------------------+
   *
   * Note that we don't know exact imm19/imm26 yet, so need fixup codes.
   */
  if (cond < ORC_ARM_COND_AL) {
    code = 0x54000000;
    code |=  cond&0xf;

    ORC_ASM_CODE(compiler,"  b.%s .L%d\n", orc_arm_cond_name(cond), label);
  } else {
    code = 0x14000000;

    ORC_ASM_CODE(compiler,"  b .L%d\n", label);
  }

  orc_arm_add_fixup (compiler, label, 0);
  orc_arm_emit (compiler, code);
}

void
orc_arm64_emit_data (OrcCompiler *compiler, orc_uint32 data)
{
  if (compiler->target_flags & ORC_TARGET_CLEAN_COMPILE) {
    orc_arm64_emit_nop (compiler);
  } else {
    ORC_ASM_CODE(compiler,"  .word 0x%08x\n", data);
    orc_arm_emit (compiler, data);
  }
}
