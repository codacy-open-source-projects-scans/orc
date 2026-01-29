#pragma once

#include <orc/orcutils.h>
#include <orc/orcsse.h>
#include <orc/orcsse-private.h>

ORC_BEGIN_DECLS

/* orcavxinsn.c */
/* This extends OrcSSEInsnOperandFlag */
typedef enum _OrcAVXInsnOperandFlag {
  ORC_AVX_INSN_OPERAND_OP1_YMM = (1 << (ORC_SSE_INSN_OPERAND_FLAG_LAST + 1)), 
  ORC_AVX_INSN_OPERAND_OP2_YMM = (1 << (ORC_SSE_INSN_OPERAND_FLAG_LAST + 2)),
  ORC_AVX_INSN_OPERAND_OP3_YMM = (1 << (ORC_SSE_INSN_OPERAND_FLAG_LAST + 3)),
  ORC_AVX_INSN_OPERAND_OP4_YMM = (1 << (ORC_SSE_INSN_OPERAND_FLAG_LAST + 4)),
  ORC_AVX_INSN_OPERAND_OP3_XMM = (1 << (ORC_SSE_INSN_OPERAND_FLAG_LAST + 5)),
  ORC_AVX_INSN_OPERAND_OP4_XMM = (1 << (ORC_SSE_INSN_OPERAND_FLAG_LAST + 6)),
} OrcAVXInsnOperandFlag;

#define ORC_AVX_INSN_OPERAND_FLAG_LAST (ORC_SSE_INSN_OPERAND_FLAG_LAST + 6)

#define ORC_AVX_INSN_TYPE_MEM_AVX (\
  ORC_X86_INSN_OPERAND_OP1_MEM     \
), ORC_AVX_INSN_OPERAND_OP2_YMM

#define ORC_AVX_INSN_TYPE_SSE_SSE_MEM (\
  ORC_X86_INSN_OPERAND_OP1_REG |       \
  ORC_X86_INSN_OPERAND_OP2_REG |       \
  ORC_X86_INSN_OPERAND_OP3_MEM         \
), (                                   \
  ORC_AVX_INSN_OPERAND_OP1_YMM |       \
  ORC_AVX_INSN_OPERAND_OP2_YMM         \
)

#define ORC_AVX_INSN_TYPE_AVX_AVXM (\
  ORC_X86_INSN_OPERAND_REG_REGM     \
), (                                \
  ORC_AVX_INSN_OPERAND_OP1_YMM |    \
  ORC_AVX_INSN_OPERAND_OP2_YMM      \
)

#define ORC_AVX_INSN_TYPE_AVX_REG32 (\
  ORC_X86_INSN_OPERAND_OP1_REG |     \
  ORC_X86_INSN_OPERAND_OP2_REG |     \
  ORC_X86_INSN_OPERAND_OP2_32        \
), ORC_AVX_INSN_OPERAND_OP1_YMM

#define ORC_AVX_INSN_TYPE_AVX_REG64 (\
  ORC_X86_INSN_OPERAND_OP1_REG |     \
  ORC_X86_INSN_OPERAND_OP2_REG |     \
  ORC_X86_INSN_OPERAND_OP2_64        \
), ORC_AVX_INSN_OPERAND_OP1_YMM

#define ORC_AVX_INSN_TYPE_AVX_REG32TO64 (\
  ORC_X86_INSN_OPERAND_OP1_REG |         \
  ORC_X86_INSN_OPERAND_OP2_REG |         \
  ORC_X86_INSN_OPERAND_OP2_32 |          \
  ORC_X86_INSN_OPERAND_OP2_64            \
), ORC_AVX_INSN_OPERAND_OP1_YMM

#define ORC_AVX_INSN_TYPE_SSE_AVXM (\
  ORC_X86_INSN_OPERAND_REG_REGM     \
), (                                \
  ORC_SSE_INSN_OPERAND_OP1_XMM |    \
  ORC_AVX_INSN_OPERAND_OP2_YMM      \
)

#define ORC_AVX_INSN_TYPE_AVX_AVXM_IMM8 (\
  ORC_X86_INSN_OPERAND_REG_REGM_IMM |    \
  ORC_X86_INSN_OPERAND_OP3_8             \
), (                                     \
  ORC_AVX_INSN_OPERAND_OP1_YMM |         \
  ORC_AVX_INSN_OPERAND_OP2_YMM           \
)

#define ORC_AVX_INSN_TYPE_AVX_AVX_IMM8 (\
  ORC_X86_INSN_OPERAND_REG_REG_IMM |    \
  ORC_X86_INSN_OPERAND_OP3_8            \
), (                                    \
  ORC_AVX_INSN_OPERAND_OP1_YMM |        \
  ORC_AVX_INSN_OPERAND_OP2_YMM          \
)

#define ORC_AVX_INSN_TYPE_AVX_AVX_AVXM (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM     \
), (                                    \
  ORC_AVX_INSN_OPERAND_OP1_YMM |        \
  ORC_AVX_INSN_OPERAND_OP2_YMM |        \
  ORC_AVX_INSN_OPERAND_OP3_YMM          \
)

#define ORC_AVX_INSN_TYPE_AVX_AVX_SSEM (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM     \
), (                                    \
  ORC_AVX_INSN_OPERAND_OP1_YMM |        \
  ORC_AVX_INSN_OPERAND_OP2_YMM |        \
  ORC_AVX_INSN_OPERAND_OP3_XMM          \
)

#define ORC_AVX_INSN_TYPE_SSE_SSE_SSEM (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM     \
), (                                    \
  ORC_SSE_INSN_OPERAND_OP1_XMM |        \
  ORC_SSE_INSN_OPERAND_OP2_XMM |        \
  ORC_AVX_INSN_OPERAND_OP3_XMM          \
)

#define ORC_AVX_INSN_TYPE_SSE_SSE_IMM8 (\
  ORC_X86_INSN_OPERAND_REG_REG_IMM |    \
  ORC_X86_INSN_OPERAND_OP3_8            \
), (                                    \
  ORC_SSE_INSN_OPERAND_OP1_XMM |        \
  ORC_SSE_INSN_OPERAND_OP2_XMM          \
)

#define ORC_AVX_INSN_TYPE_SSE_SSE_REG32M16_IMM8 (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM_IMM |      \
  ORC_X86_INSN_OPERAND_OP3_32 |                \
  ORC_X86_INSN_OPERAND_OP4_8 |                 \
  ORC_X86_INSN_OPERAND_MEM16                   \
), (                                           \
  ORC_SSE_INSN_OPERAND_OP1_XMM |               \
  ORC_SSE_INSN_OPERAND_OP2_XMM                 \
)

#define ORC_AVX_INSN_TYPE_SSE_SSE_SSEM_IMM8 (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM_IMM |    \
  ORC_X86_INSN_OPERAND_OP4_8                 \
), (                                         \
  ORC_SSE_INSN_OPERAND_OP1_XMM |             \
  ORC_SSE_INSN_OPERAND_OP2_XMM |             \
  ORC_AVX_INSN_OPERAND_OP3_XMM               \
)

#define ORC_AVX_INSN_TYPE_AVX_AVX_AVXM_IMM8 (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM_IMM |    \
  ORC_X86_INSN_OPERAND_OP4_8                 \
), (                                         \
  ORC_AVX_INSN_OPERAND_OP1_YMM |             \
  ORC_AVX_INSN_OPERAND_OP2_YMM |             \
  ORC_AVX_INSN_OPERAND_OP3_YMM               \
)

#define ORC_AVX_INSN_TYPE_AVX_AVX_SSEM_IMM8 (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM_IMM |    \
  ORC_X86_INSN_OPERAND_OP4_8                 \
), (                                         \
  ORC_AVX_INSN_OPERAND_OP1_YMM |             \
  ORC_AVX_INSN_OPERAND_OP2_YMM |             \
  ORC_AVX_INSN_OPERAND_OP3_XMM               \
)

#define ORC_AVX_INSN_TYPE_AVX_AVX_AVXM_AVX (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM_REG     \
), (                                        \
  ORC_AVX_INSN_OPERAND_OP1_YMM |            \
  ORC_AVX_INSN_OPERAND_OP2_YMM |            \
  ORC_AVX_INSN_OPERAND_OP3_YMM |            \
  ORC_AVX_INSN_OPERAND_OP4_YMM              \
)

#define ORC_AVX_INSN_TYPE_SSE_SSE_SSEM_SSE (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM_REG     \
), (                                        \
  ORC_SSE_INSN_OPERAND_OP1_XMM |            \
  ORC_SSE_INSN_OPERAND_OP2_XMM |            \
  ORC_AVX_INSN_OPERAND_OP3_XMM |            \
  ORC_AVX_INSN_OPERAND_OP4_XMM              \
)

#define ORC_AVX_INSN_TYPE_AVX_SSEM (\
  ORC_X86_INSN_OPERAND_REG_REGM     \
), (                                \
  ORC_AVX_INSN_OPERAND_OP1_YMM |    \
  ORC_SSE_INSN_OPERAND_OP2_XMM      \
)

#define ORC_AVX_INSN_TYPE_AVX_SSEM8 (\
  ORC_X86_INSN_OPERAND_REG_REGM |    \
  ORC_X86_INSN_OPERAND_MEM8          \
), (                                 \
  ORC_AVX_INSN_OPERAND_OP1_YMM |     \
  ORC_SSE_INSN_OPERAND_OP2_XMM       \
)

#define ORC_AVX_INSN_TYPE_AVX_SSEM16 (\
  ORC_X86_INSN_OPERAND_REG_REGM |     \
  ORC_X86_INSN_OPERAND_MEM16          \
), (                                  \
  ORC_AVX_INSN_OPERAND_OP1_YMM |      \
  ORC_SSE_INSN_OPERAND_OP2_XMM        \
)

#define ORC_AVX_INSN_TYPE_AVX_SSEM32 (\
  ORC_X86_INSN_OPERAND_REG_REGM |     \
  ORC_X86_INSN_OPERAND_MEM32          \
), (                                  \
  ORC_AVX_INSN_OPERAND_OP1_YMM |      \
  ORC_SSE_INSN_OPERAND_OP2_XMM        \
)

#define ORC_AVX_INSN_TYPE_AVX_SSEM64 (\
  ORC_X86_INSN_OPERAND_REG_REGM |     \
  ORC_X86_INSN_OPERAND_MEM64          \
), (                                  \
  ORC_AVX_INSN_OPERAND_OP1_YMM |      \
  ORC_SSE_INSN_OPERAND_OP2_XMM        \
)

#define ORC_AVX_INSN_TYPE_SSEM_AVX (\
  ORC_X86_INSN_OPERAND_REGM_REG     \
), (                                \
  ORC_SSE_INSN_OPERAND_OP1_XMM |    \
  ORC_AVX_INSN_OPERAND_OP2_YMM      \
)

#define ORC_AVX_INSN_TYPE_SSEM_AVX_IMM8 (\
  ORC_X86_INSN_OPERAND_REGM_REG_IMM |    \
  ORC_X86_INSN_OPERAND_OP3_8             \
), (                                     \
  ORC_SSE_INSN_OPERAND_OP1_XMM |         \
  ORC_AVX_INSN_OPERAND_OP2_YMM           \
)

#define ORC_AVX_INSN_TYPE_AVXM_AVX (\
  ORC_X86_INSN_OPERAND_REGM_REG     \
), (                                \
  ORC_AVX_INSN_OPERAND_OP1_YMM |    \
  ORC_AVX_INSN_OPERAND_OP2_YMM      \
)

#define ORC_AVX_INSN_TYPE_SSE_SSE_REG32M8_IMM8  (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM_IMM |       \
  ORC_X86_INSN_OPERAND_OP3_32 |                 \
  ORC_X86_INSN_OPERAND_OP4_8 |                  \
  ORC_X86_INSN_OPERAND_MEM8                     \
), (                                            \
  ORC_SSE_INSN_OPERAND_OP1_XMM |                \
  ORC_SSE_INSN_OPERAND_OP2_XMM                  \
)

#define ORC_AVX_INSN_TYPE_SSE_SSE_REG32M32_IMM8  (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM_IMM |       \
  ORC_X86_INSN_OPERAND_OP3_32 |                 \
  ORC_X86_INSN_OPERAND_OP4_8 |                  \
  ORC_X86_INSN_OPERAND_MEM32                    \
), (                                            \
  ORC_SSE_INSN_OPERAND_OP1_XMM |                \
  ORC_SSE_INSN_OPERAND_OP2_XMM                  \
)

#define ORC_AVX_INSN_TYPE_SSE_SSE_REG32TO64M32_IMM8 (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM_IMM |          \
  ORC_X86_INSN_OPERAND_OP3_32 |                    \
  ORC_X86_INSN_OPERAND_OP3_64 |                    \
  ORC_X86_INSN_OPERAND_OP4_8 |                     \
  ORC_X86_INSN_OPERAND_MEM32                       \
), (                                               \
  ORC_SSE_INSN_OPERAND_OP1_XMM |                   \
  ORC_SSE_INSN_OPERAND_OP2_XMM                     \
)


ORC_INTERNAL void orc_compiler_avx_register_rules (OrcTarget *target);

ORC_END_DECLS
