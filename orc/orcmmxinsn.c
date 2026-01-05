#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <inttypes.h>

#include <orc/orc.h>
#include <orc/orcinternal.h>
#include <orc/orcx86.h>
#include <orc/orcx86-private.h>
#include <orc/orcx86insn.h>
#include <orc/orcmmx.h>
#include <orc/orcmmxinsn.h>
#include <orc/orcmmx-private.h>

#define ORC_MMX_INSN_VALIDATE_OPERAND_12(op, n, reg)                           \
  if ((op->mmx_operands & ORC_MMX_INSN_OPERAND_OP##n##_MM) &&                  \
      orc_mmx_insn_validate_reg (reg)) {                                       \
    return TRUE;                                                               \
  } else {                                                                     \
    ORC_ERROR ("Unsupported operand %d with reg %d for op %s", n, reg,         \
        op->name);                                                             \
    return FALSE;                                                              \
  }

/*
 * To review, checking https://en.wikipedia.org/wiki/X86_SIMD_instruction_listings
 * PSRLQ: missing extension
 * PSLLQ: missing extension
 * PALIGNR wrong definition of prefix
 * CMPEQPS does not exist, it is CMPPS. The intrinsic is called cmpeqps tho
 * AND 0x25 /4 does not exist
 * pmovzxbw
 * pmovzxbd
 * pmovzxbq
 * pmovzxwd
 * pmovzxwq
 * pmovzxdq
 */

#define ORC_MMX_INSN_TYPE_MMX_REGM32 (\
  ORC_X86_INSN_OPERAND_REG_REGM |     \
  ORC_X86_INSN_OPERAND_OP2_32         \
), ORC_MMX_INSN_OPERAND_OP1_MM

#define ORC_MMX_INSN_TYPE_MMX_REGM64 (\
  ORC_X86_INSN_OPERAND_REG_REGM |     \
  ORC_X86_INSN_OPERAND_OP2_64         \
), ORC_MMX_INSN_OPERAND_OP1_MM

/* For example MOVD r/m32, mm */
#define ORC_MMX_INSN_TYPE_REGM32_MMX (\
  ORC_X86_INSN_OPERAND_REGM_REG |     \
  ORC_X86_INSN_OPERAND_OP1_32         \
), ORC_MMX_INSN_OPERAND_OP2_MM

#define ORC_MMX_INSN_TYPE_MMX_MMXM_IMM8 (\
  ORC_X86_INSN_OPERAND_REG_REGM_IMM |    \
  ORC_X86_INSN_OPERAND_OP3_8             \
), (ORC_MMX_INSN_OPERAND_OP2_MM |         \
  ORC_MMX_INSN_OPERAND_OP1_MM             \
)

#define ORC_MMX_INSN_TYPE_MMXM_MMX (\
  ORC_X86_INSN_OPERAND_REGM_REG     \
), (ORC_MMX_INSN_OPERAND_OP1_MM |    \
  ORC_MMX_INSN_OPERAND_OP2_MM        \
)

#define ORC_MMX_INSN_TYPE_MMX_MMXM (\
  ORC_X86_INSN_OPERAND_REG_REGM     \
), (ORC_MMX_INSN_OPERAND_OP1_MM |    \
  ORC_MMX_INSN_OPERAND_OP2_MM        \
)

/* uses mmx register and imm */
#define ORC_MMX_INSN_TYPE_MMX_REG32M16_IMM8 (\
  ORC_X86_INSN_OPERAND_REG_REGM_IMM |        \
  ORC_X86_INSN_OPERAND_OP3_8 |               \
  ORC_X86_INSN_OPERAND_OP2_32 |              \
  ORC_X86_INSN_OPERAND_OP2_16                \
), ORC_MMX_INSN_OPERAND_OP1_MM

/* For example PSRLW mm, imm8 */
#define ORC_MMX_INSN_TYPE_MMX_IMM8 (\
  ORC_X86_INSN_OPERAND_REGM_IMM |   \
  ORC_X86_INSN_OPERAND_OP2_8        \
), ORC_MMX_INSN_OPERAND_OP1_MM

typedef struct _OrcMMXInsnOpcode {
  char name[16];
  unsigned int target_flags;
  unsigned int operands;
  unsigned int mmx_operands;
  orc_uint32 opcode;
  orc_uint8 extension;
} OrcMMXInsnOp;

/* All MMX instructions have a 0x0f escape byte sequence 
 * Only the instructions with SSE3 have either the 0x3a or 0x38 escape byte sequences
 * Only ORC_MMX_palignr has the 0x3a escape byte sequence
 */

/* clang-format off */
static OrcMMXInsnOp orc_mmx_opcodes[] = {
  /* Original MMX */
  { "punpcklbw"    , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x60 },
  { "punpcklwd"    , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x61 },
  { "punpckldq"    , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x62 },
  { "packsswb"     , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x63 },
  { "pcmpgtb"      , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x64 },
  { "pcmpgtw"      , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x65 },
  { "pcmpgtd"      , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x66 },
  { "packuswb"     , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x67 },
  { "punpckhbw"    , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x68 },
  { "punpckhwd"    , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x69 },
  /* 10 */
  { "punpckhdq"    , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x6a },
  { "packssdw"     , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x6b },
  { "movd"         , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_REGM32, 0x6e },
  { "movq"         , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_REGM64, 0x6e },
  { "movq"         , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x6f },
  { "psrlw"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_IMM8, 0x71, 2 },
  { "psraw"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_IMM8, 0x71, 4 },
  { "psllw"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_IMM8, 0x71, 6 },
  { "psrld"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_IMM8, 0x72, 2 },
  { "psrad"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_IMM8, 0x72, 4 },
  { "pslld"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_IMM8, 0x72, 6 },
  /* 20 */
  { "psrlq"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_IMM8, 0x73, 2 },
  { "psllq"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_IMM8, 0x73, 6 },
  { "pcmpeqb"      , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x74 },
  { "pcmpeqw"      , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x75 },
  { "pcmpeqd"      , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x76 },
  { "emms"         , ORC_TARGET_MMX_MMX   , ORC_X86_INSN_OPERAND_NONE, 0, 0x77 },
  { "movd"         , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_REGM32_MMX, 0x7e },
  { "movq"         , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMXM_MMX, 0x7f },
  { "psrlw"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xd1 },
  { "psrld"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xd2 },
  /* 30 */
  { "psrlq"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xd3 },
  { "paddq"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xd4 },
  { "pmullw"       , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xd5 },
  { "psubusb"      , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xd8 },
  { "psubusw"      , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xd9 },
  { "pand"         , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xdb },
  { "paddusb"      , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xdc },
  { "paddusw"      , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xdd },
  { "pandn"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xdf },
  { "psraw"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xe1 },
  /* 40 */
  { "psrad"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xe2 },
  { "pmulhw"       , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xe5 },
  { "psubsb"       , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xe8 },
  { "psubsw"       , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xe9 },
  { "por"          , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xeb },
  { "paddsb"       , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xec },
  { "paddsw"       , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xed },
  { "pxor"         , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xef },
  { "psllw"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xf1 },
  { "pslld"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xf2 },
  /* 50 */
  { "psllq"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xf3 },
  { "pmaddwd"      , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xf5 },
  { "psubb"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xf8 },
  { "psubw"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xf9 },
  { "psubd"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xfa },
  { "paddb"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xfc },
  { "paddw"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xfd },
  { "paddd"        , ORC_TARGET_MMX_MMX   , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xfe },

  /* MMXEXT or SSE */
  { "pshufw"       , ORC_TARGET_MMX_MMXEXT, ORC_MMX_INSN_TYPE_MMX_MMXM_IMM8, 0x70 },
  { "pinsrw"       , ORC_TARGET_MMX_MMXEXT, ORC_MMX_INSN_TYPE_MMX_REG32M16_IMM8, 0xc4 },
  /* 60 */

  /* missing PEXTRW 0xc5 */
  /* missing PMOVMSKB 0xd7 */
  { "pminub"       , ORC_TARGET_MMX_MMXEXT, ORC_MMX_INSN_TYPE_MMX_MMXM, 0xda },
  { "pmaxub"       , ORC_TARGET_MMX_MMXEXT, ORC_MMX_INSN_TYPE_MMX_MMXM, 0xde },
  { "pavgb"        , ORC_TARGET_MMX_MMXEXT, ORC_MMX_INSN_TYPE_MMX_MMXM, 0xe0 },
  { "pavgw"        , ORC_TARGET_MMX_MMXEXT, ORC_MMX_INSN_TYPE_MMX_MMXM, 0xe3 },
  { "pmulhuw"      , ORC_TARGET_MMX_MMXEXT, ORC_MMX_INSN_TYPE_MMX_MMXM, 0xe4 },
  /* missing MOVNTQ 0xe7 */
  { "pminsw"       , ORC_TARGET_MMX_MMXEXT, ORC_MMX_INSN_TYPE_MMX_MMXM, 0xea },
  { "pmaxsw"       , ORC_TARGET_MMX_MMXEXT, ORC_MMX_INSN_TYPE_MMX_MMXM, 0xee },
  { "psadbw"       , ORC_TARGET_MMX_MMXEXT, ORC_MMX_INSN_TYPE_MMX_MMXM, 0xf6 },
  /* missing MASKMOVQ 0xf7 */

  /* MMX with SSE2 */
  { "psubq"        , ORC_TARGET_MMX_SSE2 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xfb },
  { "pmuludq"      , ORC_TARGET_MMX_SSE2 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0xf4 },
  /* 70 */

  /* MMX with SSE3 */
  { "pshufb"       , ORC_TARGET_MMX_SSSE3 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x00 },
  { "phaddw"       , ORC_TARGET_MMX_SSSE3 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x01 },
  { "phaddd"       , ORC_TARGET_MMX_SSSE3 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x02 },
  { "phaddsw"      , ORC_TARGET_MMX_SSSE3 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x03 },
  { "pmaddubsw"    , ORC_TARGET_MMX_SSSE3 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x04 },
  { "phsubw"       , ORC_TARGET_MMX_SSSE3 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x05 },
  { "phsubd"       , ORC_TARGET_MMX_SSSE3 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x06 },
  { "phsubsw"      , ORC_TARGET_MMX_SSSE3 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x07 },
  { "psignb"       , ORC_TARGET_MMX_SSSE3 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x08 },
  { "psignw"       , ORC_TARGET_MMX_SSSE3 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x09 },
  /* 80 */
  { "psignd"       , ORC_TARGET_MMX_SSSE3 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x0a },
  { "pmulhrsw"     , ORC_TARGET_MMX_SSSE3 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x0b },
  { "pabsb"        , ORC_TARGET_MMX_SSSE3 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x1c },
  { "pabsw"        , ORC_TARGET_MMX_SSSE3 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x1d },
  { "pabsd"        , ORC_TARGET_MMX_SSSE3 , ORC_MMX_INSN_TYPE_MMX_MMXM, 0x1e },
  { "palignr"      , ORC_TARGET_MMX_SSSE3 , ORC_MMX_INSN_TYPE_MMX_MMXM_IMM8, 0x0f },
};
/* clang-format on */

static orc_bool
orc_mmx_insn_validate_reg (int reg)
{
  if (reg >= X86_MM0 && reg < X86_MM0 + 16)
    return TRUE;
  else
    return FALSE;
}

static void
orc_mmx_insn_from_opcode (OrcX86Insn *insn, OrcMMXInsnIdx index, const OrcMMXInsnOp *opcode)
{
  insn->name = opcode->name;
  insn->opcode = opcode->opcode;
  insn->opcode_prefix = ORC_X86_INSN_OPCODE_PREFIX_NONE;
  insn->opcode_type = ORC_X86_INSN_OPCODE_TYPE_OTHER;
  if (index <= ORC_MMX_pmuludq) {
    insn->opcode_escape = ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F;
  } else if (index < ORC_MMX_palignr) {
    insn->opcode_escape = ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38;
  } else {
    insn->opcode_escape = ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F3A;
  }
  insn->prefix = ORC_X86_INSN_PREFIX_NO_PREFIX;
  insn->extension = opcode->extension;
  if (!orc_x86_insn_encoding_from_operands (&insn->encoding, opcode->operands,
      opcode->extension, FALSE)) {
    ORC_ERROR ("Can not encode %s (%d) opcode", opcode->name, index);
  }
}

static orc_bool
orc_mmx_insn_validate_operand (OrcX86InsnOperandNum n, int reg, void *data)
{
  OrcMMXInsnOp *op = (OrcMMXInsnOp *)data;
  switch (n) {
    case ORC_X86_INSN_OPERAND_NUM_1:
      ORC_MMX_INSN_VALIDATE_OPERAND_12 (op, 1, reg)
      break;
    case ORC_X86_INSN_OPERAND_NUM_2:
      ORC_MMX_INSN_VALIDATE_OPERAND_12 (op, 2, reg)
      break;
    default:
      ORC_ERROR ("Unsupported operand num %d", n);
      return FALSE;
  }
  return TRUE;
}

static orc_bool
orc_mmx_insn_validate_operands (OrcCompiler *c, int index,
    OrcX86InsnOperandSize size, const int dest, const int src, orc_int64 imm)
{
  const OrcMMXInsnOp *op = orc_mmx_opcodes + index;

  if (!orc_x86_insn_validate_operands (c, op->operands, size, dest, src,
      ORC_REG_INVALID, ORC_REG_INVALID, imm,
      &orc_mmx_insn_validate_operand, (void *)op)) {
    ORC_ERROR ("Failed validating %s (%d)", op->name, index);
    return FALSE;
  }

  return TRUE;
}

void
orc_mmx_emit_cpuinsn_none (OrcCompiler *p, int index)
{
  OrcX86Insn *xinsn = orc_x86_get_output_insn (p);
  const OrcMMXInsnOp *opcode = orc_mmx_opcodes + index;

  /* checks */
  if (!orc_mmx_insn_validate_operands (p, index, ORC_X86_INSN_OPERAND_SIZE_NONE,
        ORC_REG_INVALID, ORC_REG_INVALID, 0))
    return;

  orc_mmx_insn_from_opcode (xinsn, index, opcode);
  orc_x86_insn_set_operands (xinsn, opcode->operands,
      ORC_X86_INSN_OPERAND_SIZE_NONE, ORC_REG_INVALID, ORC_REG_INVALID,
      ORC_REG_INVALID, ORC_REG_INVALID);
}

void
orc_mmx_emit_cpuinsn_mmx (OrcCompiler *p, int index, int src, int dest)
{
  OrcX86Insn *xinsn;
  const OrcMMXInsnOp *opcode = orc_mmx_opcodes + index;

  /* checks */
  if (!orc_mmx_insn_validate_operands (p, index, ORC_X86_INSN_OPERAND_SIZE_NONE,
       dest, src, 0))
    return;

  xinsn = orc_x86_get_output_insn (p);
  orc_mmx_insn_from_opcode (xinsn, index, opcode);
  orc_x86_insn_set_operands (xinsn, opcode->operands,
      ORC_X86_INSN_OPERAND_SIZE_NONE, dest, src, ORC_REG_INVALID,
      ORC_REG_INVALID);
}

/* FIXME rename this function to orc_mmx_emit_cpuinsn_reg */
void
orc_mmx_emit_cpuinsn_size (OrcCompiler *p, int index, int size, int src, int dest)
{
  OrcX86Insn *xinsn;
  const OrcMMXInsnOp *opcode = orc_mmx_opcodes + index;
  const OrcX86InsnOperandSize opsize = orc_x86_insn_size_to_operand_size (size);

  /* checks */
  if (!orc_mmx_insn_validate_operands (p, index, opsize, dest, src, 0))
    return;

  xinsn = orc_x86_get_output_insn (p);
  orc_mmx_insn_from_opcode (xinsn, index, opcode);
  orc_x86_insn_set_operands (xinsn, opcode->operands, opsize, dest, src,
      ORC_REG_INVALID, ORC_REG_INVALID);
}

/* Using a REG32/64 is the case of palignr and pinsrw */
void
orc_mmx_emit_cpuinsn_imm (OrcCompiler *p, int index, int size, int imm,
    int src, int dest)
{
  OrcX86Insn *xinsn;
  const OrcMMXInsnOp *opcode = orc_mmx_opcodes + index;
  const OrcX86InsnOperandSize opsize = orc_x86_insn_size_to_operand_size (size);

  /* checks */
  if (!orc_mmx_insn_validate_operands (p, index, opsize, dest, src, imm))
    return;
  
  xinsn = orc_x86_get_output_insn (p);
  orc_mmx_insn_from_opcode (xinsn, index, opcode);
  orc_x86_insn_set_operands (xinsn, opcode->operands, opsize, dest, src,
      ORC_REG_INVALID, ORC_REG_INVALID);
  orc_x86_insn_set_imm (xinsn, opcode->operands, ORC_X86_INSN_OPERAND_SIZE_8, imm);
}

/* imm is conditional used
 * only used for pinsrw, movd_load, movq_mmx_load
 */
void
orc_mmx_emit_cpuinsn_load_memoffset (OrcCompiler *p, int index, int imm,
    int offset, int src, int dest)
{
  OrcX86Insn *xinsn;
  const OrcMMXInsnOp *opcode = orc_mmx_opcodes + index;
  const OrcX86InsnOperandSize size = p->is_64bit ?
      ORC_X86_INSN_OPERAND_SIZE_64 : ORC_X86_INSN_OPERAND_SIZE_32;

  /* checks */
  if (!orc_mmx_insn_validate_operands (p, index, size, dest, src, imm))
    return;

  xinsn = orc_x86_get_output_insn (p);
  orc_mmx_insn_from_opcode (xinsn, index, opcode);
  orc_x86_insn_set_operands (xinsn, opcode->operands,
      ORC_X86_INSN_OPERAND_SIZE_NONE, dest, ORC_REG_INVALID, ORC_REG_INVALID,
      ORC_REG_INVALID);
  orc_x86_insn_set_mem (xinsn, opcode->operands, size,
      ORC_X86_INSN_OPERAND_TYPE_OFF, ORC_REG_INVALID, src, ORC_REG_INVALID,
      ORC_REG_INVALID);
  orc_x86_insn_set_imm (xinsn, opcode->operands, ORC_X86_INSN_OPERAND_SIZE_8, imm);
  xinsn->offset = offset;
}

void
orc_mmx_emit_cpuinsn_load_memindex (OrcCompiler *p, int index, int imm,
    int offset, int src, int src_index, int shift, int dest)
{
  OrcX86Insn *xinsn;
  const OrcMMXInsnOp *opcode = orc_mmx_opcodes + index;
  const OrcX86InsnOperandSize size = p->is_64bit ?
      ORC_X86_INSN_OPERAND_SIZE_64 : ORC_X86_INSN_OPERAND_SIZE_32;

  /* checks */
  if (!orc_mmx_insn_validate_operands (p, index, size, dest, src, imm))
    return;

  xinsn = orc_x86_get_output_insn (p);
  orc_mmx_insn_from_opcode (xinsn, index, opcode);
  orc_x86_insn_set_operands (xinsn, opcode->operands,
      ORC_X86_INSN_OPERAND_SIZE_NONE, dest, ORC_REG_INVALID, ORC_REG_INVALID,
      ORC_REG_INVALID);
  orc_x86_insn_set_mem (xinsn, opcode->operands, size,
      ORC_X86_INSN_OPERAND_TYPE_IDX, ORC_REG_INVALID, src, ORC_REG_INVALID,
      ORC_REG_INVALID);
  orc_x86_insn_set_imm (xinsn, opcode->operands, ORC_X86_INSN_OPERAND_SIZE_8, imm);
  xinsn->offset = offset;
  xinsn->index_reg = src_index;
  xinsn->shift = shift;
}

void
orc_mmx_emit_cpuinsn_store_memoffset (OrcCompiler *p, int index, int imm,
    int offset, int src, int dest)
{
  OrcX86Insn *xinsn;
  const OrcMMXInsnOp *opcode = orc_mmx_opcodes + index;
  const OrcX86InsnOperandSize size = p->is_64bit ?
      ORC_X86_INSN_OPERAND_SIZE_64 : ORC_X86_INSN_OPERAND_SIZE_32;

  /* checks */
  if (!orc_mmx_insn_validate_operands (p, index, size, dest, src, imm))
    return;

  xinsn = orc_x86_get_output_insn (p);
  orc_mmx_insn_from_opcode (xinsn, index, opcode);
  orc_x86_insn_set_operands (xinsn, opcode->operands,
      ORC_X86_INSN_OPERAND_SIZE_NONE, ORC_REG_INVALID, src, ORC_REG_INVALID,
      ORC_REG_INVALID);
  orc_x86_insn_set_mem (xinsn, opcode->operands, size,
      ORC_X86_INSN_OPERAND_TYPE_OFF, dest, ORC_REG_INVALID, ORC_REG_INVALID,
      ORC_REG_INVALID);
  orc_x86_insn_set_imm (xinsn, opcode->operands, ORC_X86_INSN_OPERAND_SIZE_8, imm);
  xinsn->offset = offset;
}
