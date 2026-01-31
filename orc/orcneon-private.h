#ifndef _ORC_NEON_PRIVATE_H_
#define _ORC_NEON_PRIVATE_H_

#include <orc/orcutils.h>

ORC_INTERNAL void orc_compiler_neon_register_rules (OrcTarget *target);

ORC_INTERNAL void orc_neon32_compiler_init (OrcCompiler *compiler);
ORC_INTERNAL void orc_neon32_compile (OrcCompiler *compiler);
ORC_INTERNAL void orc_neon64_compiler_init (OrcCompiler *compiler);
ORC_INTERNAL void orc_neon64_compile (OrcCompiler *compiler);

enum {
  LABEL_ONE_REGION = 1,
  LABEL_ONE_REGION_AFTER,
  LABEL_REGION0_LOOP,
  LABEL_REGION0_SKIP,
  LABEL_REGION1_LOOP,
  LABEL_REGION1_SKIP,
  LABEL_REGION2_LOOP_SMALL,
  LABEL_REGION2_LOOP_MEDIUM,
  LABEL_REGION2_LOOP_LARGE,
  LABEL_REGION2_SMALL,
  LABEL_REGION2_MEDIUM,
  LABEL_REGION2_SKIP,
  LABEL_REGION3_LOOP,
  LABEL_REGION3_SKIP,
  LABEL_OUTER_LOOP,
  LABEL_OUTER_LOOP_SKIP,
  LABEL_L1L2_AFTER,
};

enum RegionFlags {
  FLAG_NUNROLL = (1 << 0),
  FLAG_REGION1 = (1 << 1),
  FLAG_REGION2 = (1 << 2),
  FLAG_REGION3 = (1 << 3),
};

#define ORC_NEON_ALIGNED_DEST_CUTOFF 64

ORC_INTERNAL void orc_neon_emit_loop_common (OrcCompiler *compiler, int unroll_index);
ORC_INTERNAL void orc_neon_compiler_init_common (OrcCompiler *compiler);
ORC_INTERNAL enum RegionFlags orc_compiler_neon_calc_regions (const OrcCompiler *compiler);
ORC_INTERNAL unsigned int orc_neon_get_callee_saved_regs (const OrcCompiler *compiler, int base, int n);
ORC_INTERNAL int orc_neon_get_align_var (OrcCompiler *compiler);

typedef struct _NeonInsn {
  const char *name;
  orc_uint32 code;
  int vec_shift;
} OrcNeonInsn;

typedef struct _OrcNeonRule {
  const char *orc_opcode;
  void (*rule)(OrcCompiler *, void *, OrcInstruction *);
} OrcNeonRule;

enum OrcNeonOpcode {
  ORC_NEON_OP_ABSB,
  ORC_NEON_OP_ADDB,
  ORC_NEON_OP_ADDSSB,
  ORC_NEON_OP_ADDUSB,
  ORC_NEON_OP_ANDB,
/*ORC_NEON_OP_ANDNB,*/
  ORC_NEON_OP_AVGSB,
  ORC_NEON_OP_AVGUB,
  ORC_NEON_OP_CMPEQB,
  ORC_NEON_OP_CMPGTSB,
  ORC_NEON_OP_COPYB,
  ORC_NEON_OP_MAXSB,
  ORC_NEON_OP_MAXUB,
  ORC_NEON_OP_MINSB,
  ORC_NEON_OP_MINUB,
  ORC_NEON_OP_MULLB,
  ORC_NEON_OP_ORB,
/*ORC_NEON_OP_SHLB,*/
/*ORC_NEON_OP_SHRSB,*/
/*ORC_NEON_OP_SHRUB,*/
  ORC_NEON_OP_SUBB,
  ORC_NEON_OP_SUBSSB,
  ORC_NEON_OP_SUBUSB,
  ORC_NEON_OP_XORB,
  ORC_NEON_OP_ABSW,
  ORC_NEON_OP_ADDW,
  ORC_NEON_OP_ADDSSW,
  ORC_NEON_OP_ADDUSW,
  ORC_NEON_OP_ANDW,
/*ORC_NEON_OP_ANDNW,*/
  ORC_NEON_OP_AVGSW,
  ORC_NEON_OP_AVGUW,
  ORC_NEON_OP_CMPEQW,
  ORC_NEON_OP_CMPGTSW,
  ORC_NEON_OP_COPYW,
  ORC_NEON_OP_MAXSW,
  ORC_NEON_OP_MAXUW,
  ORC_NEON_OP_MINSW,
  ORC_NEON_OP_MINUW,
  ORC_NEON_OP_MULLW,
  ORC_NEON_OP_ORW,
/*ORC_NEON_OP_SHLW,*/
/*ORC_NEON_OP_SHRSW,*/
/*ORC_NEON_OP_SHRUW,*/
  ORC_NEON_OP_SUBW,
  ORC_NEON_OP_SUBSSW,
  ORC_NEON_OP_SUBUSW,
  ORC_NEON_OP_XORW,
  ORC_NEON_OP_ABSL,
  ORC_NEON_OP_ADDL,
  ORC_NEON_OP_ADDSSL,
  ORC_NEON_OP_ADDUSL,
  ORC_NEON_OP_ANDL,
/*ORC_NEON_OP_ANDNL,*/
  ORC_NEON_OP_AVGSL,
  ORC_NEON_OP_AVGUL,
  ORC_NEON_OP_CMPEQL,
  ORC_NEON_OP_CMPGTSL,
  ORC_NEON_OP_COPYL,
  ORC_NEON_OP_MAXSL,
  ORC_NEON_OP_MAXUL,
  ORC_NEON_OP_MINSL,
  ORC_NEON_OP_MINUL,
  ORC_NEON_OP_MULLL,
  ORC_NEON_OP_ORL,
/*ORC_NEON_OP_SHLL,*/
/*ORC_NEON_OP_SHRSL,*/
/*ORC_NEON_OP_SHRUL,*/
  ORC_NEON_OP_SUBL,
  ORC_NEON_OP_SUBSSL,
  ORC_NEON_OP_SUBUSL,
  ORC_NEON_OP_XORL,
/*ORC_NEON_OP_ABSQ,*/
  ORC_NEON_OP_ADDQ,
/*ORC_NEON_OP_ADDSSQ,*/
/*ORC_NEON_OP_ADDUSQ,*/
  ORC_NEON_OP_ANDQ,
/*ORC_NEON_OP_AVGSQ,*/
/*ORC_NEON_OP_AVGUQ,*/
/*ORC_NEON_OP_CMPEQQ,*/
/*ORC_NEON_OP_CMPGTSQ,*/
  ORC_NEON_OP_COPYQ,
/*ORC_NEON_OP_MAXSQ,*/
/*ORC_NEON_OP_MAXUQ,*/
/*ORC_NEON_OP_MINSQ,*/
/*ORC_NEON_OP_MINUQ,*/
/*ORC_NEON_OP_MULLQ,*/
  ORC_NEON_OP_ORQ,
  ORC_NEON_OP_SUBQ,
/*ORC_NEON_OP_SUBSSQ,*/
/*ORC_NEON_OP_SUBUSQ,*/
  ORC_NEON_OP_XORQ,
  ORC_NEON_OP_CONVSBW,
  ORC_NEON_OP_CONVUBW,
  ORC_NEON_OP_CONVSWL,
  ORC_NEON_OP_CONVUWL,
  ORC_NEON_OP_CONVSLQ,
  ORC_NEON_OP_CONVULQ,
  ORC_NEON_OP_CONVWB,
  ORC_NEON_OP_CONVSSSWB,
  ORC_NEON_OP_CONVSUSWB,
  ORC_NEON_OP_CONVUUSWB,
  ORC_NEON_OP_CONVLW,
  ORC_NEON_OP_CONVQL,
  ORC_NEON_OP_CONVSSSLW,
  ORC_NEON_OP_CONVSUSLW,
  ORC_NEON_OP_CONVUUSLW,
  ORC_NEON_OP_CONVSSSQL,
  ORC_NEON_OP_CONVSUSQL,
  ORC_NEON_OP_CONVUUSQL,
  ORC_NEON_OP_MULSBW,
  ORC_NEON_OP_MULUBW,
  ORC_NEON_OP_MULSWL,
  ORC_NEON_OP_MULUWL,
  ORC_NEON_OP_SWAPW,
  ORC_NEON_OP_SWAPL,
  ORC_NEON_OP_SWAPQ,
  ORC_NEON_OP_SWAPWL,
  ORC_NEON_OP_SWAPLQ,
  ORC_NEON_OP_SELECT0QL,
  ORC_NEON_OP_SELECT0LW,
  ORC_NEON_OP_SELECT0WB,
  ORC_NEON_OP_ADDF,
  ORC_NEON_OP_SUBF,
  ORC_NEON_OP_MULF,
  ORC_NEON_OP_MAXF,
  ORC_NEON_OP_MINF,
  ORC_NEON_OP_CMPEQF,
/*ORC_NEON_OP_CMPLTF,*/
/*ORC_NEON_OP_CMPLEF,*/
  ORC_NEON_OP_CONVFL,
  ORC_NEON_OP_CONVLF,
  ORC_NEON_OP_ADDD,
  ORC_NEON_OP_SUBD,
  ORC_NEON_OP_MULD,
  ORC_NEON_OP_DIVD,
  ORC_NEON_OP_DIVF,
  ORC_NEON_OP_SQRTD,
  ORC_NEON_OP_SQRTF,
/*ORC_NEON_OP_CMPEQD,*/
  ORC_NEON_OP_CONVDF,
  ORC_NEON_OP_CONVFD,
  _ORC_NEON_OP_MAX_,
};

typedef struct {
  orc_uint32 code;
  char *name;
  int negate;
  int bits;
  int vec_shift;
} ShiftInfo;

ORC_INTERNAL extern const OrcNeonInsn orc_neon32_insns[_ORC_NEON_OP_MAX_];
ORC_INTERNAL extern const OrcNeonInsn orc_neon64_insns[_ORC_NEON_OP_MAX_];

ORC_INTERNAL void orc_neon32_binary_long_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_binary_narrow_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_binary_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_binary_vfp_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_move_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_unary_long_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_unary_narrow_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_unary_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_unary_vfp_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);

ORC_INTERNAL void orc_neon32_emit_loadil (OrcCompiler *compiler, OrcVariable *dest, int value);

ORC_INTERNAL void orc_neon32_rule_loadupdb(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_loadpX(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_loadX(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_storeX(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_shift(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_andn(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_accw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_accl(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_select1wb(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_select1lw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_select1ql(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_convhwb(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_convhlw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_mergebw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_mergewl(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_mergelq(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_splatbw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_splatbl(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_splatw3q(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_accsadubl(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_signw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_signb(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_signl(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_mulhub(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_mulhsb(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_mulhuw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_mulhsw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_mulhul(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_mulhsl(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_splitql(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_splitlw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_splitwb(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon32_rule_div255w(OrcCompiler *p, void *user, OrcInstruction *insn);

ORC_INTERNAL void orc_neon64_binary_long_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_binary_narrow_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_binary_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_binary_vfp_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_move_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_unary_long_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_unary_narrow_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_unary_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_unary_vfp_rule(OrcCompiler *compiler, void *user, OrcInstruction *insn);

ORC_INTERNAL void orc_neon64_emit_loadil (OrcCompiler *compiler, OrcVariable *dest, int value);

ORC_INTERNAL void orc_neon64_rule_loadupdb(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_loadpX(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_loadX(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_storeX(OrcCompiler *compiler, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_shift(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_andn(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_accw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_accl(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_select1wb(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_select1lw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_select1ql(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_convhwb(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_convhlw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_mergebw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_mergewl(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_mergelq(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_splatbw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_splatbl(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_splatw3q(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_accsadubl(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_signw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_signb(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_signl(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_mulhub(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_mulhsb(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_mulhuw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_mulhsw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_mulhul(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_mulhsl(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_splitql(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_splitlw(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_splitwb(OrcCompiler *p, void *user, OrcInstruction *insn);
ORC_INTERNAL void orc_neon64_rule_div255w(OrcCompiler *p, void *user, OrcInstruction *insn);

#endif
