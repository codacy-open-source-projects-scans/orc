#include "config.h"

#include <stdint.h>

#include <orc/orc.h>
#include <orc/orctarget.h>
#include <orc/orcprogram.h>
#include <orc/orccompiler.h>
#include <orc/orcinternal.h>
#include <orc/orcx86insn.h>
#include <orc/avx512/orcavx512.h>
#include <orc/avx512/orcavx512-private.h>
#include <orc/avx512/orcavx512insn.h>

/*
 * Performance notes on adaptive size-based optimization. This is based
 * on tests done on and Zen3: AMD Ryzen 9 8945HS, CPU family 25
 * 
 * Adaptive optimization uses different register sizes (ZMM/YMM/XMM) based on
 * data size. All use EVEX encoding, but microarchitecture behavior differs.
 *
 * SUCCESSFUL adaptive opcodes (improved vs AVX):
 * - avgsl: -1.7% → +0.85% (improved by 2.5%)
 * - maxf: Adaptive improves program-level perf (59/173 slow vs 60/173 ZMM-only)
 * - addd, muld, subd: Adaptive reduces slowdown vs AVX
 * - sqrtd: -12% → +5.3% (huge improvement with adaptive)
 * - splitlw, splitwb: Significantly improved (avoid expensive ZMM permutes)
 * - Conversion opcodes (convubw, etc.): Generally improved
 *
 * FAILED adaptive opcodes (worse with adaptive vs ZMM-only):
 * - shrsw: -4.9% (ZMM) → -7.2% (adaptive)
 * - shll: -3.4% (ZMM) → -4.8% (adaptive)
 * - avgul: -1.3% (ZMM) → -4.3% (adaptive)
 * - minf, mind, maxd: Making these adaptive degraded program-level performance
 *   (63/173 programs slower vs 59/173 with only maxf adaptive)
 * Hypothesis: For simple operations, using smaller register sizes may have
 * microarchitectural penalties (port pressure, latency differences) that
 * outweigh any benefits. ZMM operations may be better optimized in hardware.
 *
 * ADAPTIVE but still SLOW vs AVX (5%+ slower despite being adaptive):
 * - absl: -6.9% (already adaptive)
 * - maxsl: -5.3% (already adaptive)
 * - andq: -7.6% (already adaptive)
 * - xorq: -6.7% (already adaptive)
 * - avguw: -6.9% (already adaptive)
 * These simple operations show consistent slowdown regardless of register size,
 * suggesting the EVEX instruction encoding or execution has inherent overhead
 * vs VEX (AVX) encoding for simple operations on this microarchitecture.
 *
 * CANNOT OPTIMIZE (architectural limitations):
 * - Comparison opcodes (cmpeqw, cmpgtsw, cmpeqq, etc.): All EVEX comparisons
 *   write to mask registers requiring vpmovm2* conversion. No EVEX variant
 *   exists that writes directly to vector registers.
 *
 * IMPORTANT: Always validate with cmp_parse (program-level performance), not
 * just cmp_opcodes_sys. Small opcode improvements (<10%) may not reflect real
 * program behavior and can be misleading due to measurement variability.
 *
 * Conclusion: Adaptive optimization works best for complex multi-instruction
 * sequences (like splitlw, avgsl, sqrtd) where smaller register sizes avoid expensive
 * operations (permutes, masks) that are particularly costly at ZMM width.
 * Simple single-instruction opcodes often don't benefit, possibly due to
 * microarchitectural differences in how different register sizes are handled.
 */

#define ORC_AVX512_PERMQ(p7, p6, p5, p4, p3, p2, p1, p0) \
  (p0 & 0xff) | \
  ((p1 & 0xff) << 8) | \
  ((p2 & 0xff) << 16) | \
  ((p3 & 0xff) << 24) | \
  (((orc_uint64)p4 & 0xff) << 32) | \
  (((orc_uint64)p5 & 0xff) << 40) | \
  (((orc_uint64)p6 & 0xff) << 48) | \
  (((orc_uint64)p7 & 0xff) << 56) 

#define DEFINE_BINARY(opcode, insn)                                    \
  static void orc_avx512_rule_##opcode (OrcCompiler *c, void *user,    \
      OrcInstruction *insn)                                            \
  {                                                                    \
    int src0 = ORC_SRC_ARG (c, insn, 0);                               \
    int src1 = ORC_SRC_ARG (c, insn, 1);                               \
    int dest = ORC_DEST_ARG (c, insn, 0);                              \
    orc_avx512_insn_emit_##insn (c, src0, src1, dest, ORC_REG_INVALID, \
        FALSE);                                                        \
  }

#define DEFINE_UNARY(opcode, insn)                                     \
  static void orc_avx512_rule_##opcode (OrcCompiler *c, void *user,    \
      OrcInstruction *insn)                                            \
  {                                                                    \
    int src0 = ORC_SRC_ARG (c, insn, 0);                               \
    int dest = ORC_DEST_ARG (c, insn, 0);                              \
    orc_avx512_insn_emit_##insn (c, src0, dest, ORC_REG_INVALID,       \
        FALSE);                                                        \
  }

#define DEFINE_UNARY_AVX_AVX512(opcode, insn)                          \
  static void orc_avx512_rule_##opcode (OrcCompiler *c, void *user,    \
      OrcInstruction *insn)                                            \
  {                                                                    \
    int src0 = ORC_SRC_ARG (c, insn, 0);                               \
    int dest = ORC_DEST_ARG (c, insn, 0);                              \
    orc_avx512_insn_emit_##insn (c, src0, ORC_AVX512_AVX_REG (dest),   \
        ORC_REG_INVALID, FALSE);                                       \
  }

#define DEFINE_UNARY_AVX512_AVX(opcode, insn)                          \
  static void orc_avx512_rule_##opcode (OrcCompiler *c, void *user,    \
      OrcInstruction *insn)                                            \
  {                                                                    \
    int src0 = ORC_SRC_ARG (c, insn, 0);                               \
    int dest = ORC_DEST_ARG (c, insn, 0);                              \
    orc_avx512_insn_emit_##insn (c, ORC_AVX512_AVX_REG (src0), dest,   \
        ORC_REG_INVALID, FALSE);                                       \
  }

/* Adaptive macros that choose instruction based on data size */
#define DEFINE_UNARY_AVX512_AVX_ADAPTIVE(opcode, insn)                 \
  static void orc_avx512_rule_##opcode (OrcCompiler *c, void *user,    \
      OrcInstruction *insn_arg)                                        \
  {                                                                    \
    int src0 = ORC_SRC_ARG (c, insn_arg, 0);                           \
    int dest = ORC_DEST_ARG (c, insn_arg, 0);                          \
    const int size = ORC_SIZE (c, ORC_SRC_SIZE (c, insn_arg, 0));      \
    if (size >= 32) {                                                  \
      /* YMM source -> ZMM dest */                                     \
      orc_avx512_insn_emit_##insn (c, ORC_AVX512_AVX_REG (src0), dest, \
          ORC_REG_INVALID, FALSE);                                     \
    } else if (size >= 16) {                                           \
      /* XMM source -> YMM dest */                                     \
      orc_avx512_insn_avx_emit_##insn (c, ORC_AVX512_SSE_REG (src0),   \
          ORC_AVX512_AVX_REG (dest), ORC_REG_INVALID, FALSE);          \
    } else {                                                           \
      /* XMM source -> XMM dest */                                     \
      orc_avx512_insn_sse_emit_##insn (c, ORC_AVX512_SSE_REG (src0),   \
          ORC_AVX512_SSE_REG (dest), ORC_REG_INVALID, FALSE);          \
    }                                                                  \
  }

#define DEFINE_BINARY_ADAPTIVE(opcode, insn)                           \
  static void orc_avx512_rule_##opcode (OrcCompiler *c, void *user,    \
      OrcInstruction *insn_arg)                                        \
  {                                                                    \
    int src0 = ORC_SRC_ARG (c, insn_arg, 0);                           \
    int src1 = ORC_SRC_ARG (c, insn_arg, 1);                           \
    int dest = ORC_DEST_ARG (c, insn_arg, 0);                          \
    const int size = ORC_SIZE (c, ORC_SRC_SIZE (c, insn_arg, 0));      \
    if (size >= 64) {                                                  \
      orc_avx512_insn_emit_##insn (c, src0, src1, dest,                \
          ORC_REG_INVALID, FALSE);                                     \
    } else if (size >= 32) {                                           \
      orc_avx512_insn_avx_emit_##insn (c, ORC_AVX512_AVX_REG (src0),   \
          ORC_AVX512_AVX_REG (src1), ORC_AVX512_AVX_REG (dest),        \
          ORC_REG_INVALID, FALSE);                                     \
    } else {                                                           \
      orc_avx512_insn_sse_emit_##insn (c, ORC_AVX512_SSE_REG (src0),   \
          ORC_AVX512_SSE_REG (src1), ORC_AVX512_SSE_REG (dest),        \
          ORC_REG_INVALID, FALSE);                                     \
    }                                                                  \
  }

#define DEFINE_UNARY_ADAPTIVE(opcode, insn)                            \
  static void orc_avx512_rule_##opcode (OrcCompiler *c, void *user,    \
      OrcInstruction *insn_arg)                                        \
  {                                                                    \
    int src0 = ORC_SRC_ARG (c, insn_arg, 0);                           \
    int dest = ORC_DEST_ARG (c, insn_arg, 0);                          \
    const int size = ORC_SIZE (c, ORC_SRC_SIZE (c, insn_arg, 0));      \
    if (size >= 64) {                                                  \
      orc_avx512_insn_emit_##insn (c, src0, dest, ORC_REG_INVALID,     \
          FALSE);                                                      \
    } else if (size >= 32) {                                           \
      orc_avx512_insn_avx_emit_##insn (c, ORC_AVX512_AVX_REG (src0),   \
          ORC_AVX512_AVX_REG (dest));                                  \
    } else {                                                           \
      orc_avx512_insn_sse_emit_##insn (c, ORC_AVX512_SSE_REG (src0),   \
          ORC_AVX512_SSE_REG (dest));                                  \
    }                                                                  \
  }

#define REGISTER_RULE(opcode) orc_rule_register (rule_set, #opcode, \
    orc_avx512_rule_##opcode, NULL)
#define REGISTER_RULE_WITH_GENERIC(opcode, generic) \
  orc_rule_register (rule_set, #opcode, orc_avx512_rule_##generic, NULL)
#define REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD(opcode, generic, payload) \
  orc_rule_register (rule_set, #opcode, orc_avx512_rule_##generic,       \
  (void *)payload)

/* FIXME to be added into orcprogram.h? similar to ORC_SRC_ARG */
/* The size in bytes */
#define ORC_SIZE(c,sz) sz << c->loop_shift
#define ORC_SRC_PTR(p,i,n) ((p)->vars[(i)->src_args[(n)]].ptr_register)
#define ORC_SRC_SIZE(p,i,n) ((p)->vars[(i)->src_args[(n)]].size)
#define ORC_SRC_IS_ALIGNED(p,i,n) ((p)->vars[(i)->src_args[(n)]].is_aligned)
#define ORC_SRC_VAR(c,i,n) ((c)->vars + (i)->src_args[n]);
#define ORC_DEST_VAR(c,i,n) ((c)->vars + (i)->dest_args[n]);

DEFINE_BINARY_ADAPTIVE (mulll, vpmulld)
DEFINE_BINARY_ADAPTIVE (mullw, vpmullw)
DEFINE_BINARY_ADAPTIVE (mulhsw, vpmulhw)
DEFINE_BINARY_ADAPTIVE (mulhuw, vpmulhuw)
DEFINE_BINARY_ADAPTIVE (xorb, vpxord)
DEFINE_BINARY_ADAPTIVE (xorw, vpxord)
DEFINE_BINARY_ADAPTIVE (xorl, vpxord)
DEFINE_BINARY_ADAPTIVE (xorq, vpxord)
DEFINE_UNARY_ADAPTIVE (absb, vpabsb)
DEFINE_UNARY_ADAPTIVE (absw, vpabsw)
DEFINE_UNARY_ADAPTIVE (absl, vpabsd)
DEFINE_BINARY_ADAPTIVE (addb, vpaddb)
DEFINE_BINARY_ADAPTIVE (addw, vpaddw)
DEFINE_BINARY_ADAPTIVE (addl, vpaddd)
DEFINE_BINARY_ADAPTIVE (addq, vpaddq)

DEFINE_BINARY_ADAPTIVE (orb, vpord)
DEFINE_BINARY_ADAPTIVE (orw, vpord)
DEFINE_BINARY_ADAPTIVE (orl, vpord)
DEFINE_BINARY_ADAPTIVE (orq, vpord)
DEFINE_BINARY_ADAPTIVE (andb, vpandd)
DEFINE_BINARY_ADAPTIVE (andw, vpandd)
DEFINE_BINARY_ADAPTIVE (andl, vpandd)
DEFINE_BINARY_ADAPTIVE (andq, vpandd)
DEFINE_BINARY_ADAPTIVE (andnb, vpandnd)
DEFINE_BINARY_ADAPTIVE (andnw, vpandnd)
DEFINE_BINARY_ADAPTIVE (andnl, vpandnd)
DEFINE_BINARY_ADAPTIVE (andnq, vpandnd)

DEFINE_BINARY (addf, vaddps);
DEFINE_BINARY (subf, vsubps);
DEFINE_BINARY (mulf, vmulps);
DEFINE_BINARY (divf, vdivps);
DEFINE_UNARY (sqrtf, vsqrtps);
DEFINE_BINARY (orf, vorps);
DEFINE_BINARY (andf, vandps);

DEFINE_BINARY_ADAPTIVE (addd, vaddpd);
DEFINE_BINARY_ADAPTIVE (subd, vsubpd);
DEFINE_BINARY_ADAPTIVE (muld, vmulpd);
DEFINE_BINARY (divd, vdivpd);
DEFINE_UNARY_ADAPTIVE (sqrtd, vsqrtpd);

DEFINE_UNARY_AVX512_AVX_ADAPTIVE (convfd, vcvtps2pd)
DEFINE_UNARY_AVX512_AVX_ADAPTIVE (convld, vcvtdq2pd)
DEFINE_UNARY_AVX_AVX512 (convdf, vcvtpd2ps)

DEFINE_UNARY (convlf, vcvtdq2ps)

DEFINE_BINARY_ADAPTIVE (minsb, vpminsb)
DEFINE_BINARY_ADAPTIVE (minub, vpminub)
DEFINE_BINARY_ADAPTIVE (maxsb, vpmaxsb)
DEFINE_BINARY_ADAPTIVE (maxub, vpmaxub)

DEFINE_BINARY_ADAPTIVE (minsw, vpminsw)
DEFINE_BINARY_ADAPTIVE (minuw, vpminuw)
DEFINE_BINARY_ADAPTIVE (maxsw, vpmaxsw)
DEFINE_BINARY_ADAPTIVE (maxuw, vpmaxuw)

DEFINE_BINARY_ADAPTIVE (minsl, vpminsd)
DEFINE_BINARY_ADAPTIVE (minul, vpminud)
DEFINE_BINARY_ADAPTIVE (maxsl, vpmaxsd)
DEFINE_BINARY_ADAPTIVE (maxul, vpmaxud)

DEFINE_BINARY_ADAPTIVE (subb, vpsubb)
DEFINE_BINARY_ADAPTIVE (subw, vpsubw)
DEFINE_BINARY_ADAPTIVE (subq, vpsubq)

DEFINE_BINARY_ADAPTIVE (avgub, vpavgb)

DEFINE_UNARY_AVX512_AVX_ADAPTIVE (convsbw, vpmovsxbw)
DEFINE_UNARY_AVX512_AVX_ADAPTIVE (convswl, vpmovsxwd)
DEFINE_UNARY_AVX512_AVX_ADAPTIVE (convslq, vpmovsxdq)

DEFINE_UNARY_AVX512_AVX_ADAPTIVE (convubw, vpmovzxbw)
DEFINE_UNARY_AVX512_AVX_ADAPTIVE (convuwl, vpmovzxwd)
DEFINE_UNARY_AVX512_AVX_ADAPTIVE (convulq, vpmovzxdq)

DEFINE_BINARY_ADAPTIVE (addusb, vpaddusb)
DEFINE_BINARY_ADAPTIVE (addusw, vpaddusw)

DEFINE_BINARY_ADAPTIVE (subusb, vpsubusb)
DEFINE_BINARY_ADAPTIVE (subusw, vpsubusw)

DEFINE_BINARY_ADAPTIVE (subl, vpsubd)

DEFINE_BINARY_ADAPTIVE (avguw, vpavgw)

DEFINE_BINARY_ADAPTIVE (addssb, vpaddsb)
DEFINE_BINARY_ADAPTIVE (addssw, vpaddsw)

DEFINE_BINARY_ADAPTIVE (subssb, vpsubsb)
DEFINE_BINARY_ADAPTIVE (subssw, vpsubsw)

static int
orc_x86_compiler_load_array (OrcCompiler *c, int i)
{
  orc_x86_emit_mov_memoffset_reg (c, c->is_64bit ? 8 : 4,
      ORC_STRUCT_OFFSET (OrcExecutor, arrays[i]), c->exec_reg,
      c->gp_tmpreg);
  return c->gp_tmpreg;
}

/* FIXME share this */
/**
 * orc_x86_compiler_load_src_array:
 *
 * Loads the address of a source array argument into a register
 *
 * @c: The #OrcCompiler that loads the array argument
 * @insn: The #OrcInstruction to ge the argument from
 * @i: The index of the argument
 */ 
static int
orc_x86_compiler_load_src_argument (OrcCompiler *c, OrcInstruction *insn, int i)
{
  const int src_var = insn->src_args[i];
  const OrcVariable *src = c->vars + src_var;

  if (src->ptr_register)
    return src->ptr_register;

  return orc_x86_compiler_load_array (c, src_var);
}

/* FIXME share this */
/**
 * orc_x86_compiler_load_dest_argument:
 *
 * Loads the address of a destination array argument into a register
 *
 * @c: The #OrcCompiler that loads the array argument
 * @insn: The #OrcInstruction to ge the argument from
 * @i: The index of the argument
 */ 
static int
orc_x86_compiler_load_dest_argument (OrcCompiler *c, OrcInstruction *insn, int i)
{
  const int dest_var = insn->dest_args[i];
  const OrcVariable *dest = c->vars + dest_var;

  if (dest->ptr_register)
    return dest->ptr_register;

  return orc_x86_compiler_load_array (c, dest_var);
}

static void
orc_avx512_compiler_load_param (OrcCompiler *c, OrcInstruction *insn, int i, int reg,
  int param_size, int reg_size)
{
  const int src_var = insn->src_args[i];

  orc_x86_emit_cpuinsn_comment (c, "# loading param of size %d", param_size);
  /* In OrcExecutor, params are 32 bits, having the hi 32-bits (for cases of 64-bits)
   * in the params_hi field
   */
  orc_avx512_insn_emit_mov_memoffset_reg (c, ORC_X86_INSN_OPERAND_SIZE_32,
      ORC_STRUCT_OFFSET (OrcExecutor, params[src_var]), c->exec_reg,
      reg, FALSE);
  if (param_size > 4 && reg_size > 4) {
      orc_avx512_insn_emit_vpinsrd_r_r_m_i (c, 1,
          ORC_STRUCT_OFFSET (OrcExecutor, params[src_var + (ORC_N_PARAMS)]),
          ORC_AVX512_SSE_REG (reg), c->exec_reg, ORC_AVX512_SSE_REG (reg));
  }
  /* Expand the value to the register size */
  switch (param_size) {
    case 1:
      orc_avx512_insn_emit_vpbroadcastb (c, ORC_AVX512_SSE_REG (reg), reg,
          ORC_REG_INVALID, FALSE);
      break;
    case 2:
      orc_avx512_insn_emit_vpbroadcastw (c, ORC_AVX512_SSE_REG (reg), reg,
          ORC_REG_INVALID, FALSE);
      break;
    case 4:
      orc_avx512_insn_emit_vpbroadcastd (c, ORC_AVX512_SSE_REG (reg), reg,
          ORC_REG_INVALID, FALSE);
      break;
    case 8:
      orc_avx512_insn_emit_vpbroadcastq (c, ORC_AVX512_SSE_REG (reg), reg,
          ORC_REG_INVALID, FALSE);
      break;
  }
}

static int
orc_avx512_compiler_load_param2 (OrcCompiler *c, OrcInstruction *insn, int i)
{
  const int src_var = insn->src_args[i];
  const OrcVariable *src = c->vars + src_var;
  int tmp;

  /* FIXME orccompiler.c has a bug that does not load params
   * automatically (through a loadp) when the orc opcode has the scalar
   * flag. We load it ourselves for now
   */
  tmp = orc_compiler_get_temp_reg (c);
  orc_avx512_compiler_load_param (c, insn, i, tmp, src->size, src->size);
  return tmp;
}

static void
orc_avx512_rule_loadpX (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcVariable *src = ORC_SRC_VAR (c, insn, 0);
  OrcVariable *dest = ORC_DEST_VAR (c, insn, 0);
  int size = ORC_PTR_TO_INT (user);

  if (src->vartype == ORC_VAR_TYPE_PARAM) {
    orc_avx512_compiler_load_param (c, insn, 0, dest->alloc, size, src->size);
  } else if (src->vartype == ORC_VAR_TYPE_CONST) {
    orc_compiler_load_constant_from_size_and_value (c, dest->alloc, size,
        src->value.i);
  } else {
    ORC_COMPILER_ERROR (c, "Unknown variable type %d", src->vartype);
  }
}

static void
orc_avx512_rule_loadX (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcVariable *src = ORC_SRC_VAR (c, insn, 0);
  OrcVariable *dest = ORC_DEST_VAR (c, insn, 0);
  const int offset = c->offset * src->size;
  int ptr_reg;

  ptr_reg = orc_x86_compiler_load_src_argument (c, insn, 0);
  orc_avx512_insn_emit_mov_memoffset_reg (c, ORC_SIZE (c, src->size), offset,
      ptr_reg, dest->alloc, dest->is_aligned);
  src->update_type = ORC_VARIABLE_UPDATE_TYPE_FULL;
}

static void
orc_avx512_rule_storeX (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcVariable *src = ORC_SRC_VAR (c, insn, 0);
  OrcVariable *dest = ORC_DEST_VAR (c, insn, 0);
  const int offset = c->offset * dest->size;
  int ptr_reg;

  ptr_reg = orc_x86_compiler_load_dest_argument (c, insn, 0);
  orc_avx512_insn_emit_mov_reg_memoffset (c, ORC_SIZE (c, dest->size),
      src->alloc, offset, ptr_reg, dest->is_aligned, dest->is_uncached);
  dest->update_type = ORC_VARIABLE_UPDATE_TYPE_FULL;
}

static void
orc_avx512_rule_copyX (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);

  if (src == dest)
    return;
  orc_avx512_insn_emit_vmovdqa32_r_r (c, src, dest, ORC_REG_INVALID, FALSE);
}

/* load upsampled interpolate (byte) */
static void
orc_avx512_rule_loadupib (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcVariable *src = ORC_SRC_VAR (c, insn, 0);
  OrcVariable *dest = ORC_DEST_VAR (c, insn, 0);
  int ptr_reg;
  int tmp;
  const int offset = (c->offset * src->size) >> 1;
  const int size = ORC_SIZE (c, ORC_SRC_SIZE (c, insn, 0));
  /* ensure we load at least 1 byte */
  const int load_size = (size <= 2) ? 1 : (size >> 1);

  ptr_reg = orc_x86_compiler_load_src_argument (c, insn, 0);
  tmp = orc_compiler_get_temp_reg (c);

  /* Load two adjacent half-size chunks for interpolation */
  orc_avx512_insn_emit_mov_memoffset_reg (c, load_size, offset, ptr_reg,
      dest->alloc, FALSE);
  orc_avx512_insn_emit_mov_memoffset_reg (c, load_size, offset + 1,
      ptr_reg, tmp, FALSE);

  /* Average the two chunks - vpavgb always takes ZMM register indices */
  orc_avx512_insn_emit_vpavgb (c, dest->alloc, tmp, tmp,
      ORC_REG_INVALID, FALSE);

  /* Interleave average with original (interpolate) */
  if (size >= 64) {
    int tmp2 = orc_compiler_get_temp_reg (c);
    int tmp3 = orc_compiler_get_temp_reg (c);
    int avg_orig = orc_compiler_get_temp_reg (c);
    
    /* Save original dest (32 bytes in lower 256 bits) */
    orc_avx512_insn_emit_vmovdqa32_r_r (c, dest->alloc, tmp3, ORC_REG_INVALID, FALSE);
    /* Save original averaged tmp (for second half processing) */
    orc_avx512_insn_emit_vmovdqa32_r_r (c, tmp, avg_orig, ORC_REG_INVALID, FALSE);
    
    /* Process lower 16 input bytes (bytes 0-15) → 32 output bytes */
    /* Interleave high bytes: tmp3[15:8] with avg_orig[15:8] → tmp2[255:128] */
    orc_avx512_insn_avx_emit_vpunpckhbw (c, ORC_AVX512_AVX_REG (tmp3),
        ORC_AVX512_AVX_REG (avg_orig), ORC_AVX512_AVX_REG (tmp2),
        ORC_REG_INVALID, FALSE);
    /* Interleave low bytes: tmp3[7:0] with avg_orig[7:0] → tmp[255:0] */
    orc_avx512_insn_avx_emit_vpunpcklbw (c, ORC_AVX512_AVX_REG (tmp3),
        ORC_AVX512_AVX_REG (avg_orig), ORC_AVX512_AVX_REG (tmp),
        ORC_REG_INVALID, FALSE);
    /* Combine: insert tmp2[127:0] into tmp[255:128] */
    orc_avx512_insn_avx_emit_vinserti32x4 (c, 1, ORC_AVX512_AVX_REG (tmp),
        ORC_AVX512_AVX_REG (tmp2), ORC_AVX512_AVX_REG (tmp),
        ORC_REG_INVALID, FALSE);
    /* Now tmp[255:0] has 32 output bytes from first 16 input bytes */
    /* Put into lower 256 bits of dest */
    orc_avx512_insn_emit_vinserti32x8 (c, 0, dest->alloc, tmp,
        dest->alloc, ORC_REG_INVALID, FALSE);
    
    /* Process upper 16 input bytes (bytes 16-31) from saved tmp3 */
    /* Extract upper 128 bits of the saved 256-bit input into lower 128 of tmp3 */
    orc_avx512_insn_avx_emit_vextracti32x4 (c, 1, ORC_AVX512_AVX_REG (tmp3),
        ORC_AVX512_SSE_REG (tmp3), ORC_REG_INVALID, FALSE);
    /* Extract upper 128 bits of the averaged data */
    orc_avx512_insn_avx_emit_vextracti32x4 (c, 1, ORC_AVX512_AVX_REG (avg_orig),
        ORC_AVX512_SSE_REG (avg_orig), ORC_REG_INVALID, FALSE);
    /* Now tmp3[127:0] has bytes 16-31 of original input */
    /* Now avg_orig[127:0] has bytes 16-31 of averaged data */
    /* Interleave high bytes: tmp3[31:24] with avg_orig[31:24] → tmp2[255:128] */
    orc_avx512_insn_avx_emit_vpunpckhbw (c, ORC_AVX512_AVX_REG (tmp3),
        ORC_AVX512_AVX_REG (avg_orig), ORC_AVX512_AVX_REG (tmp2),
        ORC_REG_INVALID, FALSE);
    /* Interleave low bytes: tmp3[23:16] with avg_orig[23:16] → tmp[255:0] */
    orc_avx512_insn_avx_emit_vpunpcklbw (c, ORC_AVX512_AVX_REG (tmp3),
        ORC_AVX512_AVX_REG (avg_orig), ORC_AVX512_AVX_REG (tmp),
        ORC_REG_INVALID, FALSE);
    /* Combine: insert tmp2[127:0] into tmp[255:128] */
    orc_avx512_insn_avx_emit_vinserti32x4 (c, 1, ORC_AVX512_AVX_REG (tmp),
        ORC_AVX512_AVX_REG (tmp2), ORC_AVX512_AVX_REG (tmp),
        ORC_REG_INVALID, FALSE);
    /* Now tmp[255:0] has 32 output bytes from second 16 input bytes */
    /* Put into upper 256 bits of dest */
    orc_avx512_insn_emit_vinserti32x8 (c, 1, dest->alloc, tmp,
        dest->alloc, ORC_REG_INVALID, FALSE);
    
    orc_compiler_release_temp_reg (c, avg_orig);
    orc_compiler_release_temp_reg (c, tmp3);
    orc_compiler_release_temp_reg (c, tmp2);
  } else if (size >= 32) {
    int tmp2 = orc_compiler_get_temp_reg (c);
    /* Interleave high bytes of dest with average (tmp) */
    orc_avx512_insn_avx_emit_vpunpckhbw (c, ORC_AVX512_AVX_REG (dest->alloc),
        ORC_AVX512_AVX_REG (tmp), ORC_AVX512_AVX_REG (tmp2),
        ORC_REG_INVALID, FALSE);
    /* Interleave low bytes of dest with average (tmp) */
    orc_avx512_insn_avx_emit_vpunpcklbw (c, ORC_AVX512_AVX_REG (dest->alloc),
        ORC_AVX512_AVX_REG (tmp), ORC_AVX512_AVX_REG (dest->alloc),
        ORC_REG_INVALID, FALSE);
    /* Insert high 128-bit lane from tmp2 into dest upper lane */
    orc_avx512_insn_avx_emit_vinserti32x4 (c, 1, ORC_AVX512_AVX_REG (dest->alloc), 
        ORC_AVX512_AVX_REG (tmp2), ORC_AVX512_AVX_REG (dest->alloc), 
        ORC_REG_INVALID, FALSE);
    orc_compiler_release_temp_reg (c, tmp2);
  } else {
    orc_avx512_insn_sse_emit_vpunpcklbw (c, ORC_AVX512_SSE_REG (dest->alloc),
        ORC_AVX512_SSE_REG (tmp), ORC_AVX512_SSE_REG (dest->alloc),
        ORC_REG_INVALID, FALSE);
  }

  orc_compiler_release_temp_reg (c, tmp);
  src->update_type = ORC_VARIABLE_UPDATE_TYPE_HALF;
}

/* load upsampled duplicate (byte) */
static void
orc_avx512_rule_loadupdb (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcVariable *src = ORC_SRC_VAR (c, insn, 0);
  OrcVariable *dest = ORC_DEST_VAR (c, insn, 0);
  int ptr_reg;
  const int offset = (c->offset * src->size) >> 1;
  const int size = ORC_SIZE (c, ORC_SRC_SIZE (c, insn, 0));
  /* load half the data, but at least 1 byte for sizes 1-2 */
  const int load_size = (size <= 2) ? 1 : (size >> 1);

  ptr_reg = orc_x86_compiler_load_src_argument (c, insn, 0);

  /* Load phase */
  orc_avx512_insn_emit_mov_memoffset_reg (c, load_size, offset, ptr_reg,
      dest->alloc, (size <= 2) ? FALSE : src->is_aligned);

  /* Duplicate phase: switch on element size, use appropriate method for total size */
  int tmp = 0;
  if (size >= 32) {
    tmp = orc_compiler_get_temp_reg (c);
  }
  
  switch (src->size) {
    case 1:
      if (size >= 64) {
        /* 64-byte output from 32-byte input: operate on ZMM (512-bit) */
        /* Input is 32 bytes in dest[255:0] */
        /* Need to duplicate each byte: 32 input → 64 output */
        int tmp2 = orc_compiler_get_temp_reg (c);
        int tmp3 = orc_compiler_get_temp_reg (c);
        
        /* Save the original input (32 bytes in lower 256 bits) */
        orc_avx512_insn_emit_vmovdqa32_r_r (c, dest->alloc, tmp3, ORC_REG_INVALID, FALSE);
        
        /* Process lower 16 input bytes (bytes 0-15) → 32 output bytes */
        /* High bytes: bytes 8-15 → bytes 16-31 */
        orc_avx512_insn_avx_emit_vpunpckhbw (c, ORC_AVX512_AVX_REG (tmp3),
            ORC_AVX512_AVX_REG (tmp3), ORC_AVX512_AVX_REG (tmp2),
            ORC_REG_INVALID, FALSE);
        /* Low bytes: bytes 0-7 → bytes 0-15 */
        orc_avx512_insn_avx_emit_vpunpcklbw (c, ORC_AVX512_AVX_REG (tmp3),
            ORC_AVX512_AVX_REG (tmp3), ORC_AVX512_AVX_REG (tmp),
            ORC_REG_INVALID, FALSE);
        /* Combine: insert tmp2[127:0] into tmp[255:128] */
        orc_avx512_insn_avx_emit_vinserti32x4 (c, 1, ORC_AVX512_AVX_REG (tmp),
            ORC_AVX512_AVX_REG (tmp2), ORC_AVX512_AVX_REG (tmp),
            ORC_REG_INVALID, FALSE);
        /* Now tmp[255:0] has 32 output bytes from first 16 input bytes */
        /* Put into lower 256 bits of dest */
        orc_avx512_insn_emit_vinserti32x8 (c, 0, dest->alloc, tmp,
            dest->alloc, ORC_REG_INVALID, FALSE);
        
        /* Process upper 16 input bytes (bytes 16-31) from saved tmp3 */
        /* Extract upper 128 bits of the saved 256-bit input into lower 128 of tmp3 */
        orc_avx512_insn_avx_emit_vextracti32x4 (c, 1, ORC_AVX512_AVX_REG (tmp3),
            ORC_AVX512_SSE_REG (tmp3), ORC_REG_INVALID, FALSE);
        /* Now tmp3[127:0] has bytes 16-31 of original input */
        /* High bytes: bytes 24-31 → bytes 16-31 of tmp2 */
        orc_avx512_insn_avx_emit_vpunpckhbw (c, ORC_AVX512_AVX_REG (tmp3),
            ORC_AVX512_AVX_REG (tmp3), ORC_AVX512_AVX_REG (tmp2),
            ORC_REG_INVALID, FALSE);
        /* Low bytes: bytes 16-23 → bytes 0-15 of tmp */
        orc_avx512_insn_avx_emit_vpunpcklbw (c, ORC_AVX512_AVX_REG (tmp3),
            ORC_AVX512_AVX_REG (tmp3), ORC_AVX512_AVX_REG (tmp),
            ORC_REG_INVALID, FALSE);
        /* Combine: insert tmp2[127:0] into tmp[255:128] */
        orc_avx512_insn_avx_emit_vinserti32x4 (c, 1, ORC_AVX512_AVX_REG (tmp),
            ORC_AVX512_AVX_REG (tmp2), ORC_AVX512_AVX_REG (tmp),
            ORC_REG_INVALID, FALSE);
        /* Now tmp[255:0] has 32 output bytes from second 16 input bytes */
        /* Put into upper 256 bits of dest */
        orc_avx512_insn_emit_vinserti32x8 (c, 1, dest->alloc, tmp,
            dest->alloc, ORC_REG_INVALID, FALSE);
        
        orc_compiler_release_temp_reg (c, tmp3);
        orc_compiler_release_temp_reg (c, tmp2);
      } else if (size >= 32) {
        /* 32-byte output: operate on YMM (256-bit) - need high+low+insert like AVX */
        orc_avx512_insn_avx_emit_vpunpckhbw (c, ORC_AVX512_AVX_REG (dest->alloc),
            ORC_AVX512_AVX_REG (dest->alloc), ORC_AVX512_AVX_REG (tmp),
            ORC_REG_INVALID, FALSE);
        orc_avx512_insn_avx_emit_vpunpcklbw (c, ORC_AVX512_AVX_REG (dest->alloc),
            ORC_AVX512_AVX_REG (dest->alloc), ORC_AVX512_AVX_REG (dest->alloc),
            ORC_REG_INVALID, FALSE);
        /* Insert high 128-bit lane from tmp into dest[127:0] at position 1 (upper lane) */
        orc_avx512_insn_avx_emit_vinserti32x4 (c, 1, ORC_AVX512_AVX_REG (dest->alloc), 
            ORC_AVX512_AVX_REG (tmp), ORC_AVX512_AVX_REG (dest->alloc), 
            ORC_REG_INVALID, FALSE);
      } else {
        /* 16-byte and smaller: operate on XMM (128-bit) - only need low */
        orc_avx512_insn_sse_emit_vpunpcklbw (c, ORC_AVX512_SSE_REG (dest->alloc),
            ORC_AVX512_SSE_REG (dest->alloc), ORC_AVX512_SSE_REG (dest->alloc),
            ORC_REG_INVALID, FALSE);
      }
      break;
    case 2:
      if (size >= 64) {
        orc_avx512_insn_emit_vmovdqa32_r_r (c, dest->alloc, tmp, ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vpunpcklwd (c, tmp, tmp,
            dest->alloc, ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vpunpckhwd (c, tmp, tmp,
            tmp, ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vinserti32x8 (c, 1, dest->alloc, tmp,
            dest->alloc, ORC_REG_INVALID, FALSE);
      } else if (size >= 32) {
        orc_avx512_insn_avx_emit_vpunpckhwd (c, ORC_AVX512_AVX_REG (dest->alloc),
            ORC_AVX512_AVX_REG (dest->alloc), ORC_AVX512_AVX_REG (tmp),
            ORC_REG_INVALID, FALSE);
        orc_avx512_insn_avx_emit_vpunpcklwd (c, ORC_AVX512_AVX_REG (dest->alloc),
            ORC_AVX512_AVX_REG (dest->alloc), ORC_AVX512_AVX_REG (dest->alloc),
            ORC_REG_INVALID, FALSE);
        orc_avx512_insn_avx_emit_vinserti32x4 (c, 1, ORC_AVX512_AVX_REG (dest->alloc), 
            ORC_AVX512_AVX_REG (tmp), ORC_AVX512_AVX_REG (dest->alloc), 
            ORC_REG_INVALID, FALSE);
      } else {
        orc_avx512_insn_sse_emit_vpunpcklwd (c, ORC_AVX512_SSE_REG (dest->alloc),
            ORC_AVX512_SSE_REG (dest->alloc), ORC_AVX512_SSE_REG (dest->alloc),
            ORC_REG_INVALID, FALSE);
      }
      break;
    case 4:
      if (size >= 64) {
        orc_avx512_insn_emit_vmovdqa32_r_r (c, dest->alloc, tmp, ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vpunpckldq (c, tmp, tmp,
            dest->alloc, ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vpunpckhdq (c, tmp, tmp,
            tmp, ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vinserti32x8 (c, 1, dest->alloc, tmp,
            dest->alloc, ORC_REG_INVALID, FALSE);
      } else if (size >= 32) {
        orc_avx512_insn_avx_emit_vpunpckhdq (c, ORC_AVX512_AVX_REG (dest->alloc),
            ORC_AVX512_AVX_REG (dest->alloc), ORC_AVX512_AVX_REG (tmp),
            ORC_REG_INVALID, FALSE);
        orc_avx512_insn_avx_emit_vpunpckldq (c, ORC_AVX512_AVX_REG (dest->alloc),
            ORC_AVX512_AVX_REG (dest->alloc), ORC_AVX512_AVX_REG (dest->alloc),
            ORC_REG_INVALID, FALSE);
        orc_avx512_insn_avx_emit_vinserti32x4 (c, 1, ORC_AVX512_AVX_REG (dest->alloc), 
            ORC_AVX512_AVX_REG (tmp), ORC_AVX512_AVX_REG (dest->alloc), 
            ORC_REG_INVALID, FALSE);
      } else {
        orc_avx512_insn_sse_emit_vpunpckldq (c, ORC_AVX512_SSE_REG (dest->alloc),
            ORC_AVX512_SSE_REG (dest->alloc), ORC_AVX512_SSE_REG (dest->alloc),
            ORC_REG_INVALID, FALSE);
      }
      break;
  }

  if (size >= 32) {
    orc_compiler_release_temp_reg (c, tmp);
  }

  src->update_type = ORC_VARIABLE_UPDATE_TYPE_HALF;
}

static void
orc_avx512_rule_loadoffX (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcVariable *src0 = ORC_SRC_VAR (c, insn, 0);
  OrcVariable *src1 = ORC_SRC_VAR (c, insn, 1);
  OrcVariable *dest = ORC_DEST_VAR (c, insn, 0);
  int offset = (c->offset + src1->value.i) * src0->size;
  int ptr_reg;

  if (src1->vartype != ORC_VAR_TYPE_CONST) {
    ORC_COMPILER_ERROR (c, 
        "code generation rule for %s only works with constant offset",
        insn->opcode->name);
    return;
  }

  ptr_reg = orc_x86_compiler_load_src_argument (c, insn, 0);
  orc_avx512_insn_emit_mov_memoffset_reg (c, ORC_SIZE (c, src0->size), offset,
      ptr_reg, dest->alloc, dest->is_aligned);
  src0->update_type = ORC_VARIABLE_UPDATE_TYPE_FULL;
}

/* Generic shift implementation for word/dword/qword left/right/arithmetic shifts
 * 
 * Performance note: Adaptive implementation with size-based branching (ZMM/YMM/XMM)
 * was tested and found to hurt performance compared to ZMM-only approach:
 * - shrsw: -7.2% (adaptive) vs -4.9% (ZMM-only) 
 * - shlw: -6.1% both
 * - shll: -4.8% (adaptive) vs -3.4% (ZMM-only)
 * - orc_dequantise_s16_2d_4xn: -30% (adaptive) vs -26% (ZMM-only)
 * 
 * The branching overhead outweighs benefits for shift operations.
 * Keep ZMM-only implementation for best performance.
 */
static void
orc_avx512_rule_shiftX (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src1_arg = insn->src_args[1];
  OrcVariable *src1 = c->vars + src1_arg;
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int type = ORC_PTR_TO_INT (user);

  OrcAVX512Insn opcodes[] = {
    ORC_AVX512_INSN_vpsllw_r_r_rm,
    ORC_AVX512_INSN_vpsrlw_r_r_rm,
    ORC_AVX512_INSN_vpsraw_r_r_rm,
    ORC_AVX512_INSN_vpslld_r_r_rm,
    ORC_AVX512_INSN_vpsrld_r_r_rm,
    ORC_AVX512_INSN_vpsrad_r_r_rm,
    ORC_AVX512_INSN_vpsllq_r_r_rm,
    ORC_AVX512_INSN_vpsrlq_r_r_rm,
    ORC_AVX512_INSN_vpsraq_r_r_rm
  };
  OrcAVX512Insn opcodes_imm[] = {
    ORC_AVX512_INSN_vpsllw_r_rm_i,
    ORC_AVX512_INSN_vpsrlw_r_rm_i,
    ORC_AVX512_INSN_vpsraw_r_rm_i,
    ORC_AVX512_INSN_vpslld_r_rm_i,
    ORC_AVX512_INSN_vpsrld_r_rm_i,
    ORC_AVX512_INSN_vpsrad_r_rm_i,
    ORC_AVX512_INSN_vpsllq_r_rm_i,
    ORC_AVX512_INSN_vpsrlq_r_rm_i,
    ORC_AVX512_INSN_vpsraq_r_rm_i
  };

  switch (src1->vartype) {
    case ORC_VAR_TYPE_CONST:
      orc_avx512_insn_emit_imm (c, opcodes_imm[type],
          ORC_X86_INSN_OPERAND_SIZE_NONE, src1->value.i, src0, ORC_REG_INVALID,
          dest, ORC_REG_INVALID, FALSE);
      break;

    case ORC_VAR_TYPE_PARAM:
      {
        int tmp;
        /* FIXME orccompiler.c has a bug that does not load params
         * automatically (through a loadp) when the orc opcode has the scalar
         * flag. We load it ourselves for now
         */
        tmp = orc_compiler_get_temp_reg (c);
        orc_avx512_insn_emit_mov_memoffset_reg (c, ORC_X86_INSN_OPERAND_SIZE_32,
            ORC_STRUCT_OFFSET (OrcExecutor, params[src1_arg]), c->exec_reg,
            tmp, FALSE);
        orc_avx512_insn_emit_avx512 (c, opcodes[type], src0,
            ORC_AVX512_SSE_REG (tmp), ORC_REG_INVALID, dest, ORC_REG_INVALID,
            FALSE);
        orc_compiler_release_temp_reg (c, tmp);
      }
      break;

    default:
      ORC_COMPILER_ERROR (c, "code generation rule for %s only works with "
          "constant or parameter shifts", insn->opcode->name);
      break;
  }
}

/* TODO
 * This one can be optimized by using VPERMB or VCOMPRESS8 and avoid
 * the shifts and the or
 */
static void
orc_avx512_rule_mullb (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  int tmp2 = orc_compiler_get_temp_reg (c);

  /* Keep src0 as it is dest too */
  orc_avx512_insn_emit_vmovdqa32_r_r (c, src0, tmp, ORC_REG_INVALID, FALSE);
  /* multiply the low byte */
  orc_avx512_insn_emit_vpmullw (c, src0, src1, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsllw_r_r_i (c, 8, dest, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsrlw_r_r_i (c, 8, dest, dest, ORC_REG_INVALID, FALSE);
  /* multiply the high byte */
  orc_avx512_insn_emit_vmovdqa32_r_r (c, src1, tmp2, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsraw_r_r_i (c, 8, tmp2, tmp2, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsraw_r_r_i (c, 8, tmp, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpmullw (c, tmp, tmp2, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsllw_r_r_i (c, 8, tmp, tmp, ORC_REG_INVALID, FALSE);
  /* or both */
  orc_avx512_insn_emit_vpord (c, dest, tmp, dest, ORC_REG_INVALID, FALSE);
  orc_compiler_release_temp_reg (c, tmp);
  orc_compiler_release_temp_reg (c, tmp2);
}

static void
orc_avx512_rule_mulsbw (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);

  /* AVX512 optimization: use vpmovsxbw to sign-extend bytes to words
   * This is more efficient than the SSE/AVX approach of unpacking and shifting
   * Note: Use AVX_REG (YMM/256-bit) as source to process 32 bytes -> 32 words
   */
  
  /* Sign-extend src0 (32 bytes -> 32 words in ZMM) */
  orc_avx512_insn_emit_vpmovsxbw (c, ORC_AVX512_AVX_REG (src0), dest,
      ORC_REG_INVALID, FALSE);
  
  /* Sign-extend src1 (32 bytes -> 32 words in ZMM) */
  orc_avx512_insn_emit_vpmovsxbw (c, ORC_AVX512_AVX_REG (src1), tmp,
      ORC_REG_INVALID, FALSE);
  
  /* Multiply signed words */
  orc_avx512_insn_emit_vpmullw (c, dest, tmp, dest, ORC_REG_INVALID, FALSE);
  
  orc_compiler_release_temp_reg (c, tmp);
}

static void
orc_avx512_rule_mulubw (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);

  /* AVX512 optimization: use vpmovzxbw to zero-extend bytes to words
   * This is more efficient than the SSE/AVX approach of unpacking and shifting
   * Note: Use AVX_REG (YMM/256-bit) as source to process 32 bytes -> 32 words
   */
  
  /* Zero-extend src0 (32 bytes -> 32 words in ZMM) */
  orc_avx512_insn_emit_vpmovzxbw (c, ORC_AVX512_AVX_REG (src0), dest,
      ORC_REG_INVALID, FALSE);
  
  /* Zero-extend src1 (32 bytes -> 32 words in ZMM) */
  orc_avx512_insn_emit_vpmovzxbw (c, ORC_AVX512_AVX_REG (src1), tmp,
      ORC_REG_INVALID, FALSE);
  
  /* Multiply unsigned words */
  orc_avx512_insn_emit_vpmullw (c, dest, tmp, dest, ORC_REG_INVALID, FALSE);
  
  orc_compiler_release_temp_reg (c, tmp);
}

static void
orc_avx512_rule_mulhsb (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  int tmp2 = orc_compiler_get_temp_reg (c);

  /* Similar to SSE: process even and odd bytes separately within each word
   * Even bytes: low byte of each word
   * Odd bytes: high byte of each word
   */

  /* Copy sources to work on them */
  orc_avx512_insn_emit_vmovdqa32_r_r (c, src1, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vmovdqa32_r_r (c, src0, tmp2, ORC_REG_INVALID, FALSE);

  /* Process even bytes (low byte of each word)
   * Shift left 8 to move to high byte position, then arithmetic shift right
   * to sign-extend */
  orc_avx512_insn_emit_vpsllw_r_r_i (c, 8, tmp, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsraw_r_r_i (c, 8, tmp, tmp, ORC_REG_INVALID, FALSE);

  orc_avx512_insn_emit_vpsllw_r_r_i (c, 8, src0, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsraw_r_r_i (c, 8, dest, dest, ORC_REG_INVALID, FALSE);

  /* Multiply even bytes */
  orc_avx512_insn_emit_vpmullw (c, tmp, dest, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsrlw_r_r_i (c, 8, dest, dest, ORC_REG_INVALID, FALSE);

  /* Process odd bytes (high byte of each word)
   * Arithmetic shift right 8 to sign-extend into word */
  orc_avx512_insn_emit_vmovdqa32_r_r (c, src1, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsraw_r_r_i (c, 8, tmp, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsraw_r_r_i (c, 8, tmp2, tmp2, ORC_REG_INVALID, FALSE);

  /* Multiply odd bytes */
  orc_avx512_insn_emit_vpmullw (c, tmp, tmp2, tmp2, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsrlw_r_r_i (c, 8, tmp2, tmp2, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsllw_r_r_i (c, 8, tmp2, tmp2, ORC_REG_INVALID, FALSE);

  /* Combine even and odd results */
  orc_avx512_insn_emit_vpord (c, tmp2, dest, dest, ORC_REG_INVALID, FALSE);

  orc_compiler_release_temp_reg (c, tmp);
  orc_compiler_release_temp_reg (c, tmp2);
}

static void
orc_avx512_rule_mulhub (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  int tmp2 = orc_compiler_get_temp_reg (c);

  /* Similar to SSE: process even and odd bytes separately within each word
   * Even bytes: low byte of each word
   * Odd bytes: high byte of each word
   */

  /* Copy sources to work on them */
  orc_avx512_insn_emit_vmovdqa32_r_r (c, src1, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vmovdqa32_r_r (c, src0, tmp2, ORC_REG_INVALID, FALSE);

  /* Process even bytes (low byte of each word)
   * Shift left 8 to move to high byte position, then logical shift right
   * to zero-extend */
  orc_avx512_insn_emit_vpsllw_r_r_i (c, 8, tmp, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsrlw_r_r_i (c, 8, tmp, tmp, ORC_REG_INVALID, FALSE);

  orc_avx512_insn_emit_vpsllw_r_r_i (c, 8, src0, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsrlw_r_r_i (c, 8, dest, dest, ORC_REG_INVALID, FALSE);

  /* Multiply even bytes */
  orc_avx512_insn_emit_vpmullw (c, tmp, dest, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsrlw_r_r_i (c, 8, dest, dest, ORC_REG_INVALID, FALSE);

  /* Process odd bytes (high byte of each word)
   * Logical shift right 8 to zero-extend into word */
  orc_avx512_insn_emit_vmovdqa32_r_r (c, src1, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsrlw_r_r_i (c, 8, tmp, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsrlw_r_r_i (c, 8, tmp2, tmp2, ORC_REG_INVALID, FALSE);

  /* Multiply odd bytes */
  orc_avx512_insn_emit_vpmullw (c, tmp, tmp2, tmp2, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsrlw_r_r_i (c, 8, tmp2, tmp2, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsllw_r_r_i (c, 8, tmp2, tmp2, ORC_REG_INVALID, FALSE);

  /* Combine even and odd results */
  orc_avx512_insn_emit_vpord (c, tmp2, dest, dest, ORC_REG_INVALID, FALSE);

  orc_compiler_release_temp_reg (c, tmp);
  orc_compiler_release_temp_reg (c, tmp2);
}

static void
orc_avx512_rule_mulhsl (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  int tmp2 = orc_compiler_get_temp_reg (c);

  /* Use vpmuldq to multiply signed 32-bit integers to 64-bit results
   * We need to process even and odd dwords separately, similar to SSE
   * vpmuldq multiplies elements at indices 0,2,4,6... (even dwords)
   * We shuffle to process odd dwords, then recombine
   */

  orc_avx512_insn_emit_vmovdqa32_r_r (c, src0, dest, ORC_REG_INVALID, FALSE);

  /* Shuffle to swap even/odd dwords: [0,1,2,3] -> [1,0,3,2] */
  orc_avx512_insn_emit_vpshufd (c, ORC_AVX512_SHUF(2,3,0,1), dest, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpshufd (c, ORC_AVX512_SHUF(2,3,0,1), src1, tmp2, ORC_REG_INVALID, FALSE);

  /* Multiply even dwords (0,2,4,...) -> 64-bit results */
  orc_avx512_insn_emit_vpmuldq (c, src1, dest, dest, ORC_REG_INVALID, FALSE);
  
  /* Multiply odd dwords (1,3,5,...) -> 64-bit results */
  orc_avx512_insn_emit_vpmuldq (c, tmp2, tmp, tmp2, ORC_REG_INVALID, FALSE);

  /* Extract high 32 bits from each 64-bit result and repack
   * Shuffle [low,HIGH,low,HIGH] -> [HIGH,low,HIGH,low] */
  orc_avx512_insn_emit_vpshufd (c, ORC_AVX512_SHUF(2,0,3,1), dest, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpshufd (c, ORC_AVX512_SHUF(2,0,3,1), tmp2, tmp2, ORC_REG_INVALID, FALSE);

  /* Interleave to get final result */
  orc_avx512_insn_emit_vpunpckldq (c, dest, tmp2, dest, ORC_REG_INVALID, FALSE);

  orc_compiler_release_temp_reg (c, tmp);
  orc_compiler_release_temp_reg (c, tmp2);
}

static void
orc_avx512_rule_mulhul (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  int tmp2 = orc_compiler_get_temp_reg (c);

  /* Use vpmuludq to multiply unsigned 32-bit integers to 64-bit results
   * Same algorithm as mulhsl but with unsigned multiply
   */

  orc_avx512_insn_emit_vmovdqa32_r_r (c, src0, dest, ORC_REG_INVALID, FALSE);

  /* Shuffle to swap even/odd dwords: [0,1,2,3] -> [1,0,3,2] */
  orc_avx512_insn_emit_vpshufd (c, ORC_AVX512_SHUF(2,3,0,1), dest, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpshufd (c, ORC_AVX512_SHUF(2,3,0,1), src1, tmp2, ORC_REG_INVALID, FALSE);

  /* Multiply even dwords (0,2,4,...) -> 64-bit results */
  orc_avx512_insn_emit_vpmuludq (c, src1, dest, dest, ORC_REG_INVALID, FALSE);
  
  /* Multiply odd dwords (1,3,5,...) -> 64-bit results */
  orc_avx512_insn_emit_vpmuludq (c, tmp2, tmp, tmp2, ORC_REG_INVALID, FALSE);

  /* Extract high 32 bits from each 64-bit result and repack */
  orc_avx512_insn_emit_vpshufd (c, ORC_AVX512_SHUF(2,0,3,1), dest, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpshufd (c, ORC_AVX512_SHUF(2,0,3,1), tmp2, tmp2, ORC_REG_INVALID, FALSE);

  /* Interleave to get final result */
  orc_avx512_insn_emit_vpunpckldq (c, dest, tmp2, dest, ORC_REG_INVALID, FALSE);

  orc_compiler_release_temp_reg (c, tmp);
  orc_compiler_release_temp_reg (c, tmp2);
}

static void
orc_avx512_rule_avgsl (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  int size = c->vars[insn->dest_args[0]].size << c->loop_shift;

  /* Compute (a+b+1)>>1 using: (a|b) - ((a^b)>>1)
   * This avoids overflow issues in the addition
   */

  if (size >= 64) {
    /* ZMM (512-bit) */
    orc_avx512_insn_emit_vpxord (c, src0, src1, tmp, ORC_REG_INVALID, FALSE);
    orc_avx512_insn_emit_vpsrad_r_r_i (c, 1, tmp, tmp, ORC_REG_INVALID, FALSE);
    orc_avx512_insn_emit_vpord (c, src0, src1, dest, ORC_REG_INVALID, FALSE);
    orc_avx512_insn_emit_vpsubd (c, dest, tmp, dest, ORC_REG_INVALID, FALSE);
  } else if (size >= 32) {
    /* YMM (256-bit) */
    orc_avx512_insn_avx_emit_vpxord (c, ORC_AVX512_AVX_REG(src0), ORC_AVX512_AVX_REG(src1), 
                                      ORC_AVX512_AVX_REG(tmp), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_avx_emit_vpsrad_r_r_i (c, 1, ORC_AVX512_AVX_REG(tmp), 
                                             ORC_AVX512_AVX_REG(tmp), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_avx_emit_vpord (c, ORC_AVX512_AVX_REG(src0), ORC_AVX512_AVX_REG(src1), 
                                     ORC_AVX512_AVX_REG(dest), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_avx_emit_vpsubd (c, ORC_AVX512_AVX_REG(dest), ORC_AVX512_AVX_REG(tmp), 
                                      ORC_AVX512_AVX_REG(dest), ORC_REG_INVALID, FALSE);
  } else {
    /* XMM (128-bit) */
    orc_avx512_insn_sse_emit_vpxord (c, ORC_AVX512_SSE_REG(src0), ORC_AVX512_SSE_REG(src1), 
                                      ORC_AVX512_SSE_REG(tmp), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpsrad_r_r_i (c, 1, ORC_AVX512_SSE_REG(tmp), 
                                             ORC_AVX512_SSE_REG(tmp), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpord (c, ORC_AVX512_SSE_REG(src0), ORC_AVX512_SSE_REG(src1), 
                                     ORC_AVX512_SSE_REG(dest), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpsubd (c, ORC_AVX512_SSE_REG(dest), ORC_AVX512_SSE_REG(tmp), 
                                      ORC_AVX512_SSE_REG(dest), ORC_REG_INVALID, FALSE);
  }

  orc_compiler_release_temp_reg (c, tmp);
}

static void
orc_avx512_rule_avgul (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  int size = c->vars[insn->dest_args[0]].size << c->loop_shift;

  /* Compute (a+b+1)>>1 using: (a|b) - ((a^b)>>1)
   * This avoids overflow issues in the addition
   */

  if (size >= 64) {
    /* ZMM (512-bit) */
    orc_avx512_insn_emit_vpxord (c, src0, src1, tmp, ORC_REG_INVALID, FALSE);
    orc_avx512_insn_emit_vpsrld_r_r_i (c, 1, tmp, tmp, ORC_REG_INVALID, FALSE);
    orc_avx512_insn_emit_vpord (c, src0, src1, dest, ORC_REG_INVALID, FALSE);
    orc_avx512_insn_emit_vpsubd (c, dest, tmp, dest, ORC_REG_INVALID, FALSE);
  } else if (size >= 32) {
    /* YMM (256-bit) */
    orc_avx512_insn_avx_emit_vpxord (c, ORC_AVX512_AVX_REG(src0), ORC_AVX512_AVX_REG(src1), 
                                      ORC_AVX512_AVX_REG(tmp), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_avx_emit_vpsrld_r_r_i (c, 1, ORC_AVX512_AVX_REG(tmp), 
                                             ORC_AVX512_AVX_REG(tmp), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_avx_emit_vpord (c, ORC_AVX512_AVX_REG(src0), ORC_AVX512_AVX_REG(src1), 
                                     ORC_AVX512_AVX_REG(dest), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_avx_emit_vpsubd (c, ORC_AVX512_AVX_REG(dest), ORC_AVX512_AVX_REG(tmp), 
                                      ORC_AVX512_AVX_REG(dest), ORC_REG_INVALID, FALSE);
  } else {
    /* XMM (128-bit) */
    orc_avx512_insn_sse_emit_vpxord (c, ORC_AVX512_SSE_REG(src0), ORC_AVX512_SSE_REG(src1), 
                                      ORC_AVX512_SSE_REG(tmp), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpsrld_r_r_i (c, 1, ORC_AVX512_SSE_REG(tmp), 
                                             ORC_AVX512_SSE_REG(tmp), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpord (c, ORC_AVX512_SSE_REG(src0), ORC_AVX512_SSE_REG(src1), 
                                     ORC_AVX512_SSE_REG(dest), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpsubd (c, ORC_AVX512_SSE_REG(dest), ORC_AVX512_SSE_REG(tmp), 
                                      ORC_AVX512_SSE_REG(dest), ORC_REG_INVALID, FALSE);
  }

  orc_compiler_release_temp_reg (c, tmp);
}

static void
orc_avx512_rule_addusl (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  int tmp2 = orc_compiler_get_temp_reg (c);

  /* Unsigned saturating add: clamp(a + b) to 0xFFFFFFFF
   * Algorithm: tmp = (a & b) + ((a ^ b) >> 1)
   *            if tmp has sign bit set, overflow occurred
   *            dest = a + b
   *            if overflow, dest = 0xFFFFFFFF
   */

  /* tmp = src0 & src1 */
  orc_avx512_insn_emit_vpandd (c, src0, src1, tmp, ORC_REG_INVALID, FALSE);

  /* tmp2 = src0 ^ src1, then shift right by 1 */
  orc_avx512_insn_emit_vpxord (c, src0, src1, tmp2, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsrld_r_r_i (c, 1, tmp2, tmp2, ORC_REG_INVALID, FALSE);
  
  /* tmp = tmp + tmp2 (overflow detection) */
  orc_avx512_insn_emit_vpaddd (c, tmp, tmp2, tmp, ORC_REG_INVALID, FALSE);

  /* tmp = arithmetic shift right by 31 (creates mask: 0xFFFFFFFF if overflow) */
  orc_avx512_insn_emit_vpsrad_r_r_i (c, 31, tmp, tmp, ORC_REG_INVALID, FALSE);

  /* dest = src0 + src1 */
  orc_avx512_insn_emit_vpaddd (c, src0, src1, dest, ORC_REG_INVALID, FALSE);

  /* dest = dest | tmp (saturate to 0xFFFFFFFF if overflow) */
  orc_avx512_insn_emit_vpord (c, dest, tmp, dest, ORC_REG_INVALID, FALSE);

  orc_compiler_release_temp_reg (c, tmp);
  orc_compiler_release_temp_reg (c, tmp2);
}

static void
orc_avx512_rule_addssl (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int s = orc_compiler_get_temp_reg (c);
  int t = orc_compiler_get_temp_reg (c);

  /* Signed saturating add using Tim Terriberry's algorithm:
   * m=0xFFFFFFFF; s=a; t=a; s^=b; a+=b; t^=a; t^=m; m>>=1;
   * s|=t; t=b; s>>=31; t>>=31; a&=s; t^=m; s=~s&t; a|=s;
   */

  /* s = src0, t = src0 */
  orc_avx512_insn_emit_vmovdqa32_r_r (c, src0, s, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vmovdqa32_r_r (c, src0, t, ORC_REG_INVALID, FALSE);

  /* s = s ^ src1 */
  orc_avx512_insn_emit_vpxord (c, s, src1, s, ORC_REG_INVALID, FALSE);

  /* dest = src0 + src1 */
  orc_avx512_insn_emit_vpaddd (c, src0, src1, dest, ORC_REG_INVALID, FALSE);

  /* t = t ^ dest */
  orc_avx512_insn_emit_vpxord (c, t, dest, t, ORC_REG_INVALID, FALSE);

  /* t = t ^ 0xFFFFFFFF */
  int tmp_const = orc_compiler_get_constant (c, 4, 0xffffffff);
  orc_avx512_insn_emit_vpxord (c, t, tmp_const, t, ORC_REG_INVALID, FALSE);

  /* s = s | t */
  orc_avx512_insn_emit_vpord (c, s, t, s, ORC_REG_INVALID, FALSE);

  /* s = s >> 31 (arithmetic) */
  orc_avx512_insn_emit_vpsrad_r_r_i (c, 31, s, s, ORC_REG_INVALID, FALSE);

  /* t = src1 >> 31 (arithmetic) */
  orc_avx512_insn_emit_vpsrad_r_r_i (c, 31, src1, t, ORC_REG_INVALID, FALSE);

  /* dest = dest & s */
  orc_avx512_insn_emit_vpandd (c, dest, s, dest, ORC_REG_INVALID, FALSE);

  /* t = t ^ 0x7FFFFFFF */
  tmp_const = orc_compiler_get_constant (c, 4, 0x7fffffff);
  orc_avx512_insn_emit_vpxord (c, t, tmp_const, t, ORC_REG_INVALID, FALSE);

  /* s = ~s & t (use andnot: vpandnd computes ~s0 & s1) */
  orc_avx512_insn_emit_vpandnd (c, s, t, s, ORC_REG_INVALID, FALSE);

  /* dest = dest | s */
  orc_avx512_insn_emit_vpord (c, dest, s, dest, ORC_REG_INVALID, FALSE);

  orc_compiler_release_temp_reg (c, s);
  orc_compiler_release_temp_reg (c, t);
}

static void
orc_avx512_rule_subusl (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  int tmp2 = orc_compiler_get_temp_reg (c);

  /* Unsigned saturating subtract: clamp(a - b) to 0
   * Algorithm: shift both by 1, subtract, check sign bit
   *            if negative, result = 0
   */

  /* tmp2 = src1 >> 1 */
  orc_avx512_insn_emit_vpsrld_r_r_i (c, 1, src1, tmp2, ORC_REG_INVALID, FALSE);

  /* tmp = src0 >> 1 */
  orc_avx512_insn_emit_vpsrld_r_r_i (c, 1, src0, tmp, ORC_REG_INVALID, FALSE);
  
  /* tmp2 = tmp - tmp2 (detect underflow) */
  orc_avx512_insn_emit_vpsubd (c, tmp2, tmp, tmp2, ORC_REG_INVALID, FALSE);

  /* tmp2 = arithmetic shift right by 31 (creates mask: 0xFFFFFFFF if no underflow, 0 if underflow) */
  orc_avx512_insn_emit_vpsrad_r_r_i (c, 31, tmp2, tmp2, ORC_REG_INVALID, FALSE);

  /* dest = src0 - src1 */
  orc_avx512_insn_emit_vpsubd (c, src0, src1, dest, ORC_REG_INVALID, FALSE);

  /* dest = dest & tmp2 (clamp to 0 if underflow) */
  orc_avx512_insn_emit_vpandd (c, dest, tmp2, dest, ORC_REG_INVALID, FALSE);

  orc_compiler_release_temp_reg (c, tmp);
  orc_compiler_release_temp_reg (c, tmp2);
}

static void
orc_avx512_rule_subssl (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_constant (c, 4, 0xffffffff);
  int tmp2 = orc_compiler_get_temp_reg (c);
  int tmp3 = orc_compiler_get_temp_reg (c);

  /* Signed saturating subtract using similar algorithm to addssl */

  /* tmp = 0xFFFFFFFF ^ src1 */
  orc_avx512_insn_emit_vpxord (c, tmp, src1, tmp, ORC_REG_INVALID, FALSE);
  
  /* tmp2 = tmp ^ src0 */
  orc_avx512_insn_emit_vpxord (c, tmp, src0, tmp2, ORC_REG_INVALID, FALSE);
  
  /* tmp = tmp | src0 */
  orc_avx512_insn_emit_vpord (c, tmp, src0, tmp, ORC_REG_INVALID, FALSE);
  
  /* tmp2 = tmp2 >> 1 (arithmetic) */
  orc_avx512_insn_emit_vpsrad_r_r_i (c, 1, tmp2, tmp2, ORC_REG_INVALID, FALSE);
  
  /* tmp = tmp - tmp2 */
  orc_avx512_insn_emit_vpsubd (c, tmp, tmp2, tmp, ORC_REG_INVALID, FALSE);

  /* tmp = tmp >> 30 then << 30 (isolate overflow bits) */
  orc_avx512_insn_emit_vpsrad_r_r_i (c, 30, tmp, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpslld_r_r_i (c, 30, tmp, tmp, ORC_REG_INVALID, FALSE);
  
  /* tmp2 = tmp << 1 */
  orc_avx512_insn_emit_vpslld_r_r_i (c, 1, tmp, tmp2, ORC_REG_INVALID, FALSE);
  
  /* tmp3 = tmp ^ tmp2 */
  orc_avx512_insn_emit_vpxord (c, tmp, tmp2, tmp3, ORC_REG_INVALID, FALSE);
  
  /* tmp3 = tmp3 >> 31 (create mask) */
  orc_avx512_insn_emit_vpsrad_r_r_i (c, 31, tmp3, tmp3, ORC_REG_INVALID, FALSE);

  /* tmp2 = tmp2 >> 31 */
  orc_avx512_insn_emit_vpsrad_r_r_i (c, 31, tmp2, tmp2, ORC_REG_INVALID, FALSE);
  
  /* tmp2 = tmp2 ^ 0x80000000 (clamped value) */
  tmp = orc_compiler_get_constant (c, 4, 0x80000000);
  orc_avx512_insn_emit_vpxord (c, tmp2, tmp, tmp2, ORC_REG_INVALID, FALSE);
  
  /* tmp2 = tmp2 & tmp3 */
  orc_avx512_insn_emit_vpandd (c, tmp2, tmp3, tmp2, ORC_REG_INVALID, FALSE);

  /* dest = src0 - src1 */
  orc_avx512_insn_emit_vpsubd (c, src0, src1, dest, ORC_REG_INVALID, FALSE);
  
  /* dest = ~tmp3 & dest (use andnot) */
  orc_avx512_insn_emit_vpandnd (c, tmp3, dest, dest, ORC_REG_INVALID, FALSE);

  /* dest = dest | tmp2 */
  orc_avx512_insn_emit_vpord (c, dest, tmp2, dest, ORC_REG_INVALID, FALSE);

  orc_compiler_release_temp_reg (c, tmp2);
  orc_compiler_release_temp_reg (c, tmp3);
}

/* Generic min/max for minf, mind, maxd - uses ZMM-only
 * NOTE: Do NOT make this adaptive. Testing showed:
 *   - Making all min/max adaptive: 63/173 programs slower (worse)
 *   - Keeping only maxf adaptive: 59/173 programs slower (best)
 *   - ZMM-only for minf/mind/maxd: necessary for optimal program-level performance
 * Reason: Simple single-instruction operations don't benefit from adaptive approach.
 * Only maxf has a separate adaptive implementation for better overall performance.
 */
static void
orc_avx512_rule_minX_maxX (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int type = ORC_PTR_TO_INT (user);

  OrcAVX512Insn opcodes[] = {
    ORC_AVX512_INSN_vminps,
    ORC_AVX512_INSN_vmaxps,
    ORC_AVX512_INSN_vminpd,
    ORC_AVX512_INSN_vmaxpd
  };

  if (c->target_flags & ORC_TARGET_FAST_NAN) {
    orc_avx512_insn_emit_avx512 (c, opcodes[type], src0, src1, ORC_REG_INVALID,
        dest, ORC_REG_INVALID, FALSE);
  } else {
    int tmp = orc_compiler_get_temp_reg (c);
    orc_avx512_insn_emit_avx512 (c, opcodes[type], src0, src1, ORC_REG_INVALID,
        tmp, ORC_REG_INVALID, FALSE);
    orc_avx512_insn_emit_avx512 (c, opcodes[type], src1, src0, ORC_REG_INVALID,
        dest, ORC_REG_INVALID, FALSE);
    orc_avx512_insn_emit_vorps (c, dest, tmp, dest, ORC_REG_INVALID, FALSE);
    orc_compiler_release_temp_reg (c, tmp);
  }
}

/* Adaptive maxf - best program-level performance (55/173 slower vs 60/173 with ZMM-only) */
static void
orc_avx512_rule_maxf (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int size = c->vars[insn->dest_args[0]].size << c->loop_shift;

  OrcAVX512Insn opcodes[] = {
    ORC_AVX512_INSN_vmaxps,
    ORC_AVX512_INSN_AVX_vmaxps,
    ORC_AVX512_INSN_SSE_vmaxps
  };

  int opcode_idx = (size >= 64) ? 0 : (size >= 32) ? 1 : 2;

  if (c->target_flags & ORC_TARGET_FAST_NAN) {
    if (size >= 64) {
      orc_avx512_insn_emit_avx512 (c, opcodes[opcode_idx], src0, src1, ORC_REG_INVALID,
          dest, ORC_REG_INVALID, FALSE);
    } else if (size >= 32) {
      orc_avx512_insn_emit_avx512 (c, opcodes[opcode_idx], ORC_AVX512_AVX_REG(src0),
          ORC_AVX512_AVX_REG(src1), ORC_REG_INVALID, ORC_AVX512_AVX_REG(dest),
          ORC_REG_INVALID, FALSE);
    } else {
      orc_avx512_insn_emit_avx512 (c, opcodes[opcode_idx], ORC_AVX512_SSE_REG(src0),
          ORC_AVX512_SSE_REG(src1), ORC_REG_INVALID, ORC_AVX512_SSE_REG(dest),
          ORC_REG_INVALID, FALSE);
    }
  } else {
    int tmp = orc_compiler_get_temp_reg (c);
    if (size >= 64) {
      orc_avx512_insn_emit_avx512 (c, opcodes[opcode_idx], src0, src1, ORC_REG_INVALID,
          tmp, ORC_REG_INVALID, FALSE);
      orc_avx512_insn_emit_avx512 (c, opcodes[opcode_idx], src1, src0, ORC_REG_INVALID,
          dest, ORC_REG_INVALID, FALSE);
      orc_avx512_insn_emit_vorps (c, dest, tmp, dest, ORC_REG_INVALID, FALSE);
    } else if (size >= 32) {
      orc_avx512_insn_emit_avx512 (c, opcodes[opcode_idx], ORC_AVX512_AVX_REG(src0),
          ORC_AVX512_AVX_REG(src1), ORC_REG_INVALID, ORC_AVX512_AVX_REG(tmp),
          ORC_REG_INVALID, FALSE);
      orc_avx512_insn_emit_avx512 (c, opcodes[opcode_idx], ORC_AVX512_AVX_REG(src1),
          ORC_AVX512_AVX_REG(src0), ORC_REG_INVALID, ORC_AVX512_AVX_REG(dest),
          ORC_REG_INVALID, FALSE);
      orc_avx512_insn_avx_emit_vorps (c, ORC_AVX512_AVX_REG(dest), ORC_AVX512_AVX_REG(tmp),
          ORC_AVX512_AVX_REG(dest), ORC_REG_INVALID, FALSE);
    } else {
      orc_avx512_insn_emit_avx512 (c, opcodes[opcode_idx], ORC_AVX512_SSE_REG(src0),
          ORC_AVX512_SSE_REG(src1), ORC_REG_INVALID, ORC_AVX512_SSE_REG(tmp),
          ORC_REG_INVALID, FALSE);
      orc_avx512_insn_emit_avx512 (c, opcodes[opcode_idx], ORC_AVX512_SSE_REG(src1),
          ORC_AVX512_SSE_REG(src0), ORC_REG_INVALID, ORC_AVX512_SSE_REG(dest),
          ORC_REG_INVALID, FALSE);
      orc_avx512_insn_sse_emit_vorps (c, ORC_AVX512_SSE_REG(dest), ORC_AVX512_SSE_REG(tmp),
          ORC_AVX512_SSE_REG(dest), ORC_REG_INVALID, FALSE);
    }
    orc_compiler_release_temp_reg (c, tmp);
  }
}

static void
orc_avx512_rule_cmpX (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int type = ORC_PTR_TO_INT (user);
  int k = orc_avx512_compiler_get_mask_reg (c, FALSE);

  OrcAVX512Insn opcodes[] = {
    /* float */
    ORC_AVX512_INSN_vcmpeqps,
    ORC_AVX512_INSN_vcmpltps,
    ORC_AVX512_INSN_vcmpleps,
    /* double */
    ORC_AVX512_INSN_vcmpeqpd,
    ORC_AVX512_INSN_vcmpltpd,
    ORC_AVX512_INSN_vcmplepd,
    /* byte */
    ORC_AVX512_INSN_vpcmpeqb,
    ORC_AVX512_INSN_vpcmpgtb,
    /* word */
    ORC_AVX512_INSN_vpcmpeqw,
    ORC_AVX512_INSN_vpcmpgtw,
    /* double-word */
    ORC_AVX512_INSN_vpcmpeqd,
    ORC_AVX512_INSN_vpcmpgtd,
    /* quad-word */
    ORC_AVX512_INSN_vpcmpeqq,
    ORC_AVX512_INSN_vpcmpgtq,
  };

  OrcAVX512Insn opcodes_movm[] = {
    /* float */
    ORC_AVX512_INSN_vpmovm2d,
    ORC_AVX512_INSN_vpmovm2d,
    ORC_AVX512_INSN_vpmovm2d,
    /* double */
    ORC_AVX512_INSN_vpmovm2q,
    ORC_AVX512_INSN_vpmovm2q,
    ORC_AVX512_INSN_vpmovm2q,
    /* byte */
    ORC_AVX512_INSN_vpmovm2b,
    ORC_AVX512_INSN_vpmovm2b,
    /* word */
    ORC_AVX512_INSN_vpmovm2w,
    ORC_AVX512_INSN_vpmovm2w,
    /* double-word */
    ORC_AVX512_INSN_vpmovm2d,
    ORC_AVX512_INSN_vpmovm2d,
    /* quad-word */
    ORC_AVX512_INSN_vpmovm2q,
    ORC_AVX512_INSN_vpmovm2q,
  };

  /* The comparison instructions store the result on a mask register */
  orc_avx512_insn_emit_avx512 (c, opcodes[type], src0, src1, ORC_REG_INVALID,
      k, ORC_REG_INVALID, FALSE);
  /* Move the mask register value to a AVX512 register */
  orc_avx512_insn_emit_avx512 (c, opcodes_movm[type], k, ORC_REG_INVALID,
      ORC_REG_INVALID, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_compiler_release_mask_reg (c, k);
}

/* For positive numbers and indefinite conversion, INT32_MAX */
static void
orc_avx512_rule_convfl (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  /* Indefinite integer value 2^(32-1) if can not be represented */
  OrcConstant ind_c = ORC_CONSTANT_INIT_U32 (0x80000000);
  int src = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int ind = orc_compiler_get_constant_full (c, &ind_c);
  int tmp1 = orc_compiler_get_temp_reg (c);
  int tmp2 = orc_compiler_get_temp_reg (c);
  int mask = orc_avx512_compiler_get_mask_reg (c, TRUE);

  orc_avx512_insn_emit_vpsrad_r_r_i (c, 31, src, tmp1, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vcvttps2dq (c, src, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpcmpeqd (c, ind, dest, mask, ORC_REG_INVALID);
  orc_avx512_insn_emit_vpmovm2d (c, mask, tmp2);
  orc_avx512_insn_emit_vpandnd (c, tmp1, tmp2, tmp1, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpaddd (c, dest, tmp1, dest, ORC_REG_INVALID, FALSE);

  orc_compiler_release_temp_reg (c, tmp1);
  orc_compiler_release_temp_reg (c, tmp2);
  orc_avx512_compiler_release_mask_reg (c, mask);
}

/* Handle saturation on indefinite values (out of range doubles) */
static void
orc_avx512_rule_convdl (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  /* Indefinite integer value 2^31 (0x80000000) returned on overflow/NaN */
  OrcConstant ind_c = ORC_CONSTANT_INIT_U32 (0x80000000);
  int src = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int ind = orc_compiler_get_constant_full (c, &ind_c);
  int tmp1 = orc_compiler_get_temp_reg (c);
  int tmp2 = orc_compiler_get_temp_reg (c);
  int mask = orc_avx512_compiler_get_mask_reg (c, TRUE);

  /* Extract sign bits from source doubles (qwords) */
  orc_avx512_insn_emit_vpsraq_r_r_i (c, 63, src, tmp1, ORC_REG_INVALID, FALSE);
  /* Convert doubles to int32 with truncation - narrows ZMM->YMM */
  orc_avx512_insn_emit_vcvttpd2dq (c, src, dest, ORC_REG_INVALID, FALSE);
  /* Narrow the sign bits qwords->dwords to match result width */
  orc_avx512_insn_emit_vpmovqd (c, tmp1, tmp1, ORC_REG_INVALID, FALSE);
  /* Compare result with 0x80000000 (indefinite) */
  orc_avx512_insn_emit_vpcmpeqd (c, ind, dest, mask, ORC_REG_INVALID);
  /* Convert mask to vector */
  orc_avx512_insn_emit_vpmovm2d (c, mask, tmp2);
  /* AND-NOT: keep only positive overflows (sign=0 and result=0x80000000) */
  orc_avx512_insn_emit_vpandnd (c, tmp1, tmp2, tmp1, ORC_REG_INVALID, FALSE);
  /* Add correction: 0x80000000 + 0xFFFFFFFF = 0x7FFFFFFF (INT32_MAX) */
  orc_avx512_insn_emit_vpaddd (c, dest, tmp1, dest, ORC_REG_INVALID, FALSE);

  orc_compiler_release_temp_reg (c, tmp1);
  orc_compiler_release_temp_reg (c, tmp2);
  orc_avx512_compiler_release_mask_reg (c, mask);
}

static void
orc_avx512_rule_convwf (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);

  orc_avx512_insn_emit_vpmovsxwd (c, ORC_AVX512_AVX_REG (src), dest,
      ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vcvtdq2ps (c, dest, dest, ORC_REG_INVALID, FALSE);
}

#if 0
static void
orc_avx512_rule_mergeXX (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int size = ORC_SIZE (c, ORC_SRC_SIZE (c, insn, 0));
  int type = ORC_PTR_TO_INT (user);
  OrcAVX512Insn opcodes[][4] = {
    {
      ORC_AVX512_INSN_AVX_vpunpckhbw,
      ORC_AVX512_INSN_AVX_vpunpcklbw,
      ORC_AVX512_INSN_SSE_vpunpckhbw,
      ORC_AVX512_INSN_SSE_vpunpcklbw,
    },
    {
      ORC_AVX512_INSN_AVX_vpunpckhwd,
      ORC_AVX512_INSN_AVX_vpunpcklwd,
      ORC_AVX512_INSN_SSE_vpunpckhwd,
      ORC_AVX512_INSN_SSE_vpunpcklwd,
    }
  }; 

  switch (size) {
    case 32:
      {
        /*
         * src0  [X, X, X, X, A, B, C, D]
         * src1  [X, X, X, X, a, b, c, d]
         * tmp   [   X,    X,  IaA,  IcC] vpunpckhbw(src0, src1, tmp)
         * dest  [   X,    X,  IbB,  IdD] vpunpcklbw(src0, src1, dest)
         * permr [   1,    5,    0,    4] vpmovzxbq(permr, permr)
         * dest  [ IaA,  IbB,  IcC,  IdD] vpermt2q(permr, tmp, dest)
         */
        int tmp = orc_compiler_get_temp_reg (c);
        int permr = orc_compiler_get_temp_reg (c);
        orc_uint64 perm = ORC_AVX512_PERMQ (11, 10, 3, 2, 9, 8, 1, 0);

        orc_avx512_insn_emit_avx512 (c, opcodes[type][0], ORC_AVX512_AVX_REG (src0),
            ORC_AVX512_AVX_REG (src1), ORC_REG_INVALID, ORC_AVX512_AVX_REG (tmp),
            ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_avx512 (c, opcodes[type][1], ORC_AVX512_AVX_REG (src0),
            ORC_AVX512_AVX_REG (src1), ORC_REG_INVALID, ORC_AVX512_AVX_REG (dest),
            ORC_REG_INVALID, FALSE);
        /* FIXME this should be a constant and properly defined to be expanded
         * to AVX512
         */
        orc_x86_emit_mov_imm_reg64 (c, 8, perm, c->gp_tmpreg);
        orc_avx512_insn_emit_vmovq (c, c->gp_tmpreg, ORC_AVX512_SSE_REG (permr)); 

        /* permute both intermediate results based on the permr indexes */
        orc_avx512_insn_emit_vpmovzxbq (c, ORC_AVX512_SSE_REG (permr), permr,
            ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vpermt2q (c, permr, tmp, dest, ORC_REG_INVALID,
            FALSE);
        orc_compiler_release_temp_reg (c, tmp);
        orc_compiler_release_temp_reg (c, permr);
      }
      break;

    case 16:
      {
        /* src0        [A, B]
         * src1        [a, b]
         * tmp         [ IaA] vpunpckhbw(src0, src1, tmp)
         * dest        [ IbB] vpunpcklbw(src0, src1, dest)
         * dest  [ IaA,  IbB] vinserti32x4(1, dest, tmp, dest)
         */
        int tmp = orc_compiler_get_temp_reg (c);
        orc_avx512_insn_emit_avx512 (c, opcodes[type][2],
            ORC_AVX512_SSE_REG (src0), ORC_AVX512_SSE_REG (src1),
            ORC_REG_INVALID, ORC_AVX512_SSE_REG (tmp), ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_avx512 (c, opcodes[type][3],
            ORC_AVX512_SSE_REG (src0), ORC_AVX512_SSE_REG (src1),
            ORC_REG_INVALID, ORC_AVX512_SSE_REG (dest), ORC_REG_INVALID,
            FALSE);
        /* only two lanes, no need to permute */
        orc_avx512_insn_emit_avx_vinserti32x4 (c, 1, dest,
            ORC_AVX512_SSE_REG (tmp), dest, ORC_REG_INVALID, FALSE);
        orc_compiler_release_temp_reg (c, tmp);
      }
      break;
 
    default:
      /* Simplest case, only low half is needed */
      orc_avx512_insn_emit_avx512 (c, opcodes[type][3], ORC_AVX512_SSE_REG (src0),
          ORC_AVX512_SSE_REG (src1), ORC_REG_INVALID,
          ORC_AVX512_SSE_REG (dest), ORC_REG_INVALID, FALSE);
      break;
  }
}
#endif

static void
orc_avx512_rule_mergebw (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_bw = ORC_CONSTANT_INIT_U512 (
      0x4303420241014000UL,
      0x4707460645054404UL,
      0x4b0b4a0a49094808UL,
      0x4f0f4e0e4d0d4c0cUL,
      0x5313521251115010UL,
      0x5717561655155414UL,
      0x5b1b5a1a59195818UL,
      0x5f1f5e1e5d1d5c1cUL
  );
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int bw = orc_compiler_get_constant_full (c, &perm_bw);

  orc_avx512_insn_emit_vmovdqa32_r_r (c, src0, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpermt2b (c, bw, src1, dest, ORC_REG_INVALID,
      FALSE);
}

static void
orc_avx512_rule_mergewl (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_wl = ORC_CONSTANT_INIT_U512 (
      0x0021000100200000UL,
      0x0023000300220002UL,
      0x0025000500240004UL,
      0x0027000700260006UL,
      0x0029000900280008UL,
      0x002b000b002a000aUL,
      0x002d000d002c000cUL,
      0x002f000f002e000eUL
  );
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int wl = orc_compiler_get_constant_full (c, &perm_wl);

  orc_avx512_insn_emit_vmovdqa32_r_r (c, src0, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpermt2w (c, wl, src1, dest, ORC_REG_INVALID,
      FALSE);
}

static void
orc_avx512_rule_mergelq (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_lq = ORC_CONSTANT_INIT_U512 (
      0x0000001000000000UL,
      0x0000001100000001UL,
      0x0000001200000002UL,
      0x0000001300000003UL,
      0x0000001400000004UL,
      0x0000001500000005UL,
      0x0000001600000006UL,
      0x0000001700000007UL
  );
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int lq = orc_compiler_get_constant_full (c, &perm_lq);

  orc_avx512_insn_emit_vmovdqa32_r_r (c, src0, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpermt2d (c, lq, src1, dest, ORC_REG_INVALID,
      FALSE);
}

/* AVX512 does not have a psign[b,w,d] anymore, so this one is slow
 * Check https://lemire.me/blog/2024/01/11/implementing-the-missing-sign-instruction-in-avx-512/
 * TODO we can simplify the above algorithm if this should only work for 0,-1,1
 */
static void
orc_avx512_rule_signX (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int type = ORC_PTR_TO_INT (user);
  int klt = orc_avx512_compiler_get_mask_reg (c, TRUE);
  int kle = orc_avx512_compiler_get_mask_reg (c, TRUE);
  int zero = orc_compiler_get_temp_reg (c);
  int tmpc = orc_compiler_get_temp_constant (c, 1 << type, 1);
  OrcAVX512Insn opcodes[][4] = {
    {
      ORC_AVX512_INSN_vpmovb2m,
      ORC_AVX512_INSN_vpcmpleb,
      ORC_AVX512_INSN_vpblendmb,
      ORC_AVX512_INSN_vpsubb,
    },
    {
      ORC_AVX512_INSN_vpmovw2m,
      ORC_AVX512_INSN_vpcmplew,
      ORC_AVX512_INSN_vpblendmw,
      ORC_AVX512_INSN_vpsubw,
    },
    {
      ORC_AVX512_INSN_vpmovd2m,
      ORC_AVX512_INSN_vpcmpled,
      ORC_AVX512_INSN_vpblendmd,
      ORC_AVX512_INSN_vpsubd,
    },
  };

  orc_avx512_insn_emit_vpxord (c, zero, zero, zero, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_avx512 (c, opcodes[type][0], src0, ORC_REG_INVALID, ORC_REG_INVALID, klt, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_avx512 (c, opcodes[type][1], src0, zero, ORC_REG_INVALID, kle, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_avx512 (c, opcodes[type][2], tmpc, zero, ORC_REG_INVALID, dest, klt, FALSE);
  orc_avx512_insn_emit_avx512 (c, opcodes[type][3], dest, tmpc, ORC_REG_INVALID, dest, kle, FALSE);

  orc_compiler_release_temp_reg (c, zero);
  orc_avx512_compiler_release_mask_reg (c, klt);
  orc_avx512_compiler_release_mask_reg (c, kle);
}

static void
orc_avx512_rule_splitql (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_even = ORC_CONSTANT_INIT_U256 (
      0x0000000200000000UL,
      0x0000000600000004UL,
      0x0000000a00000008UL,
      0x0000000e0000000cUL
  );
  OrcConstant perm_odd = ORC_CONSTANT_INIT_U256 (
      0x0000000300000001UL,
      0x0000000700000005UL,
      0x0000000b00000009UL,
      0x0000000f0000000dUL
  );

  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int dest1 = ORC_DEST_ARG (c, insn, 1);
  int even = orc_compiler_get_constant_full (c, &perm_even);
  int odd = orc_compiler_get_constant_full (c, &perm_odd);
  int k = orc_avx512_compiler_get_mask_reg (c, TRUE);

  orc_avx512_insn_emit_kxnorw (c, k, k, k);
  orc_avx512_insn_emit_kshiftrw (c, 8, k, k);
 
  orc_avx512_insn_emit_vpermd (c, odd, src0, dest0, k, TRUE);
  orc_avx512_insn_emit_vpermd (c, even, src0, dest1, k, TRUE);
  orc_avx512_compiler_release_mask_reg (c, k);
}

static void
orc_avx512_rule_splitlw (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_even = ORC_CONSTANT_INIT_U256 (
      0x0006000400020000UL,
      0x000e000c000a0008UL,
      0x0016001400120010UL,
      0x001e001c001a0018UL
  );
  OrcConstant perm_odd = ORC_CONSTANT_INIT_U256 (
      0x0007000500030001UL,
      0x000f000d000b0009UL,
      0x0017001500130011UL,
      0x001f001d001b0019UL
  );

  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int dest1 = ORC_DEST_ARG (c, insn, 1);
  const int size = ORC_SIZE (c, ORC_SRC_SIZE (c, insn, 0));

  if (size >= 64) {
    /* ZMM: Use mask-based vpermw approach */
    int even = orc_compiler_get_constant_full (c, &perm_even);
    int odd = orc_compiler_get_constant_full (c, &perm_odd);
    int k = orc_avx512_compiler_get_mask_reg (c, TRUE);

    orc_avx512_insn_emit_kxnord (c, k, k, k);
    orc_avx512_insn_emit_kshiftrd (c, 16, k, k);
    orc_avx512_insn_emit_vpermw (c, odd, src0, dest0, k, TRUE);
    orc_avx512_insn_emit_vpermw (c, even, src0, dest1, k, TRUE);
    orc_avx512_compiler_release_mask_reg (c, k);
  } else if (size >= 32) {
    /* YMM: Use AVX-style shift+pack approach with AVX512 instructions */
    orc_avx512_insn_avx_emit_vpsrad_r_r_i (c, 16, ORC_AVX512_AVX_REG(src0), 
        ORC_AVX512_AVX_REG(dest0), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_avx_emit_vpackssdw (c, ORC_AVX512_AVX_REG(dest0), 
        ORC_AVX512_AVX_REG(dest0), ORC_AVX512_AVX_REG(dest0), 
        ORC_REG_INVALID, FALSE);
    orc_avx512_insn_avx_emit_vpermq_r_r_i (c, ORC_AVX512_SHUF(3, 1, 2, 0), 
        ORC_AVX512_AVX_REG(dest0), ORC_AVX512_AVX_REG(dest0), 
        ORC_REG_INVALID, FALSE);

    orc_avx512_insn_avx_emit_vpslld_r_r_i (c, 16, ORC_AVX512_AVX_REG(src0), 
        ORC_AVX512_AVX_REG(dest1), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_avx_emit_vpsrad_r_r_i (c, 16, ORC_AVX512_AVX_REG(dest1), 
        ORC_AVX512_AVX_REG(dest1), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_avx_emit_vpackssdw (c, ORC_AVX512_AVX_REG(dest1), 
        ORC_AVX512_AVX_REG(dest1), ORC_AVX512_AVX_REG(dest1), 
        ORC_REG_INVALID, FALSE);
    orc_avx512_insn_avx_emit_vpermq_r_r_i (c, ORC_AVX512_SHUF(3, 1, 2, 0), 
        ORC_AVX512_AVX_REG(dest1), ORC_AVX512_AVX_REG(dest1), 
        ORC_REG_INVALID, FALSE);
  } else {
    /* XMM: Use SSE-style shift+pack approach with AVX512 instructions */
    orc_avx512_insn_sse_emit_vpsrad_r_r_i (c, 16, ORC_AVX512_SSE_REG(src0), 
        ORC_AVX512_SSE_REG(dest0), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpackssdw (c, ORC_AVX512_SSE_REG(dest0), 
        ORC_AVX512_SSE_REG(dest0), ORC_AVX512_SSE_REG(dest0), 
        ORC_REG_INVALID, FALSE);

    orc_avx512_insn_sse_emit_vpslld_r_r_i (c, 16, ORC_AVX512_SSE_REG(src0), 
        ORC_AVX512_SSE_REG(dest1), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpsrad_r_r_i (c, 16, ORC_AVX512_SSE_REG(dest1), 
        ORC_AVX512_SSE_REG(dest1), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpackssdw (c, ORC_AVX512_SSE_REG(dest1), 
        ORC_AVX512_SSE_REG(dest1), ORC_AVX512_SSE_REG(dest1), 
        ORC_REG_INVALID, FALSE);
  }
}

static void
orc_avx512_rule_splitwb (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_even = ORC_CONSTANT_INIT_U256 (
      0x0e0c0a0806040200UL,
      0x1e1c1a1816141210UL,
      0x2e2c2a2826242220UL,
      0x3e3c3a3836343230UL
  );
  OrcConstant perm_odd = ORC_CONSTANT_INIT_U256 (
      0x0f0d0b0907050301UL,
      0x1f1d1b1917151311UL,
      0x2f2d2b2927252321UL,
      0x3f3d3b3937353331UL
  );

  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int dest1 = ORC_DEST_ARG (c, insn, 1);
  const int size = ORC_SIZE (c, ORC_SRC_SIZE (c, insn, 0));

  if (size >= 64) {
    /* ZMM: Use mask-based vpermb approach */
    int even = orc_compiler_get_constant_full (c, &perm_even);
    int odd = orc_compiler_get_constant_full (c, &perm_odd);
    int k = orc_avx512_compiler_get_mask_reg (c, TRUE);

    orc_avx512_insn_emit_kxnorq (c, k, k, k);
    orc_avx512_insn_emit_kshiftrq (c, 32, k, k);
    orc_avx512_insn_emit_vpermb (c, odd, src0, dest0, k, TRUE);
    orc_avx512_insn_emit_vpermb (c, even, src0, dest1, k, TRUE);
    orc_avx512_compiler_release_mask_reg (c, k);
  } else if (size >= 32) {
    /* YMM: Use AVX-style shift+pack approach with AVX512 instructions */
    int tmp = orc_compiler_get_constant (c, 2, 0xff);
    
    orc_avx512_insn_avx_emit_vpsraw_r_r_i (c, 8, ORC_AVX512_AVX_REG(src0), 
        ORC_AVX512_AVX_REG(dest0), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_avx_emit_vpacksswb (c, ORC_AVX512_AVX_REG(dest0), 
        ORC_AVX512_AVX_REG(dest0), ORC_AVX512_AVX_REG(dest0), 
        ORC_REG_INVALID, FALSE);
    orc_avx512_insn_avx_emit_vpermq_r_r_i (c, ORC_AVX512_SHUF(3, 1, 2, 0), 
        ORC_AVX512_AVX_REG(dest0), ORC_AVX512_AVX_REG(dest0), 
        ORC_REG_INVALID, FALSE);

    orc_avx512_insn_avx_emit_vpandd (c, ORC_AVX512_AVX_REG(src0), 
        ORC_AVX512_AVX_REG(tmp), ORC_AVX512_AVX_REG(dest1), 
        ORC_REG_INVALID, FALSE);
    orc_avx512_insn_avx_emit_vpackuswb (c, ORC_AVX512_AVX_REG(dest1), 
        ORC_AVX512_AVX_REG(dest1), ORC_AVX512_AVX_REG(dest1), 
        ORC_REG_INVALID, FALSE);
    orc_avx512_insn_avx_emit_vpermq_r_r_i (c, ORC_AVX512_SHUF(3, 1, 2, 0), 
        ORC_AVX512_AVX_REG(dest1), ORC_AVX512_AVX_REG(dest1), 
        ORC_REG_INVALID, FALSE);
  } else {
    /* XMM: Use SSE-style shift+pack approach with AVX512 instructions */
    int tmp = orc_compiler_get_constant (c, 2, 0xff);
    
    orc_avx512_insn_sse_emit_vpsraw_r_r_i (c, 8, ORC_AVX512_SSE_REG(src0), 
        ORC_AVX512_SSE_REG(dest0), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpacksswb (c, ORC_AVX512_SSE_REG(dest0), 
        ORC_AVX512_SSE_REG(dest0), ORC_AVX512_SSE_REG(dest0), 
        ORC_REG_INVALID, FALSE);

    orc_avx512_insn_sse_emit_vpandd (c, ORC_AVX512_SSE_REG(src0), 
        ORC_AVX512_SSE_REG(tmp), ORC_AVX512_SSE_REG(dest1), 
        ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpackuswb (c, ORC_AVX512_SSE_REG(dest1), 
        ORC_AVX512_SSE_REG(dest1), ORC_AVX512_SSE_REG(dest1), 
        ORC_REG_INVALID, FALSE);
  }
}

static void
orc_avx512_rule_splatbw (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_bw = ORC_CONSTANT_INIT_U512 (
      0x0303020201010000UL,
      0x0707060605050404UL,
      0x0b0b0a0a09090808UL,
      0x0f0f0e0e0d0d0c0cUL,
      0x1313121211111010UL,
      0x1717161615151414UL,
      0x1b1b1a1a19191818UL,
      0x1f1f1e1e1d1d1c1cUL
  );

  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int bw = orc_compiler_get_constant_full (c, &perm_bw);

  orc_avx512_insn_emit_vpermb (c, bw, src0, dest0, ORC_REG_INVALID,
      FALSE);
}

static void
orc_avx512_rule_splatbl (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_bl = ORC_CONSTANT_INIT_U512 (
      0x0101010100000000UL,
      0x0303030302020202UL,
      0x0505050504040404UL,
      0x0707070706060606UL,
      0x0909090908080808UL,
      0x0b0b0b0b0a0a0a0aUL,
      0x0d0d0d0d0c0c0c0cUL,
      0x0f0f0f0f0e0e0e0eUL
  );

  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int bl = orc_compiler_get_constant_full (c, &perm_bl);

  orc_avx512_insn_emit_vpermb (c, bl, src0, dest0, ORC_REG_INVALID,
      FALSE);
}

static void
orc_avx512_rule_swapw (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_w = ORC_CONSTANT_INIT_U512 (
      0x0607040502030001UL,
      0x0e0f0c0d0a0b0809UL,
      0x1617141512131011UL,
      0x1e1f1c1d1a1b1819UL,
      0x2627242522232021UL,
      0x2e2f2c2d2a2b2829UL,
      0x3637343532333031UL,
      0x3e3f3c3d3a3b3839UL
  );

  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int w = orc_compiler_get_constant_full (c, &perm_w);

  orc_avx512_insn_emit_vpermb (c, w, src0, dest0, ORC_REG_INVALID,
      FALSE);
}

static void
orc_avx512_rule_swapl (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_l = ORC_CONSTANT_INIT_U512 (
      0x0405060700010203UL,
      0x0c0d0e0f08090a0bUL,
      0x1415161710111213UL,
      0x1c1d1e1f18191a1bUL,
      0x2425262720212223UL,
      0x2c2d2e2f28292a2bUL,
      0x3435363730313233UL,
      0x3c3d3e3f38393a3bUL
  );

  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int l = orc_compiler_get_constant_full (c, &perm_l);

  orc_avx512_insn_emit_vpermb (c, l, src0, dest0, ORC_REG_INVALID,
      FALSE);
}

static void
orc_avx512_rule_swapq (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_q = ORC_CONSTANT_INIT_U512 (
      0x0001020304050607UL,
      0x08090a0b0c0d0e0fUL,
      0x1011121314151617UL,
      0x18191a1b1c1d1e1fUL,
      0x2021222324252627UL,
      0x28292a2b2c2d2e2fUL,
      0x3031323334353637UL,
      0x38393a3b3c3d3e3fUL
  );

  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int q = orc_compiler_get_constant_full (c, &perm_q);

  orc_avx512_insn_emit_vpermb (c, q, src0, dest0, ORC_REG_INVALID,
      FALSE);
}

static void
orc_avx512_rule_swapwl (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_l = ORC_CONSTANT_INIT_U512 (
      0x0002000300000001UL,
      0x0006000700040005UL,
      0x000a000b00080009UL,
      0x000e000f000c000dUL,
      0x0012001300100011UL,
      0x0016001700140015UL,
      0x001a001b00180019UL,
      0x001e001f001c001dUL
  );

  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int l = orc_compiler_get_constant_full (c, &perm_l);

  orc_avx512_insn_emit_vpermw (c, l, src0, dest0, ORC_REG_INVALID,
      FALSE);
}

static void
orc_avx512_rule_swaplq (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);

  orc_avx512_insn_emit_vpshufd (c, ORC_AVX512_SHUF (2, 3, 0, 1), src0, dest0,
      ORC_REG_INVALID, FALSE);
}

#define orc_avx512_rule_convwb orc_avx512_rule_select0wb

static void
orc_avx512_rule_select0wb (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_w0 = ORC_CONSTANT_INIT_U256 (
      0x0e0c0a0806040200UL,
      0x1e1c1a1816141210UL,
      0x2e2c2a2826242220UL,
      0x3e3c3a3836343230UL
  );

  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int w0 = orc_compiler_get_constant_full (c, &perm_w0);

  orc_avx512_insn_emit_vpermb (c, w0, src0, dest0, ORC_REG_INVALID,
      FALSE);
}

static void
orc_avx512_rule_convhwb (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  OrcConstant pc = ORC_CONSTANT_INIT_U256 (
      0x0000000000000000UL,
      0x0000000000000002UL,
      0x0000000000000004UL,
      0x0000000000000006UL
  );
  int p = orc_compiler_get_constant_full (c, &pc);

  orc_avx512_insn_emit_vpsrlw_r_r_i (c, 8, src0, dest0, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpackuswb (c, dest0, dest0, dest0, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpermq (c, p, dest0, dest0, ORC_REG_INVALID, FALSE);
}

static void
orc_avx512_rule_convhlw (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  OrcConstant pc = ORC_CONSTANT_INIT_U256 (
      0x0000000000000000UL,
      0x0000000000000002UL,
      0x0000000000000004UL,
      0x0000000000000006UL
  );
  int p = orc_compiler_get_constant_full (c, &pc);

  orc_avx512_insn_emit_vpsrad_r_r_i (c, 16, src0, dest0, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpackssdw (c, dest0, dest0, dest0, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpermq (c, p, dest0, dest0, ORC_REG_INVALID, FALSE);
}

static void
orc_avx512_rule_convusswb (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  OrcConstant pc = ORC_CONSTANT_INIT_U256 (
      0x0000000000000000UL,
      0x0000000000000002UL,
      0x0000000000000004UL,
      0x0000000000000006UL
  );
  int p = orc_compiler_get_constant_full (c, &pc);

  orc_compiler_load_constant_from_size_and_value (c, tmp, 2, INT8_MAX);
  orc_avx512_insn_emit_vpminuw (c, src0, tmp, dest0, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpxord (c, tmp, tmp, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpackuswb (c, dest0, tmp, dest0, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpermq (c, p, dest0, dest0, ORC_REG_INVALID, FALSE);
  
  orc_compiler_release_temp_reg (c, tmp);
}

static void
orc_avx512_rule_convuuswb (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  OrcConstant pc = ORC_CONSTANT_INIT_U256 (
      0x0000000000000000UL,
      0x0000000000000002UL,
      0x0000000000000004UL,
      0x0000000000000006UL
  );
  int p = orc_compiler_get_constant_full (c, &pc);

  orc_avx512_insn_emit_vmovdqa32_r_r (c, src0, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsrlw_r_r_i (c, 15, src0, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsllw_r_r_i (c, 14, tmp, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpord (c, src0, tmp, dest0, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsllw_r_r_i (c, 1, tmp, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpxord (c, dest0, tmp, dest0, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpackuswb (c, dest0, dest0, dest0, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpermq (c, p, dest0, dest0, ORC_REG_INVALID, FALSE);
  
  orc_compiler_release_temp_reg (c, tmp);
}

static void
orc_avx512_rule_convusslw (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  OrcConstant pc = ORC_CONSTANT_INIT_U256 (
      0x0000000000000000UL,
      0x0000000000000002UL,
      0x0000000000000004UL,
      0x0000000000000006UL
  );
  int p = orc_compiler_get_constant_full (c, &pc);

  orc_compiler_load_constant_from_size_and_value (c, tmp, 4, INT16_MAX);
  orc_avx512_insn_emit_vpminud (c, src0, tmp, dest0, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpxord (c, tmp, tmp, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpackssdw (c, dest0, tmp, dest0, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpermq (c, p, dest0, dest0, ORC_REG_INVALID, FALSE);
  
  orc_compiler_release_temp_reg (c, tmp);
}

static void
orc_avx512_rule_convuuslw (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  OrcConstant pc = ORC_CONSTANT_INIT_U256 (
      0x0000000000000000UL,
      0x0000000000000002UL,
      0x0000000000000004UL,
      0x0000000000000006UL
  );
  int p = orc_compiler_get_constant_full (c, &pc);

  orc_compiler_load_constant_from_size_and_value (c, tmp, 4, UINT16_MAX);
  orc_avx512_insn_emit_vpminud (c, src0, tmp, dest0, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpxord (c, tmp, tmp, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpackusdw (c, dest0, tmp, dest0, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpermq (c, p, dest0, dest0, ORC_REG_INVALID, FALSE);
}

static void
orc_avx512_rule_select1wb (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_w1 = ORC_CONSTANT_INIT_U256 (
      0x0f0d0b0907050301UL,
      0x1f1d1b1917151311UL,
      0x2f2d2b2927252321UL,
      0x3f3d3b3937353331UL
  );

  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int w1 = orc_compiler_get_constant_full (c, &perm_w1);
  const int size = ORC_SIZE (c, ORC_SRC_SIZE (c, insn, 0));

  orc_avx512_insn_adaptive_emit_vpermb (c, size, w1, src0, dest0, ORC_REG_INVALID, FALSE);
}


#define orc_avx512_rule_convlw orc_avx512_rule_select0lw

static void
orc_avx512_rule_select0lw (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_l0 = ORC_CONSTANT_INIT_U256 (
      0x0006000400020000UL,
      0x000e000c000a0008UL,
      0x0016001400120010UL,
      0x001e001c001a0018UL
  );

  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int l0 = orc_compiler_get_constant_full (c, &perm_l0);

  orc_avx512_insn_emit_vpermw (c, l0, src0, dest0, ORC_REG_INVALID,
      FALSE);
}

static void
orc_avx512_rule_select1lw (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_l1 = ORC_CONSTANT_INIT_U256 (
      0x0007000500030001UL,
      0x000f000d000b0009UL,
      0x0017001500130011UL,
      0x001f001d001b0019UL
  );

  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int l1 = orc_compiler_get_constant_full (c, &perm_l1);

  orc_avx512_insn_emit_vpermw (c, l1, src0, dest0, ORC_REG_INVALID,
      FALSE);
}

#define orc_avx512_rule_convql orc_avx512_rule_select0ql

static void
orc_avx512_rule_select0ql (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_q0 = ORC_CONSTANT_INIT_U256 (
      0x0000000200000000UL,
      0x0000000600000004UL,
      0x0000000a00000008UL,
      0x0000000e0000000cUL
  );

  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int q0 = orc_compiler_get_constant_full (c, &perm_q0);

  orc_avx512_insn_emit_vpermd (c, q0, src0, dest0, ORC_REG_INVALID,
      FALSE);
}

static void
orc_avx512_rule_select1ql (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant perm_q1 = ORC_CONSTANT_INIT_U256 (
      0x0000000300000001UL,
      0x0000000700000005UL,
      0x0000000b00000009UL,
      0x0000000f0000000dUL
  );

  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest0 = ORC_DEST_ARG (c, insn, 0);
  int q1 = orc_compiler_get_constant_full (c, &perm_q1);

  orc_avx512_insn_emit_vpermd (c, q1, src0, dest0, ORC_REG_INVALID,
      FALSE);
}

static void
orc_avx512_rule_convsssql (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  int mask_gt_max = orc_avx512_compiler_get_mask_reg (c, TRUE);
  int mask_lt_min = orc_avx512_compiler_get_mask_reg (c, TRUE);
  
  OrcConstant const_max = ORC_CONSTANT_INIT_U64 (INT32_MAX);
  OrcConstant const_min = ORC_CONSTANT_INIT_U64 ((orc_uint64)INT32_MIN);
  int tmpc_max = orc_compiler_get_constant_full (c, &const_max);
  int tmpc_min = orc_compiler_get_constant_full (c, &const_min);
  
  OrcConstant perm_q0 = ORC_CONSTANT_INIT_U256 (
      0x0000000200000000UL,
      0x0000000600000004UL,
      0x0000000a00000008UL,
      0x0000000e0000000cUL
  );
  int p = orc_compiler_get_constant_full (c, &perm_q0);

  /* Compare src > INT32_MAX, result stored in mask */
  orc_avx512_insn_emit_vpcmpgtq (c, src, tmpc_max, mask_gt_max, ORC_REG_INVALID);
  /* Blend: when mask=1 (src > INT32_MAX), select tmpc_max (s0), else select src (s1) */
  orc_avx512_insn_emit_vpblendmq (c, src, tmpc_max, tmp, mask_gt_max, FALSE);
  
  /* Check if tmp < INT32_MIN by comparing INT32_MIN > tmp */
  orc_avx512_insn_emit_vpcmpgtq (c, tmpc_min, tmp, mask_lt_min, ORC_REG_INVALID);
  /* Blend: when mask=1 (INT32_MIN > tmp, i.e., tmp < INT32_MIN), select tmpc_min (s0), else select tmp (s1) */
  orc_avx512_insn_emit_vpblendmq (c, tmp, tmpc_min, tmp, mask_lt_min, FALSE);
  
  /* Shuffle to pack 64-bit values to 32-bit (select low dword of each qword) */
  orc_avx512_insn_emit_vpermd (c, p, tmp, dest, ORC_REG_INVALID, FALSE);

  orc_compiler_release_temp_reg (c, tmp);
  orc_avx512_compiler_release_mask_reg (c, mask_gt_max);
  orc_avx512_compiler_release_mask_reg (c, mask_lt_min);
}

static void
orc_avx512_rule_convsusql (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  int zero = orc_compiler_get_temp_reg (c);
  int mask_gt_max = orc_avx512_compiler_get_mask_reg (c, TRUE);
  int mask_lt_zero = orc_avx512_compiler_get_mask_reg (c, TRUE);
  
  OrcConstant const_max = ORC_CONSTANT_INIT_U64 (UINT32_MAX);
  int tmpc_max = orc_compiler_get_constant_full (c, &const_max);
  
  OrcConstant perm_q0 = ORC_CONSTANT_INIT_U256 (
      0x0000000200000000UL,
      0x0000000600000004UL,
      0x0000000a00000008UL,
      0x0000000e0000000cUL
  );
  int p = orc_compiler_get_constant_full (c, &perm_q0);

  /* Zero register for comparison and blending */
  orc_avx512_insn_emit_vpxord (c, zero, zero, zero, ORC_REG_INVALID, FALSE);

  /* Compare src > UINT32_MAX (as signed), result stored in mask */
  orc_avx512_insn_emit_vpcmpgtq (c, src, tmpc_max, mask_gt_max, ORC_REG_INVALID);
  /* Blend: when mask=1 (src > UINT32_MAX), select tmpc_max, else select src */
  orc_avx512_insn_emit_vpblendmq (c, src, tmpc_max, tmp, mask_gt_max, FALSE);
  
  /* Check if tmp < 0 (negative, MSB set) by comparing zero > tmp */
  orc_avx512_insn_emit_vpcmpgtq (c, zero, tmp, mask_lt_zero, ORC_REG_INVALID);
  /* Blend: when mask=1 (tmp < 0), select zero, else select tmp */
  orc_avx512_insn_emit_vpblendmq (c, tmp, zero, tmp, mask_lt_zero, FALSE);
  
  /* Shuffle to pack 64-bit values to 32-bit (select low dword of each qword) */
  orc_avx512_insn_emit_vpermd (c, p, tmp, dest, ORC_REG_INVALID, FALSE);

  orc_compiler_release_temp_reg (c, tmp);
  orc_compiler_release_temp_reg (c, zero);
  orc_avx512_compiler_release_mask_reg (c, mask_gt_max);
  orc_avx512_compiler_release_mask_reg (c, mask_lt_zero);
}

static void
orc_avx512_rule_convussql (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  int zero = orc_compiler_get_temp_reg (c);
  int mask_negative = orc_avx512_compiler_get_mask_reg (c, TRUE);
  int mask_gt_max = orc_avx512_compiler_get_mask_reg (c, TRUE);
  
  OrcConstant const_max = ORC_CONSTANT_INIT_U64 ((orc_uint64)INT32_MAX);
  int tmpc_max = orc_compiler_get_constant_full (c, &const_max);
  
  OrcConstant perm_q0 = ORC_CONSTANT_INIT_U256 (
      0x0000000200000000UL,
      0x0000000600000004UL,
      0x0000000a00000008UL,
      0x0000000e0000000cUL
  );
  int p = orc_compiler_get_constant_full (c, &perm_q0);

  /* Zero register for comparison */
  orc_avx512_insn_emit_vpxord (c, zero, zero, zero, ORC_REG_INVALID, FALSE);

  /* Check if src < 0 (negative, which means huge when viewed as unsigned) */
  orc_avx512_insn_emit_vpcmpgtq (c, zero, src, mask_negative, ORC_REG_INVALID);
  /* Blend: when mask=1 (src < 0), select INT32_MAX (s0), else select src (s1) */
  orc_avx512_insn_emit_vpblendmq (c, tmpc_max, src, tmp, mask_negative, FALSE);
  
  /* Check if tmp > INT32_MAX (for positive values that are too large) */
  orc_avx512_insn_emit_vpcmpgtq (c, tmp, tmpc_max, mask_gt_max, ORC_REG_INVALID);
  /* Blend: when mask=1 (tmp > INT32_MAX), select INT32_MAX (s0), else select tmp (s1) */
  orc_avx512_insn_emit_vpblendmq (c, tmpc_max, tmp, tmp, mask_gt_max, FALSE);
  
  /* Shuffle to pack 64-bit values to 32-bit (select low dword of each qword) */
  orc_avx512_insn_emit_vpermd (c, p, tmp, dest, ORC_REG_INVALID, FALSE);

  orc_compiler_release_temp_reg (c, tmp);
  orc_compiler_release_temp_reg (c, zero);
  orc_avx512_compiler_release_mask_reg (c, mask_negative);
  orc_avx512_compiler_release_mask_reg (c, mask_gt_max);
}

static void
orc_avx512_rule_convuusql (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  int zero = orc_compiler_get_temp_reg (c);
  int mask_gt_max = orc_avx512_compiler_get_mask_reg (c, TRUE);
  
  OrcConstant const_max = ORC_CONSTANT_INIT_U64 ((orc_uint64)UINT32_MAX);
  int tmpc_max = orc_compiler_get_constant_full (c, &const_max);
  
  OrcConstant perm_q0 = ORC_CONSTANT_INIT_U256 (
      0x0000000200000000UL,
      0x0000000600000004UL,
      0x0000000a00000008UL,
      0x0000000e0000000cUL
  );
  int p = orc_compiler_get_constant_full (c, &perm_q0);

  /* Zero register for comparison */
  orc_avx512_insn_emit_vpxord (c, zero, zero, zero, ORC_REG_INVALID, FALSE);

  /* For unsigned comparison src > UINT32_MAX:
   * First check if src viewed as signed < 0 (high bit set, means very large unsigned)
   * This catches values > INT64_MAX which are definitely > UINT32_MAX */
  orc_avx512_insn_emit_vpcmpgtq (c, zero, src, mask_gt_max, ORC_REG_INVALID);
  /* Blend: when mask=1 (src has high bit set), select UINT32_MAX */
  orc_avx512_insn_emit_vpblendmq (c, tmpc_max, src, tmp, mask_gt_max, FALSE);
  
  /* Now check if tmp > UINT32_MAX (for remaining positive values) */
  orc_avx512_insn_emit_vpcmpgtq (c, tmp, tmpc_max, mask_gt_max, ORC_REG_INVALID);
  /* Blend: when mask=1 (tmp > UINT32_MAX as signed), select UINT32_MAX */
  orc_avx512_insn_emit_vpblendmq (c, tmpc_max, tmp, tmp, mask_gt_max, FALSE);
  
  /* Shuffle to pack 64-bit values to 32-bit (select low dword of each qword) */
  orc_avx512_insn_emit_vpermd (c, p, tmp, dest, ORC_REG_INVALID, FALSE);

  orc_avx512_compiler_release_mask_reg (c, mask_gt_max);
  orc_compiler_release_temp_reg (c, tmp);
  orc_compiler_release_temp_reg (c, zero);
}

static void
orc_avx512_rule_mulXwl (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  int size = ORC_SIZE (c, ORC_SRC_SIZE (c, insn, 0));
  int type = ORC_PTR_TO_INT (user);
  OrcAVX512Insn opcodes[2] = {
    ORC_AVX512_INSN_vpmulhuw,
    ORC_AVX512_INSN_vpmulhw
  };

  orc_avx512_insn_emit_avx512 (c, opcodes[type], src0, src1, ORC_REG_INVALID,
      tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpmullw (c, src0, src1, dest, ORC_REG_INVALID, FALSE);

  if (size >= 16) {
    OrcConstant pc = ORC_CONSTANT_INIT_U512 (
        0x0021000100200000UL,
        0x0023000300220002UL,
        0x0025000500240004UL,
        0x0027000700260006UL,
        0x0029000900280008UL,
        0x002b000b002a000aUL,
        0x002d000d002c000cUL,
        0x002f000f002e000eUL
    );
    int p = orc_compiler_get_constant_full (c, &pc);
  
    orc_avx512_insn_emit_vpermt2w (c, p, tmp, dest, ORC_REG_INVALID,
        FALSE);
  } else {
    orc_avx512_insn_emit_vpunpcklwd (c, dest, tmp, dest, ORC_REG_INVALID,
        FALSE);
  }
}

static void
orc_avx512_rule_mulXlq (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  OrcConstant pc = ORC_CONSTANT_INIT_U512 (
      0x0000000000000000UL,
      0x0000000000000001UL,
      0x0000000000000002UL,
      0x0000000000000003UL,
      0x0000000000000004UL,
      0x0000000000000005UL,
      0x0000000000000006UL,
      0x0000000000000007UL
  );
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp1 = orc_compiler_get_temp_reg (c);
  int p = orc_compiler_get_constant_full (c, &pc);
  int type = ORC_PTR_TO_INT (user);
  OrcAVX512Insn opcodes[2] = {
    ORC_AVX512_INSN_vpmuludq,
    ORC_AVX512_INSN_vpmuldq
  };

  orc_avx512_insn_emit_vpermd (c, p, src0, dest, ORC_REG_INVALID,
      FALSE);
  orc_avx512_insn_emit_vpermd (c, p, src1, tmp1, ORC_REG_INVALID,
      FALSE);
  orc_avx512_insn_emit_avx512 (c, opcodes[type], dest, tmp1, ORC_REG_INVALID,
      dest, ORC_REG_INVALID, FALSE);
}

static void
orc_avx512_rule_splatw3q (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);

  orc_avx512_insn_emit_vpshuflw (c, ORC_AVX512_SHUF (3, 3, 3, 3), src, dest,
      ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpshufhw (c, ORC_AVX512_SHUF (3, 3, 3, 3), dest, dest,
      ORC_REG_INVALID, FALSE);
}

static void
orc_avx512_rule_shlb (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);
  OrcVariable *src1 = ORC_SRC_VAR (c, insn, 1);

  switch (src1->vartype) {
    case ORC_VAR_TYPE_CONST:
      {
        OrcConstant cnst = ORC_CONSTANT_INIT_U8 (0xff << src1->value.i);
        int tmp = orc_compiler_get_constant_full (c, &cnst);
        orc_avx512_insn_emit_vpsllw_r_r_i (c, src1->value.i, src0, dest,
            ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vpandd (c, dest, tmp, dest, ORC_REG_INVALID, FALSE);
      }
      break;

    case ORC_VAR_TYPE_PARAM:
      {
        /* FIXME the bug */
        int src1_reg = orc_avx512_compiler_load_param2 (c, insn, 1);
        int tmp1 = orc_compiler_get_temp_reg (c);
        int tmp2 = orc_compiler_get_temp_reg (c);

        /* all 1's */
        /* FIXME use the load constant instead */
        orc_avx512_insn_emit_vpternlogd (c, 0xff, tmp1, tmp1, tmp1, ORC_REG_INVALID, FALSE);
        /* [XXXXXXXX 0000000S] */
        orc_avx512_insn_emit_vpsrlq_r_r_i (c, 56, src1_reg, tmp2, ORC_REG_INVALID, FALSE);
        /* 0xff << S */
        orc_avx512_insn_emit_vpsllw_r_r_r (c, tmp1, ORC_AVX512_SSE_REG (tmp2), tmp1,
            ORC_REG_INVALID, FALSE);
        /* Now we have the bit mask at tmp1 */
        orc_avx512_insn_emit_vpbroadcastb (c, ORC_AVX512_SSE_REG(tmp1), tmp1,
            ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vpsllw_r_r_r (c, src0, ORC_AVX512_SSE_REG (tmp2), dest,
            ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vpandd (c, dest, tmp1, dest, ORC_REG_INVALID, FALSE);

        orc_compiler_release_temp_reg (c, tmp1);
        orc_compiler_release_temp_reg (c, tmp2);
        orc_compiler_release_temp_reg (c, src1_reg);
      }
      break;

    default:
      ORC_COMPILER_ERROR (c, "code generation rule for %s only works with "
          "constant or parameter shifts", insn->opcode->name);
      break;
  }
}

static void
orc_avx512_rule_shrsb (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);
  OrcVariable *src1 = ORC_SRC_VAR (c, insn, 1);

  switch (src1->vartype) {
    case ORC_VAR_TYPE_CONST:
      {
        int tmp = orc_compiler_get_temp_reg (c);
        
        /* For signed byte shift right:
         * 1. Shift left by 8 to move low byte to high byte position
         * 2. Arithmetic shift right by (shift_amount) on words
         * 3. Logical shift right by 8 to move back to low byte
         * 4. Process high bytes separately with shift and position
         * 5. Combine with OR
         */
        orc_avx512_insn_emit_vpsllw_r_r_i (c, 8, src0, tmp, ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vpsraw_r_r_i (c, src1->value.i, tmp, tmp, ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vpsrlw_r_r_i (c, 8, tmp, tmp, ORC_REG_INVALID, FALSE);

        orc_avx512_insn_emit_vpsraw_r_r_i (c, 8 + src1->value.i, src0, dest, ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vpsllw_r_r_i (c, 8, dest, dest, ORC_REG_INVALID, FALSE);

        orc_avx512_insn_emit_vpord (c, dest, tmp, dest, ORC_REG_INVALID, FALSE);

        orc_compiler_release_temp_reg (c, tmp);
      }
      break;

    case ORC_VAR_TYPE_PARAM:
      {
        /* FIXME the bug */
        int src1_reg = orc_avx512_compiler_load_param2 (c, insn, 1);
        int tmp1 = orc_compiler_get_temp_reg (c);
        int tmp2 = orc_compiler_get_temp_reg (c);
        int tmp3 = orc_compiler_get_temp_reg (c);

        /* Load shift amount into XMM register for variable shift */
        orc_avx512_insn_emit_vpsrlq_r_r_i (c, 56, src1_reg, tmp3, ORC_REG_INVALID, FALSE);

        /* Process low bytes */
        orc_avx512_insn_emit_vpsllw_r_r_i (c, 8, src0, tmp1, ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vpsraw_r_r_r (c, tmp1, ORC_AVX512_SSE_REG (tmp3), tmp1, ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vpsrlw_r_r_i (c, 8, tmp1, tmp1, ORC_REG_INVALID, FALSE);

        /* Process high bytes - need to add 8 to shift amount */
        /* Add 8 as a quadword to the shift count */
        orc_avx512_insn_emit_vpaddq (c, tmp3, orc_compiler_get_constant (c, 8, 8), tmp2, ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vpsraw_r_r_r (c, src0, ORC_AVX512_SSE_REG (tmp2), dest, ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vpsllw_r_r_i (c, 8, dest, dest, ORC_REG_INVALID, FALSE);

        orc_avx512_insn_emit_vpord (c, dest, tmp1, dest, ORC_REG_INVALID, FALSE);

        orc_compiler_release_temp_reg (c, tmp1);
        orc_compiler_release_temp_reg (c, tmp2);
        orc_compiler_release_temp_reg (c, tmp3);
        orc_compiler_release_temp_reg (c, src1_reg);
      }
      break;

    default:
      ORC_COMPILER_ERROR (c, "code generation rule for %s only works with "
          "constant or parameter shifts", insn->opcode->name);
      break;
  }
}

static void
orc_avx512_rule_shrub (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);
  OrcVariable *src1 = ORC_SRC_VAR (c, insn, 1);

  switch (src1->vartype) {
    case ORC_VAR_TYPE_CONST:
      {
        /* For unsigned byte shift right:
         * 1. Logical shift right by shift_amount on words
         * 2. AND with mask (0xff >> shift_amount) to clear unwanted bits
         */
        OrcConstant cnst = ORC_CONSTANT_INIT_U8 (0xff >> src1->value.i);
        int tmp = orc_compiler_get_constant_full (c, &cnst);
        
        orc_avx512_insn_emit_vpsrlw_r_r_i (c, src1->value.i, src0, dest, ORC_REG_INVALID, FALSE);
        orc_avx512_insn_emit_vpandd (c, dest, tmp, dest, ORC_REG_INVALID, FALSE);
      }
      break;

    case ORC_VAR_TYPE_PARAM:
      {
        /* FIXME the bug */
        int src1_reg = orc_avx512_compiler_load_param2 (c, insn, 1);
        int tmp1 = orc_compiler_get_temp_reg (c);
        int tmp2 = orc_compiler_get_temp_reg (c);
        OrcConstant mask_00ff = ORC_CONSTANT_INIT_U16 (0x00ff);
        int word_mask = orc_compiler_get_constant_full (c, &mask_00ff);

        /* all 1's */
        orc_avx512_insn_emit_vpternlogd (c, 0xff, tmp1, tmp1, tmp1, ORC_REG_INVALID, FALSE);
        
        /* Mask to get 0x00FF in each word (clear high bytes before shift) */
        orc_avx512_insn_emit_vpandd (c, tmp1, word_mask, tmp1, ORC_REG_INVALID, FALSE);
        
        /* [XXXXXXXX 0000000S] */
        orc_avx512_insn_emit_vpsrlq_r_r_i (c, 56, src1_reg, tmp2, ORC_REG_INVALID, FALSE);
        
        /* 0x00FF >> S (now high byte won't bleed in) */
        orc_avx512_insn_emit_vpsrlw_r_r_r (c, tmp1, ORC_AVX512_SSE_REG (tmp2), tmp1, ORC_REG_INVALID, FALSE);
        
        /* Broadcast the low byte (which now has 0xFF >> S) to all byte positions */
        orc_avx512_insn_emit_vpbroadcastb (c, ORC_AVX512_SSE_REG(tmp1), tmp1, ORC_REG_INVALID, FALSE);
        
        /* Shift the data right by the parameter amount */
        orc_avx512_insn_emit_vpsrlw_r_r_r (c, src0, ORC_AVX512_SSE_REG (tmp2), dest, ORC_REG_INVALID, FALSE);
        
        /* Apply the mask */
        orc_avx512_insn_emit_vpandd (c, dest, tmp1, dest, ORC_REG_INVALID, FALSE);

        orc_compiler_release_temp_reg (c, tmp1);
        orc_compiler_release_temp_reg (c, tmp2);
        orc_compiler_release_temp_reg (c, src1_reg);
      }
      break;

    default:
      ORC_COMPILER_ERROR (c, "code generation rule for %s only works with "
          "constant or parameter shifts", insn->opcode->name);
      break;
  }
}

static void
orc_avx512_rule_convsXsXX (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);
  OrcConstant pc = ORC_CONSTANT_INIT_U256 (
      0x0000000000000000UL,
      0x0000000000000002UL,
      0x0000000000000004UL,
      0x0000000000000006UL
  );
  int p = orc_compiler_get_constant_full (c, &pc);
  int type = ORC_PTR_TO_INT (user);
  OrcAVX512Insn opcodes[4] = {
    ORC_AVX512_INSN_vpacksswb,
    ORC_AVX512_INSN_vpackuswb,
    ORC_AVX512_INSN_vpackssdw,
    ORC_AVX512_INSN_vpackusdw
  };

  orc_avx512_insn_emit_avx512 (c, opcodes[type], src, src, ORC_REG_INVALID, dest,
      ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpermq (c, p, dest, dest, ORC_REG_INVALID, FALSE);
}

static void
orc_avx512_rule_div255w (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);
  OrcConstant pc = ORC_CONSTANT_INIT_U16 (0x8081);
  int p = orc_compiler_get_constant_full (c, &pc);

  orc_avx512_insn_emit_vpmulhuw (c, src, p, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsrlw_r_r_i (c, 7, dest, dest, ORC_REG_INVALID, FALSE);
}

/* TODO
 * Use this for avgsl, avgul, avgsl, etc
 */
static void
orc_avx512_rule_avgsw (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);

  orc_avx512_insn_emit_vpxord (c, src0, src1, tmp, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsraw_r_r_i (c, 1, tmp, tmp, ORC_REG_INVALID, FALSE);

  orc_avx512_insn_emit_vpord (c, src0, src1, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsubw (c, dest, tmp, dest, ORC_REG_INVALID, FALSE);
}

static void
orc_avx512_rule_accw (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);

  orc_avx512_insn_emit_vpaddw (c, dest, src, dest, ORC_REG_INVALID, FALSE);
}

static void
orc_avx512_rule_accl (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src = ORC_SRC_ARG (c, insn, 0);
  int dest = ORC_DEST_ARG (c, insn, 0);

  /* The code from MMX, SSE and AVX have this. I don't understand why.
   * Initially it seems to avoid garbage in the register by keeping only the
   * valid data and the rest zeroes to handle properly the add instruction
   * but then, why are other loop_shifts not handled? and why accw does not
   * have it?
   */
#if 0
  if (c->loop_shift == 0) {
    orc_avx512_insn_emit_vpslldq_r_r_i (c, 12, src, src, ORC_REG_INVALID, FALSE);
  }
#endif
  orc_avx512_insn_emit_vpaddd (c, dest, src, dest, ORC_REG_INVALID, FALSE);
}

static void
orc_avx512_rule_accsadubl (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src1 = ORC_SRC_ARG (c, insn, 0);
  int src2 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_temp_reg (c);
  int tmp2;

  /* accsadubl: accumulate sum of absolute differences unsigned byte to long
   * Uses psadbw which produces sum of 8 absolute differences per 64-bit lane
   * 
   * Key issue: psadbw operates on 8 bytes at a time, producing a 16-bit sum
   * If loop_shift is small (processing < 16 bytes), we must prevent garbage
   * bytes from being included in the sum.
   * 
   * Strategy based on loop_shift:
   *  - loop_shift <= 2 (≤ 4 bytes): Shift both inputs left to align valid
   *    bytes at high end, then psadbw, then paddd (SSE width to avoid high lanes)
   *  - loop_shift == 3 (8 bytes): psadbw, then shift result left to zero
   *    out upper garbage sum, then paddd (SSE width)
   *  - loop_shift == 4 (16 bytes): Just psadbw + paddd (SSE width)
   *  - loop_shift > 4 (≥ 32 bytes): Full AVX512 psadbw + paddd
   */

  if (c->loop_shift <= 2) {
    /* Processing ≤ 4 bytes: shift both sources to high end of 16-byte block */
    int shift_amount = 16 - (1 << c->loop_shift);
    tmp2 = orc_compiler_get_temp_reg (c);
    
    /* Use SSE-width operations (128-bit) */
    orc_avx512_insn_sse_emit_vpslldq_r_r_i (c, shift_amount, 
        ORC_AVX512_SSE_REG (src1), ORC_AVX512_SSE_REG (tmp), 
        ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpslldq_r_r_i (c, shift_amount, 
        ORC_AVX512_SSE_REG (src2), ORC_AVX512_SSE_REG (tmp2), 
        ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpsadbw (c, ORC_AVX512_SSE_REG (tmp), 
        ORC_AVX512_SSE_REG (tmp2), ORC_AVX512_SSE_REG (tmp), 
        ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpaddd (c, ORC_AVX512_SSE_REG (dest), 
        ORC_AVX512_SSE_REG (tmp), ORC_AVX512_SSE_REG (dest), 
        ORC_REG_INVALID, FALSE);
    
    orc_compiler_release_temp_reg (c, tmp2);
  } else if (c->loop_shift == 3) {
    /* Processing 8 bytes: psadbw produces two 16-bit sums, but we only
     * want the first one (from the first 8 bytes). Shift left by 8 bytes
     * to zero out the upper sum (from garbage bytes 8-15) */
    orc_avx512_insn_sse_emit_vpsadbw (c, ORC_AVX512_SSE_REG (src1), 
        ORC_AVX512_SSE_REG (src2), ORC_AVX512_SSE_REG (tmp), 
        ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpslldq_r_r_i (c, 8, ORC_AVX512_SSE_REG (tmp), 
        ORC_AVX512_SSE_REG (tmp), ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpaddd (c, ORC_AVX512_SSE_REG (dest), 
        ORC_AVX512_SSE_REG (tmp), ORC_AVX512_SSE_REG (dest), 
        ORC_REG_INVALID, FALSE);
  } else if (c->loop_shift == 4) {
    /* Processing 16 bytes: full 128-bit lane, use SSE width only to
     * prevent high (256/512-bit) lanes from contributing garbage */
    orc_avx512_insn_sse_emit_vpsadbw (c, ORC_AVX512_SSE_REG (src1), 
        ORC_AVX512_SSE_REG (src2), ORC_AVX512_SSE_REG (tmp), 
        ORC_REG_INVALID, FALSE);
    orc_avx512_insn_sse_emit_vpaddd (c, ORC_AVX512_SSE_REG (dest), 
        ORC_AVX512_SSE_REG (tmp), ORC_AVX512_SSE_REG (dest), 
        ORC_REG_INVALID, FALSE);
  } else {
    /* Processing ≥ 32 bytes: use full AVX512 width */
    orc_avx512_insn_emit_vpsadbw (c, src1, src2, tmp, ORC_REG_INVALID, FALSE);
    orc_avx512_insn_emit_vpaddd (c, dest, tmp, dest, ORC_REG_INVALID, FALSE);
  }

  orc_compiler_release_temp_reg (c, tmp);
}

static void
orc_avx512_rule_avgsb (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int tmp = orc_compiler_get_constant (c, 1, 0x80);

  orc_avx512_insn_emit_vpxord (c, src1, tmp, src1, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpxord (c, src0, tmp, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpavgb (c, dest, src1, dest, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpxord (c, src1, tmp, src1, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpxord (c, dest, tmp, dest, ORC_REG_INVALID, FALSE);
}

static void
orc_avx512_rule_divluw (OrcCompiler *c, void *user, OrcInstruction *insn)
{
  int src0 = ORC_SRC_ARG (c, insn, 0);
  int src1 = ORC_SRC_ARG (c, insn, 1);
  int dest = ORC_DEST_ARG (c, insn, 0);
  int a = orc_compiler_get_temp_reg (c);
  int j = orc_compiler_get_temp_reg (c);
  int j2 = orc_compiler_get_temp_reg (c);
  int l = orc_compiler_get_temp_reg (c);
  int l_vec = orc_compiler_get_temp_reg (c);
  int divisor = orc_compiler_get_temp_reg (c);
  int zero = orc_compiler_get_temp_reg (c);
  int tmp = orc_compiler_get_constant (c, 2, 0x8000);
  int mask = orc_avx512_compiler_get_mask_reg (c, TRUE);

  orc_compiler_load_constant_from_size_and_value (c, a, 2, 0x00ff);
  
  /* Create a zero register */
  orc_avx512_insn_emit_vpxord (c, zero, zero, zero, ORC_REG_INVALID, FALSE);
  
  orc_avx512_insn_emit_vmovdqa32_r_r (c, src1, divisor, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsllw_r_r_i (c, 8, src1, divisor, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpsrlw_r_r_i (c, 1, divisor, divisor, ORC_REG_INVALID, FALSE);

  orc_avx512_insn_emit_vpsrlw_r_r_i (c, 8, tmp, j, ORC_REG_INVALID, FALSE);

  orc_avx512_insn_emit_vpxord (c, src0, tmp, dest, ORC_REG_INVALID, FALSE);

  for (int i = 0; i < 7; i++) {
    orc_avx512_insn_emit_vpxord (c, divisor, tmp, l, ORC_REG_INVALID, FALSE);
    orc_avx512_insn_emit_vpcmpgtw (c, l, dest, mask, ORC_REG_INVALID);
    /* Use vpblendmw: when mask=1 select divisor, when mask=0 select zero */
    /* This replaces: movdqa(l, j2); pandn(l, divisor, l) */
    orc_avx512_insn_emit_vpblendmw (c, divisor, zero, l, mask, FALSE);
    orc_avx512_insn_emit_vpsubw (c, dest, l, dest, ORC_REG_INVALID, FALSE);
    orc_avx512_insn_emit_vpsrlw_r_r_i (c, 1, divisor, divisor, ORC_REG_INVALID, FALSE);

    /* Convert mask to vector for the pand operation */
    orc_avx512_insn_emit_vpmovm2w (c, mask, l_vec);
    orc_avx512_insn_emit_vpandd (c, l_vec, j, j2, ORC_REG_INVALID, FALSE);
    orc_avx512_insn_emit_vpxord (c, a, j2, a, ORC_REG_INVALID, FALSE);
    orc_avx512_insn_emit_vpsrlw_r_r_i (c, 1, j, j, ORC_REG_INVALID, FALSE);
  }

  orc_avx512_insn_emit_vmovdqa32_r_r (c, divisor, l, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpxord (c, l, tmp, l, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpcmpgtw (c, l, dest, mask, ORC_REG_INVALID);
  orc_avx512_insn_emit_vpmovm2w (c, mask, l_vec);
  orc_avx512_insn_emit_vpandd (c, l_vec, j, l, ORC_REG_INVALID, FALSE);
  orc_avx512_insn_emit_vpxord (c, a, l, dest, ORC_REG_INVALID, FALSE);
  
  orc_compiler_release_temp_reg (c, a);
  orc_compiler_release_temp_reg (c, j);
  orc_compiler_release_temp_reg (c, j2);
  orc_compiler_release_temp_reg (c, l);
  orc_compiler_release_temp_reg (c, l_vec);
  orc_compiler_release_temp_reg (c, divisor);
  orc_compiler_release_temp_reg (c, zero);
  orc_avx512_compiler_release_mask_reg (c, mask);
}

void
orc_avx512_rules_init (OrcTarget *target)
{
  OrcRuleSet *rule_set;

  /* FIXME the rules for now fit on the generic executable flags, once we
   * have ORC_TARGET_AVX512_F only rules, we'll change this
   */
  rule_set = orc_rule_set_new (orc_opcode_set_get ("sys"), target,
      ORC_TARGET_AVX512_F | ORC_TARGET_AVX512_VL | ORC_TARGET_AVX512_BW |
      ORC_TARGET_AVX512_VBMI);
  /* Generic ones */
  REGISTER_RULE_WITH_GENERIC (loadb, loadX);
  REGISTER_RULE_WITH_GENERIC (loadw, loadX);
  REGISTER_RULE_WITH_GENERIC (loadl, loadX);
  REGISTER_RULE_WITH_GENERIC (loadq, loadX);

  REGISTER_RULE_WITH_GENERIC (storeb, storeX);
  REGISTER_RULE_WITH_GENERIC (storew, storeX);
  REGISTER_RULE_WITH_GENERIC (storel, storeX);
  REGISTER_RULE_WITH_GENERIC (storeq, storeX);

  REGISTER_RULE_WITH_GENERIC (copyb, copyX);
  REGISTER_RULE_WITH_GENERIC (copyw, copyX);
  REGISTER_RULE_WITH_GENERIC (copyl, copyX);
  REGISTER_RULE_WITH_GENERIC (copyq, copyX);

  REGISTER_RULE_WITH_GENERIC (loadoffb, loadoffX);
  REGISTER_RULE_WITH_GENERIC (loadoffw, loadoffX);
  REGISTER_RULE_WITH_GENERIC (loadoffl, loadoffX);

  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (loadpb, loadpX, 1);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (loadpw, loadpX, 2);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (loadpl, loadpX, 4);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (loadpq, loadpX, 8);

  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (shlw, shiftX, 0);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (shruw, shiftX, 1);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (shrsw, shiftX, 2);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (shll, shiftX, 3);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (shrul, shiftX, 4);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (shrsl, shiftX, 5);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (shlq, shiftX, 6);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (shruq, shiftX, 7);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (shrsq, shiftX, 8);

  REGISTER_RULE (mullb);
  REGISTER_RULE (mullw);
  REGISTER_RULE (mulll);
  REGISTER_RULE (mulhsw);
  REGISTER_RULE (mulhuw);
  REGISTER_RULE (mulsbw);
  REGISTER_RULE (mulubw);
  REGISTER_RULE (mulhsb);
  REGISTER_RULE (mulhub);
  REGISTER_RULE (mulhsl);
  REGISTER_RULE (mulhul);

  REGISTER_RULE (xorb);
  REGISTER_RULE (xorw);
  REGISTER_RULE (xorl);
  REGISTER_RULE (xorq);

  REGISTER_RULE (absb);
  REGISTER_RULE (absw);
  REGISTER_RULE (absl);

  REGISTER_RULE (addb);
  REGISTER_RULE (addw);
  REGISTER_RULE (addl);
  REGISTER_RULE (addq);

  REGISTER_RULE (orb);
  REGISTER_RULE (orw);
  REGISTER_RULE (orl);
  REGISTER_RULE (orq);

  REGISTER_RULE (andb);
  REGISTER_RULE (andw);
  REGISTER_RULE (andl);
  REGISTER_RULE (andq);
  REGISTER_RULE (mergebw);
  REGISTER_RULE (mergewl);
  REGISTER_RULE (mergelq);

  REGISTER_RULE (andnb);
  REGISTER_RULE (andnw);
  REGISTER_RULE (andnl);
  REGISTER_RULE (andnq);

  REGISTER_RULE (addf);
  REGISTER_RULE (subf);
  REGISTER_RULE (mulf);
  REGISTER_RULE (divf);
  REGISTER_RULE (sqrtf);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (cmpeqf, cmpX, 0);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (cmpltf, cmpX, 1);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (cmplef, cmpX, 2);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (minf, minX_maxX, 0);
  REGISTER_RULE (maxf);
  REGISTER_RULE (orf);
  REGISTER_RULE (andf);
  REGISTER_RULE (convwf);
  REGISTER_RULE (convdf);

  REGISTER_RULE (addd);
  REGISTER_RULE (subd);
  REGISTER_RULE (muld);
  REGISTER_RULE (divd);
  REGISTER_RULE (sqrtd);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (mind, minX_maxX, 2);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (maxd, minX_maxX, 3);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (cmpeqd, cmpX, 3);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (cmpltd, cmpX, 4);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (cmpled, cmpX, 5);

  REGISTER_RULE (convfd);
  REGISTER_RULE (convld);

  REGISTER_RULE (convfl);
  REGISTER_RULE (convdl);
  REGISTER_RULE (convlf);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (signb, signX, 0);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (signw, signX, 1);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (signl, signX, 2);

  REGISTER_RULE (splitwb);
  REGISTER_RULE (splitlw);
  REGISTER_RULE (splitql);

  REGISTER_RULE (minsb);
  REGISTER_RULE (minub);
  REGISTER_RULE (maxsb);
  REGISTER_RULE (maxub);

  REGISTER_RULE (minsw);
  REGISTER_RULE (minuw);
  REGISTER_RULE (maxsw);
  REGISTER_RULE (maxuw);

  REGISTER_RULE (minsl);
  REGISTER_RULE (minul);
  REGISTER_RULE (maxsl);
  REGISTER_RULE (maxul);

  REGISTER_RULE (subb);
  REGISTER_RULE (subw);
  REGISTER_RULE (subq);

  REGISTER_RULE (splatbl);
  REGISTER_RULE (splatbw);
  REGISTER_RULE (splatw3q);

  REGISTER_RULE (swapw);
  REGISTER_RULE (swapl);
  REGISTER_RULE (swapq);

  REGISTER_RULE (swapwl);
  REGISTER_RULE (swaplq);

  REGISTER_RULE (select0wb);
  REGISTER_RULE (select1wb);

  REGISTER_RULE (select0lw);
  REGISTER_RULE (select1lw);

  REGISTER_RULE (select0ql);
  REGISTER_RULE (select1ql);

  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (muluwl, mulXwl, 0);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (mulswl, mulXwl, 1);

  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (mululq, mulXlq, 0);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (mulslq, mulXlq, 1);

  REGISTER_RULE (avgub);

  REGISTER_RULE (convubw);
  REGISTER_RULE (convsbw);
  REGISTER_RULE (convuwl);
  REGISTER_RULE (convswl);
  REGISTER_RULE (convulq);
  REGISTER_RULE (convslq);

  REGISTER_RULE (shlb);
  REGISTER_RULE (shrsb);
  REGISTER_RULE (shrub);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (convssswb, convsXsXX, 0);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (convsuswb, convsXsXX, 1);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (convssslw, convsXsXX, 2);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (convsuslw, convsXsXX, 3);

  REGISTER_RULE (div255w);
  REGISTER_RULE (convwb);
  REGISTER_RULE (convhwb);
  REGISTER_RULE (addusb);
  REGISTER_RULE (addusw);
  REGISTER_RULE (subusb);
  REGISTER_RULE (subusw);

  REGISTER_RULE (avgsw);
  REGISTER_RULE (avguw);
  REGISTER_RULE (avgsl);
  REGISTER_RULE (avgul);

  REGISTER_RULE (addssl);
  REGISTER_RULE (addusl);
  REGISTER_RULE (subssl);
  REGISTER_RULE (subusl);

  REGISTER_RULE (subl);

  REGISTER_RULE (convlw);
  REGISTER_RULE (convhlw);
  REGISTER_RULE (convusswb);
  REGISTER_RULE (convuuswb);
  REGISTER_RULE (convusslw);
  REGISTER_RULE (convuuslw);
  REGISTER_RULE (convql);

  REGISTER_RULE (convsssql);
  REGISTER_RULE (convsusql);
  REGISTER_RULE (convussql);
  REGISTER_RULE (convuusql);

  REGISTER_RULE (accw);
  REGISTER_RULE (accl);
  REGISTER_RULE (accsadubl);

  REGISTER_RULE (divluw);

  REGISTER_RULE (addssw);
  REGISTER_RULE (addssb);

  REGISTER_RULE (avgsb);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (cmpeqb, cmpX, 6);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (cmpgtsb, cmpX, 7);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (cmpeqw, cmpX, 8);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (cmpgtsw, cmpX, 9);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (cmpeql, cmpX, 10);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (cmpgtsl, cmpX, 11);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (cmpeqq, cmpX, 12);
  REGISTER_RULE_WITH_GENERIC_AND_PAYLOAD (cmpgtsq, cmpX, 13);

  REGISTER_RULE (subssb);
  REGISTER_RULE (subssw);

  REGISTER_RULE (loadupdb);
  REGISTER_RULE (loadupib);
}
