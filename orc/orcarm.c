
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>

#include <orc/orcdebug.h>
#include <orc/orcprogram.h>
#include <orc/orcutils.h>
#include <orc/orcinternal.h>
#include <orc/orcarm.h>

#if defined(HAVE_ARM) || defined(HAVE_AARCH64)
#if defined(__APPLE__)
#include  <libkern/OSCacheControl.h>
#endif
#if defined (_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#endif

/**
 * SECTION:orcarm
 * @title: ARM
 * @short_description: code generation for ARM
 */

const char *
orc_arm_cond_name (OrcArmCond cond)
{
  static const char *cond_names[] = {
    "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt", "gt", "le", "", ""
  };
  if ((int)cond < 0 || (int)cond >= 16) {
    return "ERROR";
  }
  return cond_names[cond&0xf];
}

const char *
orc_arm_reg_name (int reg)
{
#if 0
  static const char *gp_regs[] = {
    "a1", "a2", "a3", "a4",
    "v1", "v2", "v3", "v4",
    "v5", "v6", "v7", "v8",
    "ip", "sp", "lr", "pc" };
#else
  static const char *gp_regs[] = {
    "r0", "r1", "r2", "r3",
    "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11",
    "ip", "sp", "lr", "pc" };
  /* "r12", "r13", "r14", "r15" }; */
#endif

  if (reg < ORC_GP_REG_BASE || reg >= ORC_GP_REG_BASE+16) {
    return "ERROR";
  }

  return gp_regs[reg&0xf];
}

const char *
orc_arm64_reg_name (int reg, OrcArm64RegBits bits)
{
  static const char *gp_regs64[] = {
     "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",  "x8",  "x9",
    "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19",
    "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29",
    "x30",  "sp"
  };
  static const char *gp_regs32[] = {
     "w0",  "w1",  "w2",  "w3",  "w4",  "w5",  "w6",  "w7",  "w8",  "w9",
    "w10", "w11", "w12", "w13", "w14", "w15", "w16", "w17", "w18", "w19",
    "w20", "w21", "w22", "w23", "w24", "w25", "w26", "w27", "w28", "w29",
    "w30",  "sp"
  };

  if (reg < ORC_GP_REG_BASE || reg >= ORC_GP_REG_BASE+32) {
    return "ERROR";
  }

  return bits == ORC_ARM64_REG_64 ? gp_regs64[reg&0x1f] : gp_regs32[reg&0x1f];
}

void
orc_arm_flush_cache (OrcCode *code)
{
#if defined (HAVE_ARM) || defined (HAVE_AARCH64)
#ifdef __APPLE__
  sys_dcache_flush(code->code, code->code_size);
  sys_icache_invalidate(code->exec, code->code_size);
#elif defined (_WIN32)
  HANDLE h_proc = GetCurrentProcess();

  FlushInstructionCache(h_proc, code->code, code->code_size);

  if ((void *) code->exec != (void *) code->code)
    FlushInstructionCache(h_proc, code->exec, code->code_size);
#else
  __clear_cache (code->code, code->code + code->code_size);
  if ((void *) code->exec != (void *) code->code)
    __clear_cache (code->exec, code->exec + code->code_size);
#endif
#endif
}

void
orc_arm_add_fixup (OrcCompiler *compiler, int label, int type)
{
  ORC_ASSERT (compiler->n_fixups < ORC_N_FIXUPS);

  compiler->fixups[compiler->n_fixups].ptr = compiler->codeptr;
  compiler->fixups[compiler->n_fixups].label = label;
  compiler->fixups[compiler->n_fixups].type = type;
  compiler->n_fixups++;
}

void
orc_arm_do_fixups (OrcCompiler *compiler)
{
  int i;
  for(i=0;i<compiler->n_fixups;i++){
    unsigned char *label = compiler->labels[compiler->fixups[i].label];
    unsigned char *ptr = compiler->fixups[i].ptr;
    orc_uint32 code;
    int diff;

    if (compiler->fixups[i].type == 0) {
      code = ORC_READ_UINT32_LE (ptr);
      diff = code;
      if (compiler->is_64bit) {
        diff = ((label - ptr) >> 2);   /** time 4 */
        if (diff != (diff << 6)>>6) {
          ORC_COMPILER_ERROR(compiler, "fixup out of range");
        }
        /** check whether it's conditional or unconditioanl */
        if ((code >> 30) & 1) {
          diff <<= 5;     /** for cond */
          ORC_WRITE_UINT32_LE(ptr, (code&0xff00001f) | (diff&0x00ffffe0));
        } else {
          ORC_WRITE_UINT32_LE(ptr, (code&0xfc000000) | (diff&0x03ffffff));
        }
      } else {
        diff = (diff << 8) >> 8;
        diff += ((label - ptr) >> 2);
        if (diff != (diff << 8)>>8) {
          ORC_COMPILER_ERROR(compiler, "fixup out of range");
        }
        ORC_WRITE_UINT32_LE(ptr, (code&0xff000000) | (diff&0x00ffffff));
      }
    } else {
      code = ORC_READ_UINT32_LE (ptr);
      diff = code;
      /* We store the offset in the code as signed, but the CPU considers
       * it unsigned */
      diff = (diff << 24) >> 24;
      diff += ((label - ptr) >> 2);
      if (diff != (diff & 0xff)) {
        ORC_COMPILER_ERROR(compiler, "fixup out of range (%d > 255)", diff);
      }
      ORC_WRITE_UINT32_LE(ptr, (code&0xffffff00) | (diff&0x000000ff));
    }
  }
}
