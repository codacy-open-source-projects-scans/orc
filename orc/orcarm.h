
#ifndef _ORC_ARM_H_
#define _ORC_ARM_H_

#include <orc/orc.h>

ORC_BEGIN_DECLS

#ifdef ORC_ENABLE_UNSTABLE_API

typedef enum {
  ORC_ARM_A1 = ORC_GP_REG_BASE+0,
  ORC_ARM_A2,
  ORC_ARM_A3,
  ORC_ARM_A4,
  ORC_ARM_V1,
  ORC_ARM_V2,
  ORC_ARM_V3,
  ORC_ARM_V4,
  ORC_ARM_V5,
  ORC_ARM_V6,
  ORC_ARM_V7,
  ORC_ARM_V8,
  ORC_ARM_IP,
  ORC_ARM_SP,
  ORC_ARM_LR,
  ORC_ARM_PC
} OrcArmRegister;

typedef enum {
  ORC_ARM64_R0 = ORC_GP_REG_BASE+0,
  ORC_ARM64_R1,
  ORC_ARM64_R2,
  ORC_ARM64_R3,
  ORC_ARM64_R4,
  ORC_ARM64_R5,
  ORC_ARM64_R6,
  ORC_ARM64_R7,
  ORC_ARM64_R8,
  ORC_ARM64_R9,
  ORC_ARM64_R10,
  ORC_ARM64_R11,
  ORC_ARM64_R12,
  ORC_ARM64_R13,
  ORC_ARM64_R14,
  ORC_ARM64_R15,
  ORC_ARM64_IP0,
  ORC_ARM64_IP1,
  ORC_ARM64_R18,
  ORC_ARM64_R19,
  ORC_ARM64_R20,
  ORC_ARM64_R21,
  ORC_ARM64_R22,
  ORC_ARM64_R23,
  ORC_ARM64_R24,
  ORC_ARM64_R25,
  ORC_ARM64_R26,
  ORC_ARM64_R27,
  ORC_ARM64_R28,
  ORC_ARM64_FP,
  ORC_ARM64_LR,
  ORC_ARM64_SP,
} OrcArm64Register;

typedef enum {
  ORC_ARM64_REG_32 = 32,
  ORC_ARM64_REG_64 = 64
} OrcArm64RegBits;

typedef enum {
  ORC_ARM_DP_AND = 0,
  ORC_ARM_DP_EOR,
  ORC_ARM_DP_SUB,
  ORC_ARM_DP_RSB,
  ORC_ARM_DP_ADD,
  ORC_ARM_DP_ADC,
  ORC_ARM_DP_SBC,
  ORC_ARM_DP_RSC,
  ORC_ARM_DP_TST,
  ORC_ARM_DP_TEQ,
  ORC_ARM_DP_CMP,
  ORC_ARM_DP_CMN,
  ORC_ARM_DP_ORR,
  ORC_ARM_DP_MOV,
  ORC_ARM_DP_BIC,
  ORC_ARM_DP_MVN
} OrcArmDP;

typedef enum {
  /** arithmetic */
  ORC_ARM64_DP_ADD = 0,
  ORC_ARM64_DP_ADDS, /** alias of CMN */
  ORC_ARM64_DP_SUB,
  ORC_ARM64_DP_SUBS, /** alias of CMP */
  /** logical */
  ORC_ARM64_DP_AND,
  ORC_ARM64_DP_ORR,  /** alias of MOV */
  ORC_ARM64_DP_EOR,
  ORC_ARM64_DP_ANDS, /** alias of TST */
  /** shift */
  ORC_ARM64_DP_LSL,
  ORC_ARM64_DP_LSR,
  ORC_ARM64_DP_ASR,
  ORC_ARM64_DP_ROR,
  /** bitfield */
  ORC_ARM64_DP_SBFM,
  ORC_ARM64_DP_BFM,
  ORC_ARM64_DP_UBFM,
  /** extract */
  ORC_ARM64_DP_EXTR
} OrcArm64DP;

typedef enum {
  ORC_ARM64_MEM_STR = 0,
  ORC_ARM64_MEM_LDR
} OrcArm64Mem;

typedef enum {
  ORC_ARM64_TYPE_IMM = 0,
  ORC_ARM64_TYPE_REG,
  ORC_ARM64_TYPE_EXT
} OrcArm64Type;

typedef enum {
  ORC_ARM64_UXTB,
  ORC_ARM64_UXTH,
  ORC_ARM64_UXTW,
  ORC_ARM64_UXTX,
  ORC_ARM64_SXTB,
  ORC_ARM64_SXTH,
  ORC_ARM64_SXTW,
  ORC_ARM64_SXTX,
} OrcArm64Extend;

typedef enum {
  ORC_ARM_COND_EQ = 0,
  ORC_ARM_COND_NE,
  ORC_ARM_COND_CS,
  ORC_ARM_COND_CC,
  ORC_ARM_COND_MI,
  ORC_ARM_COND_PL,
  ORC_ARM_COND_VS,
  ORC_ARM_COND_VC,
  ORC_ARM_COND_HI,
  ORC_ARM_COND_LS,
  ORC_ARM_COND_GE,
  ORC_ARM_COND_LT,
  ORC_ARM_COND_GT,
  ORC_ARM_COND_LE,
  ORC_ARM_COND_AL,
} OrcArmCond;

typedef enum {
  ORC_ARM_LSL,
  ORC_ARM_LSR,
  ORC_ARM_ASR,
  ORC_ARM_ROR
} OrcArmShift;

ORC_API unsigned long orc_arm_get_cpu_flags (void);

ORC_API const char * orc_arm_reg_name (int reg);
ORC_API const char * orc_arm_cond_name (OrcArmCond cond);

ORC_API void orc_arm_flush_cache (OrcCode *code);

ORC_API void orc_arm_add_fixup (OrcCompiler *compiler, int label, int type);
ORC_API void orc_arm_do_fixups (OrcCompiler *compiler);

#endif

ORC_END_DECLS

#endif

