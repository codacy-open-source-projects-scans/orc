
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

void
orc_neon_preload (OrcCompiler *compiler, OrcVariable *var, int write,
    int offset)
{
  orc_uint32 code;

  /* Don't use multiprocessing extensions */
  write = FALSE;

  ORC_ASM_CODE(compiler,"  pld%s [%s, #%d]\n",
      write ? "w" : "",
      orc_arm_reg_name (var->ptr_register), offset);
  code = 0xf510f000;
  if (!write) code |= (1<<22);
  code |= (var->ptr_register&0xf) << 16;
  if (offset < 0) {
    code |= ((-offset)&0xfff) << 0;
  } else {
    code |= (offset&0xfff) << 0;
    code |= (1<<23);
  }
  orc_arm_emit (compiler, code);
}

static void
orc_neon_unary_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn *const insn32 = &orc_neon32_insns[(long int)user];
  const OrcNeonInsn *const insn64 = &orc_neon64_insns[(long int)user];

  if (p->is_64bit)
    orc_neon64_unary_rule (p, (void *)insn64, insn);
  else
    orc_neon32_unary_rule (p, (void *)insn32, insn);
}

static void
orc_neon_unary_long_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn *const insn32 = &orc_neon32_insns[(long int)user];
  const OrcNeonInsn *const insn64 = &orc_neon64_insns[(long int)user];

  if (p->is_64bit)
    orc_neon64_unary_rule (p, (void *)insn64, insn);
  else
    orc_neon32_unary_long_rule (p, (void *)insn32, insn);
}

static void
orc_neon_unary_narrow_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn *const insn32 = &orc_neon32_insns[(long int)user];
  const OrcNeonInsn *const insn64 = &orc_neon64_insns[(long int)user];

  if (p->is_64bit)
    orc_neon64_unary_rule (p, (void *)insn64, insn);
  else
    orc_neon32_unary_narrow_rule (p, (void *)insn32, insn);
}

static void
orc_neon_binary_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn *const insn32 = &orc_neon32_insns[(long int)user];
  const OrcNeonInsn *const insn64 = &orc_neon64_insns[(long int)user];

  if (p->is_64bit)
    orc_neon64_binary_rule (p, (void *)insn64, insn);
  else
    orc_neon32_binary_rule (p, (void *)insn32, insn);
}

static void
orc_neon_binary_long_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn *const insn32 = &orc_neon32_insns[(long int)user];
  const OrcNeonInsn *const insn64 = &orc_neon64_insns[(long int)user];

  if (p->is_64bit)
    orc_neon64_binary_rule (p, (void *)insn64, insn);
  else
    orc_neon32_binary_long_rule (p, (void *)insn32, insn);
}

static void
orc_neon_move_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn *const insn32 = &orc_neon32_insns[(long int)user];
  const OrcNeonInsn *const insn64 = &orc_neon64_insns[(long int)user];

  if (p->is_64bit)
    orc_neon64_move_rule (p, (void *)insn64, insn);
  else
    orc_neon32_move_rule (p, (void *)insn32, insn);
}

static void
orc_neon_unary_vfp_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn *const insn32 = &orc_neon32_insns[(long int)user];
  const OrcNeonInsn *const insn64 = &orc_neon64_insns[(long int)user];

  if (p->is_64bit)
    orc_neon64_unary_rule (p, (void *)insn64, insn);
  else
    orc_neon32_unary_vfp_rule (p, (void *)insn32, insn);
}

static void
orc_neon_binary_vfp_rule (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const OrcNeonInsn *const insn32 = &orc_neon32_insns[(long int)user];
  const OrcNeonInsn *const insn64 = &orc_neon64_insns[(long int)user];

  if (p->is_64bit)
    orc_neon64_binary_rule (p, (void *)insn64, insn);
  else
    orc_neon32_binary_vfp_rule (p, (void *)insn32, insn);
}

static void
orc_neon_rule_loadupdb (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  if (compiler->is_64bit)
    orc_neon64_rule_loadupdb (compiler, user, insn);
  else
    orc_neon32_rule_loadupdb (compiler, user, insn);
}

static void
orc_neon_rule_loadpX (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  if (compiler->is_64bit)
    orc_neon64_rule_loadpX (compiler, user, insn);
  else
    orc_neon32_rule_loadpX (compiler, user, insn);
}

static void
orc_neon_rule_loadX (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  if (compiler->is_64bit)
    orc_neon64_rule_loadX (compiler, user, insn);
  else
    orc_neon32_rule_loadX (compiler, user, insn);
}

static void
orc_neon_rule_storeX (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  if (compiler->is_64bit)
    orc_neon64_rule_storeX (compiler, user, insn);
  else
    orc_neon32_rule_storeX (compiler, user, insn);
}

static void
orc_neon_rule_shift (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_shift (p, user, insn);
  else
    orc_neon32_rule_shift (p, user, insn);
}

static void
orc_neon_rule_andn (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_andn (p, user, insn);
  else
    orc_neon32_rule_andn (p, user, insn);
}

static const OrcNeonRule orc_neon_rules[] = {
  [ORC_NEON_OP_ABSB]      = { "absb"     , orc_neon_unary_rule        },
  [ORC_NEON_OP_ADDB]      = { "addb"     , orc_neon_binary_rule       },
  [ORC_NEON_OP_ADDSSB]    = { "addssb"   , orc_neon_binary_rule       },
  [ORC_NEON_OP_ADDUSB]    = { "addusb"   , orc_neon_binary_rule       },
  [ORC_NEON_OP_ANDB]      = { "andb"     , orc_neon_binary_rule       },
/*[ORC_NEON_OP_ANDNB]     = { "andnb"    , orc_neon_binary_rule       }, */
  [ORC_NEON_OP_AVGSB]     = { "avgsb"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_AVGUB]     = { "avgub"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_CMPEQB]    = { "cmpeqb"   , orc_neon_binary_rule       },
  [ORC_NEON_OP_CMPGTSB]   = { "cmpgtsb"  , orc_neon_binary_rule       },
  [ORC_NEON_OP_COPYB]     = { "copyb"    , orc_neon_move_rule         },
  [ORC_NEON_OP_MAXSB]     = { "maxsb"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_MAXUB]     = { "maxub"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_MINSB]     = { "minsb"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_MINUB]     = { "minub"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_MULLB]     = { "mullb"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_ORB]       = { "orb"      , orc_neon_binary_rule       },
/*[ORC_NEON_OP_SHLB]      = { "shlb"     , orc_neon_lshift_rule       }, */
/*[ORC_NEON_OP_SHRSB]     = { "shrsb"    , orc_neon_rshift_rule       }, */
/*[ORC_NEON_OP_SHRUB]     = { "shrub"    , orc_neon_rshift_rule       }, */
  [ORC_NEON_OP_SUBB]      = { "subb"     , orc_neon_binary_rule       },
  [ORC_NEON_OP_SUBSSB]    = { "subssb"   , orc_neon_binary_rule       },
  [ORC_NEON_OP_SUBUSB]    = { "subusb"   , orc_neon_binary_rule       },
  [ORC_NEON_OP_XORB]      = { "xorb"     , orc_neon_binary_rule       },
  [ORC_NEON_OP_ABSW]      = { "absw"     , orc_neon_unary_rule        },
  [ORC_NEON_OP_ADDW]      = { "addw"     , orc_neon_binary_rule       },
  [ORC_NEON_OP_ADDSSW]    = { "addssw"   , orc_neon_binary_rule       },
  [ORC_NEON_OP_ADDUSW]    = { "addusw"   , orc_neon_binary_rule       },
  [ORC_NEON_OP_ANDW]      = { "andw"     , orc_neon_binary_rule       },
/*[ORC_NEON_OP_ANDNW]     = { "andnw"    , orc_neon_binary_rule       }, */
  [ORC_NEON_OP_AVGSW]     = { "avgsw"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_AVGUW]     = { "avguw"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_CMPEQW]    = { "cmpeqw"   , orc_neon_binary_rule       },
  [ORC_NEON_OP_CMPGTSW]   = { "cmpgtsw"  , orc_neon_binary_rule       },
  [ORC_NEON_OP_COPYW]     = { "copyw"    , orc_neon_move_rule         },
  [ORC_NEON_OP_MAXSW]     = { "maxsw"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_MAXUW]     = { "maxuw"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_MINSW]     = { "minsw"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_MINUW]     = { "minuw"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_MULLW]     = { "mullw"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_ORW]       = { "orw"      , orc_neon_binary_rule       },
/*[ORC_NEON_OP_SHLW]      = { "shlw"     , orc_neon_lshift_rule       }, */
/*[ORC_NEON_OP_SHRSW]     = { "shrsw"    , orc_neon_rshift_rule       }, */
/*[ORC_NEON_OP_SHRUW]     = { "shruw"    , orc_neon_rshift_rule       }, */
  [ORC_NEON_OP_SUBW]      = { "subw"     , orc_neon_binary_rule       },
  [ORC_NEON_OP_SUBSSW]    = { "subssw"   , orc_neon_binary_rule       },
  [ORC_NEON_OP_SUBUSW]    = { "subusw"   , orc_neon_binary_rule       },
  [ORC_NEON_OP_XORW]      = { "xorw"     , orc_neon_binary_rule       },
  [ORC_NEON_OP_ABSL]      = { "absl"     , orc_neon_unary_rule        },
  [ORC_NEON_OP_ADDL]      = { "addl"     , orc_neon_binary_rule       },
  [ORC_NEON_OP_ADDSSL]    = { "addssl"   , orc_neon_binary_rule       },
  [ORC_NEON_OP_ADDUSL]    = { "addusl"   , orc_neon_binary_rule       },
  [ORC_NEON_OP_ANDL]      = { "andl"     , orc_neon_binary_rule       },
/*[ORC_NEON_OP_ANDNL]     = { "andnl"    , orc_neon_binary_rule       }, */
  [ORC_NEON_OP_AVGSL]     = { "avgsl"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_AVGUL]     = { "avgul"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_CMPEQL]    = { "cmpeql"   , orc_neon_binary_rule       },
  [ORC_NEON_OP_CMPGTSL]   = { "cmpgtsl"  , orc_neon_binary_rule       },
  [ORC_NEON_OP_COPYL]     = { "copyl"    , orc_neon_move_rule         },
  [ORC_NEON_OP_MAXSL]     = { "maxsl"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_MAXUL]     = { "maxul"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_MINSL]     = { "minsl"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_MINUL]     = { "minul"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_MULLL]     = { "mulll"    , orc_neon_binary_rule       },
  [ORC_NEON_OP_ORL]       = { "orl"      , orc_neon_binary_rule       },
/*[ORC_NEON_OP_SHLL]      = { "shll"     , orc_neon_lshift_rule       }, */
/*[ORC_NEON_OP_SHRSL]     = { "shrsl"    , orc_neon_rshift_rule       }, */
/*[ORC_NEON_OP_SHRUL]     = { "shrul"    , orc_neon_rshift_rule       }, */
  [ORC_NEON_OP_SUBL]      = { "subl"     , orc_neon_binary_rule       },
  [ORC_NEON_OP_SUBSSL]    = { "subssl"   , orc_neon_binary_rule       },
  [ORC_NEON_OP_SUBUSL]    = { "subusl"   , orc_neon_binary_rule       },
  [ORC_NEON_OP_XORL]      = { "xorl"     , orc_neon_binary_rule       },
/*[ORC_NEON_OP_ABSQ]      = { "absq"     , orc_neon_unary_rule        }, */
  [ORC_NEON_OP_ADDQ]      = { "addq"     , orc_neon_binary_rule       },
/*[ORC_NEON_OP_ADDSSQ]    = { "addssq"   , orc_neon_binary_rule       }, */
/*[ORC_NEON_OP_ADDUSQ]    = { "addusq"   , orc_neon_binary_rule       }, */
  [ORC_NEON_OP_ANDQ]      = { "andq"     , orc_neon_binary_rule       },
/*[ORC_NEON_OP_AVGSQ]     = { "avgsq"    , orc_neon_binary_rule       }, */
/*[ORC_NEON_OP_AVGUQ]     = { "avguq"    , orc_neon_binary_rule       }, */
/*[ORC_NEON_OP_CMPEQQ]    = { "cmpeqq"   , orc_neon_binary_rule       }, */
/*[ORC_NEON_OP_CMPGTSQ]   = { "cmpgtsq"  , orc_neon_binary_rule       }, */
  [ORC_NEON_OP_COPYQ]     = { "copyq"    , orc_neon_move_rule         },
/*[ORC_NEON_OP_MAXSQ]     = { "maxsq"    , orc_neon_binary_rule       }, */
/*[ORC_NEON_OP_MAXUQ]     = { "maxuq"    , orc_neon_binary_rule       }, */
/*[ORC_NEON_OP_MINSQ]     = { "minsq"    , orc_neon_binary_rule       }, */
/*[ORC_NEON_OP_MINUQ]     = { "minuq"    , orc_neon_binary_rule       }, */
/*[ORC_NEON_OP_MULLQ]     = { "mullq"    , orc_neon_binary_rule       }, */
  [ORC_NEON_OP_ORQ]       = { "orq"      , orc_neon_binary_rule       },
  [ORC_NEON_OP_SUBQ]      = { "subq"     , orc_neon_binary_rule       },
/*[ORC_NEON_OP_SUBSSQ]    = { "subssq"   , orc_neon_binary_rule       }, */
/*[ORC_NEON_OP_SUBUSQ]    = { "subusq"   , orc_neon_binary_rule       }, */
  [ORC_NEON_OP_XORQ]      = { "xorq"     , orc_neon_binary_rule       },
  [ORC_NEON_OP_CONVSBW]   = { "convsbw"  , orc_neon_unary_long_rule   },
  [ORC_NEON_OP_CONVUBW]   = { "convubw"  , orc_neon_unary_long_rule   },
  [ORC_NEON_OP_CONVSWL]   = { "convswl"  , orc_neon_unary_long_rule   },
  [ORC_NEON_OP_CONVUWL]   = { "convuwl"  , orc_neon_unary_long_rule   },
  [ORC_NEON_OP_CONVSLQ]   = { "convslq"  , orc_neon_unary_long_rule   },
  [ORC_NEON_OP_CONVULQ]   = { "convulq"  , orc_neon_unary_long_rule   },
  [ORC_NEON_OP_CONVWB]    = { "convwb"   , orc_neon_unary_narrow_rule },
  [ORC_NEON_OP_CONVSSSWB] = { "convssswb", orc_neon_unary_narrow_rule },
  [ORC_NEON_OP_CONVSUSWB] = { "convsuswb", orc_neon_unary_narrow_rule },
  [ORC_NEON_OP_CONVUUSWB] = { "convuuswb", orc_neon_unary_narrow_rule },
  [ORC_NEON_OP_CONVLW]    = { "convlw"   , orc_neon_unary_narrow_rule },
  [ORC_NEON_OP_CONVQL]    = { "convql"   , orc_neon_unary_narrow_rule },
  [ORC_NEON_OP_CONVSSSLW] = { "convssslw", orc_neon_unary_narrow_rule },
  [ORC_NEON_OP_CONVSUSLW] = { "convsuslw", orc_neon_unary_narrow_rule },
  [ORC_NEON_OP_CONVUUSLW] = { "convuuslw", orc_neon_unary_narrow_rule },
  [ORC_NEON_OP_CONVSSSQL] = { "convsssql", orc_neon_unary_narrow_rule },
  [ORC_NEON_OP_CONVSUSQL] = { "convsusql", orc_neon_unary_narrow_rule },
  [ORC_NEON_OP_CONVUUSQL] = { "convuusql", orc_neon_unary_narrow_rule },
  [ORC_NEON_OP_MULSBW]    = { "mulsbw"   , orc_neon_binary_long_rule  },
  [ORC_NEON_OP_MULUBW]    = { "mulubw"   , orc_neon_binary_long_rule  },
  [ORC_NEON_OP_MULSWL]    = { "mulswl"   , orc_neon_binary_long_rule  },
  [ORC_NEON_OP_MULUWL]    = { "muluwl"   , orc_neon_binary_long_rule  },
  [ORC_NEON_OP_SWAPW]     = { "swapw"    , orc_neon_unary_rule        },
  [ORC_NEON_OP_SWAPL]     = { "swapl"    , orc_neon_unary_rule        },
  [ORC_NEON_OP_SWAPQ]     = { "swapq"    , orc_neon_unary_rule        },
  [ORC_NEON_OP_SWAPWL]    = { "swapwl"   , orc_neon_unary_rule        },
  [ORC_NEON_OP_SWAPLQ]    = { "swaplq"   , orc_neon_unary_rule        },
  [ORC_NEON_OP_SELECT0QL] = { "select0ql", orc_neon_unary_narrow_rule },
  [ORC_NEON_OP_SELECT0LW] = { "select0lw", orc_neon_unary_narrow_rule },
  [ORC_NEON_OP_SELECT0WB] = { "select0wb", orc_neon_unary_narrow_rule },
  [ORC_NEON_OP_ADDF]      = { "addf"     , orc_neon_binary_rule       },
  [ORC_NEON_OP_SUBF]      = { "subf"     , orc_neon_binary_rule       },
  [ORC_NEON_OP_MULF]      = { "mulf"     , orc_neon_binary_rule       },
  [ORC_NEON_OP_MAXF]      = { "maxf"     , orc_neon_binary_rule       },
  [ORC_NEON_OP_MINF]      = { "minf"     , orc_neon_binary_rule       },
  [ORC_NEON_OP_CMPEQF]    = { "cmpeqf"   , orc_neon_binary_rule       },
/*[ORC_NEON_OP_CMPLTF]    = { "cmpltf"   , orc_neon_binary_r_rule     }, */
/*[ORC_NEON_OP_CMPLEF]    = { "cmplef"   , orc_neon_binary_r_rule     }, */
  [ORC_NEON_OP_CONVFL]    = { "convfl"   , orc_neon_unary_rule        },
  [ORC_NEON_OP_CONVLF]    = { "convlf"   , orc_neon_unary_rule        },
  [ORC_NEON_OP_ADDD]      = { "addd"     , orc_neon_binary_vfp_rule   },
  [ORC_NEON_OP_SUBD]      = { "subd"     , orc_neon_binary_vfp_rule   },
  [ORC_NEON_OP_MULD]      = { "muld"     , orc_neon_binary_vfp_rule   },
  [ORC_NEON_OP_DIVD]      = { "divd"     , orc_neon_binary_vfp_rule   },
  [ORC_NEON_OP_DIVF]      = { "divf"     , orc_neon_binary_vfp_rule   },
  [ORC_NEON_OP_SQRTD]     = { "sqrtd"    , orc_neon_unary_vfp_rule    },
  [ORC_NEON_OP_SQRTF]     = { "sqrtf"    , orc_neon_unary_vfp_rule    },
/*[ORC_NEON_OP_CMPEQD]    = { "cmpeqd"   , orc_neon_binary_vfp_rule   }, */
  [ORC_NEON_OP_CONVDF]    = { "convdf"   , orc_neon_unary_vfp_rule    },
  [ORC_NEON_OP_CONVFD]    = { "convfd"   , orc_neon_unary_vfp_rule    },
};

static void
orc_neon_rule_accw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_accw (p, user, insn);
  else
    orc_neon32_rule_accw (p, user, insn);
}

static void
orc_neon_rule_accl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_accl (p, user, insn);
  else
    orc_neon32_rule_accl (p, user, insn);
}

static void
orc_neon_rule_select1wb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_select1wb (p, user, insn);
  else
    orc_neon32_rule_select1wb (p, user, insn);
}

static void
orc_neon_rule_select1lw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_select1lw (p, user, insn);
  else
    orc_neon32_rule_select1lw (p, user, insn);
}

static void
orc_neon_rule_select1ql (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_select1ql (p, user, insn);
  else
    orc_neon32_rule_select1ql (p, user, insn);
}

static void
orc_neon_rule_convhwb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_convhwb (p, user, insn);
  else
    orc_neon32_rule_convhwb (p, user, insn);
}

static void
orc_neon_rule_convhlw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_convhlw (p, user, insn);
  else
    orc_neon32_rule_convhlw (p, user, insn);
}

static void
orc_neon_rule_mergebw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_mergebw (p, user, insn);
  else
    orc_neon32_rule_mergebw (p, user, insn);
}

static void
orc_neon_rule_mergewl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_mergewl (p, user, insn);
  else
    orc_neon32_rule_mergewl (p, user, insn);
}

static void
orc_neon_rule_mergelq (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_mergelq (p, user, insn);
  else
    orc_neon32_rule_mergelq (p, user, insn);
}

static void
orc_neon_rule_splatbw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_splatbw (p, user, insn);
  else
    orc_neon32_rule_splatbw (p, user, insn);
}

static void
orc_neon_rule_splatbl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_splatbl (p, user, insn);
  else
    orc_neon32_rule_splatbl (p, user, insn);
}

static void
orc_neon_rule_splatw3q (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_splatw3q (p, user, insn);
  else
    orc_neon32_rule_splatw3q (p, user, insn);
}

static void
orc_neon_rule_accsadubl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_accsadubl (p, user, insn);
  else
    orc_neon32_rule_accsadubl (p, user, insn);
}

static void
orc_neon_rule_signw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_signw (p, user, insn);
  else
    orc_neon32_rule_signw (p, user, insn);
}

static void
orc_neon_rule_signb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_signb (p, user, insn);
  else
    orc_neon32_rule_signb (p, user, insn);
}

static void
orc_neon_rule_signl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_signl (p, user, insn);
  else
    orc_neon32_rule_signl (p, user, insn);
}

static void
orc_neon_rule_mulhub (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_mulhub (p, user, insn);
  else
    orc_neon32_rule_mulhub (p, user, insn);
}

static void
orc_neon_rule_mulhsb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_mulhsb (p, user, insn);
  else
    orc_neon32_rule_mulhsb (p, user, insn);
}

static void
orc_neon_rule_mulhuw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_mulhuw (p, user, insn);
  else
    orc_neon32_rule_mulhuw (p, user, insn);
}

static void
orc_neon_rule_mulhsw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_mulhsw (p, user, insn);
  else
    orc_neon32_rule_mulhsw (p, user, insn);
}

static void
orc_neon_rule_mulhul (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_mulhul (p, user, insn);
  else
    orc_neon32_rule_mulhul (p, user, insn);
}

static void
orc_neon_rule_mulhsl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_mulhsl (p, user, insn);
  else
    orc_neon32_rule_mulhsl (p, user, insn);
}

static void
orc_neon_rule_splitql (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_splitql (p, user, insn);
  else
    orc_neon32_rule_splitql (p, user, insn);
}

static void
orc_neon_rule_splitlw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_splitlw (p, user, insn);
  else
    orc_neon32_rule_splitlw (p, user, insn);
}

static void
orc_neon_rule_splitwb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_splitwb (p, user, insn);
  else
    orc_neon32_rule_splitwb (p, user, insn);
}

static void
orc_neon_rule_div255w (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->is_64bit)
    orc_neon64_rule_div255w (p, user, insn);
  else
    orc_neon32_rule_div255w (p, user, insn);
}

void
orc_compiler_neon_register_rules (OrcTarget *target)
{
  OrcRuleSet *rule_set;

  rule_set = orc_rule_set_new (orc_opcode_set_get("sys"), target, 0);

  for (long int i = 0; i < sizeof(orc_neon_rules) / sizeof(OrcNeonRule); i++) {
    const OrcNeonRule * const r = &orc_neon_rules[i];
    orc_rule_register (rule_set, r->orc_opcode, r->rule, (void *)i);
  }

#define REG(x) \
    orc_rule_register (rule_set, #x , orc_neon_rule_ ## x, NULL)

  REG(mulhsb);
  REG(mulhub);
  REG(signb);
  REG(mulhsw);
  REG(mulhuw);
  REG(signw);
  REG(mulhsl);
  REG(mulhul);
  REG(signl);
  REG(convhwb);
  REG(convhlw);
  REG(accw);
  REG(accl);
  REG(accsadubl);
  REG(select1wb);
  REG(select1lw);
  REG(select1ql);
  REG(mergebw);
  REG(mergewl);
  REG(mergelq);
  REG(splitql);
  REG(splitlw);
  REG(splitwb);
  REG(splatbw);
  REG(splatbl);
  REG(splatw3q);
  REG(div255w);

  orc_rule_register (rule_set, "loadpb", orc_neon_rule_loadpX, (void *)1);
  orc_rule_register (rule_set, "loadpw", orc_neon_rule_loadpX, (void *)2);
  orc_rule_register (rule_set, "loadpl", orc_neon_rule_loadpX, (void *)4);
  orc_rule_register (rule_set, "loadpq", orc_neon_rule_loadpX, (void *)8);
  orc_rule_register (rule_set, "loadupdb", orc_neon_rule_loadupdb, (void *)0);
  orc_rule_register (rule_set, "loadb", orc_neon_rule_loadX, (void *)0);
  orc_rule_register (rule_set, "loadw", orc_neon_rule_loadX, (void *)0);
  orc_rule_register (rule_set, "loadl", orc_neon_rule_loadX, (void *)0);
  orc_rule_register (rule_set, "loadq", orc_neon_rule_loadX, (void *)0);
  orc_rule_register (rule_set, "loadoffb", orc_neon_rule_loadX, (void *)1);
  orc_rule_register (rule_set, "loadoffw", orc_neon_rule_loadX, (void *)1);
  orc_rule_register (rule_set, "loadoffl", orc_neon_rule_loadX, (void *)1);
  orc_rule_register (rule_set, "storeb", orc_neon_rule_storeX, (void *)0);
  orc_rule_register (rule_set, "storew", orc_neon_rule_storeX, (void *)0);
  orc_rule_register (rule_set, "storel", orc_neon_rule_storeX, (void *)0);
  orc_rule_register (rule_set, "storeq", orc_neon_rule_storeX, (void *)0);

  orc_rule_register (rule_set, "shlb", orc_neon_rule_shift, (void *)0);
  orc_rule_register (rule_set, "shrsb", orc_neon_rule_shift, (void *)1);
  orc_rule_register (rule_set, "shrub", orc_neon_rule_shift, (void *)2);
  orc_rule_register (rule_set, "shlw", orc_neon_rule_shift, (void *)3);
  orc_rule_register (rule_set, "shrsw", orc_neon_rule_shift, (void *)4);
  orc_rule_register (rule_set, "shruw", orc_neon_rule_shift, (void *)5);
  orc_rule_register (rule_set, "shll", orc_neon_rule_shift, (void *)6);
  orc_rule_register (rule_set, "shrsl", orc_neon_rule_shift, (void *)7);
  orc_rule_register (rule_set, "shrul", orc_neon_rule_shift, (void *)8);

  orc_rule_register (rule_set, "andnb", orc_neon_rule_andn, (void *)3);
  orc_rule_register (rule_set, "andnw", orc_neon_rule_andn, (void *)2);
  orc_rule_register (rule_set, "andnl", orc_neon_rule_andn, (void *)1);
  orc_rule_register (rule_set, "andnq", orc_neon_rule_andn, (void *)0);
}

