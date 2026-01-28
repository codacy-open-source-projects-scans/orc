#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>

#include <orc/orcprogram.h>
#include <orc/orcinternal.h>
#include <orc/orcarm.h>
#include <orc/orcarminsn.h>
#include <orc/orcarm64insn.h>
#include <orc/orcdebug.h>
#include <orc/orcutils-private.h>

#include <orc/orcneon.h>
#include <orc/orcneon-private.h>

static void orc_neon64_emit_loadpb (OrcCompiler *compiler, int dest, int param);
static void orc_neon64_emit_loadpw (OrcCompiler *compiler, int dest, int param);
static void orc_neon64_emit_loadpl (OrcCompiler *compiler, int dest, int param);
static void orc_neon64_emit_loadib (OrcCompiler *compiler, OrcVariable *dest, int param);
static void orc_neon64_emit_loadiw (OrcCompiler *compiler, OrcVariable *dest, int param);
static void orc_neon64_emit_loadiq (OrcCompiler *compiler, OrcVariable *dest, long long param);
static void orc_neon64_emit_loadpq (OrcCompiler *compiler, int dest, int param);

const OrcNeonInsn orc_neon64_insns[_ORC_NEON_OP_MAX_] = {
  [ORC_NEON_OP_ABSB]      = { "abs"   ,0x0e20b800, 3 },
  [ORC_NEON_OP_ADDB]      = { "add"   ,0x0e208400, 3 },
  [ORC_NEON_OP_ADDSSB]    = { "sqadd" ,0x0e200c00, 3 },
  [ORC_NEON_OP_ADDUSB]    = { "uqadd" ,0x2e200c00, 3 },
  [ORC_NEON_OP_ANDB]      = { "and"   ,0x0e201c00, 3 },
/*[ORC_NEON_OP_ANDNB]     = { NULL    ,0         , 3 },*/
  [ORC_NEON_OP_AVGSB]     = { "srhadd",0x0e201400, 3 },
  [ORC_NEON_OP_AVGUB]     = { "urhadd",0x2e201400, 3 },
  [ORC_NEON_OP_CMPEQB]    = { "cmeq"  ,0x2e208c00, 3 },
  [ORC_NEON_OP_CMPGTSB]   = { "cmgt"  ,0x0e203400, 3 },
  [ORC_NEON_OP_COPYB]     = { "mov"   ,0x0ea01c00, 3 },
  [ORC_NEON_OP_MAXSB]     = { "smax"  ,0x0e206400, 3 },
  [ORC_NEON_OP_MAXUB]     = { "umax"  ,0x2e206400, 3 },
  [ORC_NEON_OP_MINSB]     = { "smin"  ,0x0e206c00, 3 },
  [ORC_NEON_OP_MINUB]     = { "umin"  ,0x2e206c00, 3 },
  [ORC_NEON_OP_MULLB]     = { "mul"   ,0x0e209c00, 3 },
  [ORC_NEON_OP_ORB]       = { "orr"   ,0x0ea01c00, 3 },
/*[ORC_NEON_OP_SHLB]      = { NULL    ,0         , 3 },*/
/*[ORC_NEON_OP_SHRSB]     = { 8       ,NULL      , 3 },*/
/*[ORC_NEON_OP_SHRUB]     = { 8       ,NULL      , 3 },*/
  [ORC_NEON_OP_SUBB]      = { "sub"   ,0x2e208400, 3 },
  [ORC_NEON_OP_SUBSSB]    = { "sqsub" ,0x0e202c00, 3 },
  [ORC_NEON_OP_SUBUSB]    = { "uqsub" ,0x2e202c00, 3 },
  [ORC_NEON_OP_XORB]      = { "eor"   ,0x2e201c00, 3 },
  [ORC_NEON_OP_ABSW]      = { "abs"   ,0x0e60b800, 2 },
  [ORC_NEON_OP_ADDW]      = { "add"   ,0x0e608400, 2 },
  [ORC_NEON_OP_ADDSSW]    = { "sqadd" ,0x0e600c00, 2 },
  [ORC_NEON_OP_ADDUSW]    = { "uqadd" ,0x2e600c00, 2 },
  [ORC_NEON_OP_ANDW]      = { "and"   ,0x0e201c00, 2 },
/*[ORC_NEON_OP_ANDNW]     = { NULL    ,0         , 2 },*/
  [ORC_NEON_OP_AVGSW]     = { "srhadd",0x0e601400, 2 },
  [ORC_NEON_OP_AVGUW]     = { "urhadd",0x2e601400, 2 },
  [ORC_NEON_OP_CMPEQW]    = { "cmeq"  ,0x2e608c00, 2 },
  [ORC_NEON_OP_CMPGTSW]   = { "cmgt"  ,0x0e603400, 2 },
  [ORC_NEON_OP_COPYW]     = { "mov"   ,0x0ea01c00, 2 },
  [ORC_NEON_OP_MAXSW]     = { "smax"  ,0x0e606400, 2 },
  [ORC_NEON_OP_MAXUW]     = { "umax"  ,0x2e606400, 2 },
  [ORC_NEON_OP_MINSW]     = { "smin"  ,0x0e606c00, 2 },
  [ORC_NEON_OP_MINUW]     = { "umin"  ,0x2e606c00, 2 },
  [ORC_NEON_OP_MULLW]     = { "mul"   ,0x0e609c00, 2 },
  [ORC_NEON_OP_ORW]       = { "orr"   ,0x0ea01c00, 2 },
/*[ORC_NEON_OP_SHLW]      = { NULL    ,0         , 2 },*/
/*[ORC_NEON_OP_SHRSW]     = { 16      ,NULL      , 2 },*/
/*[ORC_NEON_OP_SHRUW]     = { 16      ,NULL      , 2 },*/
  [ORC_NEON_OP_SUBW]      = { "sub"   ,0x2e608400, 2 },
  [ORC_NEON_OP_SUBSSW]    = { "sqsub" ,0x0e602c00, 2 },
  [ORC_NEON_OP_SUBUSW]    = { "uqsub" ,0x2e602c00, 2 },
  [ORC_NEON_OP_XORW]      = { "eor"   ,0x2e201c00, 2 },
  [ORC_NEON_OP_ABSL]      = { "abs"   ,0x0ea0b800, 1 },
  [ORC_NEON_OP_ADDL]      = { "add"   ,0x0ea08400, 1 },
  [ORC_NEON_OP_ADDSSL]    = { "sqadd" ,0x0ea00c00, 1 },
  [ORC_NEON_OP_ADDUSL]    = { "uqadd" ,0x2ea00c00, 1 },
  [ORC_NEON_OP_ANDL]      = { "and"   ,0x0e201c00, 1 },
/*[ORC_NEON_OP_ANDNL]     = { NULL    ,0         , 1 },*/
  [ORC_NEON_OP_AVGSL]     = { "srhadd",0x0ea01400, 1 },
  [ORC_NEON_OP_AVGUL]     = { "urhadd",0x2ea01400, 1 },
  [ORC_NEON_OP_CMPEQL]    = { "cmeq"  ,0x2ea08c00, 1 },
  [ORC_NEON_OP_CMPGTSL]   = { "cmgt"  ,0x0ea03400, 1 },
  [ORC_NEON_OP_COPYL]     = { "mov"   ,0x0ea01c00, 1 },
  [ORC_NEON_OP_MAXSL]     = { "smax"  ,0x0ea06400, 1 },
  [ORC_NEON_OP_MAXUL]     = { "umax"  ,0x2ea06400, 1 },
  [ORC_NEON_OP_MINSL]     = { "smin"  ,0x0ea06c00, 1 },
  [ORC_NEON_OP_MINUL]     = { "umin"  ,0x2ea06c00, 1 },
  [ORC_NEON_OP_MULLL]     = { "mul"   ,0x0ea09c00, 1 },
  [ORC_NEON_OP_ORL]       = { "orr"   ,0x0ea01c00, 1 },
/*[ORC_NEON_OP_SHLL]      = { NULL    ,0         , 1 },*/
/*[ORC_NEON_OP_SHRSL]     = { 32      ,NULL      , 1 },*/
/*[ORC_NEON_OP_SHRUL]     = { 32      ,NULL      , 1 },*/
  [ORC_NEON_OP_SUBL]      = { "sub"   ,0x2ea08400, 1 },
  [ORC_NEON_OP_SUBSSL]    = { "sqsub" ,0x0ea02c00, 1 },
  [ORC_NEON_OP_SUBUSL]    = { "uqsub" ,0x2ea02c00, 1 },
  [ORC_NEON_OP_XORL]      = { "eor"   ,0x2e201c00, 1 },
/*[ORC_NEON_OP_ABSQ]      = { "abs"   ,0xee0b800 , 0 },*/
  [ORC_NEON_OP_ADDQ]      = { "add"   ,0x4ee08400, 0 },
/*[ORC_NEON_OP_ADDSSQ]    = { "sqadd" ,0x0ee00c00, 0 },*/
/*[ORC_NEON_OP_ADDUSQ]    = { "uqadd" ,0x2ee00c00, 0 },*/
  [ORC_NEON_OP_ANDQ]      = { "and"   ,0x0e201c00, 0 },
/*[ORC_NEON_OP_AVGSQ]     = { "srhadd",0x0ee01400, 0 },*/
/*[ORC_NEON_OP_AVGUQ]     = { "urhadd",0x2ee01400, 0 },*/
/*[ORC_NEON_OP_CMPEQQ]    = { "cmeq"  ,0x2ee08c00, 0 },*/
/*[ORC_NEON_OP_CMPGTSQ]   = { "cmgt"  ,0x0ee03400, 0 },*/
  [ORC_NEON_OP_COPYQ]     = { "mov"   ,0x0ea01c00, 0 },
/*[ORC_NEON_OP_MAXSQ]     = { "smax"  ,0x0ee06400, 0 },*/
/*[ORC_NEON_OP_MAXUQ]     = { "umax"  ,0x2ee06400, 0 },*/
/*[ORC_NEON_OP_MINSQ]     = { "smin"  ,0x0ee06c00, 0 },*/
/*[ORC_NEON_OP_MINUQ]     = { "umin"  ,0x2ee06c00, 0 },*/
/*[ORC_NEON_OP_MULLQ]     = { "mul"   ,0x0ee09c00, 0 },*/
  [ORC_NEON_OP_ORQ]       = { "orr"   ,0x0ea01c00, 0 },
  [ORC_NEON_OP_SUBQ]      = { "sub"   ,0x6ee08400, 0 },
/*[ORC_NEON_OP_SUBSSQ]    = { "sqsub" ,0x0ee00c00, 0 },*/
/*[ORC_NEON_OP_SUBUSQ]    = { "uqsub" ,0x2ee00c00, 0 },*/
  [ORC_NEON_OP_XORQ]      = { "eor"   ,0x2e201c00, 0 },
  [ORC_NEON_OP_CONVSBW]   = { "sshll" ,0x0f08a400, 3 },
  [ORC_NEON_OP_CONVUBW]   = { "ushll" ,0x2f08a400, 3 },
  [ORC_NEON_OP_CONVSWL]   = { "sshll" ,0x0f10a400, 2 },
  [ORC_NEON_OP_CONVUWL]   = { "ushll" ,0x2f10a400, 2 },
  [ORC_NEON_OP_CONVSLQ]   = { "sshll" ,0x0f20a400, 1 },
  [ORC_NEON_OP_CONVULQ]   = { "ushll" ,0x2f20a400, 1 },
  [ORC_NEON_OP_CONVWB]    = { "xtn"   ,0x0e212800, 3 },
  [ORC_NEON_OP_CONVSSSWB] = { "sqxtn" ,0x0e214800, 3 },
  [ORC_NEON_OP_CONVSUSWB] = { "sqxtun",0x2e212800, 3 },
  [ORC_NEON_OP_CONVUUSWB] = { "uqxtn" ,0x2e214800, 3 },
  [ORC_NEON_OP_CONVLW]    = { "xtn"   ,0x0e612800, 2 },
  [ORC_NEON_OP_CONVQL]    = { "xtn"   ,0x0ea12800, 1 },
  [ORC_NEON_OP_CONVSSSLW] = { "sqxtn" ,0x0e614800, 2 },
  [ORC_NEON_OP_CONVSUSLW] = { "sqxtun",0x2e612800, 2 },
  [ORC_NEON_OP_CONVUUSLW] = { "uqxtn" ,0x2e614800, 2 },
  [ORC_NEON_OP_CONVSSSQL] = { "sqxtn" ,0x0ea14800, 1 },
  [ORC_NEON_OP_CONVSUSQL] = { "sqxtun",0x2ea12800, 1 },
  [ORC_NEON_OP_CONVUUSQL] = { "uqxtn" ,0x2ea14800, 1 },
  [ORC_NEON_OP_MULSBW]    = { "smull" ,0x0e20c000, 3 },
  [ORC_NEON_OP_MULUBW]    = { "umull" ,0x2e20c000, 3 },
  [ORC_NEON_OP_MULSWL]    = { "smull" ,0x0e60c000, 2 },
  [ORC_NEON_OP_MULUWL]    = { "umull" ,0x2e60c000, 2 },
  [ORC_NEON_OP_SWAPW]     = { "rev16" ,0x0e201800, 2 },
  [ORC_NEON_OP_SWAPL]     = { "rev32" ,0x2e200800, 1 },
  [ORC_NEON_OP_SWAPQ]     = { "rev64" ,0x0e200800, 0 },
  [ORC_NEON_OP_SWAPWL]    = { "rev32" ,0x2e600800, 1 },
  [ORC_NEON_OP_SWAPLQ]    = { "rev64" ,0x0ea00800, 0 },
  [ORC_NEON_OP_SELECT0QL] = { "xtn"   ,0x0ea12800, 1 },
  [ORC_NEON_OP_SELECT0LW] = { "xtn"   ,0x0e612800, 2 },
  [ORC_NEON_OP_SELECT0WB] = { "xtn"   ,0x0e212800, 3 },
  [ORC_NEON_OP_ADDF]      = { "fadd"  ,0x0e20d400, 1 },
  [ORC_NEON_OP_SUBF]      = { "fsub"  ,0x0ea0d400, 1 },
  [ORC_NEON_OP_MULF]      = { "fmul"  ,0x2e20dc00, 1 },
  [ORC_NEON_OP_MAXF]      = { "fmax"  ,0x0e20f400, 1 },
  [ORC_NEON_OP_MINF]      = { "fmin"  ,0x0ea0f400, 1 },
  [ORC_NEON_OP_CMPEQF]    = { "fcmeq" ,0x5e20e400, 1 },
/*[ORC_NEON_OP_CMPLTF]    = { "fcmlt" ,0x5ef8e800, 1 },*/
/*[ORC_NEON_OP_CMPLEF]    = { "fcmle" ,0x7ef8d800, 1 },*/
  [ORC_NEON_OP_CONVFL]    = { "fcvtzs",0x0ea1b800, 1 },
  [ORC_NEON_OP_CONVLF]    = { "scvtf" ,0x0e21d800, 1 },
  [ORC_NEON_OP_ADDD]      = { "fadd"  ,0x4e60d400, 0 },
  [ORC_NEON_OP_SUBD]      = { "fsub"  ,0x4ee0d400, 0 },
  [ORC_NEON_OP_MULD]      = { "fmul"  ,0x6e60dc00, 0 },
  [ORC_NEON_OP_DIVD]      = { "fdiv"  ,0x6e60fc00, 0 },
  [ORC_NEON_OP_DIVF]      = { "fdiv"  ,0x6e20fc00, 0 },
  [ORC_NEON_OP_SQRTD]     = { "fsqrt" ,0x6ee1f800, -1 }, // FIXME?: is this vec_shift right?
  [ORC_NEON_OP_SQRTF]     = { "fsqrt" ,0x6ea1f800, 0 },
/*[ORC_NEON_OP_CMPEQD]    = { NULL    ,0         , 0 },*/
  [ORC_NEON_OP_CONVDF]    = { "fcvtzs",0x4ee1b800, -1 }, // FIXME?: is this vec_shift right?
  [ORC_NEON_OP_CONVFD]    = { "scvtf" ,0x4e61d800, -1 }, // FIXME?: is this vec_shift right?
};

/** the names of the SIMD registers when used in a scalar way */
const char *orc_neon64_reg_name_scalar (int reg, int size)
{
  static const char *vec_regs[5][32] = {
    { /** 8-bit */
      "b0", "b1", "b2", "b3",
      "b4", "b5", "b6", "b7",
      "b8", "b9", "b10", "b11",
      "b12", "b13", "b14", "b15",
      "b16", "b17", "b18", "b19",
      "b20", "b21", "b22", "b23",
      "b24", "b25", "b26", "b27",
      "b28", "b29", "b30", "b31"
    },
    { /** 16-bit */
      "h0", "h1", "h2", "h3",
      "h4", "h5", "h6", "h7",
      "h8", "h9", "h10", "h11",
      "h12", "h13", "h14", "h15",
      "h16", "h17", "h18", "h19",
      "h20", "h21", "h22", "h23",
      "h24", "h25", "h26", "h27",
      "h28", "h29", "h30", "h31"
    },
    { /** 32-bit */
      "s0", "s1", "s2", "s3",
      "s4", "s5", "s6", "s7",
      "s8", "s9", "s10", "s11",
      "s12", "s13", "s14", "s15",
      "s16", "s17", "s18", "s19",
      "s20", "s21", "s22", "s23",
      "s24", "s25", "s26", "s27",
      "s28", "s29", "s30", "s31"
    },
    { /** 64-bit */
      "d0", "d1", "d2", "d3",
      "d4", "d5", "d6", "d7",
      "d8", "d9", "d10", "d11",
      "d12", "d13", "d14", "d15",
      "d16", "d17", "d18", "d19",
      "d20", "d21", "d22", "d23",
      "d24", "d25", "d26", "d27",
      "d28", "d29", "d30", "d31"
    },
    { /** 128-bit */
      "q0", "q1", "q2", "q3",
      "q4", "q5", "q6", "q7",
      "q8", "q9", "q10", "q11",
      "q12", "q13", "q14", "q15",
      "q16", "q17", "q18", "q19",
      "q20", "q21", "q22", "q23",
      "q24", "q25", "q26", "q27",
      "q28", "q29", "q30", "q31"
    }
  };

  if (size == 0) {
    return "ERROR";
  }

  if (reg < ORC_VEC_REG_BASE || reg >= ORC_VEC_REG_BASE+32) {
    return "ERROR";
  }

  unsigned int size_idx = orc_count_ones (size);
  if (size_idx >= 5) {
    return "ERROR";
  }

  return vec_regs[size_idx][reg&0x1f];
}

/** the names of the SIMD vector registers when used for vectorization */
const char *orc_neon64_reg_name_vector (int reg, int size, int quad)
{
  static const char *vec_regs[8][32] = {
    {
      "v0.8b", "v1.8b", "v2.8b", "v3.8b",
      "v4.8b", "v5.8b", "v6.8b", "v7.8b",
      "v8.8b", "v9.8b", "v10.8b", "v11.8b",
      "v12.8b", "v13.8b", "v14.8b", "v15.8b",
      "v16.8b", "v17.8b", "v18.8b", "v19.8b",
      "v20.8b", "v21.8b", "v22.8b", "v23.8b",
      "v24.8b", "v25.8b", "v26.8b", "v27.8b",
      "v28.8b", "v29.8b", "v30.8b", "v31.8b"
    },
    {
      "v0.16b", "v1.16b", "v2.16b", "v3.16b",
      "v4.16b", "v5.16b", "v6.16b", "v7.16b",
      "v8.16b", "v9.16b", "v10.16b", "v11.16b",
      "v12.16b", "v13.16b", "v14.16b", "v15.16b",
      "v16.16b", "v17.16b", "v18.16b", "v19.16b",
      "v20.16b", "v21.16b", "v22.16b", "v23.16b",
      "v24.16b", "v25.16b", "v26.16b", "v27.16b",
      "v28.16b", "v29.16b", "v30.16b", "v31.16b"
    },
    {
      "v0.4h", "v1.4h", "v2.4h", "v3.4h",
      "v4.4h", "v5.4h", "v6.4h", "v7.4h",
      "v8.4h", "v9.4h", "v10.4h", "v11.4h",
      "v12.4h", "v13.4h", "v14.4h", "v15.4h",
      "v16.4h", "v17.4h", "v18.4h", "v19.4h",
      "v20.4h", "v21.4h", "v22.4h", "v23.4h",
      "v24.4h", "v25.4h", "v26.4h", "v27.4h",
      "v28.4h", "v29.4h", "v30.4h", "v31.4h"
    },
    {
      "v0.8h", "v1.8h", "v2.8h", "v3.8h",
      "v4.8h", "v5.8h", "v6.8h", "v7.8h",
      "v8.8h", "v9.8h", "v10.8h", "v11.8h",
      "v12.8h", "v13.8h", "v14.8h", "v15.8h",
      "v16.8h", "v17.8h", "v18.8h", "v19.8h",
      "v20.8h", "v21.8h", "v22.8h", "v23.8h",
      "v24.8h", "v25.8h", "v26.8h", "v27.8h",
      "v28.8h", "v29.8h", "v30.8h", "v31.8h"
    },
    {
      "v0.2s", "v1.2s", "v2.2s", "v3.2s",
      "v4.2s", "v5.2s", "v6.2s", "v7.2s",
      "v8.2s", "v9.2s", "v10.2s", "v11.2s",
      "v12.2s", "v13.2s", "v14.2s", "v15.2s",
      "v16.2s", "v17.2s", "v18.2s", "v19.2s",
      "v20.2s", "v21.2s", "v22.2s", "v23.2s",
      "v24.2s", "v25.2s", "v26.2s", "v27.2s",
      "v28.2s", "v29.2s", "v30.2s", "v31.2s"
    },
    {
      "v0.4s", "v1.4s", "v2.4s", "v3.4s",
      "v4.4s", "v5.4s", "v6.4s", "v7.4s",
      "v8.4s", "v9.4s", "v10.4s", "v11.4s",
      "v12.4s", "v13.4s", "v14.4s", "v15.4s",
      "v16.4s", "v17.4s", "v18.4s", "v19.4s",
      "v20.4s", "v21.4s", "v22.4s", "v23.4s",
      "v24.4s", "v25.4s", "v26.4s", "v27.4s",
      "v28.4s", "v29.4s", "v30.4s", "v31.4s"
    },
    {
      "v0.1d", "v1.1d", "v2.1d", "v3.1d",
      "v4.1d", "v5.1d", "v6.1d", "v7.1d",
      "v8.1d", "v9.1d", "v10.1d", "v11.1d",
      "v12.1d", "v13.1d", "v14.1d", "v15.1d",
      "v16.1d", "v17.1d", "v18.1d", "v19.1d",
      "v20.1d", "v21.1d", "v22.1d", "v23.1d",
      "v24.1d", "v25.1d", "v26.1d", "v27.1d",
      "v28.1d", "v29.1d", "v30.1d", "v31.1d"
    },
    {
      "v0.2d", "v1.2d", "v2.2d", "v3.2d",
      "v4.2d", "v5.2d", "v6.2d", "v7.2d",
      "v8.2d", "v9.2d", "v10.2d", "v11.2d",
      "v12.2d", "v13.2d", "v14.2d", "v15.2d",
      "v16.2d", "v17.2d", "v18.2d", "v19.2d",
      "v20.2d", "v21.2d", "v22.2d", "v23.2d",
      "v24.2d", "v25.2d", "v26.2d", "v27.2d",
      "v28.2d", "v29.2d", "v30.2d", "v31.2d"
    }
  };

  if (size == 0) {
    return "ERROR";
  }

  if (reg < ORC_VEC_REG_BASE || reg >= ORC_VEC_REG_BASE+32) {
    return "ERROR";
  }

  unsigned int size_idx = orc_count_ones (size);
  if (size_idx >= 4) {
    return "ERROR";
  }

  if (quad != 0 && quad != 1) {
    return "ERROR";
  }

  return vec_regs[size_idx*2+quad][reg&0x1f];
}

/** a single element from a SIMD vector register as a scalar operand */
const char *orc_neon64_reg_name_vector_single (int reg, int size)
{
  static const char *vec_regs[4][32] = {
    {
      "v0.b", "v1.b", "v2.b", "v3.b",
      "v4.b", "v5.b", "v6.b", "v7.b",
      "v8.b", "v9.b", "v10.b", "v11.b",
      "v12.b", "v13.b", "v14.b", "v15.b",
      "v16.b", "v17.b", "v18.b", "v19.b",
      "v20.b", "v21.b", "v22.b", "v23.b",
      "v24.b", "v25.b", "v26.b", "v27.b",
      "v28.b", "v29.b", "v30.b", "v31.b"
    },
    {
      "v0.h", "v1.h", "v2.h", "v3.h",
      "v4.h", "v5.h", "v6.h", "v7.h",
      "v8.h", "v9.h", "v10.h", "v11.h",
      "v12.h", "v13.h", "v14.h", "v15.h",
      "v16.h", "v17.h", "v18.h", "v19.h",
      "v20.h", "v21.h", "v22.h", "v23.h",
      "v24.h", "v25.h", "v26.h", "v27.h",
      "v28.h", "v29.h", "v30.h", "v31.h"
    },
    {
      "v0.s", "v1.s", "v2.s", "v3.s",
      "v4.s", "v5.s", "v6.s", "v7.s",
      "v8.s", "v9.s", "v10.s", "v11.s",
      "v12.s", "v13.s", "v14.s", "v15.s",
      "v16.s", "v17.s", "v18.s", "v19.s",
      "v20.s", "v21.s", "v22.s", "v23.s",
      "v24.s", "v25.s", "v26.s", "v27.s",
      "v28.s", "v29.s", "v30.s", "v31.s"
    },
    {
      "v0.d", "v1.d", "v2.d", "v3.d",
      "v4.d", "v5.d", "v6.d", "v7.d",
      "v8.d", "v9.d", "v10.d", "v11.d",
      "v12.d", "v13.d", "v14.d", "v15.d",
      "v16.d", "v17.d", "v18.d", "v19.d",
      "v20.d", "v21.d", "v22.d", "v23.d",
      "v24.d", "v25.d", "v26.d", "v27.d",
      "v28.d", "v29.d", "v30.d", "v31.d"
    },
  };


  if (size == 0) {
    return "ERROR";
  }

  if (reg < ORC_VEC_REG_BASE || reg >= ORC_VEC_REG_BASE+32) {
    return "ERROR";
  }

  unsigned int size_idx = orc_count_ones (size);
  if (size_idx >= 4) {
    return "ERROR";
  }

  return vec_regs[size_idx][reg&0x1f];
}

void
orc_neon64_emit_binary (OrcCompiler *p, const char *name, unsigned int code,
    OrcVariable dest, OrcVariable src1, OrcVariable src2, int vec_shift)
{
  int is_quad = 0;

  if (p->insn_shift == vec_shift + 1) {
    is_quad = 1;
  } else if (p->insn_shift > vec_shift + 1) {
    ORC_COMPILER_ERROR(p, "out-of-shift");
    return;
  }

  ORC_ASM_CODE(p,"  %s %s, %s, %s\n", name,
      orc_neon64_reg_name_vector (dest.alloc, dest.size, is_quad),
      orc_neon64_reg_name_vector (src1.alloc, src1.size, is_quad),
      orc_neon64_reg_name_vector (src2.alloc, src2.size, is_quad));
  code |= (is_quad&0x1)<<30;
  code |= (src2.alloc&0x1f)<<16;
  code |= (src1.alloc&0x1f)<<5;
  code |= (dest.alloc&0x1f);
  orc_arm_emit (p, code);
}

void
orc_neon64_emit_unary (OrcCompiler *p, const char *name, unsigned int code,
    OrcVariable dest, OrcVariable src1, int vec_shift)
{
  int is_quad = 0;

  if (p->insn_shift == vec_shift + 1) {
    is_quad = 1;
  } else if (p->insn_shift > vec_shift + 1) {
    ORC_COMPILER_ERROR(p, "out-of-shift");
    return;
  }

  ORC_ASM_CODE(p,"  %s %s, %s\n", name,
      orc_neon64_reg_name_vector (dest.alloc, dest.size, is_quad),
      orc_neon64_reg_name_vector (src1.alloc, src1.size, is_quad));
  code |= (is_quad&0x1)<<30;
  code |= (src1.alloc&0x1f)<<5;
  code |= (dest.alloc&0x1f);
  orc_arm_emit (p, code);
}

void
orc_neon64_unary_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn * const i = (const OrcNeonInsn *) user;

  OrcVariable dest = p->vars[insn->dest_args[0]];
  OrcVariable src1 = p->vars[insn->src_args[0]];
  orc_neon64_emit_unary (p, i->name, i->code, dest, src1, i->vec_shift);
}

void
orc_neon64_binary_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn * const i = (const OrcNeonInsn *) user;

  OrcVariable dest = p->vars[insn->dest_args[0]];
  OrcVariable src1 = p->vars[insn->src_args[0]];
  OrcVariable src2 = p->vars[insn->src_args[1]];
  orc_neon64_emit_binary (p, i->name, i->code, dest, src1, src2, i->vec_shift);
}

void
orc_neon64_move_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn * const i = (const OrcNeonInsn *) user;

  OrcVariable dest = p->vars[insn->dest_args[0]];
  OrcVariable src1 = p->vars[insn->src_args[0]];

  if (dest.alloc == src1.alloc)
    return;

  orc_neon64_emit_binary (p, i->name, i->code, dest, src1, src1, i->vec_shift);
}

void
orc_neon64_rule_loadupdb (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
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
    orc_arm64_emit_add_lsr(compiler, 64, ptr_reg, src->ptr_register, src->ptr_offset, 1);
  } else {
    ptr_reg = src->ptr_register;
  }

  int opcode, flag;

  if (size >= 16) {
    /** load multiple single-element structures to one, two, three, or four registers */
    char vt_str[64];

    memset(vt_str, '\x00', 64);

    if (size == 64) {
      snprintf(vt_str, 64, "%s, %s, %s, %s",
          orc_neon64_reg_name_vector (compiler->tmpreg, 1, 1),
          orc_neon64_reg_name_vector (compiler->tmpreg + 1, 1, 1),
          orc_neon64_reg_name_vector (compiler->tmpreg + 2, 1, 1),
          orc_neon64_reg_name_vector (compiler->tmpreg + 3, 1, 1));
      opcode = 0x2;
    } else if (size == 32) {
      snprintf(vt_str, 64, "%s, %s",
          orc_neon64_reg_name_vector (compiler->tmpreg, 1, 1),
          orc_neon64_reg_name_vector (compiler->tmpreg + 1, 1, 1));
      opcode = 0xa;
    } else if (size == 16) {
      snprintf(vt_str, 64, "%s",
          orc_neon64_reg_name_vector (compiler->tmpreg, 1, 1));
      opcode = 0x7;
    } else {
      ORC_COMPILER_ERROR(compiler,"bad aligned load size %d",
          src->size << compiler->insn_shift);
      return;
    }
    flag = 0; /* Bytes */

    ORC_ASM_CODE(compiler,"  ld1 { %s }, [%s]\n",
        vt_str, orc_arm64_reg_name (ptr_reg, 64));
    code = 0x0c400000;
    code |= 0 << 30; /* Q-bit */
    code |= (flag&0x3) << 10;
    code |= (opcode&0xf) << 12;
  } else {
    /** load one single-element structure to one lane of one register */
    flag = 0;
    if (size == 8) {
      opcode = 4;
      flag = 1; /* size==01 */
    } else if (size == 4) {
      opcode = 4;
    } else if (size == 2) {
      opcode = 2;
    } else if (size == 1) {
      opcode = 0;
    } else {
      ORC_COMPILER_ERROR(compiler,"bad unaligned load size %d",
          src->size << compiler->insn_shift);
      return;
    }
    ORC_ASM_CODE(compiler,"  ld1 { %s }[0], [%s]\n",
        orc_neon64_reg_name_vector_single (compiler->tmpreg, size),
        orc_arm64_reg_name (ptr_reg, 64));
    code = 0x0d400000;
    code |= (opcode&0x7) << 13;
    code |= (flag&0x3) << 10;
  }

  code |= (ptr_reg&0x1f) << 5;
  code |= (compiler->tmpreg&0x1f);

  orc_arm_emit (compiler, code);

  OrcVariable tmpreg = { .alloc = compiler->tmpreg, .size = compiler->vars[insn->src_args[0]].size };

  switch (src->size) {
    case 1:
      orc_neon64_emit_binary (compiler, "zip1", 0x0e003800,
          compiler->vars[insn->dest_args[0]],
          tmpreg,
          tmpreg, compiler->insn_shift - 1);
      break;
    case 2:
      orc_neon64_emit_binary (compiler, "zip1", 0x0e403800,
          compiler->vars[insn->dest_args[0]],
          tmpreg,
          tmpreg, compiler->insn_shift - 1);
      break;
    case 4:
      orc_neon64_emit_binary (compiler, "zip1", 0x0e803800,
          compiler->vars[insn->dest_args[0]],
          tmpreg,
          tmpreg, compiler->insn_shift - 1);
      break;
  }

  src->update_type = 1;
}

void
orc_neon64_rule_loadpX (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int size = ORC_PTR_TO_INT (user);

  if (src->vartype == ORC_VAR_TYPE_CONST) {
    if (size == 1) {
      orc_neon64_emit_loadib (compiler, dest, src->value.i);
    } else if (size == 2) {
      orc_neon64_emit_loadiw (compiler, dest, src->value.i);
    } else if (size == 4) {
      orc_neon64_emit_loadil (compiler, dest, src->value.i);
    } else if (size == 8) {
      orc_neon64_emit_loadiq (compiler, dest, src->value.i);
    } else {
      ORC_PROGRAM_ERROR(compiler,"unimplemented");
    }
  } else {
    if (size == 1) {
      orc_neon64_emit_loadpb (compiler, dest->alloc, insn->src_args[0]);
    } else if (size == 2) {
      orc_neon64_emit_loadpw (compiler, dest->alloc, insn->src_args[0]);
    } else if (size == 4) {
      orc_neon64_emit_loadpl (compiler, dest->alloc, insn->src_args[0]);
    } else if (size == 8) {
      orc_neon64_emit_loadpq (compiler, dest->alloc, insn->src_args[0]);
    } else {
      ORC_PROGRAM_ERROR(compiler,"unimplemented");
    }
  }
}

void
orc_neon64_rule_loadX (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
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

  if (type == 1) {
    OrcVariable *src2 = compiler->vars + insn->src_args[1];

    if (src2->vartype != ORC_VAR_TYPE_CONST) {
      ORC_PROGRAM_ERROR(compiler,"unimplemented");
      return;
    }

    ptr_register = compiler->gp_tmpreg;
    if (src2->value.i < 0) {
      orc_arm64_emit_sub_imm (compiler, 64, ptr_register,
          src->ptr_register,
          src2->value.i * src->size * -1);
    }
    else
    {
      orc_arm64_emit_add_imm (compiler, 64, ptr_register,
          src->ptr_register,
          src2->value.i * src->size);
    }

    is_aligned = FALSE;
  } else {
    ptr_register = src->ptr_register;
  }

  int opcode, flag;

  if (size >= 16) {
    /** load multiple single-element structures to one, two, three, or four registers */
    char vt_str[64];

    memset(vt_str, '\x00', 64);

    if (is_aligned) {
      if (size == 64) {
        snprintf(vt_str, 64, "%s, %s, %s, %s",
            orc_neon64_reg_name_vector (dest->alloc, 8, 1),
            orc_neon64_reg_name_vector (dest->alloc + 1, 8, 1),
            orc_neon64_reg_name_vector (dest->alloc + 2, 8, 1),
            orc_neon64_reg_name_vector (dest->alloc + 3, 8, 1));
        opcode = 2;
      } else if (size == 32) {
        snprintf(vt_str, 64, "%s, %s",
            orc_neon64_reg_name_vector (dest->alloc, 8, 1),
            orc_neon64_reg_name_vector (dest->alloc + 1, 8, 1));
        opcode = 10;
      } else if (size == 16) {
        snprintf(vt_str, 64, "%s",
            orc_neon64_reg_name_vector (dest->alloc, 8, 1));
        opcode = 7;
      } else {
        ORC_COMPILER_ERROR(compiler,"bad aligned load size %d",
            src->size << compiler->insn_shift);
        return;
      }
      flag = 7;
    } else {
      if (size == 64) {
        snprintf(vt_str, 64, "%s, %s, %s, %s",
            orc_neon64_reg_name_vector (dest->alloc, 1, 1),
            orc_neon64_reg_name_vector (dest->alloc + 1, 1, 1),
            orc_neon64_reg_name_vector (dest->alloc + 2, 1, 1),
            orc_neon64_reg_name_vector (dest->alloc + 3, 1, 1));
        opcode = 2;
      } else if (size == 32) {
        snprintf(vt_str, 64, "%s, %s",
            orc_neon64_reg_name_vector (dest->alloc, 1, 1),
            orc_neon64_reg_name_vector (dest->alloc + 1, 1, 1));
        opcode = 10;
      } else if (size == 16) {
        snprintf(vt_str, 64, "%s",
            orc_neon64_reg_name_vector (dest->alloc, 1, 1));
        opcode = 7;
      } else {
        ORC_COMPILER_ERROR(compiler,"bad aligned load size %d",
            src->size << compiler->insn_shift);
        return;
      }
      flag = 1;
    }
    ORC_ASM_CODE(compiler,"  ld1 { %s }, [%s]\n",
        vt_str, orc_arm64_reg_name (ptr_register, 64));
    code = 0x0c400000;
    code |= (flag&0x1) << 30;
    code |= (flag&0x3) << 10;
    code |= (opcode&0xf) << 12;
  } else {
    /** load one single-element structure to one lane of one register */
    flag = 0;
    if (size == 8) {
      opcode = 4;
      flag = 1;
    } else if (size == 4) {
      opcode = 4;
    } else if (size == 2) {
      opcode = 2;
    } else if (size == 1) {
      opcode = 0;
    } else {
      ORC_COMPILER_ERROR(compiler,"bad unaligned load size %d",
          src->size << compiler->insn_shift);
      return;
    }
    ORC_ASM_CODE(compiler,"  ld1 { %s }[0], [%s]\n",
        orc_neon64_reg_name_vector_single (dest->alloc, size),
        orc_arm64_reg_name (ptr_register, 64));
    code = 0x0d400000;
    code |= (opcode&0x7) << 13;
    code |= (flag&0x3) << 10;
  }

  code |= (ptr_register&0x1f) << 5;
  code |= (dest->alloc&0x1f);

  orc_arm_emit (compiler, code);
}

void
orc_neon64_rule_storeX (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  unsigned int code = 0;
  int size = dest->size << compiler->insn_shift;

  int opcode, flag;

  if (size >= 16) {
    /** store multiple single-element structures to one, two, three, or four registers */
    char vt_str[64];

    memset(vt_str, '\x00', 64);

    if (dest->is_aligned) {
      if (size == 64) {
        snprintf(vt_str, 64, "%s, %s, %s, %s",
            orc_neon64_reg_name_vector (src->alloc, 8, 1),
            orc_neon64_reg_name_vector (src->alloc + 1, 8, 1),
            orc_neon64_reg_name_vector (src->alloc + 2, 8, 1),
            orc_neon64_reg_name_vector (src->alloc + 3, 8, 1));
        opcode = 0x2;
      } else if (size == 32) {
        snprintf(vt_str, 64, "%s, %s",
            orc_neon64_reg_name_vector (src->alloc, 8, 1),
            orc_neon64_reg_name_vector (src->alloc + 1, 8, 1));
        opcode = 0xa;
      } else if (size == 16) {
        snprintf(vt_str, 64, "%s",
            orc_neon64_reg_name_vector (src->alloc, 8, 1));
        opcode = 0x7;
      } else {
        ORC_COMPILER_ERROR(compiler,"bad aligned load size %d",
            src->size << compiler->insn_shift);
        return;
      }
    } else {
      if (size == 64) {
        snprintf(vt_str, 64, "%s, %s, %s, %s",
            orc_neon64_reg_name_vector (src->alloc, 1, 1),
            orc_neon64_reg_name_vector (src->alloc + 1, 1, 1),
            orc_neon64_reg_name_vector (src->alloc + 2, 1, 1),
            orc_neon64_reg_name_vector (src->alloc + 3, 1, 1));
        opcode = 0x2;
      } else if (size == 32) {
        snprintf(vt_str, 64, "%s, %s",
            orc_neon64_reg_name_vector (src->alloc, 1, 1),
            orc_neon64_reg_name_vector (src->alloc + 1, 1, 1));
        opcode = 0xa;
      } else if (size == 16) {
        snprintf(vt_str, 64, "%s",
            orc_neon64_reg_name_vector (src->alloc, 1, 1));
        opcode = 0x7;
      } else {
        ORC_COMPILER_ERROR(compiler,"bad aligned load size %d",
            src->size << compiler->insn_shift);
        return;
      }
    }
    ORC_ASM_CODE(compiler,"  st1 { %s }, [%s]\n",
        vt_str, orc_arm64_reg_name (dest->ptr_register, 64));
    code = 0x0c000000;
    code |= 1 << 30;
    code |= (opcode&0xf) << 12;
  } else {
    /** store one single-element structure to one lane of one register */
    flag = 0;
    if (size == 8) {
      opcode = 4;
      flag = 1;
    } else if (size == 4) {
      opcode = 4;
    } else if (size == 2) {
      opcode = 2;
    } else if (size == 1) {
      opcode = 0;
    } else {
      ORC_COMPILER_ERROR(compiler,"bad unaligned load size %d",
          src->size << compiler->insn_shift);
      return;
    }
    ORC_ASM_CODE(compiler,"  st1 { %s }[0], [%s]\n",
        orc_neon64_reg_name_vector_single (src->alloc, size),
        orc_arm64_reg_name (dest->ptr_register, 64));
    code = 0x0d000000;
    code |= (opcode&0x7) << 13;
    code |= (flag&0x3) << 10;
  }

  code |= (dest->ptr_register&0x1f) << 5;
  code |= (src->alloc&0x1f);

  orc_arm_emit (compiler, code);
}

static void
orc_neon64_emit_loadib (OrcCompiler *compiler, OrcVariable *dest, int value)
{
  int reg = dest->alloc;
  orc_uint32 code;

  if (value == 0) {
    orc_neon64_emit_binary (compiler, "eor", 0x2e201c00,
        *dest, *dest, *dest, compiler->insn_shift - 1);
    return;
  }

  value &= 0xff;
  ORC_ASM_CODE(compiler,"  movi %s, #%d\n",
      orc_neon64_reg_name_vector (reg, 16, 0), value);
  code = 0x0f00e400; /* 8-bit (op==0 && cmode==1110) */
  code |= (reg&0x1f) << 0;
  code |= (value&0x1f) << 5;
  code |= (value&0xe0) << 11;
  code |= 1 << 30;
  orc_arm_emit (compiler, code);
}

static void
orc_neon64_emit_loadiw (OrcCompiler *compiler, OrcVariable *dest, int value)
{
  int reg = dest->alloc;
  orc_uint32 code;

  if (value == 0) {
    orc_neon64_emit_binary (compiler, "eor", 0x2e201c00,
        *dest, *dest, *dest, compiler->insn_shift - 1);
    return;
  }

  ORC_ASM_CODE(compiler, "  movi %s, #0x%02x\n",
               orc_neon64_reg_name_vector(reg, 2, 1), value & 0xff);
  code = 0x0f008400; /* 16-bit (op==0 && cmode==10x0), x=0 is LSL #0 */
  code |= (reg&0x1f) << 0;
  code |= (value&0x1f) << 5;
  code |= (value&0xe0) << 11;
  code |= 1 << 30;
  orc_arm_emit (compiler, code);

  value >>= 8;
  if (value) {
    ORC_ASM_CODE(compiler, "  orr %s, #0x%02x, lsl #8\n",
                 orc_neon64_reg_name_vector(reg, 2, 1), value & 0xff);
    code = 0x0f00b400; /* 16-bit (cmode==10x1), x=1 is LSL #8 */
    code |= (reg&0x1f) << 0;
    code |= (value&0x1f) << 5;
    code |= (value&0xe0) << 11;
    code |= 1 << 30;
    orc_arm_emit (compiler, code);
  }
}

void
orc_neon64_emit_loadil (OrcCompiler *compiler, OrcVariable *dest, int value)
{
  int reg = dest->alloc;
  orc_uint32 code;

  if (value == 0) {
    orc_neon64_emit_binary (compiler, "eor", 0x2e201c00,
        *dest, *dest, *dest, compiler->insn_shift - 1);
    return;
  }

  ORC_ASM_CODE(compiler,"  movi %s, #0x%02x\n",
      orc_neon64_reg_name_vector (reg, 16, 0), value & 0xff);
  code = 0x0f000400; /* 32-bit (op==0 && cmode==0xx0), xx=0 is LSL #0 */
  code |= (reg&0x1f) << 0;
  code |= (value&0x1f) << 5;
  code |= (value&0xe0) << 11;
  code |= 1 << 30;
  orc_arm_emit (compiler, code);

  value >>= 8;
  if (value) {
    ORC_ASM_CODE(compiler,"  orr %s, #0x%02x, lsl #8\n",
        orc_neon64_reg_name_vector (reg, 16, 0), value & 0xff);
    code = 0x0f003400; /* 32-bit (cmode==0xx1), xx=01 is LSL #8 */
    code |= (reg&0x1f) << 0;
    code |= (value&0x1f) << 5;
    code |= (value&0xe0) << 11;
    code |= 1 << 30;
    orc_arm_emit (compiler, code);
  }
  value >>= 8;
  if (value) {
    ORC_ASM_CODE(compiler,"  orr %s, #0x%02x, lsl #16\n",
        orc_neon64_reg_name_vector (reg, 16, 0), value & 0xff);
    code = 0x0f005400; /* 32-bit (cmode==0xx1), xx=10 is LSL #16 */
    code |= (reg&0x1f) << 0;
    code |= (value&0x1f) << 5;
    code |= (value&0xe0) << 11;
    code |= 1 << 30;
    orc_arm_emit (compiler, code);
  }
  value >>= 8;
  if (value) {
    ORC_ASM_CODE(compiler,"  orr %s, #0x%02x, lsl #8\n",
        orc_neon64_reg_name_vector (reg, 16, 0), value & 0xff);
    code = 0x0f007400; /* 32-bit (cmode==0xx1), xx=11 is LSL #24 */
    code |= (reg&0x1f) << 0;
    code |= (value&0x1f) << 5;
    code |= (value&0xe0) << 11;
    code |= 1 << 30;
    orc_arm_emit (compiler, code);
  }
}

static void
orc_neon64_emit_loadiq (OrcCompiler *compiler, OrcVariable *dest, long long value)
{
  int reg = dest->alloc;

  if (value == 0) {
    orc_neon64_emit_binary (compiler, "eor", 0x2e201c00,
        *dest, *dest, *dest, compiler->insn_shift - 1);
    return;
  }

  /*
   * NOTE: This could be optimized further. The code below is 5 instructions
   * long. By locating 8-bit "islands" of bits in the value itself (8-bit is
   * the limit of IMM field in MOVI/ORR opcode), it may be possible for some
   * sparse constants (with fewer than 5 such islands) to generate far more
   * optimal (shorter than 5 instructions) load using MOVI and ORR opcodes
   * instead. However, such optimization might also be premature, since the
   * constant is usually loaded only once when the program starts, hence it
   * is not implemented below for now.
   */
  ORC_ASM_CODE(compiler,"  ldr %s, L30\n",
      orc_neon64_reg_name_vector (reg, 8, 0));
  orc_arm_emit (compiler, 0x5c000040 | (reg & 0x1f));

  orc_arm64_emit_branch (compiler, ORC_ARM_COND_AL, 30);
  orc_arm_emit (compiler, value & 0xffffffffULL);
  orc_arm_emit (compiler, value >> 32ULL);
  orc_arm_emit_label (compiler, 30);

  orc_neon64_emit_binary (compiler, "trn1", 0x0ec02800,
      *dest, *dest, *dest, compiler->insn_shift - 1);
}

static void
orc_neon64_emit_loadpb (OrcCompiler *compiler, int dest, int param)
{
  orc_uint32 code;

  orc_arm64_emit_add_imm (compiler, 64, compiler->gp_tmpreg,
      compiler->exec_reg, ORC_STRUCT_OFFSET(OrcExecutor, params[param]));

  ORC_ASM_CODE(compiler,"  ld1r {%s, %s}, [%s]\n",
      orc_neon64_reg_name_vector (dest, 1, 0),
      orc_neon64_reg_name_vector (dest+1, 1, 0),
      orc_arm64_reg_name (compiler->gp_tmpreg, 64));
  code = 0x0d40c000;
  code |= 1 << 30; /* Q-bit */
  code |= (compiler->gp_tmpreg&0x1f) << 5;
  code |= (dest&0x1f) << 0;
  orc_arm_emit (compiler, code);
}

static void
orc_neon64_emit_loadpw (OrcCompiler *compiler, int dest, int param)
{
  orc_uint32 code;

  orc_arm64_emit_add_imm (compiler, 64, compiler->gp_tmpreg,
      compiler->exec_reg, ORC_STRUCT_OFFSET(OrcExecutor, params[param]));

  ORC_ASM_CODE(compiler,"  ld1r {%s, %s}, [%s]\n",
      orc_neon64_reg_name_vector (dest, 2, 0),
      orc_neon64_reg_name_vector (dest+1, 2, 0),
      orc_arm64_reg_name (compiler->gp_tmpreg, 64));
  code = 0x0d40c400;
  code |= 1 << 30; /* Q-bit */
  code |= (compiler->gp_tmpreg&0x1f) << 5;
  code |= (dest&0x1f) << 0;
  orc_arm_emit (compiler, code);
}

static void
orc_neon64_emit_loadpl (OrcCompiler *compiler, int dest, int param)
{
  orc_uint32 code;

  orc_arm64_emit_add_imm (compiler, 64, compiler->gp_tmpreg,
      compiler->exec_reg, ORC_STRUCT_OFFSET(OrcExecutor, params[param]));

  ORC_ASM_CODE(compiler,"  ld1r {%s, %s}, [%s]\n",
      orc_neon64_reg_name_vector (dest, 4, 0),
      orc_neon64_reg_name_vector (dest+1, 4, 0),
      orc_arm64_reg_name (compiler->gp_tmpreg, 64));
  code = 0x0d40c800;
  code |= 1 << 30; /* Q-bit */
  code |= (compiler->gp_tmpreg&0x1f) << 5;
  code |= (dest&0x1f) << 0;
  orc_arm_emit (compiler, code);
}

static void
orc_neon64_emit_loadpq (OrcCompiler *compiler, int dest, int param)
{
  orc_uint32 code;

  orc_arm64_emit_add_imm (compiler, 64, compiler->gp_tmpreg,
      compiler->exec_reg, ORC_STRUCT_OFFSET(OrcExecutor, params[param]));

  /*
   * This here is a bit more complex, as the top 32 bits of the Tx are
   * stored at an offset sizeof(params) * (ORC_N_PARAMS) from
   * bottom 32 bits Px, so we do interleaved load using LD3, where the
   * (v0.4s)[0] is Px and (v2.4s)[2] is Tx, because they are exactly
   * 256 bits apart = 32 bytes = sizeof(params)*(ORC_N_PARAMS).
   *
   * The way all the LD1..LD4R opcodes work may be inobvious from the
   * ARM A64 ISA documentation. See the following article:
   * https://community.arm.com/developer/ip-products/processors/b/processors-ip-blog/posts/coding-for-neon---part-1-load-and-stores
   *
   * Specifically, LD3.32 with Q-bit set (128-bit operation) works this
   * way. Assume array of 32bit types with 12 entries:
   *
   *  uint32_t x0[12];
   *  ld3 {v0.4s, v1.4d, v2.4s}, [x0]          .--- LSB (address 0)
   *  results in:                              v
   *  v0.4s[127:0] = { x0[9],  x0[6], x0[3], x0[0] };
   *  v1.4s[127:0] = { x0[10], x0[7], x0[4], x0[1] };
   *  v2.4s[127:0] = { x0[11], x0[8], x0[5], x0[2] };
   *
   * To obtain the correct final result of loadpq, two MOV instructions
   * are necessary to generate v0.4s = { x0[8], x0[0], x0[8], x0[0] };
   * Note that there might be a better way to perform the mixing with
   * some TRN/ZIP/UZP instruction.
   */
  ORC_ASSERT((ORC_N_PARAMS) == 8);
  ORC_ASM_CODE(compiler,"  ld3 {%s - %s}, [%s]\n",
      orc_neon64_reg_name_vector (dest, 8, 0),
      orc_neon64_reg_name_vector (dest+2, 8, 0),
      orc_arm64_reg_name (compiler->gp_tmpreg, 64));
  code = 0x0c404800;
  code |= 1 << 30; /* Q-bit */
  code |= (compiler->gp_tmpreg&0x1f) << 5;
  code |= (dest&0x1f) << 0;
  orc_arm_emit (compiler, code);

  ORC_ASM_CODE(compiler,"  mov %s[1], %s[2]\n",
      orc_neon64_reg_name_vector (dest, 4, 0),
      orc_neon64_reg_name_vector (dest+2, 4, 0));
  code = 0x6e0c4400;
  code |= ((dest+2)&0x1f) << 5;
  code |= (dest&0x1f) << 0;
  orc_arm_emit (compiler, code);

  ORC_ASM_CODE(compiler,"  mov %s[1], %s[0]\n",
      orc_neon64_reg_name_vector (dest, 8, 0),
      orc_neon64_reg_name_vector (dest, 8, 0));
  code = 0x6e180400;
  code |= (dest&0x1f) << 5;
  code |= (dest&0x1f) << 0;
  orc_arm_emit (compiler, code);
}

static const ShiftInfo immshift_info_64[] = {
  { 0x0f085400, "shl", FALSE, 8, 3 }, /* shlb */
  { 0x0f080400, "sshr", TRUE, 8, 3 }, /* shrsb */
  { 0x2f080400, "ushr", TRUE, 8, 3 }, /* shrub */
  { 0x0f105400, "shl", FALSE, 16, 2 },
  { 0x0f100400, "sshr", TRUE, 16, 2 },
  { 0x2f100400, "ushr", TRUE, 16, 2 },
  { 0x0f205400, "shl", FALSE, 32, 1 },
  { 0x0f200400, "sshr", TRUE, 32, 1 },
  { 0x2f200400, "ushr", TRUE, 32, 1 }
};

static const ShiftInfo regshift_info_64[] = {
  { 0x2e204400, "ushl", FALSE, 0, 3 }, /* shlb */
  { 0x0e204400, "sshl", TRUE, 0, 3 }, /* shrsb */
  { 0x2e204400, "ushl", TRUE, 0, 3 }, /* shrub */
  { 0x2e604400, "ushl", FALSE, 0, 2 },
  { 0x0e604400, "sshl", TRUE, 0, 2 },
  { 0x2e604400, "ushl", TRUE, 0, 2 },
  { 0x2ea04400, "ushl", FALSE, 0, 1 },
  { 0x0ea04400, "sshl", TRUE, 0, 1 },
  { 0x2ea04400, "ushl", TRUE, 0, 1 }
};

void
orc_neon64_emit_shift (OrcCompiler *const p, int type,
                    const OrcVariable *const dest,
                    const OrcVariable *const src, int shift,
                    int is_quad)
{
  orc_uint32 code = 0;
  if (shift < 0) {
    ORC_COMPILER_ERROR(p, "shift negative");
    return;
  }
  if (shift >= immshift_info_64[type].bits) {
    ORC_COMPILER_ERROR(p, "shift too large");
    return;
  }

  code = immshift_info_64[type].code;
  ORC_ASM_CODE(p, "  %s %s, %s, #%d\n", immshift_info_64[type].name,
               orc_neon64_reg_name_vector(dest->alloc, dest->size, is_quad),
               orc_neon64_reg_name_vector(src->alloc, src->size, is_quad),
               shift);
  if (is_quad) {
    code |= 1 << 30;
  }
  code |= (dest->alloc & 0x1f) << 0;
  code |= (src->alloc & 0x1f) << 5;

  if (immshift_info_64[type].negate) {
    shift = immshift_info_64[type].bits - shift;
  }
  code |= shift << 16;
  orc_arm_emit(p, code);
}

void
orc_neon64_rule_shift (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int type = ORC_PTR_TO_INT(user);

  if (p->vars[insn->src_args[1]].vartype == ORC_VAR_TYPE_CONST) {
    orc_neon64_emit_shift(p, type, p->vars + insn->dest_args[0],
                        p->vars + insn->src_args[0],
                        (int)p->vars[insn->src_args[1]].value.i,
                        p->insn_shift > immshift_info_64[type].vec_shift);
  } else if (p->vars[insn->src_args[1]].vartype == ORC_VAR_TYPE_PARAM) {
    OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->src_args[0]].size };
    orc_neon64_emit_loadpb (p, p->tmpreg, insn->src_args[1]);
    if (regshift_info_64[type].negate) {
      orc_neon64_emit_unary (p, "neg", 0x2e20b800, tmpreg, tmpreg, p->insn_shift - 1);
    }

    orc_neon64_emit_binary (p, regshift_info_64[type].name,
        regshift_info_64[type].code,
        p->vars[insn->dest_args[0]],
        p->vars[insn->src_args[0]],
        tmpreg,
        p->insn_shift - !!(p->insn_shift > regshift_info_64[type].vec_shift));
  } else {
    ORC_PROGRAM_ERROR(p,"shift rule only works with constants and params");
  }
}

void
orc_neon64_rule_andn (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int max_shift = ORC_PTR_TO_INT(user);

  orc_neon64_emit_binary (p, "bic", 0x0e601c00,
      p->vars[insn->dest_args[0]],
      p->vars[insn->src_args[1]],
      p->vars[insn->src_args[0]],
      p->insn_shift - (p->insn_shift > max_shift));
}

void
orc_neon64_rule_accw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->src_args[0]].size };

  if (p->insn_shift < 2) {
    orc_neon64_emit_unary (p, "shl",
        0x0f405400 | (48 << 16),
        tmpreg, p->vars[insn->src_args[0]],
        p->insn_shift - 1);
    orc_neon64_emit_binary (p, "add", 0x0ee08400,
        p->vars[insn->dest_args[0]],
        p->vars[insn->dest_args[0]],
        tmpreg, p->insn_shift - 1);
  } else {
    orc_neon64_emit_binary (p, "add", 0x0e608400,
        p->vars[insn->dest_args[0]],
        p->vars[insn->dest_args[0]],
        p->vars[insn->src_args[0]], p->insn_shift);
  }
}

void
orc_neon64_rule_accl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->src_args[0]].size };

  if (p->insn_shift < 1) {
    orc_neon64_emit_unary (p, "shl",
        0x0f405400 | (32 << 16),
        tmpreg, p->vars[insn->src_args[0]],
        p->insn_shift - 1);
    orc_neon64_emit_binary (p, "add", 0x0ee08400,
        p->vars[insn->dest_args[0]],
        p->vars[insn->dest_args[0]],
        tmpreg, p->insn_shift - 1);
  } else {
    orc_neon64_emit_binary (p, "add", 0x0ea08400,
        p->vars[insn->dest_args[0]],
        p->vars[insn->dest_args[0]],
        p->vars[insn->src_args[0]], p->insn_shift);
  }
}

void
orc_neon64_rule_select1wb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  ORC_ASM_CODE(p,"  shrn %s, %s, #%d\n",
      orc_neon64_reg_name_vector (p->vars[insn->dest_args[0]].alloc, 8, 0),
      orc_neon64_reg_name_vector (p->vars[insn->src_args[0]].alloc, 8, 1), 8);
  orc_neon64_emit_unary (p, "shrn", 0x0f088400,
      p->vars[insn->dest_args[0]],
      p->vars[insn->src_args[0]], p->insn_shift);
}

void
orc_neon64_rule_select1lw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  ORC_ASM_CODE(p,"  shrn %s, %s, #%d\n",
      orc_neon64_reg_name_vector (p->vars[insn->dest_args[0]].alloc, 8, 0),
      orc_neon64_reg_name_vector (p->vars[insn->src_args[0]].alloc, 8, 1), 16);
  orc_neon64_emit_unary (p, "shrn", 0x0f108400,
      p->vars[insn->dest_args[0]],
      p->vars[insn->src_args[0]], p->insn_shift);
}

void
orc_neon64_rule_select1ql (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  ORC_ASM_CODE(p,"  shrn %s, %s, #%d\n",
      orc_neon64_reg_name_vector (p->vars[insn->dest_args[0]].alloc, 8, 0),
      orc_neon64_reg_name_vector (p->vars[insn->src_args[0]].alloc, 8, 1), 32);
  orc_neon64_emit_unary (p, "shrn", 0x0f208400,
      p->vars[insn->dest_args[0]],
      p->vars[insn->src_args[0]], p->insn_shift);
}

void
orc_neon64_rule_convhwb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  ORC_ASM_CODE(p,"  shrn %s, %s, #%d\n",
      orc_neon64_reg_name_vector (p->vars[insn->dest_args[0]].alloc, 8, 0),
      orc_neon64_reg_name_vector (p->vars[insn->src_args[0]].alloc, 8, 1), 8);
  orc_neon64_emit_unary (p, "shrn", 0x0f088400,
      p->vars[insn->dest_args[0]],
      p->vars[insn->src_args[0]], p->insn_shift);
}

void
orc_neon64_rule_convhlw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  ORC_ASM_CODE(p,"  shrn %s, %s\n",
      orc_neon64_reg_name_vector (p->vars[insn->dest_args[0]].alloc, 8, 0),
      orc_neon64_reg_name_vector (p->vars[insn->src_args[0]].alloc, 8, 1));
  orc_neon64_emit_unary (p, "shrn", 0x0f108400,
      p->vars[insn->dest_args[0]],
      p->vars[insn->src_args[0]], p->insn_shift);
}

void
orc_neon64_rule_mergebw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->insn_shift <= 2) {
    orc_neon64_emit_binary (p, "zip1", 0x0e003800,
        p->vars[insn->dest_args[0]],
        p->vars[insn->src_args[0]],
        p->vars[insn->src_args[1]], p->insn_shift);
  } else {
    orc_neon64_emit_binary (p, "zip1", 0x0e003800,
        p->vars[insn->dest_args[0]],
        p->vars[insn->src_args[0]],
        p->vars[insn->src_args[1]], p->insn_shift - 1);
  }
}

void
orc_neon64_rule_mergewl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->insn_shift <= 1) {
    orc_neon64_emit_binary (p, "zip1", 0x0e403800,
        p->vars[insn->dest_args[0]],
        p->vars[insn->src_args[0]],
        p->vars[insn->src_args[1]], p->insn_shift);
  } else {
    orc_neon64_emit_binary (p, "zip1", 0x0e403800,
        p->vars[insn->dest_args[0]],
        p->vars[insn->src_args[0]],
        p->vars[insn->src_args[1]], p->insn_shift - 1);
  }
}

void
orc_neon64_rule_mergelq (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->insn_shift <= 0) {
    orc_neon64_emit_binary (p, "trn1", 0x0e802800,
        p->vars[insn->dest_args[0]],
        p->vars[insn->src_args[0]],
        p->vars[insn->src_args[1]], p->insn_shift);
  } else {
    orc_neon64_emit_binary (p, "zip1", 0x0e803800,
        p->vars[insn->dest_args[0]],
        p->vars[insn->src_args[0]],
        p->vars[insn->src_args[1]], p->insn_shift - 1);
  }
}

void
orc_neon64_rule_splatbw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  orc_neon64_emit_binary (p, "zip1", 0x0e003800,
      p->vars[insn->dest_args[0]],
      p->vars[insn->src_args[0]],
      p->vars[insn->src_args[0]], p->insn_shift - (p->insn_shift > 2));
}

void
orc_neon64_rule_splatbl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->dest_args[0]].size };

  orc_neon64_emit_binary (p, "zip1", 0x0e003800,
      tmpreg,
      p->vars[insn->src_args[0]],
      p->vars[insn->src_args[0]], p->insn_shift - (p->insn_shift > 1));
  orc_neon64_emit_binary (p, "zip1", 0x0e403800,
      p->vars[insn->dest_args[0]],
      tmpreg,
      tmpreg, p->insn_shift - (p->insn_shift > 1));
}

void
orc_neon64_rule_splatw3q (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->dest_args[0]].size };
  orc_neon64_emit_binary (p, "trn2", 0x0e406800,
      tmpreg,
      p->vars[insn->src_args[0]],
      p->vars[insn->src_args[0]], p->insn_shift - (p->insn_shift > 0));
  orc_neon64_emit_binary (p, "trn2", 0x0e806800,
      p->vars[insn->dest_args[0]],
      tmpreg,
      tmpreg, p->insn_shift - (p->insn_shift > 0));
}

void
orc_neon64_rule_accsadubl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->src_args[0]].size };

  if (p->insn_shift < 2) {
    orc_neon64_emit_binary (p, "uabdl", 0x2e207000,
        tmpreg,
        p->vars[insn->src_args[0]],
        p->vars[insn->src_args[1]], p->insn_shift);
    orc_neon64_emit_unary (p, "shl",
        0x0f405400 | ((64 - (16<<p->insn_shift)) << 16),
        tmpreg, tmpreg,
  p->insn_shift - 1);
    orc_neon64_emit_unary (p, "uadalp", 0x2e606800,
        p->vars[insn->dest_args[0]],
        tmpreg, p->insn_shift);
  } else {
    orc_neon64_emit_binary (p, "uabdl", 0x2e207000,
        tmpreg,
        p->vars[insn->src_args[0]],
        p->vars[insn->src_args[1]], p->insn_shift);
    orc_neon64_emit_unary (p, "uadalp", 0x2e606800,
        p->vars[insn->dest_args[0]],
        tmpreg, p->insn_shift);
  }
}

void
orc_neon64_rule_signw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->src_args[0]].size };

  /* slow */

  orc_neon64_emit_loadiw (p, &tmpreg, 1);
  if (p->insn_shift < 3) {
    orc_neon64_emit_binary (p, "smin", 0x0e606c00,
        p->vars[insn->dest_args[0]],
        tmpreg,
        p->vars[insn->src_args[0]], p->insn_shift);
  } else {
    orc_neon64_emit_binary (p, "smin", 0x0e606c00,
        p->vars[insn->dest_args[0]],
        tmpreg,
        p->vars[insn->src_args[0]], p->insn_shift - 1);
  }
  orc_neon64_emit_loadiw (p, &tmpreg, -1);
  if (p->insn_shift < 3) {
    orc_neon64_emit_binary (p, "smax", 0x0e606400,
        p->vars[insn->dest_args[0]],
        tmpreg,
        p->vars[insn->dest_args[0]], p->insn_shift);
  } else {
    orc_neon64_emit_binary (p, "smax", 0x0e606400,
        p->vars[insn->dest_args[0]],
        tmpreg,
        p->vars[insn->dest_args[0]], p->insn_shift - 1);
  }
}

void
orc_neon64_rule_signb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->src_args[0]].size };

  /* slow */

  orc_neon64_emit_loadib (p, &tmpreg, 1);
  if (p->insn_shift < 4) {
    orc_neon64_emit_binary (p, "smin", 0x0e206c00,
        p->vars[insn->dest_args[0]],
        tmpreg,
        p->vars[insn->src_args[0]], p->insn_shift);
  } else {
    orc_neon64_emit_binary (p, "smin", 0x0e206c00,
        p->vars[insn->dest_args[0]],
        tmpreg,
        p->vars[insn->src_args[0]], p->insn_shift - 1);
  }
  orc_neon64_emit_loadib (p, &tmpreg, -1);
  if (p->insn_shift < 4) {
    orc_neon64_emit_binary (p, "smax", 0x0e206400,
        p->vars[insn->dest_args[0]],
        tmpreg,
        p->vars[insn->dest_args[0]], p->insn_shift);
  } else {
    orc_neon64_emit_binary (p, "smax", 0x0e206400,
        p->vars[insn->dest_args[0]],
        tmpreg,
        p->vars[insn->dest_args[0]], p->insn_shift - 1);
  }
}

void
orc_neon64_rule_signl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg = { .alloc = p->tmpreg, .size = p->vars[insn->src_args[0]].size };

  /* slow */

  orc_neon64_emit_loadil (p, &tmpreg, 1);
  if (p->insn_shift < 2) {
    orc_neon64_emit_binary (p, "smin", 0x0ea06c00,
        p->vars[insn->dest_args[0]],
        tmpreg,
        p->vars[insn->src_args[0]], p->insn_shift);
  } else {
    orc_neon64_emit_binary (p, "smin", 0x0ea06c00,
        p->vars[insn->dest_args[0]],
        tmpreg,
        p->vars[insn->src_args[0]], p->insn_shift - 1);
  }
  orc_neon64_emit_loadil (p, &tmpreg, -1);
  if (p->insn_shift < 2) {
    orc_neon64_emit_binary (p, "smax", 0x0ea06400,
        p->vars[insn->dest_args[0]],
        tmpreg,
        p->vars[insn->dest_args[0]], p->insn_shift);
  } else {
    orc_neon64_emit_binary (p, "smax", 0x0ea06400,
        p->vars[insn->dest_args[0]],
        tmpreg,
        p->vars[insn->dest_args[0]], p->insn_shift - 1);
  }
}

void
orc_neon64_rule_mulhub (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg1 = { .alloc = p->tmpreg, .size = p->vars[insn->dest_args[0]].size };
  OrcVariable tmpreg2 = { .alloc = p->tmpreg2, .size = p->vars[insn->dest_args[0]].size };
  orc_neon64_emit_binary (p, "umull", 0x2e20c000,
      tmpreg1,
      p->vars[insn->src_args[0]],
      p->vars[insn->src_args[1]],
      p->insn_shift);
  if (p->insn_shift == 4) {
    orc_neon64_emit_binary (p, "umull", 0x2e20c000,
        tmpreg2,
        p->vars[insn->src_args[0]],
        p->vars[insn->src_args[1]],
        p->insn_shift - 1);
  }
  /*
   * WARNING:
   *   Be careful here, SHRN (without Q-bit set) will write bottom 64 bits
   *   of the $dest register with data and top 64 bits with zeroes! SHRN2
   *   (with Q-bit set) will write top 64 bits of $dest register with data
   *   and will retain bottom 64 bits content. If $dest==$src{1 or 2}, then
   *   using SHRN will lead to corruption of source data!
   */
  orc_neon64_emit_unary (p, "shrn", 0x0f088400,
      p->vars[insn->dest_args[0]],
      tmpreg1, p->insn_shift);
  if (p->insn_shift == 4) {
    orc_neon64_emit_unary (p, "shrn", 0x0f088400,
        p->vars[insn->dest_args[0]],
        tmpreg2, p->insn_shift - 1);
  }
}

void
orc_neon64_rule_mulhsb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg1 = { .alloc = p->tmpreg, .size = p->vars[insn->dest_args[0]].size };
  OrcVariable tmpreg2 = { .alloc = p->tmpreg2, .size = p->vars[insn->dest_args[0]].size };
  orc_neon64_emit_binary (p, "smull", 0x0e20c000,
      tmpreg1,
      p->vars[insn->src_args[0]],
      p->vars[insn->src_args[1]],
      p->insn_shift);
  if (p->insn_shift == 4) {
    orc_neon64_emit_binary (p, "smull", 0x0e20c000,
        tmpreg2,
        p->vars[insn->src_args[0]],
        p->vars[insn->src_args[1]],
        p->insn_shift - 1);
  }
  /*
   * WARNING:
   *   Be careful here, SHRN (without Q-bit set) will write bottom 64 bits
   *   of the $dest register with data and top 64 bits with zeroes! SHRN2
   *   (with Q-bit set) will write top 64 bits of $dest register with data
   *   and will retain bottom 64 bits content. If $dest==$src{1 or 2}, then
   *   using SHRN will lead to corruption of source data!
   */
  orc_neon64_emit_unary (p, "shrn", 0x0f088400,
      p->vars[insn->dest_args[0]],
      tmpreg1, p->insn_shift);
  if (p->insn_shift == 4) {
    orc_neon64_emit_unary (p, "shrn", 0x0f088400,
        p->vars[insn->dest_args[0]],
        tmpreg2, p->insn_shift - 1);
  }
}

void
orc_neon64_rule_mulhuw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg1 = { .alloc = p->tmpreg, .size = p->vars[insn->dest_args[0]].size };
  OrcVariable tmpreg2 = { .alloc = p->tmpreg2, .size = p->vars[insn->dest_args[0]].size };
  orc_neon64_emit_binary (p, "umull", 0x2e60c000,
      tmpreg1,
      p->vars[insn->src_args[0]],
      p->vars[insn->src_args[1]],
      p->insn_shift);
  if (p->insn_shift == 3) {
    orc_neon64_emit_binary (p, "umull", 0x2e60c000,
        tmpreg2,
        p->vars[insn->src_args[0]],
        p->vars[insn->src_args[1]],
        p->insn_shift - 1);
  }
  /*
   * WARNING:
   *   Be careful here, SHRN (without Q-bit set) will write bottom 64 bits
   *   of the $dest register with data and top 64 bits with zeroes! SHRN2
   *   (with Q-bit set) will write top 64 bits of $dest register with data
   *   and will retain bottom 64 bits content. If $dest==$src{1 or 2}, then
   *   using SHRN will lead to corruption of source data!
   */
  orc_neon64_emit_unary (p, "shrn", 0x0f108400,
      p->vars[insn->dest_args[0]],
      tmpreg1, p->insn_shift);
  if (p->insn_shift == 3) {
    orc_neon64_emit_unary (p, "shrn", 0x0f108400,
        p->vars[insn->dest_args[0]],
        tmpreg2, p->insn_shift - 1);
  }
}

void
orc_neon64_rule_mulhsw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg1 = { .alloc = p->tmpreg, .size = p->vars[insn->dest_args[0]].size };
  OrcVariable tmpreg2 = { .alloc = p->tmpreg2, .size = p->vars[insn->dest_args[0]].size };
  orc_neon64_emit_binary (p, "smull", 0x0e60c000,
      tmpreg1,
      p->vars[insn->src_args[0]],
      p->vars[insn->src_args[1]],
      p->insn_shift);
  if (p->insn_shift == 3) {
    orc_neon64_emit_binary (p, "smull", 0x0e60c000,
        tmpreg2,
        p->vars[insn->src_args[0]],
        p->vars[insn->src_args[1]],
        p->insn_shift - 1);
  }
  /*
   * WARNING:
   *   Be careful here, SHRN (without Q-bit set) will write bottom 64 bits
   *   of the $dest register with data and top 64 bits with zeroes! SHRN2
   *   (with Q-bit set) will write top 64 bits of $dest register with data
   *   and will retain bottom 64 bits content. If $dest==$src{1 or 2}, then
   *   using SHRN will lead to corruption of source data!
   */
  orc_neon64_emit_unary (p, "shrn", 0x0f108400,
      p->vars[insn->dest_args[0]],
      tmpreg1, p->insn_shift);
  if (p->insn_shift == 3) {
    orc_neon64_emit_unary (p, "shrn", 0x0f108400,
        p->vars[insn->dest_args[0]],
        tmpreg2, p->insn_shift - 1);
  }
}

void
orc_neon64_rule_mulhul (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg1 = { .alloc = p->tmpreg, .size = p->vars[insn->dest_args[0]].size };
  OrcVariable tmpreg2 = { .alloc = p->tmpreg2, .size = p->vars[insn->dest_args[0]].size };
  orc_neon64_emit_binary (p, "umull", 0x2ea0c000,
      tmpreg1,
      p->vars[insn->src_args[0]],
      p->vars[insn->src_args[1]],
      p->insn_shift);
  if (p->insn_shift == 2) {
    orc_neon64_emit_binary (p, "umull", 0x2ea0c000,
        tmpreg2,
        p->vars[insn->src_args[0]],
        p->vars[insn->src_args[1]],
        p->insn_shift - 1);
  }
  /*
   * WARNING:
   *   Be careful here, SHRN (without Q-bit set) will write bottom 64 bits
   *   of the $dest register with data and top 64 bits with zeroes! SHRN2
   *   (with Q-bit set) will write top 64 bits of $dest register with data
   *   and will retain bottom 64 bits content. If $dest==$src{1 or 2}, then
   *   using SHRN will lead to corruption of source data!
   */
  orc_neon64_emit_unary (p, "shrn", 0x0f208400,
      p->vars[insn->dest_args[0]],
      tmpreg1, p->insn_shift);
  if (p->insn_shift == 2) {
    orc_neon64_emit_unary (p, "shrn", 0x0f208400,
        p->vars[insn->dest_args[0]],
        tmpreg2, p->insn_shift - 1);
  }
}

void
orc_neon64_rule_mulhsl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  OrcVariable tmpreg1 = { .alloc = p->tmpreg, .size = p->vars[insn->dest_args[0]].size };
  OrcVariable tmpreg2 = { .alloc = p->tmpreg2, .size = p->vars[insn->dest_args[0]].size };
  orc_neon64_emit_binary (p, "smull", 0x0ea0c000,
      tmpreg1,
      p->vars[insn->src_args[0]],
      p->vars[insn->src_args[1]],
      p->insn_shift);
  if (p->insn_shift == 2) {
    orc_neon64_emit_binary (p, "smull", 0x0ea0c000,
        tmpreg2,
        p->vars[insn->src_args[0]],
        p->vars[insn->src_args[1]],
        p->insn_shift - 1);
  }
  /*
   * WARNING:
   *   Be careful here, SHRN (without Q-bit set) will write bottom 64 bits
   *   of the $dest register with data and top 64 bits with zeroes! SHRN2
   *   (with Q-bit set) will write top 64 bits of $dest register with data
   *   and will retain bottom 64 bits content. If $dest==$src{1 or 2}, then
   *   using SHRN will lead to corruption of source data!
   */
  orc_neon64_emit_unary (p, "shrn", 0x0f208400,
      p->vars[insn->dest_args[0]],
      tmpreg1, p->insn_shift);
  if (p->insn_shift == 2) {
    orc_neon64_emit_unary (p, "shrn", 0x0f208400,
        p->vars[insn->dest_args[0]],
        tmpreg2, p->insn_shift - 1);
  }
}

void
orc_neon64_rule_splitql (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int dest0 = p->vars[insn->dest_args[0]].alloc;
  int src = p->vars[insn->src_args[0]].alloc;

  if (src != dest0) {
    orc_neon64_emit_binary (p, "uzp2", 0x0e805800,
        p->vars[insn->dest_args[0]], p->vars[insn->src_args[0]],
        p->vars[insn->src_args[0]], p->insn_shift - (p->insn_shift >= 1));
    orc_neon64_emit_binary (p, "uzp1", 0x0e801800,
        p->vars[insn->dest_args[1]], p->vars[insn->src_args[0]],
        p->vars[insn->src_args[0]], p->insn_shift - (p->insn_shift >= 1));
  } else {
    orc_neon64_emit_binary (p, "uzp1", 0x0e801800,
        p->vars[insn->dest_args[1]], p->vars[insn->src_args[0]],
        p->vars[insn->src_args[0]], p->insn_shift - (p->insn_shift >= 1));
    orc_neon64_emit_binary (p, "uzp2", 0x0e805800,
        p->vars[insn->dest_args[0]], p->vars[insn->src_args[0]],
        p->vars[insn->src_args[0]], p->insn_shift - (p->insn_shift >= 1));
  }
}

void
orc_neon64_rule_splitlw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int dest0 = p->vars[insn->dest_args[0]].alloc;
  int src = p->vars[insn->src_args[0]].alloc;

  if (src != dest0) {
    orc_neon64_emit_binary (p, "uzp2", 0x0e405800,
        p->vars[insn->dest_args[0]], p->vars[insn->src_args[0]],
        p->vars[insn->src_args[0]], p->insn_shift - (p->insn_shift >= 2));
    orc_neon64_emit_binary (p, "uzp1", 0x0e401800,
        p->vars[insn->dest_args[1]], p->vars[insn->src_args[0]],
        p->vars[insn->src_args[0]], p->insn_shift - (p->insn_shift >= 2));
  } else {
    orc_neon64_emit_binary (p, "uzp1", 0x0e401800,
        p->vars[insn->dest_args[1]], p->vars[insn->src_args[0]],
        p->vars[insn->src_args[0]], p->insn_shift - (p->insn_shift >= 2));
    orc_neon64_emit_binary (p, "uzp2", 0x0e405800,
        p->vars[insn->dest_args[0]], p->vars[insn->src_args[0]],
        p->vars[insn->src_args[0]], p->insn_shift - (p->insn_shift >= 2));
  }
}

void
orc_neon64_rule_splitwb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int dest0 = p->vars[insn->dest_args[0]].alloc;
  int src = p->vars[insn->src_args[0]].alloc;

  if (src != dest0) {
    orc_neon64_emit_binary (p, "uzp2", 0x0e005800,
        p->vars[insn->dest_args[0]], p->vars[insn->src_args[0]],
        p->vars[insn->src_args[0]], p->insn_shift - (p->insn_shift >= 2));
    orc_neon64_emit_binary (p, "uzp1", 0x0e001800,
        p->vars[insn->dest_args[1]], p->vars[insn->src_args[0]],
        p->vars[insn->src_args[0]], p->insn_shift - (p->insn_shift >= 2));
  } else {
    orc_neon64_emit_binary (p, "uzp1", 0x0e001800,
        p->vars[insn->dest_args[1]], p->vars[insn->src_args[0]],
        p->vars[insn->src_args[0]], p->insn_shift - (p->insn_shift >= 2));
    orc_neon64_emit_binary (p, "uzp2", 0x0e005800,
        p->vars[insn->dest_args[0]], p->vars[insn->src_args[0]],
        p->vars[insn->src_args[0]], p->insn_shift - (p->insn_shift >= 2));
  }
}

void
orc_neon64_rule_div255w (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcVariable dest = p->vars[insn->dest_args[0]];
  const OrcVariable src = p->vars[insn->src_args[0]];
  OrcVariable tmp1 = { .alloc = p->tmpreg2, .size = src.size * 2 };
  OrcVariable tmp2 = { .alloc = p->tmpreg, .size = src.size };
  orc_neon64_emit_loadiw (p, &tmp2, 0x8081);

  {
    // Unfortunately, this opcode requires quad for the destination
    // so we can't use orc_neon64_emit_binary
    ORC_ASM_CODE (p, "  %s %s, %s, %s\n", "umull",
        orc_neon64_reg_name_vector (tmp1.alloc, tmp1.size, 1),
        orc_neon64_reg_name_vector (src.alloc, src.size, 0),
        orc_neon64_reg_name_vector (tmp2.alloc, tmp2.size, 0));
    int code = 0x2e60c000;
    code |= (tmp2.alloc & 0x1f) << 16;
    code |= (src.alloc & 0x1f) << 5;
    code |= (tmp1.alloc & 0x1f);
    orc_arm_emit (p, code);
  }
  {
    // vreinterpret dest here, it'll be fixed by uzp2
    const OrcVariable dest_i32 = { .alloc = tmp2.alloc, .size = dest.size * 2 };
    orc_neon64_emit_binary (p, "umull2", 0x2e60c000, dest_i32, src, tmp2,
        p->insn_shift - 1);
  }
  {
    // vreinterpret src_2, it needs full quad width
    const OrcVariable tmp1_i64 = {.alloc = tmp1.alloc, .size = dest.size };
    const OrcVariable tmp2_i64 = {.alloc = tmp2.alloc, .size = dest.size };
    const OrcVariable dest_i64 = { .alloc = dest.alloc, .size = dest.size };
    orc_neon64_emit_binary (p, "uzp2", 0x0e405800, dest_i64, tmp1_i64, tmp2_i64,
        p->insn_shift - 1);
  }
  orc_neon64_emit_shift (p, 5, &dest, &src, 7, 1);
}
