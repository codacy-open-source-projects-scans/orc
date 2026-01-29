#ifndef ORC_AVX512_PRIVATE
#define ORC_AVX512_PRIVATE

#include <orc/orcavx-internal.h>

ORC_BEGIN_DECLS

/* orcavx512.c */
ORC_INTERNAL void orc_avx512_init (void);
ORC_INTERNAL const char * orc_avx512_get_flag_name (OrcTargetAVX512Flags flag);

/* orcavx512target.c */
ORC_INTERNAL OrcTarget * orc_avx512_target_init (void);

/* orcavx512compiler.c */
ORC_INTERNAL int orc_avx512_compiler_get_mask_reg (OrcCompiler *c, orc_bool mask_op);
ORC_INTERNAL void orc_avx512_compiler_release_mask_reg (OrcCompiler *c, int reg);

/* orcavx512insn.c */
/* This extends OrcAVXInsnOperandFlag */
typedef enum _OrcAVX512InsnOperandFlag {
  ORC_AVX512_INSN_OPERAND_OP1_K   = (1 << (ORC_AVX_INSN_OPERAND_FLAG_LAST + 1)),
  ORC_AVX512_INSN_OPERAND_OP2_K   = (1 << (ORC_AVX_INSN_OPERAND_FLAG_LAST + 2)),
  ORC_AVX512_INSN_OPERAND_OP3_K   = (1 << (ORC_AVX_INSN_OPERAND_FLAG_LAST + 3)),
  ORC_AVX512_INSN_OPERAND_OP1_ZMM = (1 << (ORC_AVX_INSN_OPERAND_FLAG_LAST + 4)),
  ORC_AVX512_INSN_OPERAND_OP2_ZMM = (1 << (ORC_AVX_INSN_OPERAND_FLAG_LAST + 5)),
  ORC_AVX512_INSN_OPERAND_OP3_ZMM = (1 << (ORC_AVX_INSN_OPERAND_FLAG_LAST + 6)),
  ORC_AVX512_INSN_OPERAND_OP4_ZMM = (1 << (ORC_AVX_INSN_OPERAND_FLAG_LAST + 7)),
} OrcAVX512InsnOperandFlag;

#define ORC_AVX512_INSN_TYPE_AVX512_AVX512_SSEM (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM                 \
), (                                                \
  ORC_AVX512_INSN_OPERAND_OP1_ZMM |                 \
  ORC_AVX512_INSN_OPERAND_OP2_ZMM |                 \
  ORC_AVX_INSN_OPERAND_OP3_XMM                      \
)

#define ORC_AVX512_INSN_TYPE_AVX512_AVX512M_IMM8 (\
  ORC_X86_INSN_OPERAND_REG_REGM_IMM |             \
  ORC_X86_INSN_OPERAND_OP3_8                      \
), (                                              \
  ORC_AVX512_INSN_OPERAND_OP1_ZMM |               \
  ORC_AVX512_INSN_OPERAND_OP2_ZMM                 \
)

#define ORC_AVX512_INSN_TYPE_AVX512_REG32TO64 (\
  ORC_X86_INSN_OPERAND_OP1_REG |               \
  ORC_X86_INSN_OPERAND_OP2_REG |               \
  ORC_X86_INSN_OPERAND_OP2_32 |                \
  ORC_X86_INSN_OPERAND_OP2_64                  \
), ORC_AVX512_INSN_OPERAND_OP1_ZMM

#define ORC_AVX512_INSN_TYPE_AVX512_REG32 (\
  ORC_X86_INSN_OPERAND_OP1_REG |           \
  ORC_X86_INSN_OPERAND_OP2_REG |           \
  ORC_X86_INSN_OPERAND_OP2_32              \
), ORC_AVX512_INSN_OPERAND_OP1_ZMM

#define ORC_AVX512_INSN_TYPE_AVX512_REG64 (\
  ORC_X86_INSN_OPERAND_OP1_REG |           \
  ORC_X86_INSN_OPERAND_OP2_REG |           \
  ORC_X86_INSN_OPERAND_OP2_64              \
), ORC_AVX512_INSN_OPERAND_OP1_ZMM

#define ORC_AVX512_INSN_TYPE_MEM_AVX512 (\
  ORC_X86_INSN_OPERAND_OP1_MEM     \
), ORC_AVX512_INSN_OPERAND_OP2_ZMM

#define ORC_AVX512_INSN_TYPE_AVX512M_AVX512 (\
  ORC_X86_INSN_OPERAND_REGM_REG              \
), (                                         \
  ORC_AVX512_INSN_OPERAND_OP1_ZMM |          \
  ORC_AVX512_INSN_OPERAND_OP2_ZMM            \
)

#define ORC_AVX512_INSN_TYPE_AVX512_AVX512M (\
  ORC_X86_INSN_OPERAND_REG_REGM              \
), (                                         \
  ORC_AVX512_INSN_OPERAND_OP1_ZMM |          \
  ORC_AVX512_INSN_OPERAND_OP2_ZMM            \
)

#define ORC_AVX512_INSN_TYPE_AVX_AVX512M (\
  ORC_X86_INSN_OPERAND_REG_REGM           \
), (                                      \
  ORC_AVX_INSN_OPERAND_OP1_YMM |          \
  ORC_AVX512_INSN_OPERAND_OP2_ZMM         \
)

#define ORC_AVX512_INSN_TYPE_AVX512_AVXM (\
  ORC_X86_INSN_OPERAND_REG_REGM           \
), (                                      \
  ORC_AVX512_INSN_OPERAND_OP1_ZMM |       \
  ORC_AVX_INSN_OPERAND_OP2_YMM            \
)

#define ORC_AVX512_INSN_TYPE_AVX512_SSEM (\
  ORC_X86_INSN_OPERAND_REG_REGM           \
), (                                      \
  ORC_AVX512_INSN_OPERAND_OP1_ZMM |       \
  ORC_SSE_INSN_OPERAND_OP2_XMM            \
)

#define ORC_AVX512_INSN_TYPE_AVX512_SSEM8 (\
  ORC_X86_INSN_OPERAND_REG_REGM |          \
  ORC_X86_INSN_OPERAND_MEM8                \
), (                                       \
  ORC_AVX512_INSN_OPERAND_OP1_ZMM |        \
  ORC_SSE_INSN_OPERAND_OP2_XMM             \
)

#define ORC_AVX512_INSN_TYPE_AVX512_SSEM16 (\
  ORC_X86_INSN_OPERAND_REG_REGM |           \
  ORC_X86_INSN_OPERAND_MEM16                \
), (                                        \
  ORC_AVX512_INSN_OPERAND_OP1_ZMM |         \
  ORC_SSE_INSN_OPERAND_OP2_XMM              \
)

#define ORC_AVX512_INSN_TYPE_AVX512_SSEM32 (\
  ORC_X86_INSN_OPERAND_REG_REGM |           \
  ORC_X86_INSN_OPERAND_MEM32                \
), (                                        \
  ORC_AVX512_INSN_OPERAND_OP1_ZMM |         \
  ORC_SSE_INSN_OPERAND_OP2_XMM              \
)

#define ORC_AVX512_INSN_TYPE_AVX512_SSEM64 (\
  ORC_X86_INSN_OPERAND_REG_REGM |           \
  ORC_X86_INSN_OPERAND_MEM64                \
), (                                        \
  ORC_AVX512_INSN_OPERAND_OP1_ZMM |         \
  ORC_SSE_INSN_OPERAND_OP2_XMM              \
)

#define ORC_AVX512_INSN_TYPE_AVX512_AVX512_AVX512M (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM                 \
), (                                                \
  ORC_AVX512_INSN_OPERAND_OP1_ZMM |                 \
  ORC_AVX512_INSN_OPERAND_OP2_ZMM |                 \
  ORC_AVX512_INSN_OPERAND_OP3_ZMM                   \
)

#define ORC_AVX512_INSN_TYPE_K_SSE_SSEM (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM      \
), (                                     \
  ORC_AVX512_INSN_OPERAND_OP1_K |        \
  ORC_SSE_INSN_OPERAND_OP2_XMM |         \
  ORC_AVX_INSN_OPERAND_OP3_XMM           \
)

#define ORC_AVX512_INSN_TYPE_K_AVX_AVXM (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM      \
), (                                     \
  ORC_AVX512_INSN_OPERAND_OP1_K |        \
  ORC_AVX_INSN_OPERAND_OP2_YMM |         \
  ORC_AVX_INSN_OPERAND_OP3_YMM           \
)

#define ORC_AVX512_INSN_TYPE_K_AVX512_AVX512M (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM            \
), (                                           \
  ORC_AVX512_INSN_OPERAND_OP1_K |              \
  ORC_AVX512_INSN_OPERAND_OP2_ZMM |            \
  ORC_AVX512_INSN_OPERAND_OP3_ZMM              \
)

#define ORC_AVX512_INSN_TYPE_AVX512_AVX512_AVX512M_IMM8 (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM_IMM |                \
  ORC_X86_INSN_OPERAND_OP4_8                             \
), (                                                     \
  ORC_AVX512_INSN_OPERAND_OP1_ZMM |                      \
  ORC_AVX512_INSN_OPERAND_OP2_ZMM |                      \
  ORC_AVX512_INSN_OPERAND_OP3_ZMM                        \
)

#define ORC_AVX512_INSN_TYPE_SSE_K (\
  ORC_X86_INSN_OPERAND_OP1_REG |    \
  ORC_X86_INSN_OPERAND_OP2_REG      \
), (                                \
  ORC_SSE_INSN_OPERAND_OP1_XMM |    \
  ORC_AVX512_INSN_OPERAND_OP2_K     \
)

#define ORC_AVX512_INSN_TYPE_AVX_K (\
  ORC_X86_INSN_OPERAND_OP1_REG |    \
  ORC_X86_INSN_OPERAND_OP2_REG      \
), (                                \
  ORC_AVX_INSN_OPERAND_OP1_YMM |    \
  ORC_AVX512_INSN_OPERAND_OP2_K     \
)

#define ORC_AVX512_INSN_TYPE_AVX512_K (\
  ORC_X86_INSN_OPERAND_OP1_REG |       \
  ORC_X86_INSN_OPERAND_OP2_REG         \
), (                                   \
  ORC_AVX512_INSN_OPERAND_OP1_ZMM |    \
  ORC_AVX512_INSN_OPERAND_OP2_K        \
)

#define ORC_AVX512_INSN_TYPE_AVX512_AVX512_SSEM_IMM8 (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM_IMM |             \
  ORC_X86_INSN_OPERAND_OP4_8                          \
), (                                                  \
  ORC_AVX512_INSN_OPERAND_OP1_ZMM |                   \
  ORC_AVX512_INSN_OPERAND_OP2_ZMM |                   \
  ORC_AVX_INSN_OPERAND_OP3_XMM                        \
)

#define ORC_AVX512_INSN_TYPE_AVX512_AVX512_AVXM_IMM8 (\
  ORC_X86_INSN_OPERAND_REG_REG_REGM_IMM |             \
  ORC_X86_INSN_OPERAND_OP4_8                          \
), (                                                  \
  ORC_AVX512_INSN_OPERAND_OP1_ZMM |                   \
  ORC_AVX512_INSN_OPERAND_OP2_ZMM |                   \
  ORC_AVX_INSN_OPERAND_OP3_YMM                        \
)

#define ORC_AVX512_INSN_TYPE_K_SSE (\
  ORC_X86_INSN_OPERAND_OP1_REG |    \
  ORC_X86_INSN_OPERAND_OP2_REG      \
), (                                \
  ORC_AVX512_INSN_OPERAND_OP1_K |   \
  ORC_SSE_INSN_OPERAND_OP2_XMM      \
)

#define ORC_AVX512_INSN_TYPE_K_AVX (\
  ORC_X86_INSN_OPERAND_OP1_REG |    \
  ORC_X86_INSN_OPERAND_OP2_REG      \
), (                                \
  ORC_AVX512_INSN_OPERAND_OP1_K |   \
  ORC_AVX_INSN_OPERAND_OP2_YMM      \
)

#define ORC_AVX512_INSN_TYPE_K_AVX512 (\
  ORC_X86_INSN_OPERAND_OP1_REG |       \
  ORC_X86_INSN_OPERAND_OP2_REG         \
), (                                   \
  ORC_AVX512_INSN_OPERAND_OP1_K |      \
  ORC_AVX512_INSN_OPERAND_OP2_ZMM      \
)

#define ORC_AVX512_INSN_TYPE_K_KM8 (\
  ORC_X86_INSN_OPERAND_REG_REGM |   \
  ORC_X86_INSN_OPERAND_MEM8         \
), (                                \
  ORC_AVX512_INSN_OPERAND_OP1_K |   \
  ORC_AVX512_INSN_OPERAND_OP2_K     \
)

#define ORC_AVX512_INSN_TYPE_K_KM16 (\
  ORC_X86_INSN_OPERAND_REG_REGM |    \
  ORC_X86_INSN_OPERAND_MEM16         \
), (                                 \
  ORC_AVX512_INSN_OPERAND_OP1_K |    \
  ORC_AVX512_INSN_OPERAND_OP2_K      \
)

#define ORC_AVX512_INSN_TYPE_K_KM32 (\
  ORC_X86_INSN_OPERAND_REG_REGM |    \
  ORC_X86_INSN_OPERAND_MEM32         \
), (                                 \
  ORC_AVX512_INSN_OPERAND_OP1_K |    \
  ORC_AVX512_INSN_OPERAND_OP2_K      \
)

#define ORC_AVX512_INSN_TYPE_K_KM64 (\
  ORC_X86_INSN_OPERAND_REG_REGM |    \
  ORC_X86_INSN_OPERAND_MEM64         \
), (                                 \
  ORC_AVX512_INSN_OPERAND_OP1_K |    \
  ORC_AVX512_INSN_OPERAND_OP2_K      \
)

#define ORC_AVX512_INSN_TYPE_M8_K (\
  ORC_X86_INSN_OPERAND_OP1_MEM |   \
  ORC_X86_INSN_OPERAND_OP2_REG |   \
  ORC_X86_INSN_OPERAND_MEM8        \
), (                               \
  ORC_AVX512_INSN_OPERAND_OP2_K    \
)

#define ORC_AVX512_INSN_TYPE_M16_K (\
  ORC_X86_INSN_OPERAND_OP1_MEM |    \
  ORC_X86_INSN_OPERAND_OP2_REG |    \
  ORC_X86_INSN_OPERAND_MEM16        \
), (                                \
  ORC_AVX512_INSN_OPERAND_OP2_K     \
)

#define ORC_AVX512_INSN_TYPE_M32_K (\
  ORC_X86_INSN_OPERAND_OP1_MEM |    \
  ORC_X86_INSN_OPERAND_OP2_REG |    \
  ORC_X86_INSN_OPERAND_MEM32        \
), (                                \
  ORC_AVX512_INSN_OPERAND_OP2_K     \
)

#define ORC_AVX512_INSN_TYPE_M64_K (\
  ORC_X86_INSN_OPERAND_OP1_MEM |    \
  ORC_X86_INSN_OPERAND_OP2_REG |    \
  ORC_X86_INSN_OPERAND_MEM64        \
), (                                \
  ORC_AVX512_INSN_OPERAND_OP2_K     \
)

#define ORC_AVX512_INSN_TYPE_K_REG32 (\
  ORC_X86_INSN_OPERAND_OP1_REG |      \
  ORC_X86_INSN_OPERAND_OP2_REG |      \
  ORC_X86_INSN_OPERAND_OP2_32         \
), (                                  \
  ORC_AVX512_INSN_OPERAND_OP1_K       \
)

#define ORC_AVX512_INSN_TYPE_K_REG64 (\
  ORC_X86_INSN_OPERAND_OP1_REG |      \
  ORC_X86_INSN_OPERAND_OP2_REG |      \
  ORC_X86_INSN_OPERAND_OP2_64         \
), (                                  \
  ORC_AVX512_INSN_OPERAND_OP1_K       \
)

#define ORC_AVX512_INSN_TYPE_REG32_K (\
  ORC_X86_INSN_OPERAND_OP1_REG |      \
  ORC_X86_INSN_OPERAND_OP2_REG |      \
  ORC_X86_INSN_OPERAND_OP1_32         \
), (                                  \
  ORC_AVX512_INSN_OPERAND_OP2_K       \
)

#define ORC_AVX512_INSN_TYPE_REG64_K (\
  ORC_X86_INSN_OPERAND_OP1_REG |      \
  ORC_X86_INSN_OPERAND_OP2_REG |      \
  ORC_X86_INSN_OPERAND_OP1_64         \
), (                                  \
  ORC_AVX512_INSN_OPERAND_OP2_K       \
)

#define ORC_AVX512_INSN_TYPE_K_K_IMM8 (\
  ORC_X86_INSN_OPERAND_OP1_REG |      \
  ORC_X86_INSN_OPERAND_OP2_REG |      \
  ORC_X86_INSN_OPERAND_OP3_IMM |      \
  ORC_X86_INSN_OPERAND_OP3_8          \
), (                                  \
  ORC_AVX512_INSN_OPERAND_OP1_K |     \
  ORC_AVX512_INSN_OPERAND_OP2_K       \
)

#define ORC_AVX512_INSN_TYPE_SSEM_AVX512_IMM8 (\
  ORC_X86_INSN_OPERAND_REGM_REG_IMM |          \
  ORC_X86_INSN_OPERAND_OP3_8                   \
), (                                           \
  ORC_SSE_INSN_OPERAND_OP1_XMM |               \
  ORC_AVX512_INSN_OPERAND_OP2_ZMM              \
)

#define ORC_AVX512_INSN_TYPE_AVXM_AVX512_IMM8 (\
  ORC_X86_INSN_OPERAND_REGM_REG_IMM |          \
  ORC_X86_INSN_OPERAND_OP3_8                   \
), (                                           \
  ORC_AVX_INSN_OPERAND_OP1_YMM |               \
  ORC_AVX512_INSN_OPERAND_OP2_ZMM              \
)

#define ORC_AVX512_INSN_TYPE_AVXM_AVX512 (\
  ORC_X86_INSN_OPERAND_REGM_REG           \
), (                                      \
  ORC_AVX_INSN_OPERAND_OP1_YMM |          \
  ORC_AVX512_INSN_OPERAND_OP2_ZMM         \
)

#define ORC_AVX512_INSN_TYPE_K_K_K (\
  ORC_X86_INSN_OPERAND_OP1_REG |    \
  ORC_X86_INSN_OPERAND_OP2_REG |    \
  ORC_X86_INSN_OPERAND_OP3_REG      \
), (                                \
  ORC_AVX512_INSN_OPERAND_OP1_K |   \
  ORC_AVX512_INSN_OPERAND_OP2_K |   \
  ORC_AVX512_INSN_OPERAND_OP3_K     \
)

ORC_INTERNAL void orc_avx512_insn_emit_stmxcsr (OrcCompiler *c, int offset, int reg);
ORC_INTERNAL void orc_avx512_insn_emit_ldmxcsr (OrcCompiler *c, int offset, int reg);

/* orcavx512rules.c */
ORC_INTERNAL void orc_avx512_rules_init (OrcTarget *target);

ORC_END_DECLS

#endif
