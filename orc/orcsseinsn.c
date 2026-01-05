#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <inttypes.h>

#include <orc/orc.h>
#include <orc/orcinternal.h>
#include <orc/orcx86.h>
#include <orc/orcx86-private.h>
#include <orc/orcx86insn.h>
#include <orc/orcsse.h>
#include <orc/orcsseinsn.h>
#include <orc/orcsse-private.h>

#define ORC_SSE_INSN_VALIDATE_OPERAND_12(op, n, reg)                           \
  if ((op->sse_operands & ORC_SSE_INSN_OPERAND_OP##n##_XMM) &&                 \
      orc_sse_insn_validate_reg (reg)) {                                       \
    return TRUE;                                                               \
  } else {                                                                     \
    ORC_ERROR ("Unsupported operand %d with reg %d for op %s", n, reg,         \
        op->name);                                                             \
    return FALSE;                                                              \
  }

typedef struct _OrcSSEInsnOp {
  char name[16];
  unsigned int target_flags;
  unsigned int operands;
  unsigned int sse_operands;
  OrcX86InsnOpcodePrefix prefix;
  OrcX86InsnOpcodeEscapeSequence escape;
  orc_uint32 opcode;
  orc_uint8 extension;
} OrcSSEInsnOp;

/* clang-format off */
static OrcSSEInsnOp orc_sse_ops[] = {
  /* Missing MOVUPS */
  /* Missing MOVSS */
  /* Missing MOVUPS */
  /* Missing MOVLPS */
  /* Missing MOVHLPS */
  /* Missing MOVLPS */
  /* Missing UNPCKLPS */
  /* Missing UNPCKHPS */
  /* Missing MOVLHPS */
  { "movhps"    , 0, ORC_SSE_INSN_TYPE_SSE_MEM64, ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x16 },
  /* Missing MOVHPS */
  /* Missing MOVAPS */ 
  /* Missing MOVAPS */
  /* Missing CVTPI2PS */
  /* Missing CVTSI2SS */
  /* Missing CVTSI2SS */
  /* Missing MOVNTPS */
  /* Missing CVTTPS2PI */
  /* Missing CVTTSS2SI */
  /* Missing CVTTSS2SI */
  /* Missing CVTPS2PI  */
  /* Missing CVTSS2SI */
  /* Missing CVTSS2SI */
  /* Missing UCOMISS */
  /* Missing COMISS */
  /* Missing MOVMSKPS */
  { "sqrtps"    , 0, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x51 },
  /* Missing SQRTSS */
  /* Missing RSQRTSS */
  /* Missing RSQRTPS */
  /* Missing RCPPS */
  /* Missing RCPSS */
  { "andps"     , 0, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x54 },
  /* Missing ANDNPS */
  { "orps"      , 0, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x56 },
  /* Missing XORPS */
  { "addps"     , 0, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x58 },
  /* Missing ADDSS */
  { "mulps"     , 0, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x59 },
  /* Missing MULSS */
  { "subps"     , 0, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x5c },
  /* Missing SUBSS */
  { "minps"     , 0, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x5d },
  /* Missing MINSS */
  { "divps"     , 0, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x5e },
  /* Missing DIVSS */
  { "maxps"     , 0, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x5f },
  /* 10 */
  /* Missing MAXSS */
  { "ldmxcsr"   , 0, ORC_X86_INSN_TYPE_MEM32, 0, ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xae, 2 },
  { "stmxcsr"   , 0, ORC_X86_INSN_TYPE_MEM32, 0, ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xae, 3 },
  /* Missing CMPPS */
  /* Missing CMPSS */
  /* Missing SHUFPS */

  /* SSE with SSE2 */
  /* Missing MOVUPD */
  /* Missing MOVSD  */
  /* Missing MOVUPD */
  /* Missing MOVSD  */
  /* Missing MOVLPD */
  /* Missing MOVLPD */
  /* Missing UNPCKLPD */
  /* Missing UNPCKHPD */
  /* Missing MOVHPD */
  /* Missing MOVHPD */
  /* Missing MOVAPD */
  /* Missing MOVAPD */
  /* Missing CVTSI2SD */
  /* Missing CVTPI2PD */
  /* Missing CVTSI2SD */
  /* Missing MOVNTPD  */
  /* Missing CVTTSD2SI  */
  /* Missing CVTTPD2PI  */
  /* Missing CVTTSD2SI  */
  /* Missing CVTSD2SI */
  /* Missing CVTPD2PI */
  /* Missing CVTSD2SI */
  /* Missing UCOMISD  */
  /* Missing COMISD */
  /* Missing MOVMSKPD */
  { "sqrtpd"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x51 },
  /* Missing SQRTSD */
  /* Missing ANDPD */
  /* Missing ANDNPD */
  /* Missing ORPD */
  /* Missing XORPD */
  { "addpd"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x58 },
  /* Missing ADDSD */ 
  { "mulpd"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x59 },
  /* Missing MULSD */
  { "cvtps2pd"  , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x5a },
  { "cvtpd2ps"  , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x5a },
  /* Missing CVTSD2SS */
  /* Missing CVTSS2SD */
  { "cvtdq2ps"  , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x5b },
  /* Missing CVTPS2DQ */ 
  { "cvttps2dq" , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0XF3, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x5b },
  { "subpd"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x5c },
  /* 20 */
  /* Missing SUBSD */
  { "minpd"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x5d },
  /* Missing MINSD */
  { "divpd"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x5e },
  /* Missing DIVSD */
  { "maxpd"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x5f },
  /* Missing MAXSD */
  { "cmpeqpd"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xc2, 0 },
  { "cmpltpd"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xc2, 1 },
  { "cmplepd"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xc2, 2 },
  { "cmpeqps"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xc2, 0 },
  { "cmpltps"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xc2, 1 },
  { "cmpleps"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_NONE, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xc2, 2 },
 
  /* Missing SHUFPD */
  { "cvtdq2pd"  , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0XF3, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xe6 },
  /* Missing CVTPD2DQ */
  /* 30 */
  { "cvttpd2dq" , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xe6 },

  /* MMX ones, not all */
  { "punpcklbw" , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x60 },
  { "punpcklwd" , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x61 },
  { "punpckldq" , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x62 },
  { "packsswb"  , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x63 },
  { "pcmpgtb"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x64 },
  { "pcmpgtw"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x65 },
  { "pcmpgtd"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x66 },
  { "packuswb"  , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x67 },
  { "punpckhbw" , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x68 },
  /* 40 */
  { "punpckhwd" , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x69 },
  { "punpckhdq" , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x6a },
  { "packssdw"  , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x6b },
  { "movq"      , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_REGM64, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x6e }, // load
  { "movd"      , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_REGM32, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x6e },
  { "psrlw"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_IMM8, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x71, 2 },
  { "psraw"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_IMM8, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x71, 4 },
  { "psllw"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_IMM8, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x71, 6 },
  { "psrld"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_IMM8, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x72, 2 },
  { "psrad"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_IMM8, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x72, 4 },
  /* 50 */
  { "pslld"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_IMM8, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x72, 6 },
  { "psrlq"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_IMM8, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x73, 2 },
  { "psllq"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_IMM8, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x73, 6 },
  { "pcmpeqb"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x74 },
  { "pcmpeqw"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x75 },
  { "pcmpeqd"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x76 },
  { "movd"      , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_REGM32_SSE, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x7e },
  { "movq"      , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM64, ORC_X86_INSN_OPCODE_PREFIX_0XF3, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x7e },
  { "movq"      , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_REGM64_SSE, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x7e }, // store
  { "pinsrw"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_REG32M_IMM8, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xc4 },
  /* 60 */
  { "pextrw"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_REG32TO64_SSE_IMM8, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xc5 },
  /* Missing PEXTRW ORC_X86_INSN_OPCODE_PREFIX_0X66 ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xc5 */
  { "psrlw"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xd1 },
  { "psrld"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xd2 },
  { "psrlq"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xd3 },
  { "paddq"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xd4 },
  { "pmullw"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xd5 },
  { "movq"      , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSEM64_SSE, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xd6 },
  /* Missing PMOVMSKB */
  { "psubusb"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xd8 },
  { "psubusw"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xd9 },
  { "pminub"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xda },
  { "pand"      , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xdb },
  /* 70 */
  { "paddusb"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xdc },
  { "paddusw"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xdd },
  { "pmaxub"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xde },
  { "pandn"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xdf },
  { "pavgb"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xe0 },
  { "psraw"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xe1 },
  { "psrad"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xe2 },
  { "pavgw"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xe3 },
  { "pmulhuw"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xe4 },
  { "pmulhw"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xe5 },
  /* 80 */
  { "psubsb"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xe8 },
  { "psubsw"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xe9 },
  { "pminsw"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xea },
  { "por"       , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xeb },
  { "paddsb"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xec },
  { "paddsw"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xed },
  { "pmaxsw"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xee },
  { "pxor"      , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xef },
  { "psllw"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xf1 },
  /* 90 */
  { "pslld"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xf2 },
  { "psllq"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xf3 },
  { "pmuludq"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xf4 },
  { "pmaddwd"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xf5 },
  { "psadbw"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xf6 },
  { "psubb"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xf8 },
  { "psubw"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xf9 },
  { "psubd"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xfa },
  { "psubq"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xfb },
  { "paddb"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xfc },
  /* 100 */
  { "paddw"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xfd },
  { "paddd"     , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xfe },
  /* SSE2 for SSE only */
  { "punpcklqdq", ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x6c },
  { "punpckhqdq", ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x6d },
  { "movdqa"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x6f },
  { "pshufd"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM_IMM8, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x70 },
  { "movdqa"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSEM_SSE     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x7f },
  { "psrldq"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_IMM8     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x73, 3 },
  { "pslldq"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_IMM8     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x73, 7 },
  { "movntdq"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_MEM_SSE      , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0xe7 },
  /* 110 */
  /* Missing MASKMOVDQU */
  { "pshuflw"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM_IMM8, ORC_X86_INSN_OPCODE_PREFIX_0XF2, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x70 },
  /* Missing MOVDQ2Q */
  { "movdqu"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0XF3, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x6f },
  { "movdqu"    , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSEM_SSE     , ORC_X86_INSN_OPCODE_PREFIX_0XF3, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x7f },
  { "pshufhw"   , ORC_TARGET_SSE_SSE2, ORC_SSE_INSN_TYPE_SSE_SSEM_IMM8, ORC_X86_INSN_OPCODE_PREFIX_0XF3, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F, 0x70 },
  /* Missing MOVQ2DQ */
  /* SSE with SSE3 */
  /* Missing MOVDDUP */
  /* Missing MOVSLDUP */
  /* Missing MOVSHDUP */
  { "pshufb"    , ORC_TARGET_SSE_SSE3, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x00 },
  { "phaddw"    , ORC_TARGET_SSE_SSE3, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x01 },
  { "phaddd"    , ORC_TARGET_SSE_SSE3, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x02 },
  { "phaddsw"   , ORC_TARGET_SSE_SSE3, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x03 },
  { "pmaddubsw" , ORC_TARGET_SSE_SSE3, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x04 },
  { "phsubw"    , ORC_TARGET_SSE_SSE3, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x05 },
  /* 120 */
  { "phsubd"    , ORC_TARGET_SSE_SSE3, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x06 },
  { "phsubsw"   , ORC_TARGET_SSE_SSE3, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x07 },
  { "psignb"    , ORC_TARGET_SSE_SSE3, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x08 },
  { "psignw"    , ORC_TARGET_SSE_SSE3, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x09 },
  { "psignd"    , ORC_TARGET_SSE_SSE3, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x0a },
  { "pmulhrsw"  , ORC_TARGET_SSE_SSE3, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x0b },
  { "palignr"   , ORC_TARGET_SSE_SSE3, ORC_SSE_INSN_TYPE_SSE_SSEM_IMM8, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F3A, 0x0f },
  { "pabsb"     , ORC_TARGET_SSE_SSE3, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x1c },
  { "pabsw"     , ORC_TARGET_SSE_SSE3, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x1d },
  { "pabsd"     , ORC_TARGET_SSE_SSE3, ORC_SSE_INSN_TYPE_SSE_SSEM     , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x1e },
  /* Missing PALIGNR */
  /* Missing HADDPS */
  /* Missing HADDPD */
  /* Missing HSUBPS */
  /* Missing HSUBPD */
  /* Missing ADDSUBPS */
  /* Missing ADDSUBPD */
  /* Missing LDDQU */
  /* SSE with SSE4.1 */
  /* Missing ROUNDPS */
  /* Missing ROUNDPD */
  /* Missing ROUNDSS */
  /* Missing ROUNDSD */
  /* Missing BLENDPS */
  /* Missing BLENDPD */
  /* Missing PBLENDW */
  /* Missing PBLENDVB */
  /* Missing BLENDVPS */
  /* 130 */
  { "pextrb"    , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_REG32TO64M8_SSE_IMM8 , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F3A, 0x14 },
  { "blendvpd"  , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM             , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x15 },
  { "pextrw"    , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_REG32TO64M16_SSE_IMM8, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F3A, 0x15 },
  /* Missing PEXTRD */
  /* Missing PEXTRQ */
  /* Missing EXTRACTPS */
  /* Missing PTEST */
  { "pinsrb"    , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_REG32M8_IMM8  , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F3A, 0x20 },
  { "pmovsxbw"  , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x20 },
  /* Missing INSERTPS */
  { "pmovsxbd"  , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x21 },
  { "pinsrd"   , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_REG32M_IMM8, ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F3A, 0x22 },
  /* Missing PINSRQ */
  { "pmovsxbq"  , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x22 },
  { "pmovsxwd"  , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x23 },
  { "pmovsxwq"  , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x24 },
  /* 140 */
  { "pmovsxdq"  , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x25 },
  { "pmuldq"    , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x28 },
  { "pcmpeqq"   , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x29 },
  /* Missing MOVNTDQA */
  { "packusdw"  , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x2b },
  { "pmovzxbw"  , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x30 },
  { "pmovzxbd"  , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x31 },
  { "pmovzxbq"  , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x32 },
  { "pmovzxwd"  , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x33 },
  { "pmovzxwq"  , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x34 },
  { "pmovzxdq"  , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x35 },
  /* 150 */
  { "pminsb"    , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x38 },
  { "pminsd"    , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x39 },
  { "pminuw"    , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x3A },
  { "pminud"    , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x3B },
  { "pmaxsb"    , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x3C },
  { "pmaxsd"    , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x3D },
  { "pmaxuw"    , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x3E },
  { "pmaxud"    , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x3F },
  /* Missing DPPS */
  { "pmulld"    , ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x40 },
  /* Missing DPPD */
  { "phminposuw", ORC_TARGET_SSE_SSE4_1, ORC_SSE_INSN_TYPE_SSE_SSEM          , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x41 },
  /* Missing MPSADBW */

  /* Missing SSE with SSE4a */
  /* SSE with SSE4.2 */
  /* 160 */
  { "pcmpgtq"   , ORC_TARGET_SSE_SSE4A, ORC_SSE_INSN_TYPE_SSE_SSEM           , ORC_X86_INSN_OPCODE_PREFIX_0X66, ORC_X86_INSN_OPCODE_ESCAPE_SEQUENCE_0X0F38, 0x37 },
};
/* clang-format on */

static void
orc_sse_insn_from_opcode (OrcX86Insn *insn, int index, const OrcSSEInsnOp *op)
{
  insn->name = op->name;
  insn->opcode = op->opcode;
  insn->opcode_type = ORC_X86_INSN_OPCODE_TYPE_OTHER;
  insn->opcode_escape = op->escape;
  insn->opcode_prefix = op->prefix;
  insn->prefix = ORC_X86_INSN_PREFIX_NO_PREFIX;
  insn->extension = op->extension;

  if (!orc_x86_insn_encoding_from_operands (&insn->encoding, op->operands,
      op->extension, FALSE)) {
    ORC_ERROR ("Can not encode %s (%d) opcode", op->name, index);
  }
}

static orc_bool
orc_sse_insn_validate_reg (int reg)
{
  if (reg >= X86_XMM0 && reg < X86_XMM0 + 16)
    return TRUE;
  else
    return FALSE;
}

static orc_bool
orc_sse_insn_validate_operand (OrcX86InsnOperandNum n, int reg, void *data)
{
  OrcSSEInsnOp *op = (OrcSSEInsnOp *)data;
  switch (n) {
    case ORC_X86_INSN_OPERAND_NUM_1:
      ORC_SSE_INSN_VALIDATE_OPERAND_12 (op, 1, reg)
      break;
    case ORC_X86_INSN_OPERAND_NUM_2:
      ORC_SSE_INSN_VALIDATE_OPERAND_12 (op, 2, reg)
      break;
    default:
      ORC_ERROR ("Unsupported operand num %d", n);
      return FALSE;
  }
  return TRUE;
}

static orc_bool
orc_sse_insn_validate_operands (OrcCompiler *c, int index,
    OrcX86InsnOperandSize size, const int dest, const int src, orc_int64 imm)
{
  const OrcSSEInsnOp *op = orc_sse_ops + index;

  if (!orc_x86_insn_validate_operands (c, op->operands, size, dest, src,
      ORC_REG_INVALID, ORC_REG_INVALID, imm,
      &orc_sse_insn_validate_operand, (void *)op)) {
    ORC_ERROR ("Failed validating %s (%d)", op->name, index);
    return FALSE;
  }

  return TRUE;
}

static OrcX86Insn *
orc_sse_emit_cpuinsn_load_mem (OrcCompiler *p, int index,
    OrcX86InsnOperandType mem_type, int imm, int src, int dest)
{
  OrcX86Insn *xinsn;
  const OrcSSEInsnOp *opcode = orc_sse_ops + index;
  const OrcX86InsnOperandSize size = p->is_64bit ?
      ORC_X86_INSN_OPERAND_SIZE_64 : ORC_X86_INSN_OPERAND_SIZE_32;

  /* checks */
  if (!orc_sse_insn_validate_operands (p, index, size, dest, src, imm))
    return NULL;

  xinsn = orc_x86_get_output_insn (p);
  orc_sse_insn_from_opcode (xinsn, index, opcode);
  orc_x86_insn_set_operands (xinsn, opcode->operands,
      ORC_X86_INSN_OPERAND_SIZE_NONE, dest, ORC_REG_INVALID, ORC_REG_INVALID,
      ORC_REG_INVALID);
  orc_x86_insn_set_mem (xinsn, opcode->operands, size,
      mem_type, ORC_REG_INVALID, src, ORC_REG_INVALID, ORC_REG_INVALID);
  orc_x86_insn_set_imm (xinsn, opcode->operands, ORC_X86_INSN_OPERAND_SIZE_8,
      imm);
  return xinsn;
}

void
orc_x86_emit_mov_memoffset_sse (OrcCompiler *compiler, int size, int offset,
    int reg1, int reg2, int is_aligned)
{
  switch (size) {
    case 4:
      orc_sse_emit_movd_load_memoffset (compiler, offset, reg1, reg2);
      break;
    case 8:
      orc_sse_emit_movq_load_memoffset (compiler, offset, reg1, reg2);
      break;
    case 16:
      if (is_aligned) {
        orc_sse_emit_movdqa_load_memoffset (compiler, offset, reg1, reg2);
      } else {
        orc_sse_emit_movdqu_load_memoffset (compiler, offset, reg1, reg2);
      }
      break;
    default:
      ORC_COMPILER_ERROR(compiler, "bad size");
      break;
  }
}

void
orc_x86_emit_mov_memindex_sse (OrcCompiler *compiler, int size, int offset,
    int reg1, int regindex, int shift, int reg2, int is_aligned)
{
  switch (size) {
    case 4:
      orc_sse_emit_movd_load_memindex (compiler, offset,
          reg1, regindex, shift, reg2);
      break;
    case 8:
      orc_sse_emit_movq_load_memindex (compiler, offset,
          reg1, regindex, shift, reg2);
      break;
    case 16:
      if (is_aligned) {
        orc_sse_emit_movdqa_load_memindex (compiler, offset,
            reg1, regindex, shift, reg2);
      } else {
        orc_sse_emit_movdqu_load_memindex (compiler, offset,
            reg1, regindex, shift, reg2);
      }
      break;
    default:
      ORC_COMPILER_ERROR(compiler, "bad size");
      break;
  }
}

void
orc_x86_emit_mov_sse_memoffset (OrcCompiler *compiler, int size, int reg1, int offset,
    int reg2, int aligned, int uncached)
{
  switch (size) {
    case 4:
      orc_sse_emit_movd_store_memoffset (compiler, offset, reg1, reg2);
      break;
    case 8:
      orc_sse_emit_movq_store_memoffset (compiler, offset, reg1, reg2);
      break;
    case 16:
      if (aligned) {
        if (uncached) {
          orc_sse_emit_movntdq_store_memoffset (compiler, offset, reg1, reg2);
        } else {
          orc_sse_emit_movdqa_store_memoffset (compiler, offset, reg1, reg2);
        }
      } else {
        orc_sse_emit_movdqu_store_memoffset (compiler, offset, reg1, reg2);
      }
      break;
    default:
      ORC_COMPILER_ERROR(compiler, "bad size");
      break;
  }
}

void
orc_sse_set_mxcsr (OrcCompiler *compiler)
{
  orc_sse_emit_cpuinsn_store_memoffset (compiler, ORC_SSE_stmxcsr, 0,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,params[ORC_VAR_A4]),
      0, compiler->exec_reg);

  orc_x86_emit_mov_memoffset_reg (compiler, 4,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,params[ORC_VAR_A4]),
      compiler->exec_reg, compiler->gp_tmpreg);

  orc_x86_emit_mov_reg_memoffset (compiler, 4, compiler->gp_tmpreg,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,params[ORC_VAR_C1]),
      compiler->exec_reg);

  orc_x86_emit_cpuinsn_imm_reg (compiler, ORC_X86_or_imm32_rm, 4,
      0x8040, compiler->gp_tmpreg);

  orc_x86_emit_mov_reg_memoffset (compiler, 4, compiler->gp_tmpreg,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,params[ORC_VAR_A4]),
      compiler->exec_reg);

  orc_sse_emit_cpuinsn_store_memoffset (compiler, ORC_SSE_ldmxcsr, 0,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,params[ORC_VAR_A4]),
      0, compiler->exec_reg);
}

void
orc_sse_restore_mxcsr (OrcCompiler *compiler)
{
  orc_sse_emit_cpuinsn_store_memoffset (compiler, ORC_SSE_ldmxcsr, 0,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,params[ORC_VAR_A4]),
      0, compiler->exec_reg);
}

void
orc_sse_emit_cpuinsn_sse (OrcCompiler *p, int index, int src, int dest)
{
  OrcX86Insn *xinsn;
  const OrcSSEInsnOp *opcode = orc_sse_ops + index;

  /* checks */
  if (!orc_sse_insn_validate_operands (p, index, ORC_X86_INSN_OPERAND_SIZE_NONE,
       dest, src, 0))
    return;

  xinsn = orc_x86_get_output_insn (p);
  orc_sse_insn_from_opcode (xinsn, index, opcode);
  orc_x86_insn_set_operands (xinsn, opcode->operands,
      ORC_X86_INSN_OPERAND_SIZE_NONE, dest, src, ORC_REG_INVALID,
      ORC_REG_INVALID);
  /* Special case for cmpeqpd until cmpleps, all are REG_REGM_IMM8 but faked on
   * the definition
   */
  if (index >= ORC_SSE_cmpeqpd && index <= ORC_SSE_cmpleps) {
    xinsn->encoding = ORC_X86_INSN_ENCODING_RMI;
    orc_x86_insn_operand_set (&xinsn->operands[2],
        ORC_X86_INSN_OPERAND_TYPE_IMM, ORC_X86_INSN_OPERAND_SIZE_8, 0);
    xinsn->imm = opcode->extension;
  }
}

void
orc_sse_emit_cpuinsn_size (OrcCompiler *p, int index, int size, int src, int dest)
{
  OrcX86Insn *xinsn;
  const OrcSSEInsnOp *opcode = orc_sse_ops + index;
  const OrcX86InsnOperandSize opsize = orc_x86_insn_size_to_operand_size (size);

  /* checks */
  if (!orc_sse_insn_validate_operands (p, index, opsize, dest, src, 0))
    return;

  xinsn = orc_x86_get_output_insn (p);
  orc_sse_insn_from_opcode (xinsn, index, opcode);
  orc_x86_insn_set_operands (xinsn, opcode->operands, opsize, dest, src,
      ORC_REG_INVALID, ORC_REG_INVALID);
}

/*
 * Used in
 * ORC_SSE_palignr (SSE_SSEM_IMM8)
 * ORC_SSE_pshufhw (SSE_SSEM_IMM8)
 * ORC_SSE_pshuflw (SSE_SSEM_IMM8)
 * ORC_SSE_pslldq (SSE_IMM8) 
 * ORC_SSE_psrlldq (SSE_IMM8)
 * ORC_SSE_pshufd (SSE_SSEM_IMM8)
 * ORC_SSE_pinsrw (SSE_REG32M_IMM8)
 * ORC_SSE_pextrw (REGM32TO64_SSE_IMM8)
 * ORC_SSE_psllq_imm (SSE_IMM8) 
 * ORC_SSE_psrlq_imm (SSE_IMM8)
 * ORC_SSE_pslld_imm (SSE_IMM8)
 * ORC_SSE_psrad_imm (SSE_IMM8)
 * ORC_SSE_psrld_imm (SSE_IMM8)
 * ORC_SSE_psllw_imm (SSE_IMM8)
 * ORC_SSE_psraw_imm (SSE_IMM8)
 * ORC_SSE_psrlw_imm (SSE_IMM8)
 */
void
orc_sse_emit_cpuinsn_imm (OrcCompiler *p, int index, int size, int imm, int src, int dest)
{
  OrcX86Insn *xinsn;
  const OrcSSEInsnOp *opcode = orc_sse_ops + index;
  const OrcX86InsnOperandSize opsize = orc_x86_insn_size_to_operand_size (size);

  /* checks */
  if (!orc_sse_insn_validate_operands (p, index, opsize, dest, src, imm))
    return;
  
  xinsn = orc_x86_get_output_insn (p);
  orc_sse_insn_from_opcode (xinsn, index, opcode);
  orc_x86_insn_set_operands (xinsn, opcode->operands, opsize, dest, src,
      ORC_REG_INVALID, ORC_REG_INVALID);
  orc_x86_insn_set_imm (xinsn, opcode->operands, ORC_X86_INSN_OPERAND_SIZE_8, imm);
}

/*
 * Used in
 * ORC_SSE_movhps_load (SSE_REGM)
 * ORC_SSE_movd_load (SSE_REGM32)
 * ORC_SSE_movq_sse_load (SSE_SSEM)
 * ORC_SSE_pinsrw (SSE_REG32M_IMM8)
 * ORC_SSE_movdqa_load (SSE_SSEM)
 * ORC_SSE_movdqu_load (SSE_SSEM)
 * ORC_SSE_pinsrb SSE_REG32M_IMM8)
 */
void
orc_sse_emit_cpuinsn_load_memoffset (OrcCompiler *p, int index, int imm,
    int offset, int src, int dest)
{
  OrcX86Insn *xinsn;
  xinsn = orc_sse_emit_cpuinsn_load_mem (p, index,
      ORC_X86_INSN_OPERAND_TYPE_OFF, imm, src, dest);
  if (xinsn) {
    xinsn->offset = offset;
  }
}

/*
 * Used in
 * ORC_SSE_movhps_load (SSE_REGM)
 * ORC_SSE_movd_load (SSE_REGM32)
 * ORC_SSE_movq_sse_load (SSE_SSEM)
 * ORC_SSE_pinsrw (SSE_REG32M16_IMM8)
 * ORC_SSE_movdqa_load (SSE_SSEM)
 * ORC_SSE_movdqu_load (SSE_SSEM)
 */
void
orc_sse_emit_cpuinsn_load_memindex (OrcCompiler *p, int index, int imm,
    int offset, int src, int src_index, int shift, int dest)
{
  OrcX86Insn *xinsn;

  xinsn = orc_sse_emit_cpuinsn_load_mem (p, index,
      ORC_X86_INSN_OPERAND_TYPE_IDX, imm, src, dest);
  if (xinsn) {
    xinsn->offset = offset;
    xinsn->index_reg = src_index;
    xinsn->shift = shift;
  }
}

/*
 * Used in
 * ORC_SSE_movdqa_store (SSEM_SSE)
 * ORC_SSE_movntdq_store (SSEM_SSE)
 * ORC_SSE_movdqu_store (SSEM_SSE)
 * ORC_SSE_pextrb (REGM32TO64_SSE_IMM8)
 * ORC_SSE_pextrw_mem (REGM32TO64_SSE_IMM8)
 * ORC_SSE_stmxcsr (M32)
 * ORC_SSE_ldmxcsr (M32)
 */
void
orc_sse_emit_cpuinsn_store_memoffset (OrcCompiler *p, int index,
    int imm, int offset, int src, int dest)
{
  OrcX86Insn *xinsn;
  const OrcSSEInsnOp *opcode = orc_sse_ops + index;
  const OrcX86InsnOperandSize size = p->is_64bit ?
      ORC_X86_INSN_OPERAND_SIZE_64 : ORC_X86_INSN_OPERAND_SIZE_32;

  /* checks */
  if (!orc_sse_insn_validate_operands (p, index, size, dest, src, imm))
    return;

  xinsn = orc_x86_get_output_insn (p);
  orc_sse_insn_from_opcode (xinsn, index, opcode);
  orc_x86_insn_set_operands (xinsn, opcode->operands,
      ORC_X86_INSN_OPERAND_SIZE_NONE, dest, src, ORC_REG_INVALID,
      ORC_REG_INVALID);
  orc_x86_insn_set_mem (xinsn, opcode->operands, size,
      ORC_X86_INSN_OPERAND_TYPE_OFF, dest, src, ORC_REG_INVALID, 
      ORC_REG_INVALID);
  orc_x86_insn_set_imm (xinsn, opcode->operands, ORC_X86_INSN_OPERAND_SIZE_8, imm);
  xinsn->offset = offset;
}
