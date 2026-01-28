#ifndef _ORC_ARM_INSN_H_
#define _ORC_ARM_INSN_H_

#include <orc/orc.h>
#include <orc/orcarm.h>

ORC_BEGIN_DECLS

#ifdef ORC_ENABLE_UNSTABLE_API

ORC_API void orc_arm_emit (OrcCompiler *compiler, orc_uint32 insn);
ORC_API void orc_arm_emit_bx_lr (OrcCompiler *compiler);
ORC_API void orc_arm_emit_load_imm (OrcCompiler *compiler, int dest, int imm);

ORC_API void orc_arm_emit_ldrb (OrcCompiler *compiler, int dest, int src1, int offset);
ORC_API void orc_arm_emit_strb (OrcCompiler *compiler, int dest, int offset, int src1);
ORC_API void orc_arm_emit_ldrh (OrcCompiler *compiler, int dest, int src1, int offset);
ORC_API void orc_arm_emit_strh (OrcCompiler *compiler, int dest, int offset, int src1);
ORC_API void orc_arm_emit_ldr (OrcCompiler *compiler, int dest, int src1, int offset);
ORC_API void orc_arm_emit_str (OrcCompiler *compiler, int dest, int offset, int src1);

ORC_API void orc_arm_emit_add (OrcCompiler *compiler, int dest, int src1, int src2);
ORC_API void orc_arm_emit_sub (OrcCompiler *compiler, int dest, int src1, int src2);
ORC_API void orc_arm_emit_add_imm (OrcCompiler *compiler, int dest, int src1, int value);
ORC_API void orc_arm_emit_and_imm (OrcCompiler *compiler, int dest, int src1, int value);
ORC_API void orc_arm_emit_sub_imm (OrcCompiler *compiler, int dest, int src1, int value, int record);
ORC_API void orc_arm_emit_asr_imm (OrcCompiler *compiler, int dest, int src1, int value);
ORC_API void orc_arm_emit_lsl_imm (OrcCompiler *compiler, int dest, int src1, int value);
ORC_API void orc_arm_emit_cmp_imm (OrcCompiler *compiler, int src1, int value);
ORC_API void orc_arm_emit_cmp (OrcCompiler *compiler, int src1, int src2);
ORC_API void orc_arm_emit_mov (OrcCompiler *compiler, int dest, int src);

ORC_API void orc_arm_emit_align (OrcCompiler *compiler, int align_shift);
ORC_API void orc_arm_emit_label (OrcCompiler *compiler, int label);
ORC_API void orc_arm_emit_push (OrcCompiler *compiler, int regs, orc_uint32 vregs);
ORC_API void orc_arm_emit_pop (OrcCompiler *compiler, int regs, orc_uint32 vregs);
ORC_API void orc_arm_emit_branch (OrcCompiler *compiler, int cond, int label);
ORC_API void orc_arm_emit_data (OrcCompiler *compiler, orc_uint32 data);

ORC_API void orc_arm_emit_load_reg (OrcCompiler *compiler, int dest, int src1, int offset);
ORC_API void orc_arm_emit_store_reg (OrcCompiler *compiler, int src, int dest, int offset);

ORC_API void orc_arm_emit_dp (OrcCompiler *p, int type, OrcArmCond cond, OrcArmDP opcode,
    int S, int Rd, int Rn, int Rm, int shift, orc_uint32 val);
ORC_API void orc_arm_emit_par (OrcCompiler *p, int op, int mode, OrcArmCond cond,
    int Rd, int Rn, int Rm);
ORC_API void orc_arm_emit_xt (OrcCompiler *p, int op, OrcArmCond cond,
    int Rd, int Rn, int Rm, int r8);
ORC_API void orc_arm_emit_pkh (OrcCompiler *p, int op, OrcArmCond cond,
    int Rd, int Rn, int Rm, int sh);
ORC_API void orc_arm_emit_sat (OrcCompiler *p, int op, OrcArmCond cond,
    int Rd, int sat, int Rm, int sh, int asr);
ORC_API void orc_arm_emit_rv (OrcCompiler *p, int op, OrcArmCond cond,
    int Rd, int Rm);
ORC_API void orc_arm_emit_nop (OrcCompiler *compiler);

/* ALL cpus */
/* data procesing instructions */
/* <op>{<cond>}{s} {<Rd>}, <Rn>, #imm */
#define orc_arm_emit_and_i(p,cond,S,Rd,Rn,imm)        orc_arm_emit_dp(p,0,cond,ORC_ARM_DP_AND,S,Rd,Rn,0,0,imm)
#define orc_arm_emit_eor_i(p,cond,S,Rd,Rn,imm)        orc_arm_emit_dp(p,0,cond,ORC_ARM_DP_EOR,S,Rd,Rn,0,0,imm)
#define orc_arm_emit_sub_i(p,cond,S,Rd,Rn,imm)        orc_arm_emit_dp(p,0,cond,ORC_ARM_DP_SUB,S,Rd,Rn,0,0,imm)
#define orc_arm_emit_rsb_i(p,cond,S,Rd,Rn,imm)        orc_arm_emit_dp(p,0,cond,ORC_ARM_DP_RSB,S,Rd,Rn,0,0,imm)
#define orc_arm_emit_add_i(p,cond,S,Rd,Rn,imm)        orc_arm_emit_dp(p,0,cond,ORC_ARM_DP_ADD,S,Rd,Rn,0,0,imm)
#define orc_arm_emit_adc_i(p,cond,S,Rd,Rn,imm)        orc_arm_emit_dp(p,0,cond,ORC_ARM_DP_ADC,S,Rd,Rn,0,0,imm)
#define orc_arm_emit_sbc_i(p,cond,S,Rd,Rn,imm)        orc_arm_emit_dp(p,0,cond,ORC_ARM_DP_SBC,S,Rd,Rn,0,0,imm)
#define orc_arm_emit_rsc_i(p,cond,S,Rd,Rn,imm)        orc_arm_emit_dp(p,0,cond,ORC_ARM_DP_RSC,S,Rd,Rn,0,0,imm)
#define orc_arm_emit_tst_i(p,cond,Rn,imm)             orc_arm_emit_dp(p,0,cond,ORC_ARM_DP_TST,1, 0,Rn,0,0,imm)
#define orc_arm_emit_teq_i(p,cond,Rn,imm)             orc_arm_emit_dp(p,0,cond,ORC_ARM_DP_TEQ,1, 0,Rn,0,0,imm)
#define orc_arm_emit_cmp_i(p,cond,Rn,imm)             orc_arm_emit_dp(p,0,cond,ORC_ARM_DP_CMP,1, 0,Rn,0,0,imm)
#define orc_arm_emit_cmn_i(p,cond,Rn,imm)             orc_arm_emit_dp(p,0,cond,ORC_ARM_DP_CMN,1, 0,Rn,0,0,imm)
#define orc_arm_emit_orr_i(p,cond,S,Rd,Rn,imm)        orc_arm_emit_dp(p,0,cond,ORC_ARM_DP_ORR,S,Rd,Rn,0,0,imm)
#define orc_arm_emit_mov_i(p,cond,S,Rd,imm)           orc_arm_emit_dp(p,0,cond,ORC_ARM_DP_MOV,S,Rd, 0,0,0,imm)
#define orc_arm_emit_bic_i(p,cond,S,Rd,Rn,imm)        orc_arm_emit_dp(p,0,cond,ORC_ARM_DP_BIC,S,Rd,Rn,0,0,imm)
#define orc_arm_emit_mvn_i(p,cond,S,Rd,imm)           orc_arm_emit_dp(p,0,cond,ORC_ARM_DP_MVN,S,Rd, 0,0,0,imm)

/* <op>{<cond>}{s} {<Rd>}, <Rn>, <Rm> */
#define orc_arm_emit_and_r(p,cond,S,Rd,Rn,Rm)         orc_arm_emit_dp(p,1,cond,ORC_ARM_DP_AND,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_eor_r(p,cond,S,Rd,Rn,Rm)         orc_arm_emit_dp(p,1,cond,ORC_ARM_DP_EOR,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_sub_r(p,cond,S,Rd,Rn,Rm)         orc_arm_emit_dp(p,1,cond,ORC_ARM_DP_SUB,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_rsb_r(p,cond,S,Rd,Rn,Rm)         orc_arm_emit_dp(p,1,cond,ORC_ARM_DP_RSB,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_add_r(p,cond,S,Rd,Rn,Rm)         orc_arm_emit_dp(p,1,cond,ORC_ARM_DP_ADD,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_adc_r(p,cond,S,Rd,Rn,Rm)         orc_arm_emit_dp(p,1,cond,ORC_ARM_DP_ADC,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_sbc_r(p,cond,S,Rd,Rn,Rm)         orc_arm_emit_dp(p,1,cond,ORC_ARM_DP_SBC,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_rsc_r(p,cond,S,Rd,Rn,Rm)         orc_arm_emit_dp(p,1,cond,ORC_ARM_DP_RSC,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_tst_r(p,cond,Rn,Rm)              orc_arm_emit_dp(p,1,cond,ORC_ARM_DP_TST,1, 0,Rn,Rm,0,0)
#define orc_arm_emit_teq_r(p,cond,Rn,Rm)              orc_arm_emit_dp(p,1,cond,ORC_ARM_DP_TEQ,1, 0,Rn,Rm,0,0)
#define orc_arm_emit_cmp_r(p,cond,Rn,Rm)              orc_arm_emit_dp(p,1,cond,ORC_ARM_DP_CMP,1, 0,Rn,Rm,0,0)
#define orc_arm_emit_cmn_r(p,cond,Rn,Rm)              orc_arm_emit_dp(p,1,cond,ORC_ARM_DP_CMN,1, 0,Rn,Rm,0,0)
#define orc_arm_emit_orr_r(p,cond,S,Rd,Rn,Rm)         orc_arm_emit_dp(p,1,cond,ORC_ARM_DP_ORR,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_mov_r(p,cond,S,Rd,Rm)            orc_arm_emit_dp(p,1,cond,ORC_ARM_DP_MOV,S,Rd, 0,Rm,0,0)
#define orc_arm_emit_bic_r(p,cond,S,Rd,Rn,Rm)         orc_arm_emit_dp(p,1,cond,ORC_ARM_DP_BIC,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_mvn_r(p,cond,S,Rd,Rm)            orc_arm_emit_dp(p,1,cond,ORC_ARM_DP_MVN,S,Rd, 0,Rm,0,0)

/* <op>{<cond>}{s} {<Rd>}, <Rn>, <Rm>, [LSL|LSR|ASR] #imm */
#define orc_arm_emit_and_rsi(p,cond,S,Rd,Rn,Rm,sh,im) orc_arm_emit_dp(p,2,cond,ORC_ARM_DP_AND,S,Rd,Rn,Rm,sh,im)
#define orc_arm_emit_eor_rsi(p,cond,S,Rd,Rn,Rm,sh,im) orc_arm_emit_dp(p,2,cond,ORC_ARM_DP_EOR,S,Rd,Rn,Rm,sh,im)
#define orc_arm_emit_sub_rsi(p,cond,S,Rd,Rn,Rm,sh,im) orc_arm_emit_dp(p,2,cond,ORC_ARM_DP_SUB,S,Rd,Rn,Rm,sh,im)
#define orc_arm_emit_rsb_rsi(p,cond,S,Rd,Rn,Rm,sh,im) orc_arm_emit_dp(p,2,cond,ORC_ARM_DP_RSB,S,Rd,Rn,Rm,sh,im)
#define orc_arm_emit_add_rsi(p,cond,S,Rd,Rn,Rm,sh,im) orc_arm_emit_dp(p,2,cond,ORC_ARM_DP_ADD,S,Rd,Rn,Rm,sh,im)
#define orc_arm_emit_adc_rsi(p,cond,S,Rd,Rn,Rm,sh,im) orc_arm_emit_dp(p,2,cond,ORC_ARM_DP_ADC,S,Rd,Rn,Rm,sh,im)
#define orc_arm_emit_sbc_rsi(p,cond,S,Rd,Rn,Rm,sh,im) orc_arm_emit_dp(p,2,cond,ORC_ARM_DP_SBC,S,Rd,Rn,Rm,sh,im)
#define orc_arm_emit_rsc_rsi(p,cond,S,Rd,Rn,Rm,sh,im) orc_arm_emit_dp(p,2,cond,ORC_ARM_DP_RSC,S,Rd,Rn,Rm,sh,im)
#define orc_arm_emit_tst_rsi(p,cond,Rn,Rm,sh,im)      orc_arm_emit_dp(p,2,cond,ORC_ARM_DP_TST,1, 0,Rn,Rm,sh,im)
#define orc_arm_emit_teq_rsi(p,cond,Rn,Rm,sh,im)      orc_arm_emit_dp(p,2,cond,ORC_ARM_DP_TEQ,1, 0,Rn,Rm,sh,im)
#define orc_arm_emit_cmp_rsi(p,cond,Rn,Rm,sh,im)      orc_arm_emit_dp(p,2,cond,ORC_ARM_DP_CMP,1, 0,Rn,Rm,sh,im)
#define orc_arm_emit_cmn_rsi(p,cond,Rn,Rm,sh,im)      orc_arm_emit_dp(p,2,cond,ORC_ARM_DP_CMN,1, 0,Rn,Rm,sh,im)
#define orc_arm_emit_orr_rsi(p,cond,S,Rd,Rn,Rm,sh,im) orc_arm_emit_dp(p,2,cond,ORC_ARM_DP_ORR,S,Rd,Rn,Rm,sh,im)
#define orc_arm_emit_mov_rsi(p,cond,S,Rd,Rm,sh,im)    orc_arm_emit_dp(p,2,cond,ORC_ARM_DP_MOV,S,Rd, 0,Rm,sh,im)
#define orc_arm_emit_bic_rsi(p,cond,S,Rd,Rn,Rm,sh,im) orc_arm_emit_dp(p,2,cond,ORC_ARM_DP_BIC,S,Rd,Rn,Rm,sh,im)
#define orc_arm_emit_mvn_rsi(p,cond,S,Rd,Rm,sh,im)    orc_arm_emit_dp(p,2,cond,ORC_ARM_DP_MVN,S,Rd, 0,Rm,sh,im)

/* <op>{<cond>}{s} {<Rd>}, <Rn>, <Rm>, [LSL|LSR|ASR] <Rs> */
#define orc_arm_emit_and_rsr(p,cond,S,Rd,Rn,Rm,sh,Rs) orc_arm_emit_dp(p,3,cond,ORC_ARM_DP_AND,S,Rd,Rn,Rm,sh,Rs)
#define orc_arm_emit_eor_rsr(p,cond,S,Rd,Rn,Rm,sh,Rs) orc_arm_emit_dp(p,3,cond,ORC_ARM_DP_EOR,S,Rd,Rn,Rm,sh,Rs)
#define orc_arm_emit_sub_rsr(p,cond,S,Rd,Rn,Rm,sh,Rs) orc_arm_emit_dp(p,3,cond,ORC_ARM_DP_SUB,S,Rd,Rn,Rm,sh,Rs)
#define orc_arm_emit_rsb_rsr(p,cond,S,Rd,Rn,Rm,sh,Rs) orc_arm_emit_dp(p,3,cond,ORC_ARM_DP_RSB,S,Rd,Rn,Rm,sh,Rs)
#define orc_arm_emit_add_rsr(p,cond,S,Rd,Rn,Rm,sh,Rs) orc_arm_emit_dp(p,3,cond,ORC_ARM_DP_ADD,S,Rd,Rn,Rm,sh,Rs)
#define orc_arm_emit_adc_rsr(p,cond,S,Rd,Rn,Rm,sh,Rs) orc_arm_emit_dp(p,3,cond,ORC_ARM_DP_ADC,S,Rd,Rn,Rm,sh,Rs)
#define orc_arm_emit_sbc_rsr(p,cond,S,Rd,Rn,Rm,sh,Rs) orc_arm_emit_dp(p,3,cond,ORC_ARM_DP_SBC,S,Rd,Rn,Rm,sh,Rs)
#define orc_arm_emit_rsc_rsr(p,cond,S,Rd,Rn,Rm,sh,Rs) orc_arm_emit_dp(p,3,cond,ORC_ARM_DP_RSC,S,Rd,Rn,Rm,sh,Rs)
#define orc_arm_emit_tst_rsr(p,cond,Rn,Rm,sh,Rs)      orc_arm_emit_dp(p,3,cond,ORC_ARM_DP_TST,1, 0,Rn,Rm,sh,Rs)
#define orc_arm_emit_teq_rsr(p,cond,Rn,Rm,sh,Rs)      orc_arm_emit_dp(p,3,cond,ORC_ARM_DP_TEQ,1, 0,Rn,Rm,sh,Rs)
#define orc_arm_emit_cmp_rsr(p,cond,Rn,Rm,sh,Rs)      orc_arm_emit_dp(p,3,cond,ORC_ARM_DP_CMP,1, 0,Rn,Rm,sh,Rs)
#define orc_arm_emit_cmn_rsr(p,cond,Rn,Rm,sh,Rs)      orc_arm_emit_dp(p,3,cond,ORC_ARM_DP_CMN,1, 0,Rn,Rm,sh,Rs)
#define orc_arm_emit_orr_rsr(p,cond,S,Rd,Rn,Rm,sh,Rs) orc_arm_emit_dp(p,3,cond,ORC_ARM_DP_ORR,S,Rd,Rn,Rm,sh,Rs)
#define orc_arm_emit_mov_rsr(p,cond,S,Rd,Rm,sh,Rs)    orc_arm_emit_dp(p,3,cond,ORC_ARM_DP_MOV,S,Rd, 0,Rm,sh,Rs)
#define orc_arm_emit_bic_rsr(p,cond,S,Rd,Rn,Rm,sh,Rs) orc_arm_emit_dp(p,3,cond,ORC_ARM_DP_BIC,S,Rd,Rn,Rm,sh,Rs)
#define orc_arm_emit_mvn_rsr(p,cond,S,Rd,Rm,sh,Rs)    orc_arm_emit_dp(p,3,cond,ORC_ARM_DP_MVN,S,Rd, 0,Rm,sh,Rs)

/* <op>{<cond>}{s} {<Rd>,} <Rn>, <Rm>, RRX */
#define orc_arm_emit_and_rrx(p,cond,S,Rd,Rn,Rm)       orc_arm_emit_dp(p,4,cond,ORC_ARM_DP_AND,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_eor_rrx(p,cond,S,Rd,Rn,Rm)       orc_arm_emit_dp(p,4,cond,ORC_ARM_DP_EOR,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_sub_rrx(p,cond,S,Rd,Rn,Rm)       orc_arm_emit_dp(p,4,cond,ORC_ARM_DP_SUB,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_rsb_rrx(p,cond,S,Rd,Rn,Rm)       orc_arm_emit_dp(p,4,cond,ORC_ARM_DP_RSB,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_add_rrx(p,cond,S,Rd,Rn,Rm)       orc_arm_emit_dp(p,4,cond,ORC_ARM_DP_ADD,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_adc_rrx(p,cond,S,Rd,Rn,Rm)       orc_arm_emit_dp(p,4,cond,ORC_ARM_DP_ADC,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_sbc_rrx(p,cond,S,Rd,Rn,Rm)       orc_arm_emit_dp(p,4,cond,ORC_ARM_DP_SBC,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_rsc_rrx(p,cond,S,Rd,Rn,Rm)       orc_arm_emit_dp(p,4,cond,ORC_ARM_DP_RSC,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_tst_rrx(p,cond,Rn,Rm)            orc_arm_emit_dp(p,4,cond,ORC_ARM_DP_TST,1, 0,Rn,Rm,0,0)
#define orc_arm_emit_teq_rrx(p,cond,Rn,Rm)            orc_arm_emit_dp(p,4,cond,ORC_ARM_DP_TEQ,1, 0,Rn,Rm,0,0)
#define orc_arm_emit_cmp_rrx(p,cond,Rn,Rm)            orc_arm_emit_dp(p,4,cond,ORC_ARM_DP_CMP,1, 0,Rn,Rm,0,0)
#define orc_arm_emit_cmn_rrx(p,cond,Rn,Rm)            orc_arm_emit_dp(p,4,cond,ORC_ARM_DP_CMN,1, 0,Rn,Rm,0,0)
#define orc_arm_emit_orr_rrx(p,cond,S,Rd,Rn,Rm)       orc_arm_emit_dp(p,4,cond,ORC_ARM_DP_ORR,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_mov_rrx(p,cond,S,Rd,Rm)          orc_arm_emit_dp(p,4,cond,ORC_ARM_DP_MOV,S,Rd, 0,Rm,0,0)
#define orc_arm_emit_bic_rrx(p,cond,S,Rd,Rn,Rm)       orc_arm_emit_dp(p,4,cond,ORC_ARM_DP_BIC,S,Rd,Rn,Rm,0,0)
#define orc_arm_emit_mvn_rrx(p,cond,S,Rd,Rm)          orc_arm_emit_dp(p,4,cond,ORC_ARM_DP_MVN,S,Rd, 0,Rm,0,0)

/* parallel instructions */
#define orc_arm_emit_sadd16(p,cond,Rd,Rn,Rm)          orc_arm_emit_par(p,0,0,cond,Rd,Rn,Rm)
#define orc_arm_emit_qadd16(p,cond,Rd,Rn,Rm)          orc_arm_emit_par(p,0,1,cond,Rd,Rn,Rm)
#define orc_arm_emit_shadd16(p,cond,Rd,Rn,Rm)         orc_arm_emit_par(p,0,2,cond,Rd,Rn,Rm)
#define orc_arm_emit_uadd16(p,cond,Rd,Rn,Rm)          orc_arm_emit_par(p,0,3,cond,Rd,Rn,Rm)
#define orc_arm_emit_uqadd16(p,cond,Rd,Rn,Rm)         orc_arm_emit_par(p,0,4,cond,Rd,Rn,Rm)
#define orc_arm_emit_uhadd16(p,cond,Rd,Rn,Rm)         orc_arm_emit_par(p,0,5,cond,Rd,Rn,Rm)

#define orc_arm_emit_saddsubx(p,cond,Rd,Rn,Rm)        orc_arm_emit_par(p,1,0,cond,Rd,Rn,Rm)
#define orc_arm_emit_qaddsubx(p,cond,Rd,Rn,Rm)        orc_arm_emit_par(p,1,1,cond,Rd,Rn,Rm)
#define orc_arm_emit_shaddsubx(p,cond,Rd,Rn,Rm)       orc_arm_emit_par(p,1,2,cond,Rd,Rn,Rm)
#define orc_arm_emit_uaddsubx(p,cond,Rd,Rn,Rm)        orc_arm_emit_par(p,1,3,cond,Rd,Rn,Rm)
#define orc_arm_emit_uqaddsubx(p,cond,Rd,Rn,Rm)       orc_arm_emit_par(p,1,4,cond,Rd,Rn,Rm)
#define orc_arm_emit_uhaddsubx(p,cond,Rd,Rn,Rm)       orc_arm_emit_par(p,1,5,cond,Rd,Rn,Rm)

#define orc_arm_emit_ssubaddx(p,cond,Rd,Rn,Rm)        orc_arm_emit_par(p,2,0,cond,Rd,Rn,Rm)
#define orc_arm_emit_qsubaddx(p,cond,Rd,Rn,Rm)        orc_arm_emit_par(p,2,1,cond,Rd,Rn,Rm)
#define orc_arm_emit_shsubaddx(p,cond,Rd,Rn,Rm)       orc_arm_emit_par(p,2,2,cond,Rd,Rn,Rm)
#define orc_arm_emit_usubaddx(p,cond,Rd,Rn,Rm)        orc_arm_emit_par(p,2,3,cond,Rd,Rn,Rm)
#define orc_arm_emit_uqsubaddx(p,cond,Rd,Rn,Rm)       orc_arm_emit_par(p,2,4,cond,Rd,Rn,Rm)
#define orc_arm_emit_uhsubaddx(p,cond,Rd,Rn,Rm)       orc_arm_emit_par(p,2,5,cond,Rd,Rn,Rm)

#define orc_arm_emit_ssub16(p,cond,Rd,Rn,Rm)          orc_arm_emit_par(p,3,0,cond,Rd,Rn,Rm)
#define orc_arm_emit_qsub16(p,cond,Rd,Rn,Rm)          orc_arm_emit_par(p,3,1,cond,Rd,Rn,Rm)
#define orc_arm_emit_shsub16(p,cond,Rd,Rn,Rm)         orc_arm_emit_par(p,3,2,cond,Rd,Rn,Rm)
#define orc_arm_emit_usub16(p,cond,Rd,Rn,Rm)          orc_arm_emit_par(p,3,3,cond,Rd,Rn,Rm)
#define orc_arm_emit_uqsub16(p,cond,Rd,Rn,Rm)         orc_arm_emit_par(p,3,4,cond,Rd,Rn,Rm)
#define orc_arm_emit_uhsub16(p,cond,Rd,Rn,Rm)         orc_arm_emit_par(p,3,5,cond,Rd,Rn,Rm)

#define orc_arm_emit_sadd8(p,cond,Rd,Rn,Rm)           orc_arm_emit_par(p,4,0,cond,Rd,Rn,Rm)
#define orc_arm_emit_qadd8(p,cond,Rd,Rn,Rm)           orc_arm_emit_par(p,4,1,cond,Rd,Rn,Rm)
#define orc_arm_emit_shadd8(p,cond,Rd,Rn,Rm)          orc_arm_emit_par(p,4,2,cond,Rd,Rn,Rm)
#define orc_arm_emit_uadd8(p,cond,Rd,Rn,Rm)           orc_arm_emit_par(p,4,3,cond,Rd,Rn,Rm)
#define orc_arm_emit_uqadd8(p,cond,Rd,Rn,Rm)          orc_arm_emit_par(p,4,4,cond,Rd,Rn,Rm)
#define orc_arm_emit_uhadd8(p,cond,Rd,Rn,Rm)          orc_arm_emit_par(p,4,5,cond,Rd,Rn,Rm)

#define orc_arm_emit_ssub8(p,cond,Rd,Rn,Rm)           orc_arm_emit_par(p,5,0,cond,Rd,Rn,Rm)
#define orc_arm_emit_qsub8(p,cond,Rd,Rn,Rm)           orc_arm_emit_par(p,5,1,cond,Rd,Rn,Rm)
#define orc_arm_emit_shsub8(p,cond,Rd,Rn,Rm)          orc_arm_emit_par(p,5,2,cond,Rd,Rn,Rm)
#define orc_arm_emit_usub8(p,cond,Rd,Rn,Rm)           orc_arm_emit_par(p,5,3,cond,Rd,Rn,Rm)
#define orc_arm_emit_uqsub8(p,cond,Rd,Rn,Rm)          orc_arm_emit_par(p,5,4,cond,Rd,Rn,Rm)
#define orc_arm_emit_uhsub8(p,cond,Rd,Rn,Rm)          orc_arm_emit_par(p,5,5,cond,Rd,Rn,Rm)

/* selection */
#define orc_arm_emit_sel(p,cond,Rd,Rn,Rm)             orc_arm_emit_par(p,6,6,cond,Rd,Rn,Rm)

/* saturating add */
#define orc_arm_emit_qadd(p,cond,Rd,Rn,Rm)            orc_arm_emit_par(p,7,7, cond,Rd,Rn,Rm)
#define orc_arm_emit_qsub(p,cond,Rd,Rn,Rm)            orc_arm_emit_par(p,8,8, cond,Rd,Rn,Rm)
#define orc_arm_emit_qdadd(p,cond,Rd,Rn,Rm)           orc_arm_emit_par(p,7,9, cond,Rd,Rn,Rm)
#define orc_arm_emit_qdsub(p,cond,Rd,Rn,Rm)           orc_arm_emit_par(p,8,10,cond,Rd,Rn,Rm)

/* extend instructions */
/* with ROR #r8, r8 should be a multiple of 8 */
#define orc_arm_emit_sxtb16_r8(p,cond,Rd,Rm,r8)       orc_arm_emit_xt(p,0, cond,Rd,15,Rm,r8)
#define orc_arm_emit_sxtb_r8(p,cond,Rd,Rm,r8)         orc_arm_emit_xt(p,1, cond,Rd,15,Rm,r8)
#define orc_arm_emit_sxth_r8(p,cond,Rd,Rm,r8)         orc_arm_emit_xt(p,2, cond,Rd,15,Rm,r8)
#define orc_arm_emit_uxtb16_r8(p,cond,Rd,Rm,r8)       orc_arm_emit_xt(p,3, cond,Rd,15,Rm,r8)
#define orc_arm_emit_uxtb_r8(p,cond,Rd,Rm,r8)         orc_arm_emit_xt(p,4, cond,Rd,15,Rm,r8)
#define orc_arm_emit_uxth_r8(p,cond,Rd,Rm,r8)         orc_arm_emit_xt(p,5, cond,Rd,15,Rm,r8)
#define orc_arm_emit_sxtab16_r8(p,cond,Rd,Rn,Rm,r8)   orc_arm_emit_xt(p,6, cond,Rd,Rn,Rm,r8)
#define orc_arm_emit_sxtab_r8(p,cond,Rd,Rn,Rm,r8)     orc_arm_emit_xt(p,7, cond,Rd,Rn,Rm,r8)
#define orc_arm_emit_sxtah_r8(p,cond,Rd,Rn,Rm,r8)     orc_arm_emit_xt(p,8, cond,Rd,Rn,Rm,r8)
#define orc_arm_emit_uxtab16_r8(p,cond,Rd,Rn,Rm,r8)   orc_arm_emit_xt(p,9, cond,Rd,Rn,Rm,r8)
#define orc_arm_emit_uxtab_r8(p,cond,Rd,Rn,Rm,r8)     orc_arm_emit_xt(p,10,cond,Rd,Rn,Rm,r8)
#define orc_arm_emit_uxtah_r8(p,cond,Rd,Rn,Rm,r8)     orc_arm_emit_xt(p,11,cond,Rd,Rn,Rm,r8)
/* with out rotate */
#define orc_arm_emit_sxtb16(p,cond,Rd,Rm)             orc_arm_emit_sxtb_r8(p,cond,Rd,Rm,0)
#define orc_arm_emit_sxtb(p,cond,Rd,Rm)               orc_arm_emit_sxtb_r8(p,cond,Rd,Rm,0)
#define orc_arm_emit_sxth(p,cond,Rd,Rm)               orc_arm_emit_sxtb_r8(p,cond,Rd,Rm,0)
#define orc_arm_emit_uxtb16(p,cond,Rd,Rm)             orc_arm_emit_sxtb_r8(p,cond,Rd,Rm,0)
#define orc_arm_emit_uxtb(p,cond,Rd,Rm)               orc_arm_emit_sxtb_r8(p,cond,Rd,Rm,0)
#define orc_arm_emit_uxth(p,cond,Rd,Rm)               orc_arm_emit_sxtb_r8(p,cond,Rd,Rm,0)
#define orc_arm_emit_sxtab16(p,cond,Rd,Rn,Rm)         orc_arm_emit_sxtb_r8(p,cond,Rd,Rm,0)
#define orc_arm_emit_sxtab(p,cond,Rd,Rn,Rm)           orc_arm_emit_sxtb_r8(p,cond,Rd,Rm,0)
#define orc_arm_emit_sxtah(p,cond,Rd,Rn,Rm)           orc_arm_emit_sxtb_r8(p,cond,Rd,Rm,0)
#define orc_arm_emit_uxtab16(p,cond,Rd,Rn,Rm)         orc_arm_emit_sxtb_r8(p,cond,Rd,Rm,0)
#define orc_arm_emit_uxtab(p,cond,Rd,Rn,Rm)           orc_arm_emit_sxtb_r8(p,cond,Rd,Rm,0)
#define orc_arm_emit_uxtah(p,cond,Rd,Rn,Rm)           orc_arm_emit_sxtb_r8(p,cond,Rd,Rm,0)

/* misc instructions */

/* packing */
#define orc_arm_emit_pkhbt(p,cond,Rd,Rn,Rm)           orc_arm_emit_pkh(p,0,cond,Rd,Rn,Rm,0)
#define orc_arm_emit_pkhtb(p,cond,Rd,Rn,Rm)           orc_arm_emit_pkh(p,1,cond,Rd,Rn,Rm,0)
/* with  [LSL|ASR] #imm */
#define orc_arm_emit_pkhbt_s(p,cond,Rd,Rn,Rm,lsl)     orc_arm_emit_pkh(p,0,cond,Rd,Rn,Rm,lsl)
#define orc_arm_emit_pkhtb_s(p,cond,Rd,Rn,Rm,asr)     orc_arm_emit_pkh(p,1,cond,Rd,Rn,Rm,asr)

/* saturation */
#define orc_arm_emit_ssat(p,cond,Rd,sat,Rm)           orc_arm_emit_sat(p,0,cond,Rd,sat,Rm,0, 0)
#define orc_arm_emit_usat(p,cond,Rd,sat,Rm)           orc_arm_emit_sat(p,1,cond,Rd,sat,Rm,0, 0)
#define orc_arm_emit_ssat_lsl(p,cond,Rd,sat,Rm,sh)    orc_arm_emit_sat(p,0,cond,Rd,sat,Rm,sh,0)
#define orc_arm_emit_usat_lsl(p,cond,Rd,sat,Rm,sh)    orc_arm_emit_sat(p,1,cond,Rd,sat,Rm,sh,0)
#define orc_arm_emit_ssat_asr(p,cond,Rd,sat,Rm,sh)    orc_arm_emit_sat(p,0,cond,Rd,sat,Rm,sh,1)
#define orc_arm_emit_usat_asr(p,cond,Rd,sat,Rm,sh)    orc_arm_emit_sat(p,1,cond,Rd,sat,Rm,sh,1)
#define orc_arm_emit_ssat16(p,cond,Rd,sat,Rm)         orc_arm_emit_sat(p,2,cond,Rd,sat,Rm,0, 0)
#define orc_arm_emit_usat16(p,cond,Rd,sat,Rm)         orc_arm_emit_sat(p,3,cond,Rd,sat,Rm,0, 0)

/* reversing */
#define orc_arm_emit_rev(p,cond,Rd,Rm)                orc_arm_emit_rv (p,0,cond,Rd,Rm)
#define orc_arm_emit_rev16(p,cond,Rd,Rm)              orc_arm_emit_rv (p,1,cond,Rd,Rm)

/* multiplies */
#define orc_arm_smulxy(cond,x,y,Rd,Rm,Rs) (0x01600080|((cond)<<28)|(((Rd)&15)<<16)|(((Rs)&15)<<8)|((y)<<6)|((x)<<5)|((Rm)&15))
#define orc_arm_emit_smulbb(p,cond,Rd,Rm,Rs) do { \
  ORC_ASM_CODE (p, "smulbb %s, %s, %s\n", orc_arm_reg_name (Rd), \
      orc_arm_reg_name (Rm), orc_arm_reg_name(Rs)); \
  orc_arm_emit (p, orc_arm_smulxy (cond,0,0,Rd,Rm,Rs)); \
} while (0)
#define orc_arm_emit_smulbt(p,cond,Rd,Rm,Rs) do { \
  ORC_ASM_CODE (p, "smulbt %s, %s, %s\n", orc_arm_reg_name (Rd), \
      orc_arm_reg_name (Rm), orc_arm_reg_name(Rs)); \
  orc_arm_emit (p, orc_arm_smulxy (cond,0,1,Rd,Rm,Rs)); \
} while (0)
#define orc_arm_emit_smultb(p,cond,Rd,Rm,Rs) do { \
  ORC_ASM_CODE (p, "smultb %s, %s, %s\n", orc_arm_reg_name (Rd), \
      orc_arm_reg_name (Rm), orc_arm_reg_name(Rs)); \
  orc_arm_emit (p, orc_arm_smulxy (cond,1,0,Rd,Rm,Rs)); \
} while (0)
#define orc_arm_emit_smultt(p,cond,Rd,Rm,Rs) do { \
  ORC_ASM_CODE (p, "smultt %s, %s, %s\n", orc_arm_reg_name (Rd), \
      orc_arm_reg_name (Rm), orc_arm_reg_name(Rs)); \
  orc_arm_emit (p, orc_arm_smulxy (cond,1,1,Rd,Rm,Rs)); \
} while (0)

#define orc_arm_mul(cond,S,Rd,Rm,Rs) (0x00000090|((cond)<<28)|((S)<<20)|(((Rd)&15)<<16)|(((Rs)&15)<<8)|((Rm)&15))
#define orc_arm_emit_mul(p,cond,S,Rd,Rm,Rs) do { \
  ORC_ASM_CODE (p, "mul %s, %s, %s\n", orc_arm_reg_name (Rd), \
      orc_arm_reg_name (Rm), orc_arm_reg_name(Rs)); \
  orc_arm_emit (p, orc_arm_mul (cond,S,Rd,Rm,Rs)); \
} while (0)

#define orc_arm_mull(op,cond,S,RdL,RdH,Rn,Rm) (op|((cond)<<28)|((S)<<20)|(((Rn)&15)<<16)|(((RdL)&15)<<12)|(((RdH)&15)<<8)|((Rm)&15))
#define orc_arm_emit_smull(p,cond,S,RdL,RdH,Rn,Rm) do { \
  ORC_ASM_CODE (p, "smull %s, %s, %s, %s\n", orc_arm_reg_name (RdL), \
      orc_arm_reg_name (RdH), \
      orc_arm_reg_name (Rn), orc_arm_reg_name(Rm)); \
  orc_arm_emit(p,orc_arm_mull (0x00c00090,cond,S,RdL,Rm,RdH,Rn)); \
} while (0)
#define orc_arm_emit_umull(p,cond,S,RdL,RdH,Rn,Rm) do { \
  ORC_ASM_CODE (p, "umull %s, %s, %s, %s\n", orc_arm_reg_name (RdL), \
      orc_arm_reg_name (RdH), \
      orc_arm_reg_name (Rn), orc_arm_reg_name(Rm)); \
  orc_arm_emit(p,orc_arm_mull (0x00800090,cond,S,RdL,RdH,Rn,Rm)); \
} while (0)

#endif

ORC_END_DECLS

#endif
