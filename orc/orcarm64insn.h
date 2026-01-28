#ifndef _ORC_ARM64_INSN_H_
#define _ORC_ARM64_INSN_H_

#include <orc/orc.h>
#include <orc/orcarm.h>

ORC_BEGIN_DECLS

#ifdef ORC_ENABLE_UNSTABLE_API

ORC_API const char * orc_arm64_reg_name (int reg, OrcArm64RegBits bits);
ORC_API void orc_arm64_emit_am (OrcCompiler *p, OrcArm64RegBits bits, OrcArm64DP opcode,
    OrcArm64Type type, int opt, int Rd, int Rn, int Rm, orc_uint64 val);
ORC_API void orc_arm64_emit_lg (OrcCompiler *p, OrcArm64RegBits bits, OrcArm64DP opcode,
    OrcArm64Type type, int opt, int Rd, int Rn, int Rm, orc_uint64 val);
ORC_API void orc_arm64_emit_mov_wide (OrcCompiler *p, OrcArm64RegBits bits, int mov_op, int hw,
    int Rd, orc_uint64 val);
ORC_API void orc_arm64_emit_sft (OrcCompiler *p, OrcArm64RegBits bits, OrcArmShift shift,
    int Rd, int Rn, int Rm);
ORC_API void orc_arm64_emit_bfm (OrcCompiler *p, OrcArm64RegBits bits, OrcArm64DP opcode,
    int Rd, int Rn, orc_uint32 immr, orc_uint32 imms);
ORC_API void orc_arm64_emit_extr (OrcCompiler *p, OrcArm64RegBits bits,
    int Rd, int Rn, int Rm, orc_uint32 imm);
ORC_API void orc_arm64_emit_mem (OrcCompiler *p, OrcArm64RegBits bits, OrcArm64Mem opcode,
    OrcArm64Type type, int opt, int Rt, int Rn, int Rm, orc_uint32 val);
ORC_API void orc_arm64_emit_mem_pair (OrcCompiler *p, OrcArm64RegBits bits, OrcArm64Mem opcode,
    int opt, int Rt, int Rt2, int Rn, orc_int32 imm);
ORC_API void orc_arm64_emit_ret (OrcCompiler *p, int Rn);
ORC_API void orc_arm64_emit_push (OrcCompiler *compiler, int regs, orc_uint32 vregs);
ORC_API void orc_arm64_emit_pop (OrcCompiler *compiler, int regs, orc_uint32 vregs);
ORC_API void orc_arm64_emit_align (OrcCompiler *compiler, int align_shift);
ORC_API void orc_arm64_emit_nop (OrcCompiler *compiler);
ORC_API void orc_arm64_emit_branch (OrcCompiler *compiler, int cond, int label);
ORC_API void orc_arm64_emit_data (OrcCompiler *compiler, orc_uint32 data);
/** @todo add arm64-specific helper functions if needed */

/** Data Procesing (DP) instructions */

/** ORC_ARM64_DP_ADD */
/** ORC_ARM64_TYPE_IMM */
#define orc_arm64_emit_add_imm(p,bits,Rd,Rn,imm) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADD,ORC_ARM64_TYPE_IMM,0,Rd,Rn,0,imm)
/** ORC_ARM64_TYPE_REG */
#define orc_arm64_emit_add(p,bits,Rd,Rn,Rm) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADD,ORC_ARM64_TYPE_REG,0,Rd,Rn,Rm,0)
#define orc_arm64_emit_add_lsl(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADD,ORC_ARM64_TYPE_REG,ORC_ARM_LSL,Rd,Rn,Rm,val)
#define orc_arm64_emit_add_lsr(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADD,ORC_ARM64_TYPE_REG,ORC_ARM_LSR,Rd,Rn,Rm,val)
#define orc_arm64_emit_add_asr(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADD,ORC_ARM64_TYPE_REG,ORC_ARM_ASR,Rd,Rn,Rm,val)
#define orc_arm64_emit_add_ror(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADD,ORC_ARM64_TYPE_REG,ORC_ARM_ROR,Rd,Rn,Rm,val)
/** ORC_ARM64_TYPE_EXT */
#define orc_arm64_emit_add_uxtb(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADD,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTB,Rd,Rn,Rm,val)
#define orc_arm64_emit_add_uxth(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADD,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTH,Rd,Rn,Rm,val)
#define orc_arm64_emit_add_uxtw(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADD,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTW,Rd,Rn,Rm,val)
#define orc_arm64_emit_add_uxtx(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADD,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTX,Rd,Rn,Rm,val)
#define orc_arm64_emit_add_sxtb(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADD,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTB,Rd,Rn,Rm,val)
#define orc_arm64_emit_add_sxth(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADD,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTH,Rd,Rn,Rm,val)
#define orc_arm64_emit_add_sxtw(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADD,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTW,Rd,Rn,Rm,val)
#define orc_arm64_emit_add_sxtx(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADD,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTX,Rd,Rn,Rm,val)

/** ORC_ARM64_DP_ADDS */
/** ORC_ARM64_TYPE_IMM */
#define orc_arm64_emit_adds_imm(p,bits,Rd,Rn,imm) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_IMM,0,Rd,Rn,0,imm)
/** ORC_ARM64_TYPE_REG */
#define orc_arm64_emit_adds(p,bits,Rd,Rn,Rm) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_REG,0,Rd,Rn,Rm,0)
#define orc_arm64_emit_adds_lsl(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_REG,ORC_ARM_LSL,Rd,Rn,Rm,val)
#define orc_arm64_emit_adds_asr(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_REG,ORC_ARM_ASR,Rd,Rn,Rm,val)
#define orc_arm64_emit_adds_ror(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_REG,ORC_ARM_ROR,Rd,Rn,Rm,val)
/** ORC_ARM64_TYPE_EXT */
#define orc_arm64_emit_adds_uxtb(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTB,Rd,Rn,Rm,val)
#define orc_arm64_emit_adds_uxth(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTH,Rd,Rn,Rm,val)
#define orc_arm64_emit_adds_uxtw(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTW,Rd,Rn,Rm,val)
#define orc_arm64_emit_adds_uxtx(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTX,Rd,Rn,Rm,val)
#define orc_arm64_emit_adds_sxtb(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTB,Rd,Rn,Rm,val)
#define orc_arm64_emit_adds_sxth(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTH,Rd,Rn,Rm,val)
#define orc_arm64_emit_adds_sxtw(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTW,Rd,Rn,Rm,val)
#define orc_arm64_emit_adds_sxtx(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTX,Rd,Rn,Rm,val)

/** ORC_ARM64_DP_CMN (alias of ADDS) */
/** ORC_ARM64_TYPE_IMM */
#define orc_arm64_emit_cmn_imm(p,bits,Rn,imm) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_IMM,0,0,Rn,0,imm)
/** ORC_ARM64_TYPE_REG */
#define orc_arm64_emit_cmn(p,bits,Rn,Rm) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_REG,0,0,Rn,Rm,0)
#define orc_arm64_emit_cmn_lsl(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_REG,ORC_ARM_LSL,0,Rn,Rm,val)
#define orc_arm64_emit_cmn_asr(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_REG,ORC_ARM_ASR,0,Rn,Rm,val)
#define orc_arm64_emit_cmn_ror(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_REG,ORC_ARM_ROR,0,Rn,Rm,val)
/** ORC_ARM64_TYPE_EXT */
#define orc_arm64_emit_cmn_uxtb(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTB,0,Rn,Rm,val)
#define orc_arm64_emit_cmn_uxth(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTH,0,Rn,Rm,val)
#define orc_arm64_emit_cmn_uxtw(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTW,0,Rn,Rm,val)
#define orc_arm64_emit_cmn_uxtx(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTX,0,Rn,Rm,val)
#define orc_arm64_emit_cmn_sxtb(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTB,0,Rn,Rm,val)
#define orc_arm64_emit_cmn_sxth(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTH,0,Rn,Rm,val)
#define orc_arm64_emit_cmn_sxtw(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTW,0,Rn,Rm,val)
#define orc_arm64_emit_cmn_sxtx(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_ADDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTX,0,Rn,Rm,val)

/** ORC_ARM64_DP_SUB */
/** ORC_ARM64_TYPE_IMM */
#define orc_arm64_emit_sub_imm(p,bits,Rd,Rn,imm) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUB,ORC_ARM64_TYPE_IMM,0,Rd,Rn,0,imm)
/** ORC_ARM64_TYPE_REG */
#define orc_arm64_emit_sub(p,bits,Rd,Rn,Rm) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUB,ORC_ARM64_TYPE_REG,0,Rd,Rn,Rm,0)
#define orc_arm64_emit_sub_lsl(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUB,ORC_ARM64_TYPE_REG,ORC_ARM_LSL,Rd,Rn,Rm,val)
#define orc_arm64_emit_sub_asr(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUB,ORC_ARM64_TYPE_REG,ORC_ARM_ASR,Rd,Rn,Rm,val)
#define orc_arm64_emit_sub_ror(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUB,ORC_ARM64_TYPE_REG,ORC_ARM_ROR,Rd,Rn,Rm,val)
/** ORC_ARM64_TYPE_EXT */
#define orc_arm64_emit_sub_uxtb(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUB,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTB,Rd,Rn,Rm,val)
#define orc_arm64_emit_sub_uxth(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUB,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTH,Rd,Rn,Rm,val)
#define orc_arm64_emit_sub_uxtw(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUB,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTW,Rd,Rn,Rm,val)
#define orc_arm64_emit_sub_uxtx(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUB,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTX,Rd,Rn,Rm,val)
#define orc_arm64_emit_sub_sxtb(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUB,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTB,Rd,Rn,Rm,val)
#define orc_arm64_emit_sub_sxth(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUB,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTH,Rd,Rn,Rm,val)
#define orc_arm64_emit_sub_sxtw(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUB,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTW,Rd,Rn,Rm,val)
#define orc_arm64_emit_sub_sxtx(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUB,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTX,Rd,Rn,Rm,val)

/** ORC_ARM64_DP_SUBS */
/** ORC_ARM64_TYPE_IMM */
#define orc_arm64_emit_subs_imm(p,bits,Rd,Rn,imm) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_IMM,0,Rd,Rn,0,imm)
/** ORC_ARM64_TYPE_REG */
#define orc_arm64_emit_subs(p,bits,Rd,Rn,Rm) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_REG,0,Rd,Rn,Rm,0)
#define orc_arm64_emit_subs_lsl(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_REG,ORC_ARM_LSL,Rd,Rn,Rm,val)
#define orc_arm64_emit_subs_asr(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_REG,ORC_ARM_ASR,Rd,Rn,Rm,val)
#define orc_arm64_emit_subs_ror(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_REG,ORC_ARM_ROR,Rd,Rn,Rm,val)
/** ORC_ARM64_TYPE_EXT */
#define orc_arm64_emit_subs_uxtb(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTB,Rd,Rn,Rm,val)
#define orc_arm64_emit_subs_uxth(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTH,Rd,Rn,Rm,val)
#define orc_arm64_emit_subs_uxtw(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTW,Rd,Rn,Rm,val)
#define orc_arm64_emit_subs_uxtx(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTX,Rd,Rn,Rm,val)
#define orc_arm64_emit_subs_sxtb(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTB,Rd,Rn,Rm,val)
#define orc_arm64_emit_subs_sxth(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTH,Rd,Rn,Rm,val)
#define orc_arm64_emit_subs_sxtw(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTW,Rd,Rn,Rm,val)
#define orc_arm64_emit_subs_sxtx(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTX,Rd,Rn,Rm,val)

/** ORC_ARM64_DP_CMP (alias of SUBS) */
/** ORC_ARM64_TYPE_IMM */
#define orc_arm64_emit_cmp_imm(p,bits,Rn,imm) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_IMM,0,0,Rn,0,imm)
/** ORC_ARM64_TYPE_REG */
#define orc_arm64_emit_cmp(p,bits,Rn,Rm) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_REG,0,0,Rn,Rm,0)
#define orc_arm64_emit_cmp_lsl(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_REG,ORC_ARM_LSL,0,Rn,Rm,val)
#define orc_arm64_emit_cmp_asr(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_REG,ORC_ARM_ASR,0,Rn,Rm,val)
#define orc_arm64_emit_cmp_ror(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_REG,ORC_ARM_ROR,0,Rn,Rm,val)
/** ORC_ARM64_TYPE_EXT */
#define orc_arm64_emit_cmp_uxtb(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTB,0,Rn,Rm,val)
#define orc_arm64_emit_cmp_uxth(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTH,0,Rn,Rm,val)
#define orc_arm64_emit_cmp_uxtw(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTW,0,Rn,Rm,val)
#define orc_arm64_emit_cmp_uxtx(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTX,0,Rn,Rm,val)
#define orc_arm64_emit_cmp_sxtb(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTB,0,Rn,Rm,val)
#define orc_arm64_emit_cmp_sxth(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTH,0,Rn,Rm,val)
#define orc_arm64_emit_cmp_sxtw(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTW,0,Rn,Rm,val)
#define orc_arm64_emit_cmp_sxtx(p,bits,Rn,Rm,val) \
  orc_arm64_emit_am(p,bits,ORC_ARM64_DP_SUBS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTX,0,Rn,Rm,val)

/** ORC_ARM64_DP_AND */
/** ORC_ARM64_TYPE_IMM */
#define orc_arm64_emit_and_imm(p,bits,Rd,Rn,imm) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_AND,ORC_ARM64_TYPE_IMM,0,Rd,Rn,0,imm)
/** ORC_ARM64_TYPE_REG */
#define orc_arm64_emit_and(p,bits,Rd,Rn,Rm) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_AND,ORC_ARM64_TYPE_REG,0,Rd,Rn,0,0)
#define orc_arm64_emit_and_lsl(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_AND,ORC_ARM64_TYPE_REG,ORC_ARM_LSL,Rd,Rn,Rm,val)
#define orc_arm64_emit_and_asr(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_AND,ORC_ARM64_TYPE_REG,ORC_ARM_ASR,Rd,Rn,Rm,val)
#define orc_arm64_emit_and_ror(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_AND,ORC_ARM64_TYPE_REG,ORC_ARM_ROR,Rd,Rn,Rm,val)
/** ORC_ARM64_TYPE_EXT */
#define orc_arm64_emit_and_uxtb(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_AND,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTB,Rd,Rn,Rm,val)
#define orc_arm64_emit_and_uxth(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_AND,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTH,Rd,Rn,Rm,val)
#define orc_arm64_emit_and_uxtw(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_AND,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTW,Rd,Rn,Rm,val)
#define orc_arm64_emit_and_uxtx(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_AND,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTX,Rd,Rn,Rm,val)
#define orc_arm64_emit_and_sxtb(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_AND,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTB,Rd,Rn,Rm,val)
#define orc_arm64_emit_and_sxth(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_AND,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTH,Rd,Rn,Rm,val)
#define orc_arm64_emit_and_sxtw(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_AND,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTW,Rd,Rn,Rm,val)
#define orc_arm64_emit_and_sxtx(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_AND,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTX,Rd,Rn,Rm,val)

/** ORC_ARM64_DP_ORR */
/** ORC_ARM64_TYPE_IMM */
#define orc_arm64_emit_orr_imm(p,bits,Rd,Rn,imm) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ORR,ORC_ARM64_TYPE_IMM,0,Rd,Rn,0,imm)
/** ORC_ARM64_TYPE_REG */
#define orc_arm64_emit_orr(p,bits,Rd,Rn,Rm) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ORR,ORC_ARM64_TYPE_REG,0,Rd,Rn,Rm,0)
#define orc_arm64_emit_orr_lsl(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ORR,ORC_ARM64_TYPE_REG,ORC_ARM_LSL,Rd,Rn,Rm,val)
#define orc_arm64_emit_orr_asr(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ORR,ORC_ARM64_TYPE_REG,ORC_ARM_ASR,Rd,Rn,Rm,val)
#define orc_arm64_emit_orr_ror(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ORR,ORC_ARM64_TYPE_REG,ORC_ARM_ROR,Rd,Rn,Rm,val)
/** ORC_ARM64_TYPE_EXT */
#define orc_arm64_emit_orr_uxtb(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ORR,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTB,Rd,Rn,Rm,val)
#define orc_arm64_emit_orr_uxth(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ORR,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTH,Rd,Rn,Rm,val)
#define orc_arm64_emit_orr_uxtw(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ORR,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTW,Rd,Rn,Rm,val)
#define orc_arm64_emit_orr_uxtx(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ORR,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTX,Rd,Rn,Rm,val)
#define orc_arm64_emit_orr_sxtb(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ORR,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTB,Rd,Rn,Rm,val)
#define orc_arm64_emit_orr_sxth(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ORR,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTH,Rd,Rn,Rm,val)
#define orc_arm64_emit_orr_sxtw(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ORR,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTW,Rd,Rn,Rm,val)
#define orc_arm64_emit_orr_sxtx(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ORR,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTX,Rd,Rn,Rm,val)

/** ORC_ARM64_DP_MOV (alias of ROR, in case of imm) */
#define orc_arm64_emit_mov_imm(p,bits,Rd,imm) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ORR,ORC_ARM64_TYPE_IMM,0,Rd,0,0,imm)
#define orc_arm64_emit_mov_uimm(p,bits,Rd,imm) \
  orc_arm64_emit_movz(p,bits,0,Rd,imm)
#define orc_arm64_emit_movn(p,bits,shift,Rd,imm) \
  orc_arm64_emit_mov_wide(p,bits,0,shift,Rd,imm)
#define orc_arm64_emit_movz(p,bits,shift,Rd,imm) \
  orc_arm64_emit_mov_wide(p,bits,2,shift,Rd,imm)
#define orc_arm64_emit_movk(p,bits,shift,Rd,imm) \
  orc_arm64_emit_mov_wide(p,bits,3,shift,Rd,imm)

/** ORC_ARM64_TYPE_REG */
#define orc_arm64_emit_mov(p,bits,Rd,Rm) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ORR,ORC_ARM64_TYPE_REG,0,Rd,0,Rm,0)

/** ORC_ARM64_DP_EOR */
/** ORC_ARM64_TYPE_IMM */
#define orc_arm64_emit_eor_imm(p,bits,Rd,Rn,imm) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_EOR,ORC_ARM64_TYPE_IMM,0,Rd,Rn,0,imm)
/** ORC_ARM64_TYPE_REG */
#define orc_arm64_emit_eor(p,bits,Rd,Rn,Rm) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_EOR,ORC_ARM64_TYPE_REG,0,Rd,Rn,Rm,0)
#define orc_arm64_emit_eor_lsl(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_EOR,ORC_ARM64_TYPE_REG,ORC_ARM_LSL,Rd,Rn,Rm,val)
#define orc_arm64_emit_eor_asr(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_EOR,ORC_ARM64_TYPE_REG,ORC_ARM_ASR,Rd,Rn,Rm,val)
#define orc_arm64_emit_eor_ror(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_EOR,ORC_ARM64_TYPE_REG,ORC_ARM_ROR,Rd,Rn,Rm,val)
/** ORC_ARM64_TYPE_EXT */
#define orc_arm64_emit_eor_uxtb(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_EOR,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTB,Rd,Rn,Rm,val)
#define orc_arm64_emit_eor_uxth(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_EOR,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTH,Rd,Rn,Rm,val)
#define orc_arm64_emit_eor_uxtw(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_EOR,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTW,Rd,Rn,Rm,val)
#define orc_arm64_emit_eor_uxtx(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_EOR,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTX,Rd,Rn,Rm,val)
#define orc_arm64_emit_eor_sxtb(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_EOR,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTB,Rd,Rn,Rm,val)
#define orc_arm64_emit_eor_sxth(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_EOR,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTH,Rd,Rn,Rm,val)
#define orc_arm64_emit_eor_sxtw(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_EOR,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTW,Rd,Rn,Rm,val)
#define orc_arm64_emit_eor_sxtx(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_EOR,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTX,Rd,Rn,Rm,val)

/** ORC_ARM64_DP_ANDS */
/** ORC_ARM64_TYPE_IMM */
#define orc_arm64_emit_ands_imm(p,bits,Rd,Rn,imm) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_IMM,0,Rd,Rn,0,imm)
/** ORC_ARM64_TYPE_REG */
#define orc_arm64_emit_ands(p,bits,Rd,Rn,Rm) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_REG,0,Rd,Rn,Rm,0)
#define orc_arm64_emit_ands_lsl(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_REG,ORC_ARM_LSL,Rd,Rn,Rm,val)
#define orc_arm64_emit_ands_asr(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_REG,ORC_ARM_ASR,Rd,Rn,Rm,val)
#define orc_arm64_emit_ands_ror(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_REG,ORC_ARM_ROR,Rd,Rn,Rm,val)
/** ORC_ARM64_TYPE_EXT */
#define orc_arm64_emit_ands_uxtb(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTB,Rd,Rn,Rm,val)
#define orc_arm64_emit_ands_uxth(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTH,Rd,Rn,Rm,val)
#define orc_arm64_emit_ands_uxtw(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTW,Rd,Rn,Rm,val)
#define orc_arm64_emit_ands_uxtx(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTX,Rd,Rn,Rm,val)
#define orc_arm64_emit_ands_sxtb(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTB,Rd,Rn,Rm,val)
#define orc_arm64_emit_ands_sxth(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTH,Rd,Rn,Rm,val)
#define orc_arm64_emit_ands_sxtw(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTW,Rd,Rn,Rm,val)
#define orc_arm64_emit_ands_sxtx(p,bits,Rd,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTX,Rd,Rn,Rm,val)

/** ORC_ARM64_DP_TST (alias of ANDS) */
/** ORC_ARM64_TYPE_IMM */
#define orc_arm64_emit_tst_imm(p,bits,Rn,imm) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_IMM,0,0,Rn,0,imm)
/** ORC_ARM64_TYPE_REG */
#define orc_arm64_emit_tst(p,bits,Rn,Rm) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_REG,0,0,Rn,Rm,0)
#define orc_arm64_emit_tst_lsl(p,bits,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_REG,ORC_ARM_LSL,0,Rn,Rm,val)
#define orc_arm64_emit_tst_asr(p,bits,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_REG,ORC_ARM_ASR,0,Rn,Rm,val)
#define orc_arm64_emit_tst_ror(p,bits,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_REG,ORC_ARM_ROR,0,Rn,Rm,val)
/** ORC_ARM64_TYPE_EXT */
#define orc_arm64_emit_tst_uxtb(p,bits,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTB,0,Rn,Rm,val)
#define orc_arm64_emit_tst_uxth(p,bits,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTH,0,Rn,Rm,val)
#define orc_arm64_emit_tst_uxtw(p,bits,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTW,0,Rn,Rm,val)
#define orc_arm64_emit_tst_uxtx(p,bits,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_UXTX,0,Rn,Rm,val)
#define orc_arm64_emit_tst_sxtb(p,bits,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTB,0,Rn,Rm,val)
#define orc_arm64_emit_tst_sxth(p,bits,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTH,0,Rn,Rm,val)
#define orc_arm64_emit_tst_sxtw(p,bits,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTW,0,Rn,Rm,val)
#define orc_arm64_emit_tst_sxtx(p,bits,Rn,Rm,val) \
  orc_arm64_emit_lg(p,bits,ORC_ARM64_DP_ANDS,ORC_ARM64_TYPE_EXT,ORC_ARM64_SXTX,0,Rn,Rm,val)

/** ORC_ARM64_DP_LSL/ASR/ASR/ROR */
/** ORC_ARM64_TYPE_IMM; aliases of ORC_ARM64_DP_SBFM,UBFM,EXTR */
#define orc_arm64_emit_lsl_imm(p,bits,Rd,Rn,imm) \
  orc_arm64_emit_bfm(p,bits,ORC_ARM64_DP_UBFM,Rd,Rn,imm,imm-1)
#define orc_arm64_emit_lsr_imm(p,bits,Rd,Rn,imm) \
  orc_arm64_emit_bfm(p,bits,ORC_ARM64_DP_UBFM,Rd,Rn,imm,0x1f)
#define orc_arm64_emit_asr_imm(p,bits,Rd,Rn,imm) \
  orc_arm64_emit_bfm(p,bits,ORC_ARM64_DP_SBFM,Rd,Rn,imm,0x1f)
#define orc_arm64_emit_ror_imm(p,bits,Rd,Rn,imm) \
  orc_arm64_emit_extr(p,bits,Rd,Rn,Rn,imm)
/** ORC_ARM64_TYPE_REG */
#define orc_arm64_emit_lsl(p,bits,Rd,Rn,Rm) \
  orc_arm64_emit_sft(p,bits,ORC_ARM64_DP_LSL,Rd,Rn,Rm)
#define orc_arm64_emit_lsr(p,bits,Rd,Rn,Rm) \
  orc_arm64_emit_sft(p,bits,ORC_ARM64_DP_LSR,Rd,Rn,Rm)
#define orc_arm64_emit_asr(p,bits,Rd,Rn,Rm) \
  orc_arm64_emit_sft(p,bits,ORC_ARM64_DP_ASR,Rd,Rn,Rm)
#define orc_arm64_emit_ror(p,bits,Rd,Rn,Rm) \
  orc_arm64_emit_sft(p,bits,ORC_ARM64_DP_ROR,Rd,Rn,Rm)

/** memory access instructions */

/** ORC_ARM64_MEM_STR */
/** ORC_ARM64_TYPE_REG */
#define orc_arm64_emit_store_reg(p,b,Rt,Rn,imm) \
  orc_arm64_emit_mem(p,b,ORC_ARM64_MEM_STR,ORC_ARM64_TYPE_REG,0,Rt,Rn,0,imm)
#define orc_arm64_emit_store_pre(p,b,Rt,Rn,imm) \
  orc_arm64_emit_mem(p,b,ORC_ARM64_MEM_STR,ORC_ARM64_TYPE_REG,1,Rt,Rn,0,imm)
#define orc_arm64_emit_store_post(p,b,Rt,Rn,imm) \
  orc_arm64_emit_mem(p,b,ORC_ARM64_MEM_STR,ORC_ARM64_TYPE_REG,2,Rt,Rn,0,imm)
/** ORC_ARM64_TYPE_EXT */
#define orc_arm64_emit_store_uxtw(p,b,Rt,Rn,Rm,imm) \
  orc_arm64_emit_mem(p,b,ORC_ARM64_MEM_STR,ORC_ARM64_TYPE_EXT,ORC_ARM64_EXTEND_UXTW,Rt,Rn,Rm,imm)
#define orc_arm64_emit_store_lsl(p,b,Rt,Rn,Rm,imm) \
  orc_arm64_emit_mem(p,b,ORC_ARM64_MEM_STR,ORC_ARM64_TYPE_EXT,ORC_ARM64_EXTEND_UXTX,Rt,Rn,Rm,imm)
#define orc_arm64_emit_store_sxtw(p,b,Rt,Rn,Rm,imm) \
  orc_arm64_emit_mem(p,b,ORC_ARM64_MEM_STR,ORC_ARM64_TYPE_EXT,ORC_ARM64_EXTEND_SXTW,Rt,Rn,Rm,imm)
#define orc_arm64_emit_store_sxtx(p,b,Rt,Rn,Rm,imm) \
  orc_arm64_emit_mem(p,b,ORC_ARM64_MEM_STR,ORC_ARM64_TYPE_EXT,ORC_ARM64_EXTEND_SXTX,Rt,Rn,Rm,imm)

/** ORC_ARM64_MEM_LDR */
/** ORC_ARM64_TYPE_IMM */
#define orc_arm64_emit_load_imm(p,b,Rt,imm) \
  orc_arm64_emit_mem(p,b,ORC_ARM64_MEM_LDR,ORC_ARM64_TYPE_IMM,0,Rt,0,0,imm)
#define orc_arm64_emit_load_label(p,b,Rt,label) \
  orc_arm64_emit_mem(p,b,ORC_ARM64_MEM_LDR,ORC_ARM64_TYPE_IMM,label,Rt,0,0,0)
/** ORC_ARM64_TYPE_REG */
#define orc_arm64_emit_load_reg(p,b,Rt,Rn,imm) \
  orc_arm64_emit_mem(p,b,ORC_ARM64_MEM_LDR,ORC_ARM64_TYPE_REG,0,Rt,Rn,0,imm)
#define orc_arm64_emit_load_pre(p,b,Rt,Rn,imm) \
  orc_arm64_emit_mem(p,b,ORC_ARM64_MEM_LDR,ORC_ARM64_TYPE_REG,1,Rt,Rn,0,imm)
#define orc_arm64_emit_load_post(p,b,Rt,Rn,imm) \
  orc_arm64_emit_mem(p,b,ORC_ARM64_MEM_LDR,ORC_ARM64_TYPE_REG,2,Rt,Rn,0,imm)
/** ORC_ARM64_TYPE_EXT */
#define orc_arm64_emit_load_uxtw(p,b,Rt,Rn,Rm,imm) \
  orc_arm64_emit_mem(p,b,ORC_ARM64_MEM_LDR,ORC_ARM64_TYPE_EXT,ORC_ARM64_EXTEND_UXTW,Rt,Rn,Rm,imm)
#define orc_arm64_emit_load_lsl(p,b,Rt,Rn,Rm,imm) \
  orc_arm64_emit_mem(p,b,ORC_ARM64_MEM_LDR,ORC_ARM64_TYPE_EXT,ORC_ARM64_EXTEND_UXTX,Rt,Rn,Rm,imm)
#define orc_arm64_emit_load_sxtw(p,b,Rt,Rn,Rm,imm) \
  orc_arm64_emit_mem(p,b,ORC_ARM64_MEM_LDR,ORC_ARM64_TYPE_EXT,ORC_ARM64_EXTEND_SXTW,Rt,Rn,Rm,imm)
#define orc_arm64_emit_load_sxtx(p,b,Rt,Rn,Rm,imm) \
  orc_arm64_emit_mem(p,b,ORC_ARM64_MEM_LDR,ORC_ARM64_TYPE_EXT,ORC_ARM64_EXTEND_SXTX,Rt,Rn,Rm,imm)

/** memory access instructions (pair) */

/** ORC_ARM64_MEM_STR */
#define orc_arm64_emit_store_pair_reg(p,b,Rt,Rt2,Rn,imm) \
  orc_arm64_emit_mem_pair(p,b,ORC_ARM64_MEM_STR,2,Rt,Rt2,Rn,imm)
#define orc_arm64_emit_store_pair_pre(p,b,Rt,Rt2,Rn,imm) \
  orc_arm64_emit_mem_pair(p,b,ORC_ARM64_MEM_STR,3,Rt,Rt2,Rn,imm)
#define orc_arm64_emit_store_pair_post(p,b,Rt,Rt2,Rn,imm) \
  orc_arm64_emit_mem_pair(p,b,ORC_ARM64_MEM_STR,1,Rt,Rt2,Rn,imm)

/** ORC_ARM64_MEM_LDR */
#define orc_arm64_emit_load_pair_reg(p,b,Rt,Rt2,Rn,imm) \
  orc_arm64_emit_mem_pair(p,b,ORC_ARM64_MEM_LDR,2,Rt,Rt2,Rn,imm)
#define orc_arm64_emit_load_pair_pre(p,b,Rt,Rt2,Rn,imm) \
  orc_arm64_emit_mem_pair(p,b,ORC_ARM64_MEM_LDR,3,Rt,Rt2,Rn,imm)
#define orc_arm64_emit_load_pair_post(p,b,Rt,Rt2,Rn,imm) \
  orc_arm64_emit_mem_pair(p,b,ORC_ARM64_MEM_LDR,1,Rt,Rt2,Rn,imm)

#endif

ORC_END_DECLS

#endif
