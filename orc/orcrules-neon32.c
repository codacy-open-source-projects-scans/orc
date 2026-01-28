#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>

#include <orc/orcprogram.h>
#include <orc/orcinternal.h>
#include <orc/orcarm.h>
#include <orc/orcarminsn.h>
#include <orc/orcdebug.h>
#include <orc/orcutils-private.h>

#include <orc/orcneon.h>
#include <orc/orcneon-private.h>

static void orc_neon32_emit_loadpb (OrcCompiler *compiler, int dest, int param);
static void orc_neon32_emit_loadpw (OrcCompiler *compiler, int dest, int param);
static void orc_neon32_emit_loadpl (OrcCompiler *compiler, int dest, int param);
static void orc_neon32_emit_loadib (OrcCompiler *compiler, OrcVariable *dest, int param);
static void orc_neon32_emit_loadiw (OrcCompiler *compiler, OrcVariable *dest, int param);
static void orc_neon32_emit_loadiq (OrcCompiler *compiler, OrcVariable *dest, long long param);
static void orc_neon32_emit_loadpq (OrcCompiler *compiler, int dest, int param);

const OrcNeonInsn orc_neon32_insns[_ORC_NEON_OP_MAX_] = {
  [ORC_NEON_OP_ABSB]      = { "vabs.s8"     ,0xf3b10300, 3 },
  [ORC_NEON_OP_ADDB]      = { "vadd.i8"     ,0xf2000800, 3 },
  [ORC_NEON_OP_ADDSSB]    = { "vqadd.s8"    ,0xf2000010, 3 },
  [ORC_NEON_OP_ADDUSB]    = { "vqadd.u8"    ,0xf3000010, 3 },
  [ORC_NEON_OP_ANDB]      = { "vand"        ,0xf2000110, 3 },
/*[ORC_NEON_OP_ANDNB]     = { "vbic"        ,0xf2100110, 3 },*/
  [ORC_NEON_OP_AVGSB]     = { "vrhadd.s8"   ,0xf2000100, 3 },
  [ORC_NEON_OP_AVGUB]     = { "vrhadd.u8"   ,0xf3000100, 3 },
  [ORC_NEON_OP_CMPEQB]    = { "vceq.i8"     ,0xf3000810, 3 },
  [ORC_NEON_OP_CMPGTSB]   = { "vcgt.s8"     ,0xf2000300, 3 },
  [ORC_NEON_OP_COPYB]     = { "vmov"        ,0xf2200110, 3 },
  [ORC_NEON_OP_MAXSB]     = { "vmax.s8"     ,0xf2000600, 3 },
  [ORC_NEON_OP_MAXUB]     = { "vmax.u8"     ,0xf3000600, 3 },
  [ORC_NEON_OP_MINSB]     = { "vmin.s8"     ,0xf2000610, 3 },
  [ORC_NEON_OP_MINUB]     = { "vmin.u8"     ,0xf3000610, 3 },
  [ORC_NEON_OP_MULLB]     = { "vmul.i8"     ,0xf2000910, 3 },
  [ORC_NEON_OP_ORB]       = { "vorr"        ,0xf2200110, 3 },
/*[ORC_NEON_OP_SHLB]      = { "vshl.i8"     ,0xf2880510, 3 },*/
/*[ORC_NEON_OP_SHRSB]     = { "vshr.s8"     ,0xf2880010, 3 },*/
/*[ORC_NEON_OP_SHRUB]     = { "vshr.u8"     ,0xf3880010, 3 },*/
  [ORC_NEON_OP_SUBB]      = { "vsub.i8"     ,0xf3000800, 3 },
  [ORC_NEON_OP_SUBSSB]    = { "vqsub.s8"    ,0xf2000210, 3 },
  [ORC_NEON_OP_SUBUSB]    = { "vqsub.u8"    ,0xf3000210, 3 },
  [ORC_NEON_OP_XORB]      = { "veor"        ,0xf3000110, 3 },
  [ORC_NEON_OP_ABSW]      = { "vabs.s16"    ,0xf3b50300, 2 },
  [ORC_NEON_OP_ADDW]      = { "vadd.i16"    ,0xf2100800, 2 },
  [ORC_NEON_OP_ADDSSW]    = { "vqadd.s16"   ,0xf2100010, 2 },
  [ORC_NEON_OP_ADDUSW]    = { "vqadd.u16"   ,0xf3100010, 2 },
  [ORC_NEON_OP_ANDW]      = { "vand"        ,0xf2000110, 2 },
/*[ORC_NEON_OP_ANDNW]     = { "vbic"        ,0xf2100110, 2 },*/
  [ORC_NEON_OP_AVGSW]     = { "vrhadd.s16"  ,0xf2100100, 2 },
  [ORC_NEON_OP_AVGUW]     = { "vrhadd.u16"  ,0xf3100100, 2 },
  [ORC_NEON_OP_CMPEQW]    = { "vceq.i16"    ,0xf3100810, 2 },
  [ORC_NEON_OP_CMPGTSW]   = { "vcgt.s16"    ,0xf2100300, 2 },
  [ORC_NEON_OP_COPYW]     = { "vmov"        ,0xf2200110, 2 },
  [ORC_NEON_OP_MAXSW]     = { "vmax.s16"    ,0xf2100600, 2 },
  [ORC_NEON_OP_MAXUW]     = { "vmax.u16"    ,0xf3100600, 2 },
  [ORC_NEON_OP_MINSW]     = { "vmin.s16"    ,0xf2100610, 2 },
  [ORC_NEON_OP_MINUW]     = { "vmin.u16"    ,0xf3100610, 2 },
  [ORC_NEON_OP_MULLW]     = { "vmul.i16"    ,0xf2100910, 2 },
  [ORC_NEON_OP_ORW]       = { "vorr"        ,0xf2200110, 2 },
/*[ORC_NEON_OP_SHLW]      = { "vshl.i16"    ,0xf2900510, 2 },*/
/*[ORC_NEON_OP_SHRSW]     = { "vshr.s16"    ,0xf2900010, 2 },*/
/*[ORC_NEON_OP_SHRUW]     = { "vshr.u16"    ,0xf3900010, 2 },*/
  [ORC_NEON_OP_SUBW]      = { "vsub.i16"    ,0xf3100800, 2 },
  [ORC_NEON_OP_SUBSSW]    = { "vqsub.s16"   ,0xf2100210, 2 },
  [ORC_NEON_OP_SUBUSW]    = { "vqsub.u16"   ,0xf3100210, 2 },
  [ORC_NEON_OP_XORW]      = { "veor"        ,0xf3000110, 2 },
  [ORC_NEON_OP_ABSL]      = { "vabs.s32"    ,0xf3b90300, 1 },
  [ORC_NEON_OP_ADDL]      = { "vadd.i32"    ,0xf2200800, 1 },
  [ORC_NEON_OP_ADDSSL]    = { "vqadd.s32"   ,0xf2200010, 1 },
  [ORC_NEON_OP_ADDUSL]    = { "vqadd.u32"   ,0xf3200010, 1 },
  [ORC_NEON_OP_ANDL]      = { "vand"        ,0xf2000110, 1 },
/*[ORC_NEON_OP_ANDNL]     = { "vbic"        ,0xf2100110, 1 },*/
  [ORC_NEON_OP_AVGSL]     = { "vrhadd.s32"  ,0xf2200100, 1 },
  [ORC_NEON_OP_AVGUL]     = { "vrhadd.u32"  ,0xf3200100, 1 },
  [ORC_NEON_OP_CMPEQL]    = { "vceq.i32"    ,0xf3200810, 1 },
  [ORC_NEON_OP_CMPGTSL]   = { "vcgt.s32"    ,0xf2200300, 1 },
  [ORC_NEON_OP_COPYL]     = { "vmov"        ,0xf2200110, 1 },
  [ORC_NEON_OP_MAXSL]     = { "vmax.s32"    ,0xf2200600, 1 },
  [ORC_NEON_OP_MAXUL]     = { "vmax.u32"    ,0xf3200600, 1 },
  [ORC_NEON_OP_MINSL]     = { "vmin.s32"    ,0xf2200610, 1 },
  [ORC_NEON_OP_MINUL]     = { "vmin.u32"    ,0xf3200610, 1 },
  [ORC_NEON_OP_MULLL]     = { "vmul.i32"    ,0xf2200910, 1 },
  [ORC_NEON_OP_ORL]       = { "vorr"        ,0xf2200110, 1 },
/*[ORC_NEON_OP_SHLL]      = { "vshl.i32"    ,0xf2a00510, 1 },*/
/*[ORC_NEON_OP_SHRSL]     = { "vshr.s32"    ,0xf2a00010, 1 },*/
/*[ORC_NEON_OP_SHRUL]     = { "vshr.u32"    ,0xf3a00010, 1 },*/
  [ORC_NEON_OP_SUBL]      = { "vsub.i32"    ,0xf3200800, 1 },
  [ORC_NEON_OP_SUBSSL]    = { "vqsub.s32"   ,0xf2200210, 1 },
  [ORC_NEON_OP_SUBUSL]    = { "vqsub.u32"   ,0xf3200210, 1 },
  [ORC_NEON_OP_XORL]      = { "veor"        ,0xf3000110, 1 },
/*[ORC_NEON_OP_ABSQ]      = { "vabs.s64"    ,0xf3b10300, 0 },*/
  [ORC_NEON_OP_ADDQ]      = { "vadd.i64"    ,0xf2300800, 0 },
/*[ORC_NEON_OP_ADDSSQ]    = { "vqadd.s64"   ,0xf2000010, 0 },*/
/*[ORC_NEON_OP_ADDUSQ]    = { "vqadd.u64"   ,0xf3000010, 0 },*/
  [ORC_NEON_OP_ANDQ]      = { "vand"        ,0xf2000110, 0 },
/*[ORC_NEON_OP_AVGSQ]     = { "vrhadd.s64"  ,0xf2000100, 0 },*/
/*[ORC_NEON_OP_AVGUQ]     = { "vrhadd.u64"  ,0xf3000100, 0 },*/
/*[ORC_NEON_OP_CMPEQQ]    = { "vceq.i64"    ,0xf3000810, 0 },*/
/*[ORC_NEON_OP_CMPGTSQ]   = { "vcgt.s64"    ,0xf2000300, 0 },*/
  [ORC_NEON_OP_COPYQ]     = { "vmov"        ,0xf2200110, 0 },
/*[ORC_NEON_OP_MAXSQ]     = { "vmax.s64"    ,0xf2000600, 0 },*/
/*[ORC_NEON_OP_MAXUQ]     = { "vmax.u64"    ,0xf3000600, 0 },*/
/*[ORC_NEON_OP_MINSQ]     = { "vmin.s64"    ,0xf2000610, 0 },*/
/*[ORC_NEON_OP_MINUQ]     = { "vmin.u64"    ,0xf3000610, 0 },*/
/*[ORC_NEON_OP_MULLQ]     = { "vmul.i64"    ,0xf2000910, 0 },*/
  [ORC_NEON_OP_ORQ]       = { "vorr"        ,0xf2200110, 0 },
  [ORC_NEON_OP_SUBQ]      = { "vsub.i64"    ,0xf3300800, 0 },
/*[ORC_NEON_OP_SUBSSQ]    = { "vqsub.s64"   ,0xf2000210, 0 },*/
/*[ORC_NEON_OP_SUBUSQ]    = { "vqsub.u64"   ,0xf3000210, 0 },*/
  [ORC_NEON_OP_XORQ]      = { "veor"        ,0xf3000110, 0 },
  [ORC_NEON_OP_CONVSBW]   = { "vmovl.s8"    ,0xf2880a10, 3 },
  [ORC_NEON_OP_CONVUBW]   = { "vmovl.u8"    ,0xf3880a10, 3 },
  [ORC_NEON_OP_CONVSWL]   = { "vmovl.s16"   ,0xf2900a10, 2 },
  [ORC_NEON_OP_CONVUWL]   = { "vmovl.u16"   ,0xf3900a10, 2 },
  [ORC_NEON_OP_CONVSLQ]   = { "vmovl.s32"   ,0xf2a00a10, 1 },
  [ORC_NEON_OP_CONVULQ]   = { "vmovl.u32"   ,0xf3a00a10, 1 },
  [ORC_NEON_OP_CONVWB]    = { "vmovn.i16"   ,0xf3b20200, 3 },
  [ORC_NEON_OP_CONVSSSWB] = { "vqmovn.s16"  ,0xf3b20280, 3 },
  [ORC_NEON_OP_CONVSUSWB] = { "vqmovun.s16" ,0xf3b20240, 3 },
  [ORC_NEON_OP_CONVUUSWB] = { "vqmovn.u16"  ,0xf3b202c0, 3 },
  [ORC_NEON_OP_CONVLW]    = { "vmovn.i32"   ,0xf3b60200, 2 },
  [ORC_NEON_OP_CONVQL]    = { "vmovn.i64"   ,0xf3ba0200, 1 },
  [ORC_NEON_OP_CONVSSSLW] = { "vqmovn.s32"  ,0xf3b60280, 2 },
  [ORC_NEON_OP_CONVSUSLW] = { "vqmovun.s32" ,0xf3b60240, 2 },
  [ORC_NEON_OP_CONVUUSLW] = { "vqmovn.u32"  ,0xf3b602c0, 2 },
  [ORC_NEON_OP_CONVSSSQL] = { "vqmovn.s64"  ,0xf3ba0280, 1 },
  [ORC_NEON_OP_CONVSUSQL] = { "vqmovun.s64" ,0xf3ba0240, 1 },
  [ORC_NEON_OP_CONVUUSQL] = { "vqmovn.u64"  ,0xf3ba02c0, 1 },
  [ORC_NEON_OP_MULSBW]    = { "vmull.s8"    ,0xf2800c00, 3 },
  [ORC_NEON_OP_MULUBW]    = { "vmull.u8"    ,0xf3800c00, 3 },
  [ORC_NEON_OP_MULSWL]    = { "vmull.s16"   ,0xf2900c00, 2 },
  [ORC_NEON_OP_MULUWL]    = { "vmull.u16"   ,0xf3900c00, 2 },
  [ORC_NEON_OP_SWAPW]     = { "vrev16.i8"   ,0xf3b00100, 2 },
  [ORC_NEON_OP_SWAPL]     = { "vrev32.i8"   ,0xf3b00080, 1 },
  [ORC_NEON_OP_SWAPQ]     = { "vrev64.i8"   ,0xf3b00000, 0 },
  [ORC_NEON_OP_SWAPWL]    = { "vrev32.i16"  ,0xf3b40080, 1 },
  [ORC_NEON_OP_SWAPLQ]    = { "vrev64.i32"  ,0xf3b80000, 0 },
  [ORC_NEON_OP_SELECT0QL] = { "vmovn.i64"   ,0xf3ba0200, 1 },
  [ORC_NEON_OP_SELECT0LW] = { "vmovn.i32"   ,0xf3b60200, 2 },
  [ORC_NEON_OP_SELECT0WB] = { "vmovn.i16"   ,0xf3b20200, 3 },
  [ORC_NEON_OP_ADDF]      = { "vadd.f32"    ,0xf2000d00, 1 },
  [ORC_NEON_OP_SUBF]      = { "vsub.f32"    ,0xf2200d00, 1 },
  [ORC_NEON_OP_MULF]      = { "vmul.f32"    ,0xf3000d10, 1 },
  [ORC_NEON_OP_MAXF]      = { "vmax.f32"    ,0xf2000f00, 1 },
  [ORC_NEON_OP_MINF]      = { "vmin.f32"    ,0xf2200f00, 1 },
  [ORC_NEON_OP_CMPEQF]    = { "vceq.f32"    ,0xf2000e00, 1 },
/*[ORC_NEON_OP_CMPLTF]    = { "vclt.f32"    ,0xf3200e00, 1 },*/
/*[ORC_NEON_OP_CMPLEF]    = { "vcle.f32"    ,0xf3000e00, 1 },*/
  [ORC_NEON_OP_CONVFL]    = { "vcvt.s32.f32",0xf3bb0700, 1 },
  [ORC_NEON_OP_CONVLF]    = { "vcvt.f32.s32",0xf3bb0600, 1 },
  [ORC_NEON_OP_ADDD]      = { "vadd.f64"    ,0xee300b00, 0 },
  [ORC_NEON_OP_SUBD]      = { "vsub.f64"    ,0xee300b40, 0 },
  [ORC_NEON_OP_MULD]      = { "vmul.f64"    ,0xee200b00, 0 },
  [ORC_NEON_OP_DIVD]      = { "vdiv.f64"    ,0xee800b00, 0 },
  [ORC_NEON_OP_DIVF]      = { "vdiv.f32"    ,0xee800a00, 0 },
  [ORC_NEON_OP_SQRTD]     = { "vsqrt.f64"   ,0xeeb10b00, 0 },
  [ORC_NEON_OP_SQRTF]     = { "vsqrt.f32"   ,0xeeb10ac0, 0 },
/*[ORC_NEON_OP_CMPEQD]    = { "vcmpe.f64"   ,0xee000000, 0 },*/
  [ORC_NEON_OP_CONVDF]    = { "vcvt.f64.f32",0xee200b00, 0 },
  [ORC_NEON_OP_CONVFD]    = { "vcvt.f32.f64",0xee200b00, 0 },
};

const char *orc_neon32_reg_name (int reg)
{
  static const char *vec_regs[] = {
    "d0", "d1", "d2", "d3",
    "d4", "d5", "d6", "d7",
    "d8", "d9", "d10", "d11",
    "d12", "d13", "d14", "d15",
    "d16", "d17", "d18", "d19",
    "d20", "d21", "d22", "d23",
    "d24", "d25", "d26", "d27",
    "d28", "d29", "d30", "d31",
  };

  if (reg < ORC_VEC_REG_BASE || reg >= ORC_VEC_REG_BASE+32) {
    return "ERROR";
  }

  return vec_regs[reg&0x1f];
}

const char *orc_neon32_reg_name_quad (int reg)
{
  static const char *vec_regs[] = {
    "q0", "ERROR", "q1", "ERROR",
    "q2", "ERROR", "q3", "ERROR",
    "q4", "ERROR", "q5", "ERROR",
    "q6", "ERROR", "q7", "ERROR",
    "q8", "ERROR", "q9", "ERROR",
    "q10", "ERROR", "q11", "ERROR",
    "q12", "ERROR", "q13", "ERROR",
    "q14", "ERROR", "q15", "ERROR",
  };

  if (reg < ORC_VEC_REG_BASE || reg >= ORC_VEC_REG_BASE+32) {
    return "ERROR";
  }

  return vec_regs[reg&0x1f];
}

void
orc_neon32_emit_binary (OrcCompiler *p, const char *name, unsigned int code,
    int dest, int src1, int src2)
{
  ORC_ASSERT((code & 0x004ff0af) == 0);

  ORC_ASM_CODE(p,"  %s %s, %s, %s\n", name,
      orc_neon32_reg_name (dest), orc_neon32_reg_name (src1),
      orc_neon32_reg_name (src2));
  code |= (dest&0xf)<<12;
  code |= ((dest>>4)&0x1)<<22;
  code |= (src1&0xf)<<16;
  code |= ((src1>>4)&0x1)<<7;
  code |= (src2&0xf)<<0;
  code |= ((src2>>4)&0x1)<<5;
  orc_arm_emit (p, code);
}

#define ORC_NEON32_BINARY(code,a,b,c) \
  ((code) | \
   (((a)&0xf)<<12) | \
   ((((a)>>4)&0x1)<<22) | \
   (((b)&0xf)<<16) | \
   ((((b)>>4)&0x1)<<7) | \
   (((c)&0xf)<<0) | \
   ((((c)>>4)&0x1)<<5))

void
orc_neon32_emit_binary_long (OrcCompiler *p, const char *name, unsigned int code,
    int dest, int src1, int src2)
{
  ORC_ASSERT((code & 0x004ff0af) == 0);

  ORC_ASM_CODE(p,"  %s %s, %s, %s\n", name,
      orc_neon32_reg_name_quad (dest), orc_neon32_reg_name (src1),
      orc_neon32_reg_name (src2));
  code |= (dest&0xf)<<12;
  code |= ((dest>>4)&0x1)<<22;
  code |= (src1&0xf)<<16;
  code |= ((src1>>4)&0x1)<<7;
  code |= (src2&0xf)<<0;
  code |= ((src2>>4)&0x1)<<5;
  orc_arm_emit (p, code);
}

void
orc_neon32_emit_binary_quad (OrcCompiler *p, const char *name, unsigned int code,
    int dest, int src1, int src2)
{
  ORC_ASSERT((code & 0x004ff0af) == 0);

  ORC_ASM_CODE(p,"  %s %s, %s, %s\n", name,
      orc_neon32_reg_name_quad (dest), orc_neon32_reg_name_quad (src1),
      orc_neon32_reg_name_quad (src2));
  code |= (dest&0xf)<<12;
  code |= ((dest>>4)&0x1)<<22;
  code |= (src1&0xf)<<16;
  code |= ((src1>>4)&0x1)<<7;
  code |= (src2&0xf)<<0;
  code |= ((src2>>4)&0x1)<<5;
  code |= 0x40;
  orc_arm_emit (p, code);
}

void
orc_neon32_emit_unary (OrcCompiler *p, const char *name, unsigned int code,
    int dest, int src1)
{
  ORC_ASSERT((code & 0x0040f02f) == 0);

  ORC_ASM_CODE(p,"  %s %s, %s\n", name,
      orc_neon32_reg_name (dest), orc_neon32_reg_name (src1));
  code |= (dest&0xf)<<12;
  code |= ((dest>>4)&0x1)<<22;
  code |= (src1&0xf)<<0;
  code |= ((src1>>4)&0x1)<<5;
  orc_arm_emit (p, code);
}

void
orc_neon32_emit_unary_long (OrcCompiler *p, const char *name, unsigned int code,
    int dest, int src1)
{
  ORC_ASSERT((code & 0x0040f02f) == 0);

  ORC_ASM_CODE(p,"  %s %s, %s\n", name,
      orc_neon32_reg_name_quad (dest), orc_neon32_reg_name (src1));
  code |= (dest&0xf)<<12;
  code |= ((dest>>4)&0x1)<<22;
  code |= (src1&0xf)<<0;
  code |= ((src1>>4)&0x1)<<5;
  orc_arm_emit (p, code);
}

void
orc_neon32_emit_unary_narrow (OrcCompiler *p, const char *name, unsigned int code,
    int dest, int src1)
{
  ORC_ASSERT((code & 0x0040f02f) == 0);

  ORC_ASM_CODE(p,"  %s %s, %s\n", name,
      orc_neon32_reg_name (dest), orc_neon32_reg_name_quad (src1));
  code |= (dest&0xf)<<12;
  code |= ((dest>>4)&0x1)<<22;
  code |= (src1&0xf)<<0;
  code |= ((src1>>4)&0x1)<<5;
  orc_arm_emit (p, code);
}

void
orc_neon32_emit_unary_quad (OrcCompiler *p, const char *name, unsigned int code,
    int dest, int src1)
{
  ORC_ASSERT((code & 0x0040f02f) == 0);

  ORC_ASM_CODE(p,"  %s %s, %s\n", name,
      orc_neon32_reg_name_quad (dest), orc_neon32_reg_name_quad (src1));
  code |= (dest&0xf)<<12;
  code |= ((dest>>4)&0x1)<<22;
  code |= (src1&0xf)<<0;
  code |= ((src1>>4)&0x1)<<5;
  code |= 0x40;
  orc_arm_emit (p, code);
}

void
orc_neon32_emit_mov (OrcCompiler *compiler, OrcVariable dest, OrcVariable src)
{
  orc_neon32_emit_binary (compiler, "vorr", 0xf2200110,
      dest.alloc, src.alloc, src.alloc);
}

void
orc_neon32_emit_mov_quad (OrcCompiler *compiler, OrcVariable dest, OrcVariable src)
{
  orc_neon32_emit_binary_quad (compiler, "vorr", 0xf2200110,
      dest.alloc, src.alloc, src.alloc);
}

void
orc_neon32_unary_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn * const i = (const OrcNeonInsn *) user;

  if (p->insn_shift > i->vec_shift + 1) {
    ORC_COMPILER_ERROR(p, "shift too large");
    return;
  }

  int is_quad = p->insn_shift == (i->vec_shift + 1);
  int dest = p->vars[insn->dest_args[0]].alloc;
  int src1 = p->vars[insn->src_args[0]].alloc;

  if (is_quad)
    orc_neon32_emit_unary_quad (p, i->name, i->code, dest, src1);
  else
    orc_neon32_emit_unary (p, i->name, i->code, dest, src1);
}

void
orc_neon32_unary_long_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn * const i = (const OrcNeonInsn *) user;

  if (p->insn_shift > i->vec_shift) {
    ORC_COMPILER_ERROR(p, "shift too large");
    return;
  }

  int dest = p->vars[insn->dest_args[0]].alloc;
  int src1 = p->vars[insn->src_args[0]].alloc;
  orc_neon32_emit_unary_long (p, i->name, i->code, dest, src1);
}

void
orc_neon32_unary_narrow_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn * const i = (const OrcNeonInsn *) user;

  if (p->insn_shift > i->vec_shift) {
    ORC_COMPILER_ERROR(p, "shift too large");
    return;
  }

  int dest = p->vars[insn->dest_args[0]].alloc;
  int src1 = p->vars[insn->src_args[0]].alloc;
  orc_neon32_emit_unary_narrow (p, i->name, i->code, dest, src1);
}

void
orc_neon32_binary_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn * const i = (const OrcNeonInsn *) user;

  if (p->insn_shift > i->vec_shift + 1) {
    ORC_COMPILER_ERROR(p, "shift too large");
    return;
  }

  int is_quad = p->insn_shift == (i->vec_shift + 1);
  int dest = p->vars[insn->dest_args[0]].alloc;
  int src1 = p->vars[insn->src_args[0]].alloc;
  int src2 = p->vars[insn->src_args[1]].alloc;

  if (is_quad)
    orc_neon32_emit_binary_quad (p, i->name, i->code, dest, src1, src2);
  else
    orc_neon32_emit_binary (p, i->name, i->code, dest, src1, src2);
}

void
orc_neon32_binary_long_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn * const i = (const OrcNeonInsn *) user;

  if (p->insn_shift > i->vec_shift) {
    ORC_COMPILER_ERROR(p, "shift too large");
    return;
  }

  int dest = p->vars[insn->dest_args[0]].alloc;
  int src1 = p->vars[insn->src_args[0]].alloc;
  int src2 = p->vars[insn->src_args[1]].alloc;
  orc_neon32_emit_binary_long (p, i->name, i->code, dest, src1, src2);
}

void
orc_neon32_move_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn * const i = (const OrcNeonInsn *) user;

  if (p->insn_shift > i->vec_shift + 1) {
    ORC_COMPILER_ERROR(p, "shift too large");
    return;
  }

  int is_quad = p->insn_shift == (i->vec_shift + 1);
  int dest = p->vars[insn->dest_args[0]].alloc;
  int src1 = p->vars[insn->src_args[0]].alloc;

  if (dest == src1)
    return;

  if (is_quad)
    orc_neon32_emit_binary_quad (p, i->name, i->code, dest, src1, src1);
  else
    orc_neon32_emit_binary (p, i->name, i->code, dest, src1, src1);
}

void
orc_neon32_unary_vfp_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn * const i = (const OrcNeonInsn *) user;

  int is_quad = p->insn_shift == (i->vec_shift + 1);
  int dest = p->vars[insn->dest_args[0]].alloc;
  int src1 = p->vars[insn->src_args[0]].alloc;

  // FIXME?: Is this logic right?
  orc_neon32_emit_unary (p, i->name, i->code, dest, src1);
  if (is_quad)
    orc_neon32_emit_unary (p, i->name, i->code, dest + 1, src1 + 1);
  else
      ORC_COMPILER_ERROR(p, "shift too large");
}

void
orc_neon32_binary_vfp_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn * const i = (const OrcNeonInsn *) user;

  int is_quad = p->insn_shift == (i->vec_shift + 1);
  int dest = p->vars[insn->dest_args[0]].alloc;
  int src1 = p->vars[insn->src_args[0]].alloc;
  int src2 = p->vars[insn->src_args[1]].alloc;

  // FIXME?: Is this logic right?
  orc_neon32_emit_binary (p, i->name, i->code, dest, src1, src2);
  if (is_quad)
    orc_neon32_emit_binary (p, i->name, i->code, dest + 1, src1 + 1, src2 + 1);
  else if (p->insn_shift > i->vec_shift + 1)
    ORC_COMPILER_ERROR(p, "shift too large");
}

void
orc_neon32_rule_loadupdb (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  unsigned int code = 0;
  int size = src->size << compiler->insn_shift;
  ORC_ASSERT(src->ptr_register);	/* can ptr_register be 0 ? */
  int ptr_reg;

  /* FIXME this should be fixed at a higher level */
  if (src->vartype != ORC_VAR_TYPE_SRC && src->vartype != ORC_VAR_TYPE_DEST) {
    ORC_COMPILER_ERROR(compiler, "loadX used with non src/dest");
    return;
  }

  if (src->ptr_offset) {
    ptr_reg = compiler->gp_tmpreg;
    orc_arm_emit_add_rsi(compiler, ORC_ARM_COND_AL, 0,
                       ptr_reg, src->ptr_register,
                       src->ptr_offset, ORC_ARM_LSR, 1);
  } else {
    ptr_reg = src->ptr_register;
  }
  if (size >= 8) {
    if (src->is_aligned) {
      if (size == 32) {
        ORC_ASM_CODE(compiler,"  vld1.64 { %s, %s, %s, %s }, [%s,:256]\n",
            orc_neon32_reg_name (dest->alloc),
            orc_neon32_reg_name (dest->alloc + 1),
            orc_neon32_reg_name (dest->alloc + 2),
            orc_neon32_reg_name (dest->alloc + 3),
            orc_arm_reg_name (ptr_reg));
        code = 0xf42002dd;
      } else if (size == 16) {
        ORC_ASM_CODE(compiler,"  vld1.64 { %s, %s }, [%s,:128]\n",
            orc_neon32_reg_name (dest->alloc),
            orc_neon32_reg_name (dest->alloc + 1),
            orc_arm_reg_name (ptr_reg));
        code = 0xf4200aed;
      } else if (size == 8) {
        ORC_ASM_CODE(compiler,"  vld1.64 %s, [%s]\n",
            orc_neon32_reg_name (dest->alloc),
            orc_arm_reg_name (ptr_reg));
        code = 0xf42007cd;
      } else {
        ORC_COMPILER_ERROR(compiler,"bad aligned load size %d",
            src->size << compiler->insn_shift);
      }
    } else {
      if (size == 32) {
        ORC_ASM_CODE(compiler,"  vld1.8 { %s, %s, %s, %s }, [%s]\n",
            orc_neon32_reg_name (dest->alloc),
            orc_neon32_reg_name (dest->alloc + 1),
            orc_neon32_reg_name (dest->alloc + 2),
            orc_neon32_reg_name (dest->alloc + 3),
            orc_arm_reg_name (ptr_reg));
        code = 0xf420020d;
      } else if (size == 16) {
        ORC_ASM_CODE(compiler,"  vld1.8 { %s, %s }, [%s]\n",
            orc_neon32_reg_name (dest->alloc),
            orc_neon32_reg_name (dest->alloc + 1),
            orc_arm_reg_name (ptr_reg));
        code = 0xf4200a0d;
      } else if (size == 8) {
        ORC_ASM_CODE(compiler,"  vld1.8 %s, [%s]\n",
            orc_neon32_reg_name (dest->alloc),
            orc_arm_reg_name (ptr_reg));
        code = 0xf420070d;
      } else {
        ORC_COMPILER_ERROR(compiler,"bad unaligned load size %d",
            src->size << compiler->insn_shift);
      }
    }
  } else {
    int shift;
    if (size == 4) {
      shift = 2;
    } else if (size == 2) {
      shift = 1;
    } else {
      shift = 0;
    }
    ORC_ASM_CODE(compiler,"  vld1.%d %s[0], [%s]\n",
        8<<shift,
        orc_neon32_reg_name (dest->alloc),
        orc_arm_reg_name (ptr_reg));
    code = 0xf4a0000d;
    code |= shift<<10;
    code |= (0&7)<<5;
  }
  code |= (ptr_reg&0xf) << 16;
  code |= (dest->alloc&0xf) << 12;
  code |= ((dest->alloc>>4)&0x1) << 22;
  code |= 1 << 1;
  orc_arm_emit (compiler, code);

  switch (src->size) {
    case 1:
      orc_neon32_emit_binary (compiler, "vorr", 0xf2200110,
        compiler->vars[insn->dest_args[0]].alloc + 1,
        compiler->vars[insn->dest_args[0]].alloc,
        compiler->vars[insn->dest_args[0]].alloc);
      orc_neon32_emit_unary (compiler, "vzip.8", 0xf3b20180,
        compiler->vars[insn->dest_args[0]].alloc,
        compiler->vars[insn->dest_args[0]].alloc + 1);

      if (compiler->loop_shift == 1) {
        /* When the loop_shift is 1, it is possible that one iteration of shift 0
        has already been performed if the destination array is 8-byte aligned
        (but not 16-byte aligned).
        In this case, the output offset has been incremented by 1, and we need to
        shift the outputs of loadupdb.*/

        // set temp reg to 0
        orc_arm_emit_eor_r(compiler, ORC_ARM_COND_AL, 0,
          compiler->gp_tmpreg, compiler->gp_tmpreg, compiler->gp_tmpreg);
        // test if input offset is odd
        orc_arm_emit_tst_i(compiler, ORC_ARM_COND_AL, src->ptr_offset, 0x1);
        // if yes, tmpreg = 0xff
        orc_arm_emit_mov_i(compiler, ORC_ARM_COND_NE, 0, compiler->gp_tmpreg, 0xff);

        // fill a simd reg with value of tmpreg (0xff or 0x0)
        ORC_ASM_CODE(compiler,"  %s %s, %s\n", "vdup.8",
          orc_neon32_reg_name (dest->alloc+3), orc_arm_reg_name (compiler->gp_tmpreg));
        code = 0xeec00b10;
        code |= ((compiler->vars[insn->dest_args[0]].alloc+3)&0xf)<<16; // Vd
        code |= (compiler->gp_tmpreg&0xf) << 12; // Rt
        code |= (((compiler->vars[insn->dest_args[0]].alloc+3)>>4)&0x1) << 7; // D
        orc_arm_emit (compiler, code);

        // vext.8 with #imm=1 to create shifted output
        orc_neon32_emit_binary (compiler, "vext.8", 0xf2b00100,
          compiler->vars[insn->dest_args[0]].alloc+1,
          compiler->vars[insn->dest_args[0]].alloc,
          compiler->vars[insn->dest_args[0]].alloc+1);

        // select shifted output or not
        orc_neon32_emit_binary(compiler, "vbit.8", 0xf3200110,
          compiler->vars[insn->dest_args[0]].alloc,
          compiler->vars[insn->dest_args[0]].alloc+1,
          compiler->vars[insn->dest_args[0]].alloc+3);
      }
      break;
    case 2:
      orc_neon32_emit_binary (compiler, "vorr", 0xf2200110,
        compiler->vars[insn->dest_args[0]].alloc + 1,
        compiler->vars[insn->dest_args[0]].alloc,
        compiler->vars[insn->dest_args[0]].alloc);
      orc_neon32_emit_unary (compiler, "vzip.16", 0xf3b60180,
        compiler->vars[insn->dest_args[0]].alloc,
        compiler->vars[insn->dest_args[0]].alloc + 1);
      break;
    case 4:
      orc_neon32_emit_binary (compiler, "vorr", 0xf2200110,
        compiler->vars[insn->dest_args[0]].alloc + 1,
        compiler->vars[insn->dest_args[0]].alloc,
        compiler->vars[insn->dest_args[0]].alloc);
      orc_neon32_emit_unary_quad (compiler, "vzip.32", 0xf3ba0180,
        compiler->vars[insn->dest_args[0]].alloc,
        compiler->vars[insn->dest_args[0]].alloc + 1);
      break;
  }

  src->update_type = 1;
}

void
orc_neon32_rule_loadpX (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int size = ORC_PTR_TO_INT (user);

  if (src->vartype == ORC_VAR_TYPE_CONST) {
    if (size == 1) {
      orc_neon32_emit_loadib (compiler, dest, src->value.i);
    } else if (size == 2) {
      orc_neon32_emit_loadiw (compiler, dest, src->value.i);
    } else if (size == 4) {
      orc_neon32_emit_loadil (compiler, dest, src->value.i);
    } else if (size == 8) {
      if (src->size == 8) {
        ORC_COMPILER_ERROR(compiler,"64-bit constants not implemented");
      }
      orc_neon32_emit_loadiq (compiler, dest, src->value.i);
    } else {
      ORC_PROGRAM_ERROR(compiler,"unimplemented");
    }
  } else {
    if (size == 1) {
      orc_neon32_emit_loadpb (compiler, dest->alloc, insn->src_args[0]);
    } else if (size == 2) {
      orc_neon32_emit_loadpw (compiler, dest->alloc, insn->src_args[0]);
    } else if (size == 4) {
      orc_neon32_emit_loadpl (compiler, dest->alloc, insn->src_args[0]);
    } else if (size == 8) {
      orc_neon32_emit_loadpq (compiler, dest->alloc, insn->src_args[0]);
    } else {
      ORC_PROGRAM_ERROR(compiler,"unimplemented");
    }
  }
}

void
orc_neon32_rule_loadX (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int update = FALSE;
  unsigned int code = 0;
  int size = src->size << compiler->insn_shift;
  int type = ORC_PTR_TO_INT(user);
  int ptr_register;
  int is_aligned = src->is_aligned;

  /* FIXME this should be fixed at a higher level */
  if (src->vartype != ORC_VAR_TYPE_SRC && src->vartype != ORC_VAR_TYPE_DEST) {
    ORC_COMPILER_ERROR(compiler, "loadX used with non src/dest");
    return;
  }

  if (src->vartype == ORC_VAR_TYPE_DEST) update = FALSE;

  if (type == 1) {
    OrcVariable *src2 = compiler->vars + insn->src_args[1];

    if (src2->vartype != ORC_VAR_TYPE_CONST) {
      ORC_PROGRAM_ERROR(compiler,"unimplemented");
      return;
    }

    ptr_register = compiler->gp_tmpreg;
    if (src2->value.i < 0) {
      orc_arm_emit_sub_imm (compiler, ptr_register,
          src->ptr_register,
          src2->value.i * src->size * -1, TRUE);
    }
    else
    {
      orc_arm_emit_add_imm (compiler, ptr_register,
          src->ptr_register,
          src2->value.i * src->size);
    }

    update = FALSE;
    is_aligned = FALSE;
  } else {
    ptr_register = src->ptr_register;
  }

  if (size >= 8) {
    if (is_aligned) {
      if (size == 32) {
        ORC_ASM_CODE(compiler,"  vld1.64 { %s, %s, %s, %s }, [%s,:256]%s\n",
            orc_neon32_reg_name (dest->alloc),
            orc_neon32_reg_name (dest->alloc + 1),
            orc_neon32_reg_name (dest->alloc + 2),
            orc_neon32_reg_name (dest->alloc + 3),
            orc_arm_reg_name (ptr_register),
            update ? "!" : "");
        code = 0xf42002dd;
      } else if (size == 16) {
        ORC_ASM_CODE(compiler,"  vld1.64 { %s, %s }, [%s,:128]%s\n",
            orc_neon32_reg_name (dest->alloc),
            orc_neon32_reg_name (dest->alloc + 1),
            orc_arm_reg_name (ptr_register),
            update ? "!" : "");
        code = 0xf4200aed;
      } else if (size == 8) {
        ORC_ASM_CODE(compiler,"  vld1.64 %s, [%s]%s\n",
            orc_neon32_reg_name (dest->alloc),
            orc_arm_reg_name (ptr_register),
            update ? "!" : "");
        code = 0xf42007cd;
      } else {
        ORC_COMPILER_ERROR(compiler,"bad aligned load size %d",
            src->size << compiler->insn_shift);
      }
    } else {
      if (size == 32) {
        ORC_ASM_CODE(compiler,"  vld1.8 { %s, %s, %s, %s }, [%s]%s\n",
            orc_neon32_reg_name (dest->alloc),
            orc_neon32_reg_name (dest->alloc + 1),
            orc_neon32_reg_name (dest->alloc + 2),
            orc_neon32_reg_name (dest->alloc + 3),
            orc_arm_reg_name (ptr_register),
            update ? "!" : "");
        code = 0xf420020d;
      } else if (size == 16) {
        ORC_ASM_CODE(compiler,"  vld1.8 { %s, %s }, [%s]%s\n",
            orc_neon32_reg_name (dest->alloc),
            orc_neon32_reg_name (dest->alloc + 1),
            orc_arm_reg_name (ptr_register),
            update ? "!" : "");
        code = 0xf4200a0d;
      } else if (size == 8) {
        ORC_ASM_CODE(compiler,"  vld1.8 %s, [%s]%s\n",
            orc_neon32_reg_name (dest->alloc),
            orc_arm_reg_name (ptr_register),
            update ? "!" : "");
        code = 0xf420070d;
      } else {
        ORC_COMPILER_ERROR(compiler,"bad unaligned load size %d",
            src->size << compiler->insn_shift);
      }
    }
  } else {
    int shift;
    if (size == 4) {
      shift = 2;
    } else if (size == 2) {
      shift = 1;
    } else {
      shift = 0;
    }
    ORC_ASM_CODE(compiler,"  vld1.%d %s[0], [%s]%s\n",
        8<<shift,
        orc_neon32_reg_name (dest->alloc),
        orc_arm_reg_name (ptr_register),
        update ? "!" : "");
    code = 0xf4a0000d;
    code |= shift<<10;
    code |= (0&7)<<5;
  }
  code |= (ptr_register&0xf) << 16;
  code |= (dest->alloc&0xf) << 12;
  code |= ((dest->alloc>>4)&0x1) << 22;
  code |= (!update) << 1;
  orc_arm_emit (compiler, code);
}

void
orc_neon32_rule_storeX (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int update = FALSE;
  unsigned int code = 0;
  int size = dest->size << compiler->insn_shift;

  if (size >= 8) {
    if (dest->is_aligned) {
      if (size == 32) {
        ORC_ASM_CODE(compiler,"  vst1.64 { %s, %s, %s, %s }, [%s,:256]%s\n",
            orc_neon32_reg_name (src->alloc),
            orc_neon32_reg_name (src->alloc + 1),
            orc_neon32_reg_name (src->alloc + 2),
            orc_neon32_reg_name (src->alloc + 3),
            orc_arm_reg_name (dest->ptr_register),
            update ? "!" : "");
        code = 0xf40002dd;
      } else if (size == 16) {
        ORC_ASM_CODE(compiler,"  vst1.64 { %s, %s }, [%s,:128]%s\n",
            orc_neon32_reg_name (src->alloc),
            orc_neon32_reg_name (src->alloc + 1),
            orc_arm_reg_name (dest->ptr_register),
            update ? "!" : "");
        code = 0xf4000aed;
      } else if (size == 8) {
        ORC_ASM_CODE(compiler,"  vst1.64 %s, [%s]%s\n",
            orc_neon32_reg_name (src->alloc),
            orc_arm_reg_name (dest->ptr_register),
            update ? "!" : "");
        code = 0xf40007cd;
      } else {
        ORC_COMPILER_ERROR(compiler,"bad aligned store size %d", size);
      }
    } else {
      if (size == 32) {
        ORC_ASM_CODE(compiler,"  vst1.8 { %s, %s, %s, %s }, [%s]%s\n",
            orc_neon32_reg_name (src->alloc),
            orc_neon32_reg_name (src->alloc + 1),
            orc_neon32_reg_name (src->alloc + 2),
            orc_neon32_reg_name (src->alloc + 3),
            orc_arm_reg_name (dest->ptr_register),
            update ? "!" : "");
        code = 0xf400020d;
      } else if (size == 16) {
        ORC_ASM_CODE(compiler,"  vst1.8 { %s, %s }, [%s]%s\n",
            orc_neon32_reg_name (src->alloc),
            orc_neon32_reg_name (src->alloc + 1),
            orc_arm_reg_name (dest->ptr_register),
            update ? "!" : "");
        code = 0xf4000a0d;
      } else if (size == 8) {
        ORC_ASM_CODE(compiler,"  vst1.8 %s, [%s]%s\n",
            orc_neon32_reg_name (src->alloc),
            orc_arm_reg_name (dest->ptr_register),
            update ? "!" : "");
        code = 0xf400070d;
      } else {
        ORC_COMPILER_ERROR(compiler,"bad aligned store size %d", size);
      }
    }
  } else {
    int shift;
    if (size == 4) {
      shift = 2;
    } else if (size == 2) {
      shift = 1;
    } else {
      shift = 0;
    }
    ORC_ASM_CODE(compiler,"  vst1.%d %s[0], [%s]%s\n",
        8<<shift,
        orc_neon32_reg_name (src->alloc),
        orc_arm_reg_name (dest->ptr_register),
        update ? "!" : "");
    code = 0xf480000d;
    code |= shift<<10;
    code |= (0&7)<<5;
  }
  code |= (dest->ptr_register&0xf) << 16;
  code |= (src->alloc&0xf) << 12;
  code |= ((src->alloc>>4)&0x1) << 22;
  code |= (!update) << 1;
  orc_arm_emit (compiler, code);
}

static void
orc_neon32_emit_loadib (OrcCompiler *compiler, OrcVariable *dest, int value)
{
  int reg = dest->alloc;
  orc_uint32 code;

  if (value == 0) {
    orc_neon32_emit_binary_quad (compiler, "veor", 0xf3000110, reg, reg, reg);
    return;
  }

  value &= 0xff;
  ORC_ASM_CODE(compiler,"  vmov.i8 %s, #%d\n",
      orc_neon32_reg_name_quad (reg), value);
  code = 0xf2800e10;
  code |= (reg&0xf) << 12;
  code |= ((reg>>4)&0x1) << 22;
  code |= (value&0xf) << 0;
  code |= (value&0x70) << 12;
  code |= (value&0x80) << 17;
  code |= 0x40;
  orc_arm_emit (compiler, code);
}

static void
orc_neon32_emit_loadiw (OrcCompiler *compiler, OrcVariable *dest, int value)
{
  int reg = dest->alloc;
  orc_uint32 code;

  if (value == 0) {
    orc_neon32_emit_binary_quad (compiler, "veor", 0xf3000110, reg, reg, reg);
    return;
  }

  ORC_ASM_CODE(compiler,"  vmov.i16 %s, #0x%04x\n",
      orc_neon32_reg_name_quad (reg), value & 0xff);
  code = 0xf2800810;
  code |= (reg&0xf) << 12;
  code |= ((reg>>4)&0x1) << 22;
  code |= (value&0xf) << 0;
  code |= (value&0x70) << 12;
  code |= (value&0x80) << 17;
  code |= 0x40;
  orc_arm_emit (compiler, code);

  value >>= 8;
  if (value) {
    ORC_ASM_CODE(compiler,"  vorr.i16 %s, #0x%04x\n",
        orc_neon32_reg_name_quad (reg), (value & 0xff) << 8);
    code = 0xf2800b10;

    code |= (reg&0xf) << 12;
    code |= ((reg>>4)&0x1) << 22;
    code |= (value&0xf) << 0;
    code |= (value&0x70) << 12;
    code |= (value&0x80) << 17;
    code |= 0x40;
    orc_arm_emit (compiler, code);
  }
}

void
orc_neon32_emit_loadil (OrcCompiler *compiler, OrcVariable *dest, int value)
{
  int reg = dest->alloc;
  orc_uint32 code;

  if (value == 0) {
    orc_neon32_emit_binary_quad (compiler, "veor", 0xf3000110, reg, reg, reg);
    return;
  }

  ORC_ASM_CODE(compiler,"  vmov.i32 %s, #0x%08x\n",
      orc_neon32_reg_name_quad (reg), value & 0xff);
  code = 0xf2800010;

  code |= (reg&0xf) << 12;
  code |= ((reg>>4)&0x1) << 22;
  code |= (value&0xf) << 0;
  code |= (value&0x70) << 12;
  code |= (value&0x80) << 17;
  code |= 0x40;
  orc_arm_emit (compiler, code);

  value >>= 8;
  if (value & 0xff) {
    ORC_ASM_CODE(compiler,"  vorr.i32 %s, #0x%08x\n",
        orc_neon32_reg_name_quad (reg), (value & 0xff) << 8);
    code = 0xf2800310;

    code |= (reg&0xf) << 12;
    code |= ((reg>>4)&0x1) << 22;
    code |= (value&0xf) << 0;
    code |= (value&0x70) << 12;
    code |= (value&0x80) << 17;
    code |= 0x40;
    orc_arm_emit (compiler, code);
  }
  value >>= 8;
  if (value & 0xff) {
    ORC_ASM_CODE(compiler,"  vorr.i32 %s, #0x%08x\n",
        orc_neon32_reg_name_quad (reg), (value & 0xff) << 16);
    code = 0xf2800510;

    code |= (reg&0xf) << 12;
    code |= ((reg>>4)&0x1) << 22;
    code |= (value&0xf) << 0;
    code |= (value&0x70) << 12;
    code |= (value&0x80) << 17;
    code |= 0x40;
    orc_arm_emit (compiler, code);
  }
  value >>= 8;
  if (value & 0xff) {
    ORC_ASM_CODE(compiler,"  vorr.i32 %s, #0x%08x\n",
        orc_neon32_reg_name_quad (reg), (value & 0xff) << 24);
    code = 0xf2800710;

    code |= (reg&0xf) << 12;
    code |= ((reg>>4)&0x1) << 22;
    code |= (value&0xf) << 0;
    code |= (value&0x70) << 12;
    code |= (value&0x80) << 17;
    code |= 0x40;
    orc_arm_emit (compiler, code);
  }
}

static void
orc_neon32_emit_loadiq (OrcCompiler *compiler, OrcVariable *dest, long long value)
{
  int reg = dest->alloc;
  /* orc_uint32 code; */
  /* int shift; */
  /* int neg = FALSE; */

  if (value == 0) {
    orc_neon32_emit_binary_quad (compiler, "veor", 0xf3000110, reg, reg, reg);
    return;
  }

  if (value < 0) {
    /* neg = TRUE; */
    value = ~value;
  }
#if 0
  shift = orc_neon_get_const_shift (value);
  if ((value & (0xff<<shift)) == value) {
    value >>= shift;
    if (neg) {
      ORC_ASM_CODE(compiler,"  vmvn.i64 %s, #%d\n",
          orc_neon32_reg_name_quad (reg), value);
      code = 0xf2800030;
    } else {
      ORC_ASM_CODE(compiler,"  vmov.i64 %s, #%d\n",
          orc_neon32_reg_name_quad (reg), value);
      code = 0xf2800010;
    }
    code |= (reg&0xf) << 12;
    code |= ((reg>>4)&0x1) << 22;
    code |= (value&0xf) << 0;
    code |= (value&0x70) << 12;
    code |= (value&0x80) << 17;
    code |= 0x40;
    orc_arm_emit (compiler, code);

    if (shift > 0) {
      ORC_ASM_CODE(compiler,"  vshl.i64 %s, %s, #%d\n",
          orc_neon32_reg_name_quad (reg), orc_neon32_reg_name_quad (reg), shift);
      code = 0xf2a00510;
      code |= (reg&0xf) << 12;
      code |= ((reg>>4)&0x1) << 22;
      code |= (reg&0xf) << 0;
      code |= ((reg>>4)&0x1) << 5;
      code |= (shift&0xf) << 16;
      code |= 0x40;
      orc_arm_emit (compiler, code);
    }

    return;
  }
#endif
  ORC_COMPILER_ERROR(compiler, "unimplemented load of constant %d", value);
}

static void
orc_neon32_emit_loadpb (OrcCompiler *compiler, int dest, int param)
{
  orc_uint32 code;

  orc_arm_emit_add_imm (compiler, compiler->gp_tmpreg,
      compiler->exec_reg, ORC_STRUCT_OFFSET(OrcExecutor, params[param]));

  ORC_ASM_CODE(compiler,"  vld1.8 {%s[],%s[]}, [%s]\n",
      orc_neon32_reg_name (dest), orc_neon32_reg_name (dest+1),
      orc_arm_reg_name (compiler->gp_tmpreg));
  code = 0xf4a00c2f;
  code |= (compiler->gp_tmpreg&0xf) << 16;
  code |= (dest&0xf) << 12;
  code |= ((dest>>4)&0x1) << 22;
  orc_arm_emit (compiler, code);
}

static void
orc_neon32_emit_loadpw (OrcCompiler *compiler, int dest, int param)
{
  orc_uint32 code;

  orc_arm_emit_add_imm (compiler, compiler->gp_tmpreg,
      compiler->exec_reg, ORC_STRUCT_OFFSET(OrcExecutor, params[param]));

  ORC_ASM_CODE(compiler,"  vld1.16 {%s[],%s[]}, [%s]\n",
      orc_neon32_reg_name (dest), orc_neon32_reg_name (dest+1),
      orc_arm_reg_name (compiler->gp_tmpreg));
  code = 0xf4a00c6f;
  code |= (compiler->gp_tmpreg&0xf) << 16;
  code |= (dest&0xf) << 12;
  code |= ((dest>>4)&0x1) << 22;
  orc_arm_emit (compiler, code);
}

static void
orc_neon32_emit_loadpl (OrcCompiler *compiler, int dest, int param)
{
  orc_uint32 code;

  orc_arm_emit_add_imm (compiler, compiler->gp_tmpreg,
      compiler->exec_reg, ORC_STRUCT_OFFSET(OrcExecutor, params[param]));

  ORC_ASM_CODE(compiler,"  vld1.32 {%s[],%s[]}, [%s]\n",
      orc_neon32_reg_name (dest), orc_neon32_reg_name (dest+1),
      orc_arm_reg_name (compiler->gp_tmpreg));
  code = 0xf4a00caf;
  code |= (compiler->gp_tmpreg&0xf) << 16;
  code |= (dest&0xf) << 12;
  code |= ((dest>>4)&0x1) << 22;
  orc_arm_emit (compiler, code);
}

static void
orc_neon32_emit_loadpq (OrcCompiler *compiler, int dest, int param)
{
  orc_uint32 code;
  int update = FALSE;

  orc_arm_emit_add_imm (compiler, compiler->gp_tmpreg,
      compiler->exec_reg, ORC_STRUCT_OFFSET(OrcExecutor, params[param]));

  ORC_ASM_CODE(compiler,"  vld1.32 %s[0], [%s]%s\n",
      orc_neon32_reg_name (dest),
      orc_arm_reg_name (compiler->gp_tmpreg),
      update ? "!" : "");
  code = 0xf4a0000d;
  code |= 2<<10;
  code |= (0)<<7;
  code |= (compiler->gp_tmpreg&0xf) << 16;
  code |= (dest&0xf) << 12;
  code |= ((dest>>4)&0x1) << 22;
  code |= (!update) << 1;
  orc_arm_emit (compiler, code);

  ORC_ASM_CODE(compiler,"  vld1.32 %s[0], [%s]%s\n",
      orc_neon32_reg_name (dest+1),
      orc_arm_reg_name (compiler->gp_tmpreg),
      update ? "!" : "");
  code = 0xf4a0000d;
  code |= 2<<10;
  code |= (0)<<7;
  code |= (compiler->gp_tmpreg&0xf) << 16;
  code |= ((dest+1)&0xf) << 12;
  code |= (((dest+1)>>4)&0x1) << 22;
  code |= (!update) << 1;
  orc_arm_emit (compiler, code);

  orc_arm_emit_add_imm (compiler, compiler->gp_tmpreg,
      compiler->exec_reg, ORC_STRUCT_OFFSET(OrcExecutor,
        params[param + (ORC_N_PARAMS)]));

  ORC_ASM_CODE(compiler,"  vld1.32 %s[1], [%s]%s\n",
      orc_neon32_reg_name (dest),
      orc_arm_reg_name (compiler->gp_tmpreg),
      update ? "!" : "");
  code = 0xf4a0000d;
  code |= 2<<10;
  code |= (1)<<7;
  code |= (compiler->gp_tmpreg&0xf) << 16;
  code |= (dest&0xf) << 12;
  code |= ((dest>>4)&0x1) << 22;
  code |= (!update) << 1;
  orc_arm_emit (compiler, code);

  ORC_ASM_CODE(compiler,"  vld1.32 %s[1], [%s]%s\n",
      orc_neon32_reg_name (dest+1),
      orc_arm_reg_name (compiler->gp_tmpreg),
      update ? "!" : "");
  code = 0xf4a0000d;
  code |= 2<<10;
  code |= (1)<<7;
  code |= (compiler->gp_tmpreg&0xf) << 16;
  code |= ((dest+1)&0xf) << 12;
  code |= (((dest+1)>>4)&0x1) << 22;
  code |= (!update) << 1;
  orc_arm_emit (compiler, code);
}

static const ShiftInfo immshift_info_32[] = {
  { 0xf2880510, "vshl.i8", FALSE, 8, 3 }, /* shlb */
  { 0xf2880010, "vshr.s8", TRUE, 8, 3 }, /* shrsb */
  { 0xf3880010, "vshr.u8", TRUE, 8, 3 }, /* shrub */
  { 0xf2900510, "vshl.i16", FALSE, 16, 2 },
  { 0xf2900010, "vshr.s16", TRUE, 16, 2 },
  { 0xf3900010, "vshr.u16", TRUE, 16, 2 },
  { 0xf2a00510, "vshl.i32", FALSE, 32, 1 },
  { 0xf2a00010, "vshr.s32", TRUE, 32, 1 },
  { 0xf3a00010, "vshr.u32", TRUE, 32, 1 }
};

static const ShiftInfo regshift_info_32[] = {
  { 0xf3000400, "vshl.u8", FALSE, 0, 3 }, /* shlb */
  { 0xf2000400, "vshl.s8", TRUE, 0, 3 }, /* shrsb */
  { 0xf3000400, "vshl.u8", TRUE, 0, 3 }, /* shrub */
  { 0xf3100400, "vshl.u16", FALSE, 0, 2 },
  { 0xf2100400, "vshl.s16", TRUE, 0, 2 },
  { 0xf3100400, "vshl.u16", TRUE, 0, 2 },
  { 0xf3200400, "vshl.u32", FALSE, 0, 1 },
  { 0xf2200400, "vshl.s32", TRUE, 0, 1 },
  { 0xf3200400, "vshl.u32", TRUE, 0, 1 }
};

void
orc_neon32_emit_shift (OrcCompiler *const p, int type,
                    const OrcVariable *const dest,
                    const OrcVariable *const src, int shift,
                    int is_quad)
{
  orc_uint32 code = 0;
  if (shift < 0) {
    ORC_COMPILER_ERROR(p, "shift negative");
    return;
  }
  if (shift >= immshift_info_32[type].bits) {
    ORC_COMPILER_ERROR(p, "shift too large");
    return;
  }

  code = immshift_info_32[type].code;
  if (is_quad == 0) {
    ORC_ASM_CODE(p, "  %s %s, %s, #%d\n", immshift_info_32[type].name,
                 orc_neon32_reg_name(dest->alloc),
                 orc_neon32_reg_name(src->alloc), shift);
  } else {
    ORC_ASM_CODE(p, "  %s %s, %s, #%d\n", immshift_info_32[type].name,
                 orc_neon32_reg_name_quad(dest->alloc),
                 orc_neon32_reg_name_quad(src->alloc), shift);
    code |= 0x40;
  }
  code |= (dest->alloc & 0xf) << 12;
  code |= ((dest->alloc >> 4) & 0x1) << 22;
  code |= (src->alloc & 0xf) << 0;
  code |= ((src->alloc >> 4) & 0x1) << 5;

  if (immshift_info_32[type].negate) {
    shift = immshift_info_32[type].bits - shift;
  }
  code |= shift << 16;
  orc_arm_emit(p, code);
}

void
orc_neon32_rule_shift (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int type = ORC_PTR_TO_INT(user);
  orc_uint32 code;

  if (p->vars[insn->src_args[1]].vartype == ORC_VAR_TYPE_CONST) {
    orc_neon32_emit_shift(p, type, p->vars + insn->dest_args[0],
                        p->vars + insn->src_args[0],
                        (int)p->vars[insn->src_args[1]].value.i,
                        p->insn_shift > immshift_info_32[type].vec_shift);
  } else if (p->vars[insn->src_args[1]].vartype == ORC_VAR_TYPE_PARAM) {
    orc_neon32_emit_loadpb (p, p->tmpreg, insn->src_args[1]);
    if (regshift_info_32[type].negate) {
      orc_neon32_emit_unary_quad (p, "vneg.s8", 0xf3b10380, p->tmpreg, p->tmpreg);
    }

    code = regshift_info_32[type].code;
    if (p->insn_shift <= regshift_info_32[type].vec_shift) {
      ORC_ASM_CODE(p,"  %s %s, %s, %s\n",
          regshift_info_32[type].name,
          orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc),
          orc_neon32_reg_name (p->vars[insn->src_args[0]].alloc),
          orc_neon32_reg_name (p->tmpreg));
    } else {
      ORC_ASM_CODE(p,"  %s %s, %s, %s\n",
          regshift_info_32[type].name,
          orc_neon32_reg_name_quad (p->vars[insn->dest_args[0]].alloc),
          orc_neon32_reg_name_quad (p->vars[insn->src_args[0]].alloc),
          orc_neon32_reg_name_quad (p->tmpreg));
      code |= 0x40;
    }
    code |= (p->vars[insn->dest_args[0]].alloc&0xf)<<12;
    code |= ((p->vars[insn->dest_args[0]].alloc>>4)&0x1)<<22;
    code |= (p->vars[insn->src_args[0]].alloc&0xf)<<0;
    code |= ((p->vars[insn->src_args[0]].alloc>>4)&0x1)<<5;
    code |= (p->tmpreg&0xf)<<16;
    code |= ((p->tmpreg>>4)&0x1)<<7;
    orc_arm_emit (p, code);
  } else {
    ORC_PROGRAM_ERROR(p,"shift rule only works with constants and params");
  }
}

void
orc_neon32_rule_andn (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int max_shift = ORC_PTR_TO_INT(user);

  /* this is special because the operand order is reversed */
  if (p->insn_shift <= max_shift) {
    orc_neon32_emit_binary (p, "vbic", 0xf2100110,
        p->vars[insn->dest_args[0]].alloc,
        p->vars[insn->src_args[1]].alloc,
        p->vars[insn->src_args[0]].alloc);
  } else {
    orc_neon32_emit_binary_quad (p, "vbic", 0xf2100110,
        p->vars[insn->dest_args[0]].alloc,
        p->vars[insn->src_args[1]].alloc,
        p->vars[insn->src_args[0]].alloc);
  }
}

void
orc_neon32_rule_accw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  unsigned int code;

  if (p->insn_shift < 2) {
    ORC_ASM_CODE(p,"  vshl.i64 %s, %s, #%d\n",
        orc_neon32_reg_name (p->tmpreg),
        orc_neon32_reg_name (p->vars[insn->src_args[0]].alloc), 48);
    code = ORC_NEON32_BINARY(0xf2a00590, p->tmpreg, 0,
        p->vars[insn->src_args[0]].alloc);
    code |= (48) << 16;
    orc_arm_emit (p, code);

    orc_neon32_emit_binary (p, "vadd.i16", 0xf2100800,
        p->vars[insn->dest_args[0]].alloc,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg);
  } else {
    orc_neon32_emit_binary (p, "vadd.i16", 0xf2100800,
        p->vars[insn->dest_args[0]].alloc,
        p->vars[insn->dest_args[0]].alloc,
        p->vars[insn->src_args[0]].alloc);
  }
}

void
orc_neon32_rule_accl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  unsigned int code;

  if (p->insn_shift < 1) {
    ORC_ASM_CODE(p,"  vshl.i64 %s, %s, #%d\n",
        orc_neon32_reg_name (p->tmpreg),
        orc_neon32_reg_name (p->vars[insn->src_args[0]].alloc), 32);
    code = ORC_NEON32_BINARY(0xf2a00590, p->tmpreg, 0,
        p->vars[insn->src_args[0]].alloc);
    code |= (32) << 16;
    orc_arm_emit (p, code);

    orc_neon32_emit_binary (p, "vadd.i32", 0xf2200800,
        p->vars[insn->dest_args[0]].alloc,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg);
  } else {
    orc_neon32_emit_binary (p, "vadd.i32", 0xf2200800,
        p->vars[insn->dest_args[0]].alloc,
        p->vars[insn->dest_args[0]].alloc,
        p->vars[insn->src_args[0]].alloc);
  }
}

void
orc_neon32_rule_select1wb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  unsigned int code;

  ORC_ASM_CODE(p,"  vshrn.i16 %s, %s, #%d\n",
      orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc),
      orc_neon32_reg_name_quad (p->vars[insn->src_args[0]].alloc), 8);
  code = ORC_NEON32_BINARY (0xf2880810,
      p->vars[insn->dest_args[0]].alloc,
      0, p->vars[insn->src_args[0]].alloc);
  orc_arm_emit (p, code);
}

void
orc_neon32_rule_select1lw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  unsigned int code;

  ORC_ASM_CODE(p,"  vshrn.i32 %s, %s, #%d\n",
      orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc),
      orc_neon32_reg_name_quad (p->vars[insn->src_args[0]].alloc), 16);
  code = ORC_NEON32_BINARY (0xf2900810,
      p->vars[insn->dest_args[0]].alloc,
      0, p->vars[insn->src_args[0]].alloc);
  orc_arm_emit (p, code);
}

void
orc_neon32_rule_select1ql (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  unsigned int code;

  ORC_ASM_CODE(p,"  vtrn.32 %s, %s\n",
      orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc),
      orc_neon32_reg_name_quad (p->vars[insn->src_args[0]].alloc));
  code = ORC_NEON32_BINARY (0xf2a00810,
      p->vars[insn->dest_args[0]].alloc,
      0, p->vars[insn->src_args[0]].alloc);
  orc_arm_emit (p, code);
}

void
orc_neon32_rule_convhwb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  unsigned int code;

  ORC_ASM_CODE(p,"  vshrn.i16 %s, %s, #%d\n",
      orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc),
      orc_neon32_reg_name_quad (p->vars[insn->src_args[0]].alloc), 8);
  code = ORC_NEON32_BINARY (0xf2880810,
      p->vars[insn->dest_args[0]].alloc,
      0, p->vars[insn->src_args[0]].alloc);
  orc_arm_emit (p, code);
}

void
orc_neon32_rule_convhlw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  unsigned int code;

  ORC_ASM_CODE(p,"  vshrn.i32 %s, %s, #%d\n",
      orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc),
      orc_neon32_reg_name_quad (p->vars[insn->src_args[0]].alloc), 16);
  code = ORC_NEON32_BINARY (0xf2900810,
      p->vars[insn->dest_args[0]].alloc,
      0, p->vars[insn->src_args[0]].alloc);
  orc_arm_emit (p, code);
}

void
orc_neon32_rule_mergebw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->src_args[1]].size };

  if (p->insn_shift <= 2) {
    if (p->vars[insn->dest_args[0]].alloc != p->vars[insn->src_args[0]].alloc) {
      orc_neon32_emit_mov (p, p->vars[insn->dest_args[0]],
          p->vars[insn->src_args[0]]);
    }

    if (p->vars[insn->src_args[1]].last_use != p->insn_index ||
        p->vars[insn->src_args[1]].alloc == p->vars[insn->dest_args[0]].alloc) {
      orc_neon32_emit_mov (p, tmpreg, p->vars[insn->src_args[1]]);
      orc_neon32_emit_unary (p, "vzip.8", 0xf3b20180,
          p->vars[insn->dest_args[0]].alloc,
          p->tmpreg);
    } else {
      orc_neon32_emit_unary (p, "vzip.8", 0xf3b20180,
          p->vars[insn->dest_args[0]].alloc,
          p->vars[insn->src_args[1]].alloc);
    }
  } else {
    if (p->vars[insn->dest_args[0]].alloc != p->vars[insn->src_args[0]].alloc) {
      orc_neon32_emit_mov_quad (p, p->vars[insn->dest_args[0]],
          p->vars[insn->src_args[0]]);
    }

    orc_neon32_emit_mov_quad (p, tmpreg, p->vars[insn->src_args[1]]);
    orc_neon32_emit_unary_quad (p, "vzip.8", 0xf3b20180,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg);
  }
}

void
orc_neon32_rule_mergewl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->src_args[1]].size };

  if (p->insn_shift <= 1) {
    if (p->vars[insn->dest_args[0]].alloc != p->vars[insn->src_args[0]].alloc) {
      orc_neon32_emit_mov (p, p->vars[insn->dest_args[0]],
          p->vars[insn->src_args[0]]);
    }

    if (p->vars[insn->src_args[1]].last_use != p->insn_index ||
        p->vars[insn->src_args[1]].alloc == p->vars[insn->dest_args[0]].alloc) {
      orc_neon32_emit_mov (p, tmpreg, p->vars[insn->src_args[1]]);
      orc_neon32_emit_unary (p, "vzip.16", 0xf3b60180,
          p->vars[insn->dest_args[0]].alloc,
          p->tmpreg);
    } else {
      orc_neon32_emit_unary (p, "vzip.16", 0xf3b60180,
          p->vars[insn->dest_args[0]].alloc,
          p->vars[insn->src_args[1]].alloc);
    }
  } else {
    if (p->vars[insn->dest_args[0]].alloc != p->vars[insn->src_args[0]].alloc) {
      orc_neon32_emit_mov_quad (p, p->vars[insn->dest_args[0]],
          p->vars[insn->src_args[0]]);
    }

    if (p->vars[insn->src_args[1]].last_use != p->insn_index ||
        p->vars[insn->src_args[1]].alloc == p->vars[insn->dest_args[0]].alloc) {
      orc_neon32_emit_mov_quad (p, tmpreg, p->vars[insn->src_args[1]]);
      orc_neon32_emit_unary_quad (p, "vzip.16", 0xf3b60180,
          p->vars[insn->dest_args[0]].alloc,
          p->tmpreg);
    } else {
      orc_neon32_emit_unary_quad (p, "vzip.16", 0xf3b60180,
          p->vars[insn->dest_args[0]].alloc,
          p->vars[insn->src_args[1]].alloc);
    }
  }
}

void
orc_neon32_rule_mergelq (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->src_args[1]].size };

  if (p->insn_shift <= 0) {
    if (p->vars[insn->dest_args[0]].alloc != p->vars[insn->src_args[0]].alloc) {
      orc_neon32_emit_mov (p, p->vars[insn->dest_args[0]],
          p->vars[insn->src_args[0]]);
    }

    if (p->vars[insn->src_args[1]].last_use != p->insn_index ||
        p->vars[insn->src_args[1]].alloc == p->vars[insn->dest_args[0]].alloc) {
      orc_neon32_emit_mov (p, tmpreg, p->vars[insn->src_args[1]]);
      orc_neon32_emit_unary (p, "vtrn.32", 0xf3ba0080,
          p->vars[insn->dest_args[0]].alloc,
          p->tmpreg);
    } else {
      orc_neon32_emit_unary (p, "vtrn.32", 0xf3ba0080,
          p->vars[insn->dest_args[0]].alloc,
          p->vars[insn->src_args[1]].alloc);
    }
  } else {
    if (p->vars[insn->dest_args[0]].alloc != p->vars[insn->src_args[0]].alloc) {
      orc_neon32_emit_mov_quad (p, p->vars[insn->dest_args[0]],
          p->vars[insn->src_args[0]]);
    }

    if (p->vars[insn->src_args[1]].last_use != p->insn_index ||
        p->vars[insn->src_args[1]].alloc == p->vars[insn->dest_args[0]].alloc) {
      orc_neon32_emit_mov_quad (p, tmpreg, p->vars[insn->src_args[1]]);
      orc_neon32_emit_unary_quad (p, "vzip.32", 0xf3ba0180,
          p->vars[insn->dest_args[0]].alloc,
          p->tmpreg);
    } else {
      orc_neon32_emit_unary_quad (p, "vzip.32", 0xf3ba0180,
          p->vars[insn->dest_args[0]].alloc,
          p->vars[insn->src_args[1]].alloc);
    }
  }
}

void
orc_neon32_rule_splatbw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->dest_args[0]].size };

  if (p->insn_shift <= 2) {
    if (p->vars[insn->dest_args[0]].alloc != p->vars[insn->src_args[0]].alloc) {
      orc_neon32_emit_mov (p, p->vars[insn->dest_args[0]],
          p->vars[insn->src_args[0]]);
    }

    orc_neon32_emit_mov (p, tmpreg, p->vars[insn->dest_args[0]]);
    orc_neon32_emit_unary (p, "vzip.8", 0xf3b20180,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg);
  } else {
    if (p->vars[insn->dest_args[0]].alloc != p->vars[insn->src_args[0]].alloc) {
      orc_neon32_emit_mov_quad (p, p->vars[insn->dest_args[0]],
          p->vars[insn->src_args[0]]);
    }

    orc_neon32_emit_mov_quad (p, tmpreg, p->vars[insn->dest_args[0]]);
    orc_neon32_emit_unary_quad (p, "vzip.8", 0xf3b20180,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg);
  }
}

void
orc_neon32_rule_splatbl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->dest_args[0]].size };

  if (p->insn_shift <= 1) {
    if (p->vars[insn->dest_args[0]].alloc != p->vars[insn->src_args[0]].alloc) {
      orc_neon32_emit_mov (p, p->vars[insn->dest_args[0]],
          p->vars[insn->src_args[0]]);
    }

    orc_neon32_emit_mov (p, tmpreg, p->vars[insn->dest_args[0]]);
    orc_neon32_emit_unary (p, "vzip.8", 0xf3b20180,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg);
    orc_neon32_emit_mov (p, tmpreg, p->vars[insn->dest_args[0]]);
    orc_neon32_emit_unary (p, "vzip.16", 0xf3b60180,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg);
  } else {
    if (p->vars[insn->dest_args[0]].alloc != p->vars[insn->src_args[0]].alloc) {
      orc_neon32_emit_mov_quad (p, p->vars[insn->dest_args[0]],
          p->vars[insn->src_args[0]]);
    }

    orc_neon32_emit_mov (p, tmpreg, p->vars[insn->dest_args[0]]);
    orc_neon32_emit_unary_quad (p, "vzip.8", 0xf3b20180,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg);
    orc_neon32_emit_mov (p, tmpreg, p->vars[insn->dest_args[0]]);
    orc_neon32_emit_unary_quad (p, "vzip.16", 0xf3b60180,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg);
  }
}

void
orc_neon32_rule_splatw3q (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  orc_uint32 code;
  int offset = 0;
  int label = 20;

  orc_arm_add_fixup (p, label, 1);
  ORC_ASM_CODE(p,"  vldr %s, .L%d+%d\n",
      orc_neon32_reg_name (p->tmpreg), label, offset);
  code = 0xed9f0b00;
  code |= (p->tmpreg&0xf) << 12;
  code |= ((p->tmpreg>>4)&0x1) << 22;
  code |= ((offset - 8) >> 2)&0xff;
  orc_arm_emit (p, code);

  ORC_ASM_CODE(p,"  vtbl.8 %s, { %s, %s }, %s\n",
      orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc),
      orc_neon32_reg_name (p->vars[insn->src_args[0]].alloc),
      orc_neon32_reg_name (p->vars[insn->src_args[0]].alloc + 1),
      orc_neon32_reg_name (p->tmpreg));
  code = ORC_NEON32_BINARY(0xf3b00900,
      p->vars[insn->dest_args[0]].alloc,
      p->vars[insn->src_args[0]].alloc,
      p->tmpreg);
  orc_arm_emit (p, code);

  if (p->insn_shift > 0) {
    ORC_ASM_CODE(p,"  vtbl.8 %s, { %s }, %s\n",
        orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc+1),
        orc_neon32_reg_name (p->vars[insn->src_args[0]].alloc+1),
        orc_neon32_reg_name (p->tmpreg));
    code = ORC_NEON32_BINARY(0xf3b00800,
        p->vars[insn->dest_args[0]].alloc+1,
        p->vars[insn->src_args[0]].alloc+1,
        p->tmpreg);
    orc_arm_emit (p, code);
  }
}

void
orc_neon32_rule_accsadubl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  orc_uint32 x;
  unsigned int code;

  if (p->insn_shift < 2) {
    x = 0xf3800700;
    ORC_ASM_CODE(p,"  vabdl.u8 %s, %s, %s\n",
        orc_neon32_reg_name_quad (p->tmpreg),
        orc_neon32_reg_name (p->vars[insn->src_args[0]].alloc),
        orc_neon32_reg_name (p->vars[insn->src_args[1]].alloc));
    x |= (p->tmpreg&0xf)<<12;
    x |= ((p->tmpreg>>4)&0x1)<<22;
    x |= (p->vars[insn->src_args[0]].alloc&0xf)<<16;
    x |= ((p->vars[insn->src_args[0]].alloc>>4)&0x1)<<7;
    x |= (p->vars[insn->src_args[1]].alloc&0xf)<<0;
    x |= ((p->vars[insn->src_args[1]].alloc>>4)&0x1)<<5;
    orc_arm_emit (p, x);

    ORC_ASM_CODE(p,"  vshl.i64 %s, %s, #%d\n",
        orc_neon32_reg_name (p->tmpreg),
        orc_neon32_reg_name (p->tmpreg), 64 - (16<<p->insn_shift));
    code = ORC_NEON32_BINARY(0xf2a00590, p->tmpreg, 0, p->tmpreg);
    code |= (64 - (16<<p->insn_shift)) << 16;
    orc_arm_emit (p, code);

    orc_neon32_emit_unary (p, "vpadal.u16", 0xf3b40680,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg);
  } else {
    x = 0xf3800700;
    ORC_ASM_CODE(p,"  vabdl.u8 %s, %s, %s\n",
        orc_neon32_reg_name_quad (p->tmpreg),
        orc_neon32_reg_name (p->vars[insn->src_args[0]].alloc),
        orc_neon32_reg_name (p->vars[insn->src_args[1]].alloc));
    x |= (p->tmpreg&0xf)<<12;
    x |= ((p->tmpreg>>4)&0x1)<<22;
    x |= (p->vars[insn->src_args[0]].alloc&0xf)<<16;
    x |= ((p->vars[insn->src_args[0]].alloc>>4)&0x1)<<7;
    x |= (p->vars[insn->src_args[1]].alloc&0xf)<<0;
    x |= ((p->vars[insn->src_args[1]].alloc>>4)&0x1)<<5;
    orc_arm_emit (p, x);

    orc_neon32_emit_unary (p, "vpadal.u16", 0xf3b40680,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg);
  }
}

void
orc_neon32_rule_signw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->src_args[0]].size };

  /* slow */

  orc_neon32_emit_loadiw (p, &tmpreg, 1);
  if (p->insn_shift < 3) {
    orc_neon32_emit_binary (p, "vmin.s16", 0xf2100610,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg,
        p->vars[insn->src_args[0]].alloc);
  } else {
    orc_neon32_emit_binary_quad (p, "vmin.s16", 0xf2100610,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg,
        p->vars[insn->src_args[0]].alloc);
  }
  orc_neon32_emit_loadiw (p, &tmpreg, -1);
  if (p->insn_shift < 3) {
    orc_neon32_emit_binary (p, "vmax.s16", 0xf2100600,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg,
        p->vars[insn->dest_args[0]].alloc);
  } else {
    orc_neon32_emit_binary_quad (p, "vmax.s16", 0xf2100600,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg,
        p->vars[insn->dest_args[0]].alloc);
  }
}

void
orc_neon32_rule_signb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->src_args[0]].size };

  /* slow */

  orc_neon32_emit_loadib (p, &tmpreg, 1);
  if (p->insn_shift < 4) {
    orc_neon32_emit_binary (p, "vmin.s8", 0xf2000610,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg,
        p->vars[insn->src_args[0]].alloc);
  } else {
    orc_neon32_emit_binary_quad (p, "vmin.s8", 0xf2000610,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg,
        p->vars[insn->src_args[0]].alloc);
  }
  orc_neon32_emit_loadib (p, &tmpreg, -1);
  if (p->insn_shift < 4) {
    orc_neon32_emit_binary (p, "vmax.s8", 0xf2000600,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg,
        p->vars[insn->dest_args[0]].alloc);
  } else {
    orc_neon32_emit_binary_quad (p, "vmax.s8", 0xf2000600,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg,
        p->vars[insn->dest_args[0]].alloc);
  }
}

void
orc_neon32_rule_signl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->src_args[0]].size };

  /* slow */

  orc_neon32_emit_loadil (p, &tmpreg, 1);
  if (p->insn_shift < 2) {
    orc_neon32_emit_binary (p, "vmin.s32", 0xf2200610,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg,
        p->vars[insn->src_args[0]].alloc);
  } else {
    orc_neon32_emit_binary_quad (p, "vmin.s32", 0xf2200610,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg,
        p->vars[insn->src_args[0]].alloc);
  }
  orc_neon32_emit_loadil (p, &tmpreg, -1);
  if (p->insn_shift < 2) {
    orc_neon32_emit_binary (p, "vmax.s32", 0xf2200600,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg,
        p->vars[insn->dest_args[0]].alloc);
  } else {
    orc_neon32_emit_binary_quad (p, "vmax.s32", 0xf2200600,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg,
        p->vars[insn->dest_args[0]].alloc);
  }
}

void
orc_neon32_rule_mulhub (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  unsigned int code;

  orc_neon32_emit_binary_long (p, "vmull.u8",0xf3800c00,
      p->tmpreg,
      p->vars[insn->src_args[0]].alloc,
      p->vars[insn->src_args[1]].alloc);

  ORC_ASM_CODE(p,"  vshrn.i16 %s, %s, #%d\n",
      orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc),
      orc_neon32_reg_name_quad (p->tmpreg), 8);
  code = ORC_NEON32_BINARY (0xf2880810,
      p->vars[insn->dest_args[0]].alloc,
      p->tmpreg, 0);
  orc_arm_emit (p, code);

  if (p->insn_shift == 4) {
    orc_neon32_emit_binary_long (p, "vmull.u8",0xf3800c00,
        p->tmpreg,
        p->vars[insn->src_args[0]].alloc + 1,
        p->vars[insn->src_args[1]].alloc + 1);
    ORC_ASM_CODE(p,"  vshrn.i16 %s, %s, #%d\n",
        orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc + 1),
        orc_neon32_reg_name_quad (p->tmpreg), 8);
    code = ORC_NEON32_BINARY (0xf2880810,
        p->vars[insn->dest_args[0]].alloc + 1,
        p->tmpreg, 0);
    orc_arm_emit (p, code);
  }
}

void
orc_neon32_rule_mulhsb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  unsigned int code;

  orc_neon32_emit_binary_long (p, "vmull.s8",0xf2800c00,
      p->tmpreg,
      p->vars[insn->src_args[0]].alloc,
      p->vars[insn->src_args[1]].alloc);
  ORC_ASM_CODE(p,"  vshrn.i16 %s, %s, #%d\n",
      orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc),
      orc_neon32_reg_name_quad (p->tmpreg), 8);
  code = ORC_NEON32_BINARY (0xf2880810,
      p->vars[insn->dest_args[0]].alloc,
      p->tmpreg, 0);
  orc_arm_emit (p, code);

  if (p->insn_shift == 4) {
    orc_neon32_emit_binary_long (p, "vmull.s8",0xf2800c00,
        p->tmpreg,
        p->vars[insn->src_args[0]].alloc + 1,
        p->vars[insn->src_args[1]].alloc + 1);
    ORC_ASM_CODE(p,"  vshrn.i16 %s, %s, #%d\n",
        orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc + 1),
        orc_neon32_reg_name_quad (p->tmpreg), 8);
    code = ORC_NEON32_BINARY (0xf2880810,
        p->vars[insn->dest_args[0]].alloc + 1,
        p->tmpreg, 0);
    orc_arm_emit (p, code);
  }
}

void
orc_neon32_rule_mulhuw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  unsigned int code;

  orc_neon32_emit_binary_long (p, "vmull.u16",0xf3900c00,
      p->tmpreg,
      p->vars[insn->src_args[0]].alloc,
      p->vars[insn->src_args[1]].alloc);
  ORC_ASM_CODE(p,"  vshrn.i32 %s, %s, #%d\n",
      orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc),
      orc_neon32_reg_name_quad (p->tmpreg), 16);
  code = ORC_NEON32_BINARY (0xf2900810,
      p->vars[insn->dest_args[0]].alloc,
      p->tmpreg, 0);
  orc_arm_emit (p, code);

  if (p->insn_shift == 3) {
    orc_neon32_emit_binary_long (p, "vmull.u16",0xf3900c00,
        p->tmpreg,
        p->vars[insn->src_args[0]].alloc + 1,
        p->vars[insn->src_args[1]].alloc + 1);
    ORC_ASM_CODE(p,"  vshrn.i32 %s, %s, #%d\n",
        orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc + 1),
        orc_neon32_reg_name_quad (p->tmpreg), 16);
    code = ORC_NEON32_BINARY (0xf2900810,
        p->vars[insn->dest_args[0]].alloc + 1,
        p->tmpreg, 0);
    orc_arm_emit (p, code);
  }
}

void
orc_neon32_rule_mulhsw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  unsigned int code;

  orc_neon32_emit_binary_long (p, "vmull.s16",0xf2900c00,
      p->tmpreg,
      p->vars[insn->src_args[0]].alloc,
      p->vars[insn->src_args[1]].alloc);
  ORC_ASM_CODE(p,"  vshrn.i32 %s, %s, #%d\n",
      orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc),
      orc_neon32_reg_name_quad (p->tmpreg), 16);
  code = ORC_NEON32_BINARY (0xf2900810,
      p->vars[insn->dest_args[0]].alloc,
      p->tmpreg, 0);
  orc_arm_emit (p, code);

  if (p->insn_shift == 3) {
    orc_neon32_emit_binary_long (p, "vmull.s16",0xf2900c00,
        p->tmpreg,
        p->vars[insn->src_args[0]].alloc + 1,
        p->vars[insn->src_args[1]].alloc + 1);
    ORC_ASM_CODE(p,"  vshrn.i32 %s, %s, #%d\n",
        orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc + 1),
        orc_neon32_reg_name_quad (p->tmpreg), 16);
    code = ORC_NEON32_BINARY (0xf2900810,
        p->vars[insn->dest_args[0]].alloc + 1,
        p->tmpreg, 0);
    orc_arm_emit (p, code);
  }
}

void
orc_neon32_rule_mulhul (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  unsigned int code;

  orc_neon32_emit_binary_long (p, "vmull.u32",0xf3a00c00,
      p->tmpreg,
      p->vars[insn->src_args[0]].alloc,
      p->vars[insn->src_args[1]].alloc);
  ORC_ASM_CODE(p,"  vshrn.i64 %s, %s, #%d\n",
      orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc),
      orc_neon32_reg_name_quad (p->tmpreg), 32);
  code = ORC_NEON32_BINARY (0xf2a00810,
      p->vars[insn->dest_args[0]].alloc,
      p->tmpreg, 0);
  orc_arm_emit (p, code);

  if (p->insn_shift == 2) {
    orc_neon32_emit_binary_long (p, "vmull.u32",0xf3a00c00,
        p->tmpreg,
        p->vars[insn->src_args[0]].alloc + 1,
        p->vars[insn->src_args[1]].alloc + 1);
    ORC_ASM_CODE(p,"  vshrn.i64 %s, %s, #%d\n",
        orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc + 1),
        orc_neon32_reg_name_quad (p->tmpreg), 32);
    code = ORC_NEON32_BINARY (0xf2a00810,
        p->vars[insn->dest_args[0]].alloc + 1,
        p->tmpreg, 0);
    orc_arm_emit (p, code);
  }
}

void
orc_neon32_rule_mulhsl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  unsigned int code;

  orc_neon32_emit_binary_long (p, "vmull.s32",0xf2a00c00,
      p->tmpreg,
      p->vars[insn->src_args[0]].alloc,
      p->vars[insn->src_args[1]].alloc);
  ORC_ASM_CODE(p,"  vshrn.i64 %s, %s, #%d\n",
      orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc),
      orc_neon32_reg_name_quad (p->tmpreg), 32);
  code = ORC_NEON32_BINARY (0xf2a00810,
      p->vars[insn->dest_args[0]].alloc,
      p->tmpreg, 0);
  orc_arm_emit (p, code);

  if (p->insn_shift == 2) {
    orc_neon32_emit_binary_long (p, "vmull.s32",0xf2a00c00,
        p->tmpreg,
        p->vars[insn->src_args[0]].alloc + 1,
        p->vars[insn->src_args[1]].alloc + 1);
    ORC_ASM_CODE(p,"  vshrn.i64 %s, %s, #%d\n",
        orc_neon32_reg_name (p->vars[insn->dest_args[0]].alloc + 1),
        orc_neon32_reg_name_quad (p->tmpreg), 32);
    code = ORC_NEON32_BINARY (0xf2a00810,
        p->vars[insn->dest_args[0]].alloc + 1,
        p->tmpreg, 0);
    orc_arm_emit (p, code);
  }
}

void
orc_neon32_rule_splitql (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int dest0 = p->vars[insn->dest_args[0]].alloc;
  int dest1 = p->vars[insn->dest_args[1]].alloc;
  int src = p->vars[insn->src_args[0]].alloc;

  if (p->insn_shift < 1) {
    if (src != dest0) {
      orc_neon32_emit_mov (p, p->vars[insn->dest_args[0]], p->vars[insn->src_args[0]]);
    }
    if (src != dest1) {
      orc_neon32_emit_mov (p, p->vars[insn->dest_args[1]], p->vars[insn->src_args[0]]);
    }
    orc_neon32_emit_unary (p, "vtrn.32", 0xf3ba0080, dest1, dest0);
  } else {
    if (src != dest0) {
      orc_neon32_emit_mov_quad (p, p->vars[insn->dest_args[0]], p->vars[insn->src_args[0]]);
    }
    if (src != dest1) {
      orc_neon32_emit_mov_quad (p, p->vars[insn->dest_args[1]], p->vars[insn->src_args[0]]);
    }
    orc_neon32_emit_unary_quad (p, "vuzp.32", 0xf3ba0140, dest1, dest0);
  }
}

void
orc_neon32_rule_splitlw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int dest0 = p->vars[insn->dest_args[0]].alloc;
  int dest1 = p->vars[insn->dest_args[1]].alloc;
  int src = p->vars[insn->src_args[0]].alloc;

  if (p->insn_shift < 2) {
    if (src != dest0) {
      orc_neon32_emit_mov (p, p->vars[insn->dest_args[0]], p->vars[insn->src_args[0]]);
    }
    if (src != dest1) {
      orc_neon32_emit_mov (p, p->vars[insn->dest_args[1]], p->vars[insn->src_args[0]]);
    }
    orc_neon32_emit_unary (p, "vuzp.16", 0xf3b60100, dest1, dest0);
  } else {
    if (src != dest0) {
      orc_neon32_emit_mov_quad (p, p->vars[insn->dest_args[0]], p->vars[insn->src_args[0]]);
    }
    if (src != dest1) {
      orc_neon32_emit_mov_quad (p, p->vars[insn->dest_args[1]], p->vars[insn->src_args[0]]);
    }
    orc_neon32_emit_unary_quad (p, "vuzp.16", 0xf3b60140, dest1, dest0);
  }
}

void
orc_neon32_rule_splitwb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int dest0 = p->vars[insn->dest_args[0]].alloc;
  int dest1 = p->vars[insn->dest_args[1]].alloc;
  int src = p->vars[insn->src_args[0]].alloc;

  if (p->insn_shift < 2) {
    if (src != dest0) {
      orc_neon32_emit_mov (p, p->vars[insn->dest_args[0]], p->vars[insn->src_args[0]]);
    }
    if (src != dest1) {
      orc_neon32_emit_mov (p, p->vars[insn->dest_args[1]], p->vars[insn->src_args[0]]);
    }
    orc_neon32_emit_unary (p, "vuzp.8", 0xf3b20100, dest1, dest0);
  } else {
    if (src != dest0) {
      orc_neon32_emit_mov_quad (p, p->vars[insn->dest_args[0]], p->vars[insn->src_args[0]]);
    }
    if (src != dest1) {
      orc_neon32_emit_mov_quad (p, p->vars[insn->dest_args[1]], p->vars[insn->src_args[0]]);
    }
    orc_neon32_emit_unary_quad (p, "vuzp.8", 0xf3b20140, dest1, dest0);
  }
}

void
orc_neon32_rule_div255w (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcVariable dest = p->vars[insn->dest_args[0]];
  const OrcVariable src = p->vars[insn->src_args[0]];
  OrcVariable tmp1 = { .alloc = p->tmpreg2, .size = src.size * 2 };
  OrcVariable tmp2 = { .alloc = p->tmpreg, .size = src.size };
  orc_neon32_emit_loadiw (p, &tmp2, 0x8081);

  // Multiply low
  orc_neon32_emit_binary_long (p, "vmull.u16", 0xf3900c00, tmp1.alloc, src.alloc,
                            tmp2.alloc);
  // Multiply high
  orc_neon32_emit_binary_long (p, "vmull.u16", 0xf3900c00, dest.alloc,
                            src.alloc + 1, tmp2.alloc);
  orc_neon32_emit_unary_quad (p, "vuzp.16", 0xf3b60100, tmp1.alloc, dest.alloc);
  orc_neon32_emit_shift (p, 5, &dest, &dest, 7, 1);
}
