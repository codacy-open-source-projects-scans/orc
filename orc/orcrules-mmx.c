
#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#include <orc/orcprogram.h>
#include <orc/orcdebug.h>
#include <orc/orcmmx.h>

#define MMX 1
#ifdef MMX
#  define ORC_REG_SIZE 8
#else
#  define ORC_REG_SIZE 16
#endif
#define SIZE 65536

/* sse rules */

static void
mmx_rule_loadpX (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int reg;
  int size = ORC_PTR_TO_INT(user);

  if (src->vartype == ORC_VAR_TYPE_PARAM) {
    reg = dest->alloc;

    if (size == 8 && src->size == 8) {
      orc_x86_emit_mov_memoffset_mmx (compiler, 4,
          (int)ORC_STRUCT_OFFSET(OrcExecutor, params[insn->src_args[0]]),
          compiler->exec_reg, reg, FALSE);
#ifndef MMX
      orc_mmx_emit_movhps_load_memoffset (compiler,
          (int)ORC_STRUCT_OFFSET(OrcExecutor,
            params[insn->src_args[0] + (ORC_N_PARAMS)]),
          compiler->exec_reg, reg);
      orc_mmx_emit_pshufd (compiler, ORC_MMX_SHUF(2,0,2,0), reg, reg);
#else
      /* FIXME yes, I understand this is terrible */
      orc_mmx_emit_pinsrw_memoffset (compiler, 2,
          (int)ORC_STRUCT_OFFSET(OrcExecutor,
            params[insn->src_args[0] + (ORC_VAR_T1 - ORC_VAR_P1)]) + 0,
          compiler->exec_reg, reg);
      orc_mmx_emit_pinsrw_memoffset (compiler, 3,
          (int)ORC_STRUCT_OFFSET(OrcExecutor,
            params[insn->src_args[0] + (ORC_VAR_T1 - ORC_VAR_P1)]) + 2,
          compiler->exec_reg, reg);
#ifndef MMX
      orc_mmx_emit_pshufd (compiler, ORC_MMX_SHUF(1,0,1,0), reg, reg);
#endif
#endif
    } else {
      orc_x86_emit_mov_memoffset_mmx (compiler, 4,
          (int)ORC_STRUCT_OFFSET(OrcExecutor, params[insn->src_args[0]]),
          compiler->exec_reg, reg, FALSE);
      if (size < 8) {
        if (size == 1) {
          orc_mmx_emit_punpcklbw (compiler, reg, reg);
        }
#ifndef MMX
        if (size <= 2) {
          orc_mmx_emit_pshuflw (compiler, 0, reg, reg);
        }
        orc_mmx_emit_pshufd (compiler, 0, reg, reg);
#else
        if (size <= 2) {
          orc_mmx_emit_pshufw (compiler, ORC_MMX_SHUF(0,0,0,0), reg, reg);
        } else {
          orc_mmx_emit_pshufw (compiler, ORC_MMX_SHUF(1,0,1,0), reg, reg);
        }
#endif
      } else {
#ifndef MMX
        orc_mmx_emit_pshufd (compiler, ORC_MMX_SHUF(1,0,1,0), reg, reg);
#endif
      }
    }
  } else if (src->vartype == ORC_VAR_TYPE_CONST) {
    orc_mmx_load_constant (compiler, dest->alloc, size, src->value.i);
  } else {
    ORC_ASSERT(0);
  }
}

static void
mmx_rule_loadX (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int ptr_reg;
  int offset = 0;

  offset = compiler->offset * src->size;
  if (src->ptr_register == 0) {
    int i = insn->src_args[0];
    orc_x86_emit_mov_memoffset_reg (compiler, compiler->is_64bit ? 8 : 4,
        (int)ORC_STRUCT_OFFSET(OrcExecutor, arrays[i]),
        compiler->exec_reg, compiler->gp_tmpreg);
    ptr_reg = compiler->gp_tmpreg;
  } else {
    ptr_reg = src->ptr_register;
  } 
  switch (src->size << compiler->loop_shift) {
    case 1:
#ifndef MMX
      if (compiler->target_flags & ORC_TARGET_MMX_SSE4_1) {
        orc_mmx_emit_pxor (compiler, dest->alloc, dest->alloc);
        orc_mmx_emit_pinsrb_memoffset (compiler, 0, offset, ptr_reg, dest->alloc);
      } else {
#endif
        orc_x86_emit_mov_memoffset_reg (compiler, 1, offset, ptr_reg,
            compiler->gp_tmpreg);
        orc_mmx_emit_movd_load_register (compiler, compiler->gp_tmpreg,
            dest->alloc);
#ifndef MMX
      }
#endif
      break;
    case 2:
      orc_mmx_emit_pxor (compiler, dest->alloc, dest->alloc);
      orc_mmx_emit_pinsrw_memoffset (compiler, 0, offset, ptr_reg, dest->alloc);
      break;
    case 4:
      orc_x86_emit_mov_memoffset_mmx (compiler, 4, offset, ptr_reg,
          dest->alloc, src->is_aligned);
      break;
    case 8:
      orc_x86_emit_mov_memoffset_mmx (compiler, 8, offset, ptr_reg,
          dest->alloc, src->is_aligned);
      break;
    case 16:
      orc_x86_emit_mov_memoffset_mmx (compiler, 16, offset, ptr_reg,
          dest->alloc, src->is_aligned);
      break;
    default:
      orc_compiler_error (compiler, "bad load size %d",
          src->size << compiler->loop_shift);
      break;
  }

  src->update_type = 2;
}

static void
mmx_rule_loadoffX (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int ptr_reg;
  int offset = 0;

  if (compiler->vars[insn->src_args[1]].vartype != ORC_VAR_TYPE_CONST) {
    orc_compiler_error (compiler, "code generation rule for %s only works with constant offset",
        insn->opcode->name);
    return;
  }

  offset = (compiler->offset + compiler->vars[insn->src_args[1]].value.i) *
    src->size;
  if (src->ptr_register == 0) {
    int i = insn->src_args[0];
    orc_x86_emit_mov_memoffset_reg (compiler, compiler->is_64bit ? 8 : 4,
        (int)ORC_STRUCT_OFFSET(OrcExecutor, arrays[i]),
        compiler->exec_reg, compiler->gp_tmpreg);
    ptr_reg = compiler->gp_tmpreg;
  } else {
    ptr_reg = src->ptr_register;
  } 
  switch (src->size << compiler->loop_shift) {
    case 1:
#ifndef MMX
      if (compiler->target_flags & ORC_TARGET_MMX_SSE4_1) {
        orc_mmx_emit_pxor (compiler, dest->alloc, dest->alloc);
        orc_mmx_emit_pinsrb_memoffset (compiler, 0, offset, ptr_reg, dest->alloc);
      } else {
#endif
        orc_x86_emit_mov_memoffset_reg (compiler, 1, offset, ptr_reg,
            compiler->gp_tmpreg);
        orc_mmx_emit_movd_load_register (compiler, compiler->gp_tmpreg,
            dest->alloc);
#ifndef MMX
      }
#endif
      break;
    case 2:
      orc_mmx_emit_pxor (compiler, dest->alloc, dest->alloc);
      orc_mmx_emit_pinsrw_memoffset (compiler, 0, offset, ptr_reg, dest->alloc);
      break;
    case 4:
      orc_x86_emit_mov_memoffset_mmx (compiler, 4, offset, ptr_reg,
          dest->alloc, src->is_aligned);
      break;
    case 8:
      orc_x86_emit_mov_memoffset_mmx (compiler, 8, offset, ptr_reg,
          dest->alloc, src->is_aligned);
      break;
    case 16:
      orc_x86_emit_mov_memoffset_mmx (compiler, 16, offset, ptr_reg,
          dest->alloc, src->is_aligned);
      break;
    default:
      orc_compiler_error (compiler,"bad load size %d",
          src->size << compiler->loop_shift);
      break;
  }

  src->update_type = 2;
}

static void
mmx_rule_loadupib (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int ptr_reg;
  int offset = 0;
  int tmp = orc_compiler_get_temp_reg (compiler);

  offset = (compiler->offset * src->size) >> 1;
  if (src->ptr_register == 0) {
    int i = insn->src_args[0];
    orc_x86_emit_mov_memoffset_reg (compiler, compiler->is_64bit ? 8 : 4,
        (int)ORC_STRUCT_OFFSET(OrcExecutor, arrays[i]),
        compiler->exec_reg, compiler->gp_tmpreg);
    ptr_reg = compiler->gp_tmpreg;
  } else {
    ptr_reg = src->ptr_register;
  } 
  switch (src->size << compiler->loop_shift) {
    case 1:
#ifndef MMX
      if (compiler->target_flags & ORC_TARGET_MMX_SSE4_1) {
        orc_mmx_emit_pxor (compiler, dest->alloc, dest->alloc);
        orc_mmx_emit_pinsrb_memoffset (compiler, 0, offset, ptr_reg, dest->alloc);
        orc_mmx_emit_movq (compiler, dest->alloc, tmp);
      } else {
#endif
        orc_mmx_emit_pinsrw_memoffset (compiler, 0, offset, ptr_reg, dest->alloc);
        orc_mmx_emit_movq (compiler, dest->alloc, tmp);
        orc_mmx_emit_psrlw_imm (compiler, 8, tmp);
#ifndef MMX
      }
#endif
      break;
    case 2:
      orc_mmx_emit_pinsrw_memoffset (compiler, 0, offset, ptr_reg, dest->alloc);
      orc_mmx_emit_movq (compiler, dest->alloc, tmp);
      orc_mmx_emit_psrlw_imm (compiler, 8, tmp);
      break;
    case 4:
      orc_mmx_emit_pinsrw_memoffset (compiler, 0, offset, ptr_reg, dest->alloc);
      orc_mmx_emit_pinsrw_memoffset (compiler, 0, offset + 1, ptr_reg, tmp);
      break;
    case 8:
      orc_x86_emit_mov_memoffset_mmx (compiler, 4, offset, ptr_reg,
          dest->alloc, FALSE);
      orc_x86_emit_mov_memoffset_mmx (compiler, 4, offset + 1, ptr_reg,
          tmp, FALSE);
      break;
    case 16:
      orc_x86_emit_mov_memoffset_mmx (compiler, 8, offset, ptr_reg,
          dest->alloc, FALSE);
      orc_x86_emit_mov_memoffset_mmx (compiler, 8, offset + 1, ptr_reg,
          tmp, FALSE);
      break;
    case 32:
      orc_x86_emit_mov_memoffset_mmx (compiler, 16, offset, ptr_reg,
          dest->alloc, FALSE);
      orc_x86_emit_mov_memoffset_mmx (compiler, 16, offset + 1, ptr_reg,
          tmp, FALSE);
      break;
    default:
      orc_compiler_error(compiler,"bad load size %d",
          src->size << compiler->loop_shift);
      break;
  }

  orc_mmx_emit_pavgb (compiler, dest->alloc, tmp);
  orc_mmx_emit_punpcklbw (compiler, tmp, dest->alloc);

  src->update_type = 1;
}

static void
mmx_rule_loadupdb (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int ptr_reg;
  int offset = 0;

  offset = (compiler->offset * src->size) >> 1;
  if (src->ptr_register == 0) {
    int i = insn->src_args[0];
    orc_x86_emit_mov_memoffset_reg (compiler, compiler->is_64bit ? 8 : 4,
        (int)ORC_STRUCT_OFFSET(OrcExecutor, arrays[i]),
        compiler->exec_reg, compiler->gp_tmpreg);
    ptr_reg = compiler->gp_tmpreg;
  } else {
    ptr_reg = src->ptr_register;
  } 
  switch (src->size << compiler->loop_shift) {
    case 1:
    case 2:
      orc_x86_emit_mov_memoffset_reg (compiler, 1, offset, ptr_reg,
          compiler->gp_tmpreg);
      orc_mmx_emit_movd_load_register (compiler, compiler->gp_tmpreg, dest->alloc);
      break;
    case 4:
      orc_mmx_emit_pinsrw_memoffset (compiler, 0, offset, ptr_reg, dest->alloc);
      break;
    case 8:
      orc_x86_emit_mov_memoffset_mmx (compiler, 4, offset, ptr_reg,
          dest->alloc, src->is_aligned);
      break;
    case 16:
      orc_x86_emit_mov_memoffset_mmx (compiler, 8, offset, ptr_reg,
          dest->alloc, src->is_aligned);
      break;
    case 32:
      orc_x86_emit_mov_memoffset_mmx (compiler, 16, offset, ptr_reg,
          dest->alloc, src->is_aligned);
      break;
    default:
      orc_compiler_error(compiler,"bad load size %d",
          src->size << compiler->loop_shift);
      break;
  }
  switch (src->size) {
    case 1:
      orc_mmx_emit_punpcklbw (compiler, dest->alloc, dest->alloc);
      break;
    case 2:
      orc_mmx_emit_punpcklwd (compiler, dest->alloc, dest->alloc);
      break;
    case 4:
      orc_mmx_emit_punpckldq (compiler, dest->alloc, dest->alloc);
      break;
  }

  src->update_type = 1;
}

static void
mmx_rule_storeX (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int offset;
  int ptr_reg;

  offset = compiler->offset * dest->size;
  if (dest->ptr_register == 0) {
    orc_x86_emit_mov_memoffset_reg (compiler, compiler->is_64bit ? 8 : 4,
        dest->ptr_offset, compiler->exec_reg, compiler->gp_tmpreg);
    ptr_reg = compiler->gp_tmpreg; 
  } else {
    ptr_reg = dest->ptr_register;
  } 
  switch (dest->size << compiler->loop_shift) {
    case 1:
#ifndef MMX
      if (compiler->target_flags & ORC_TARGET_MMX_SSE4_1) {
        // Note: this instruction uses VPEXTRB + memory address, that's SSE 4.1
        orc_mmx_emit_pextrb_memoffset (compiler, 0, offset, src->alloc,
          ptr_reg);
      } else {
#endif
        /* FIXME we might be using ecx twice here */
        if (ptr_reg == compiler->gp_tmpreg) {
          orc_compiler_error (compiler, "unimplemented corner case in %s",
              insn->opcode->name);
        }
        orc_mmx_emit_movd_store_register (compiler, src->alloc, compiler->gp_tmpreg);
        orc_x86_emit_mov_reg_memoffset (compiler, 1, compiler->gp_tmpreg,
            offset, ptr_reg);
#ifndef MMX
      }
#endif
      break;
    case 2:
#ifndef MMX
      if (compiler->target_flags & ORC_TARGET_MMX_SSE4_1) {
        // Note: this instruction uses VPEXTRW + memory address, that's SSE 4.1
        orc_mmx_emit_pextrw_memoffset (compiler, 0, offset, src->alloc,
            ptr_reg);
      } else {
#endif
        /* FIXME we might be using ecx twice here */
        if (ptr_reg == compiler->gp_tmpreg) {
          orc_compiler_error(compiler, "unimplemented corner case in %s",
              insn->opcode->name);
        } 
        orc_mmx_emit_movd_store_register (compiler, src->alloc, compiler->gp_tmpreg);
        orc_x86_emit_mov_reg_memoffset (compiler, 2, compiler->gp_tmpreg,
            offset, ptr_reg);
#ifndef MMX
      }
#endif
      break;
    case 4:
      orc_x86_emit_mov_mmx_memoffset (compiler, 4, src->alloc, offset, ptr_reg,
          dest->is_aligned, dest->is_uncached);
      break;
    case 8:
      orc_x86_emit_mov_mmx_memoffset (compiler, 8, src->alloc, offset, ptr_reg,
          dest->is_aligned, dest->is_uncached);
      break;
    case 16:
      orc_x86_emit_mov_mmx_memoffset (compiler, 16, src->alloc, offset, ptr_reg,
          dest->is_aligned, dest->is_uncached);
      break;
    default:
      orc_compiler_error (compiler, "bad size");
      break;
  }

  dest->update_type = 2;
}

#if try1
static void
mmx_rule_ldresnearl (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int tmp = orc_compiler_get_temp_reg (compiler);
  int tmp2 = orc_compiler_get_temp_reg (compiler);
  int tmpc;

  orc_mmx_emit_movd_store_register (compiler, X86_MM6, compiler->gp_tmpreg);
  orc_x86_emit_sar_imm_reg (compiler, 4, 16, compiler->gp_tmpreg);

  orc_mmx_emit_movdqu_load_memindex (compiler, 0, src->ptr_register,
      compiler->gp_tmpreg, 4, dest->alloc);

#if 0
  orc_mmx_emit_movq (compiler, X86_MM6, tmp);
  orc_mmx_emit_pslld_imm (compiler, 10, tmp);
  orc_mmx_emit_psrld_imm (compiler, 26, tmp);
  orc_mmx_emit_pslld_imm (compiler, 2, tmp);

  orc_mmx_emit_movq (compiler, tmp, tmp2);
  orc_mmx_emit_pslld_imm (compiler, 8, tmp2);
  orc_mmx_emit_por (compiler, tmp2, tmp);
  orc_mmx_emit_movq (compiler, tmp, tmp2);
  orc_mmx_emit_pslld_imm (compiler, 16, tmp2);
  orc_mmx_emit_por (compiler, tmp2, tmp);
#else
  orc_mmx_emit_movq (compiler, X86_MM6, tmp);
  tmpc = orc_compiler_get_constant_long (compiler, 0x02020202,
      0x06060606, 0x0a0a0a0a, 0x0e0e0e0e);
  orc_mmx_emit_pshufb (compiler, tmpc, tmp);
  orc_mmx_emit_paddb (compiler, tmp, tmp);
  orc_mmx_emit_paddb (compiler, tmp, tmp);
#endif

  orc_mmx_emit_pshufd (compiler, ORC_MMX_SHUF(0,0,0,0), tmp, tmp2);
  orc_mmx_emit_psubd (compiler, tmp2, tmp);
  tmpc = orc_compiler_get_constant (compiler, 4, 0x03020100);
  orc_mmx_emit_paddd (compiler, tmpc, tmp);

  orc_mmx_emit_pshufb (compiler, tmp, dest->alloc);

  orc_mmx_emit_movq (compiler, X86_MM7, tmp);
  orc_mmx_emit_pslld_imm (compiler, compiler->loop_shift, tmp);

  orc_mmx_emit_paddd (compiler, tmp, X86_MM6);

  src->update_type = 0;
}
#endif

static void
mmx_rule_ldresnearl (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  int increment_var = insn->src_args[2];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int tmp = orc_compiler_get_temp_reg (compiler);
  int i;

  for(i=0;i<(1<<compiler->loop_shift);i++){
    if (i == 0) {
      orc_x86_emit_mov_memoffset_mmx (compiler, 4, 0,
          src->ptr_register, dest->alloc, FALSE);
    } else {
      orc_x86_emit_mov_memindex_mmx (compiler, 4, 0,
          src->ptr_register, compiler->gp_tmpreg, 2, tmp, FALSE);
#ifdef MMX
      /* orc_mmx_emit_punpckldq (compiler, tmp, dest->alloc); */
      orc_mmx_emit_psllq_imm (compiler, 8*4*i, tmp);
      orc_mmx_emit_por (compiler, tmp, dest->alloc);
#else
      orc_mmx_emit_pslldq_imm (compiler, 4*i, tmp);
      orc_mmx_emit_por (compiler, tmp, dest->alloc);
#endif
    }

    if (compiler->vars[increment_var].vartype == ORC_VAR_TYPE_PARAM) {
      orc_x86_emit_add_memoffset_reg (compiler, 4,
          (int)ORC_STRUCT_OFFSET(OrcExecutor, params[increment_var]),
          compiler->exec_reg, src->ptr_offset);
    } else {
      orc_x86_emit_add_imm_reg (compiler, 4,
          compiler->vars[increment_var].value.i,
          src->ptr_offset, FALSE);
    }

    orc_x86_emit_mov_reg_reg (compiler, 4, src->ptr_offset, compiler->gp_tmpreg);
    orc_x86_emit_sar_imm_reg (compiler, 4, 16, compiler->gp_tmpreg);
  }

  orc_x86_emit_add_reg_reg_shift (compiler, compiler->is_64bit ? 8 : 4,
      compiler->gp_tmpreg,
      src->ptr_register, 2);
  orc_x86_emit_and_imm_reg (compiler, 4, 0xffff, src->ptr_offset);

  src->update_type = 0;
}

#ifndef MMX
static void
mmx_rule_ldreslinl (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  int increment_var = insn->src_args[2];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int tmp = orc_compiler_get_temp_reg (compiler);
  int tmp2 = orc_compiler_get_temp_reg (compiler);
  int regsize = compiler->is_64bit ? 8 : 4;
  int i;

  if (compiler->loop_shift == 0) {
    orc_x86_emit_mov_memoffset_mmx (compiler, 8, 0,
        src->ptr_register, tmp, FALSE);

    orc_mmx_emit_pxor (compiler, tmp2, tmp2);
    orc_mmx_emit_punpcklbw (compiler, tmp2, tmp);
    orc_mmx_emit_pshufd (compiler, ORC_MMX_SHUF(3,2,3,2), tmp, tmp2);
    orc_mmx_emit_psubw (compiler, tmp, tmp2);

    orc_mmx_emit_movd_load_register (compiler, src->ptr_offset, tmp);
    orc_mmx_emit_pshuflw (compiler, ORC_MMX_SHUF(0,0,0,0), tmp, tmp);
    orc_mmx_emit_psrlw_imm (compiler, 8, tmp);
    orc_mmx_emit_pmullw (compiler, tmp2, tmp);
    orc_mmx_emit_psraw_imm (compiler, 8, tmp);
    orc_mmx_emit_pxor (compiler, tmp2, tmp2);
    orc_mmx_emit_packsswb (compiler, tmp2, tmp);

    orc_x86_emit_mov_memoffset_mmx (compiler, 4, 0,
        src->ptr_register, dest->alloc, FALSE);
    orc_mmx_emit_paddb (compiler, tmp, dest->alloc);

    if (compiler->vars[increment_var].vartype == ORC_VAR_TYPE_PARAM) {
      orc_x86_emit_add_memoffset_reg (compiler, 4,
          (int)ORC_STRUCT_OFFSET(OrcExecutor, params[increment_var]),
          compiler->exec_reg, src->ptr_offset);
    } else {
      orc_x86_emit_add_imm_reg (compiler, regsize,
          compiler->vars[increment_var].value.i,
          src->ptr_offset, FALSE);
    }

    orc_x86_emit_mov_reg_reg (compiler, 4, src->ptr_offset, compiler->gp_tmpreg);
    orc_x86_emit_sar_imm_reg (compiler, 4, 16, compiler->gp_tmpreg);

    orc_x86_emit_add_reg_reg_shift (compiler, regsize, compiler->gp_tmpreg,
        src->ptr_register, 2);
    orc_x86_emit_and_imm_reg (compiler, 4, 0xffff, src->ptr_offset);
  } else {
    int tmp3 = orc_compiler_get_temp_reg (compiler);
    int tmp4 = orc_compiler_get_temp_reg (compiler);

    for(i=0;i<(1<<compiler->loop_shift);i+=2){
      orc_x86_emit_mov_memoffset_mmx (compiler, 8, 0,
          src->ptr_register, tmp, FALSE);
      orc_mmx_emit_movd_load_register (compiler, src->ptr_offset, tmp4);

      if (compiler->vars[increment_var].vartype == ORC_VAR_TYPE_PARAM) {
        orc_x86_emit_add_memoffset_reg (compiler, 4,
            (int)ORC_STRUCT_OFFSET(OrcExecutor, params[increment_var]),
            compiler->exec_reg, src->ptr_offset);
      } else {
        orc_x86_emit_add_imm_reg (compiler, 4,
            compiler->vars[increment_var].value.i,
            src->ptr_offset, FALSE);
      }
      orc_x86_emit_mov_reg_reg (compiler, 4, src->ptr_offset, compiler->gp_tmpreg);
      orc_x86_emit_sar_imm_reg (compiler, 4, 16, compiler->gp_tmpreg);

      orc_x86_emit_mov_memindex_mmx (compiler, 8, 0,
          src->ptr_register, compiler->gp_tmpreg, 2, tmp2, FALSE);

      orc_mmx_emit_punpckldq (compiler, tmp2, tmp);
      orc_mmx_emit_movq (compiler, tmp, tmp2);
      if (i == 0) {
        orc_mmx_emit_movq (compiler, tmp, dest->alloc);
      } else {
        orc_mmx_emit_punpcklqdq (compiler, tmp, dest->alloc);
      }

      orc_mmx_emit_pxor (compiler, tmp3, tmp3);
      orc_mmx_emit_punpcklbw (compiler, tmp3, tmp);
      orc_mmx_emit_punpckhbw (compiler, tmp3, tmp2);

      orc_mmx_emit_psubw (compiler, tmp, tmp2);

      orc_mmx_emit_pinsrw_register (compiler, 1, src->ptr_offset, tmp4);

#if 0
      orc_mmx_emit_punpcklwd (compiler, tmp4, tmp4);
      orc_mmx_emit_punpckldq (compiler, tmp4, tmp4);
#else
      orc_mmx_emit_pshuflw (compiler, ORC_MMX_SHUF(1,1,0,0), tmp4, tmp4);
      orc_mmx_emit_pshufd (compiler, ORC_MMX_SHUF(1,1,0,0), tmp4, tmp4);
#endif
      orc_mmx_emit_psrlw_imm (compiler, 8, tmp4);
      orc_mmx_emit_pmullw (compiler, tmp4, tmp2);
      orc_mmx_emit_psraw_imm (compiler, 8, tmp2);
      orc_mmx_emit_pxor (compiler, tmp, tmp);
      orc_mmx_emit_packsswb (compiler, tmp, tmp2);

      if (i != 0) {
        orc_mmx_emit_pslldq_imm (compiler, 8, tmp2);
      }
      orc_mmx_emit_paddb (compiler, tmp2, dest->alloc);

      if (compiler->vars[increment_var].vartype == ORC_VAR_TYPE_PARAM) {
        orc_x86_emit_add_memoffset_reg (compiler, 4,
            (int)ORC_STRUCT_OFFSET(OrcExecutor, params[increment_var]),
            compiler->exec_reg, src->ptr_offset);
      } else {
        orc_x86_emit_add_imm_reg (compiler, 4,
            compiler->vars[increment_var].value.i,
            src->ptr_offset, FALSE);
      }

      orc_x86_emit_mov_reg_reg (compiler, 4, src->ptr_offset, compiler->gp_tmpreg);
      orc_x86_emit_sar_imm_reg (compiler, 4, 16, compiler->gp_tmpreg);

      orc_x86_emit_add_reg_reg_shift (compiler, 8, compiler->gp_tmpreg,
          src->ptr_register, 2);
      orc_x86_emit_and_imm_reg (compiler, 4, 0xffff, src->ptr_offset);
    }
  }

  src->update_type = 0;
}
#else
static void
mmx_rule_ldreslinl (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  int increment_var = insn->src_args[2];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int tmp = orc_compiler_get_temp_reg (compiler);
  int tmp2 = orc_compiler_get_temp_reg (compiler);
  int zero;
  int regsize = compiler->is_64bit ? 8 : 4;
  int i;

  zero = orc_compiler_get_constant (compiler, 1, 0);
  for(i=0;i<(1<<compiler->loop_shift);i++){
    orc_x86_emit_mov_memoffset_mmx (compiler, 4, 0,
        src->ptr_register, tmp, FALSE);
    orc_x86_emit_mov_memoffset_mmx (compiler, 4, 4,
        src->ptr_register, tmp2, FALSE);

    orc_mmx_emit_punpcklbw (compiler, zero, tmp);
    orc_mmx_emit_punpcklbw (compiler, zero, tmp2);
    orc_mmx_emit_psubw (compiler, tmp, tmp2);

    orc_mmx_emit_movd_load_register (compiler, src->ptr_offset, tmp);
    orc_mmx_emit_pshufw (compiler, ORC_MMX_SHUF(0,0,0,0), tmp, tmp);
    orc_mmx_emit_psrlw_imm (compiler, 8, tmp);
    orc_mmx_emit_pmullw (compiler, tmp2, tmp);
    orc_mmx_emit_psraw_imm (compiler, 8, tmp);
    orc_mmx_emit_pxor (compiler, tmp2, tmp2);
    orc_mmx_emit_packsswb (compiler, tmp2, tmp);

    if (i == 0) {
      orc_x86_emit_mov_memoffset_mmx (compiler, 4, 0,
          src->ptr_register, dest->alloc, FALSE);
      orc_mmx_emit_paddb (compiler, tmp, dest->alloc);
    } else {
      orc_x86_emit_mov_memoffset_mmx (compiler, 4, 0,
          src->ptr_register, tmp2, FALSE);
      orc_mmx_emit_paddb (compiler, tmp, tmp2);
      orc_mmx_emit_psllq_imm (compiler, 32, tmp2);
      orc_mmx_emit_por (compiler, tmp2, dest->alloc);
    }

    if (compiler->vars[increment_var].vartype == ORC_VAR_TYPE_PARAM) {
      orc_x86_emit_add_memoffset_reg (compiler, 4,
          (int)ORC_STRUCT_OFFSET(OrcExecutor, params[increment_var]),
          compiler->exec_reg, src->ptr_offset);
    } else {
      orc_x86_emit_add_imm_reg (compiler, regsize,
          compiler->vars[increment_var].value.i,
          src->ptr_offset, FALSE);
    }

    orc_x86_emit_mov_reg_reg (compiler, 4, src->ptr_offset, compiler->gp_tmpreg);
    orc_x86_emit_sar_imm_reg (compiler, 4, 16, compiler->gp_tmpreg);

    orc_x86_emit_add_reg_reg_shift (compiler, regsize, compiler->gp_tmpreg,
        src->ptr_register, 2);
    orc_x86_emit_and_imm_reg (compiler, 4, 0xffff, src->ptr_offset);
  }

  src->update_type = 0;
}
#endif

static void
mmx_rule_copyx (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->vars[insn->src_args[0]].alloc == p->vars[insn->dest_args[0]].alloc) {
    return;
  }

  orc_mmx_emit_movq (p,
      p->vars[insn->src_args[0]].alloc,
      p->vars[insn->dest_args[0]].alloc);
}

#define UNARY(opcode,insn_name,code) \
static void \
mmx_rule_ ## opcode (OrcCompiler *p, void *user, OrcInstruction *insn) \
{ \
  orc_mmx_emit_ ## insn_name (p, \
      p->vars[insn->src_args[0]].alloc, \
      p->vars[insn->dest_args[0]].alloc); \
}

#define BINARY(opcode, insn_name, code) \
  static void mmx_rule_##opcode (OrcCompiler *p, void *user, \
      OrcInstruction *insn) \
  { \
    if (p->vars[insn->src_args[0]].alloc \
        != p->vars[insn->dest_args[0]].alloc) { \
      orc_mmx_emit_movq (p, p->vars[insn->src_args[0]].alloc, \
          p->vars[insn->dest_args[0]].alloc); \
    } \
    orc_mmx_emit_##insn_name (p, p->vars[insn->src_args[1]].alloc, \
        p->vars[insn->dest_args[0]].alloc); \
  }

UNARY(absb,pabsb,0x381c)
BINARY(addb,paddb,0xfc)
BINARY(addssb,paddsb,0xec)
BINARY(addusb,paddusb,0xdc)
BINARY(andb,pand,0xdb)
BINARY(andnb,pandn,0xdf)
BINARY(avgub,pavgb,0xe0)
BINARY(cmpeqb,pcmpeqb,0x74)
BINARY(cmpgtsb,pcmpgtb,0x64)
BINARY(maxub,pmaxub,0xde)
BINARY(minub,pminub,0xda)
/* BINARY(mullb,pmullb,0xd5) */
/* BINARY(mulhsb,pmulhb,0xe5) */
/* BINARY(mulhub,pmulhub,0xe4) */
BINARY(orb,por,0xeb)
/* UNARY(signb,psignb,0x3808) */
BINARY(subb,psubb,0xf8)
BINARY(subssb,psubsb,0xe8)
BINARY(subusb,psubusb,0xd8)
BINARY(xorb,pxor,0xef)

UNARY(absw,pabsw,0x381d)
BINARY(addw,paddw,0xfd)
BINARY(addssw,paddsw,0xed)
BINARY(addusw,paddusw,0xdd)
BINARY(andw,pand,0xdb)
BINARY(andnw,pandn,0xdf)
BINARY(avguw,pavgw,0xe3)
BINARY(cmpeqw,pcmpeqw,0x75)
BINARY(cmpgtsw,pcmpgtw,0x65)
BINARY(maxsw,pmaxsw,0xee)
BINARY(minsw,pminsw,0xea)
BINARY(mullw,pmullw,0xd5)
BINARY(mulhsw,pmulhw,0xe5)
BINARY(mulhuw,pmulhuw,0xe4)
BINARY(orw,por,0xeb)
/* UNARY(signw,psignw,0x3809) */
BINARY(subw,psubw,0xf9)
BINARY(subssw,psubsw,0xe9)
BINARY(subusw,psubusw,0xd9)
BINARY(xorw,pxor,0xef)

UNARY(absl,pabsd,0x381e)
BINARY(addl,paddd,0xfe)
/* BINARY(addssl,paddsd,0xed) */
/* BINARY(addusl,paddusd,0xdd) */
BINARY(andl,pand,0xdb)
BINARY(andnl,pandn,0xdf)
/* BINARY(avgul,pavgd,0xe3) */
BINARY(cmpeql,pcmpeqd,0x76)
BINARY(cmpgtsl,pcmpgtd,0x66)
/* BINARY(mulhsl,pmulhd,0xe5) */
/* BINARY(mulhul,pmulhud,0xe4) */
BINARY(orl,por,0xeb)
/* UNARY(signl,psignd,0x380a) */
BINARY(subl,psubd,0xfa)
/* BINARY(subssl,psubsd,0xe9) */
/* BINARY(subusl,psubusd,0xd9) */
BINARY(xorl,pxor,0xef)

BINARY(andq,pand,0xdb)
BINARY(andnq,pandn,0xdf)
BINARY(orq,por,0xeb)
BINARY(xorq,pxor,0xef)
BINARY(cmpgtsq,pcmpgtq,0x3837)

#ifndef MMX
BINARY(maxsb,pmaxsb,0x383c)
BINARY(minsb,pminsb,0x3838)
BINARY(maxuw,pmaxuw,0x383e)
BINARY(minuw,pminuw,0x383a)
BINARY(maxsl,pmaxsd,0x383d)
BINARY(maxul,pmaxud,0x383f)
BINARY(minsl,pminsd,0x3839)
BINARY(minul,pminud,0x383b)
BINARY(mulll,pmulld,0x3840)
BINARY(cmpeqq,pcmpeqq,0x3829)
BINARY(addq,paddq,0xd4)
BINARY(subq,psubq,0xfb)
#endif

static void
mmx_rule_accw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  orc_mmx_emit_paddw (p, src, dest);
}

static void
mmx_rule_accl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

#ifndef MMX
  if (p->loop_shift == 0) {
    orc_mmx_emit_pslldq_imm (p, 12, src);
  }
#endif
  orc_mmx_emit_paddd (p, src, dest);
}

static void
mmx_rule_accsadubl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src1 = p->vars[insn->src_args[0]].alloc;
  const int src2 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);
  const int tmp2 = orc_compiler_get_temp_reg (p);

#ifndef MMX
  if (p->loop_shift <= 2) {
    orc_mmx_emit_movq (p, src1, tmp);
    orc_mmx_emit_pslldq_imm (p, 16 - (1<<p->loop_shift), tmp);
    orc_mmx_emit_movq (p, src2, tmp2);
    orc_mmx_emit_pslldq_imm (p, 16 - (1<<p->loop_shift), tmp2);
    orc_mmx_emit_psadbw (p, tmp2, tmp);
  } else if (p->loop_shift == 3) {
    orc_mmx_emit_movq (p, src1, tmp);
    orc_mmx_emit_psadbw (p, src2, tmp);
    orc_mmx_emit_pslldq_imm (p, 8, tmp);
  } else {
    orc_mmx_emit_movq (p, src1, tmp);
    orc_mmx_emit_psadbw (p, src2, tmp);
  }
#else
  if (p->loop_shift <= 2) {
    orc_mmx_emit_movq (p, src1, tmp);
    orc_mmx_emit_psllq_imm (p, 8*(8 - (1<<p->loop_shift)), tmp);
    orc_mmx_emit_movq (p, src2, tmp2);
    orc_mmx_emit_psllq_imm (p, 8*(8 - (1<<p->loop_shift)), tmp2);
    orc_mmx_emit_psadbw (p, tmp2, tmp);
  } else {
    orc_mmx_emit_movq (p, src1, tmp);
    orc_mmx_emit_psadbw (p, src2, tmp);
  }
#endif
  orc_mmx_emit_paddd (p, tmp, dest);
}

#ifndef MMX
static void
mmx_rule_signX_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int opcodes[] = { ORC_X86_psignb, ORC_X86_psignw, ORC_X86_psignd };
  const int type = ORC_PTR_TO_INT (user);

  const int tmpc = orc_compiler_get_temp_constant (p, 1 << type, 1);

  if (src == dest) {
    orc_x86_emit_cpuinsn_size (p, opcodes[type], 16, src, tmpc);
    orc_mmx_emit_movq (p, tmpc, dest);
  } else {
    orc_mmx_emit_movq (p, tmpc, dest);
    orc_x86_emit_cpuinsn_size (p, opcodes[type], 16, src, dest);
  }
}
#endif

static void
mmx_rule_signw_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = orc_compiler_get_constant (p, 2, 0x0001);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_pminsw (p, tmp, dest);
  tmp = orc_compiler_get_constant (p, 2, 0xffff);
  orc_mmx_emit_pmaxsw (p, tmp, dest);
}

static void
mmx_rule_absb_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_pxor (p, tmp, tmp);
  orc_mmx_emit_pcmpgtb (p, src, tmp);
  orc_mmx_emit_pxor (p, tmp, dest);
  orc_mmx_emit_psubb (p, tmp, dest);
}

static void
mmx_rule_absw_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src == dest) {
    orc_mmx_emit_movq (p, src, tmp);
  } else {
    orc_mmx_emit_movq (p, src, tmp);
    orc_mmx_emit_movq (p, tmp, dest);
  }

  orc_mmx_emit_psraw_imm (p, 15, tmp);
  orc_mmx_emit_pxor (p, tmp, dest);
  orc_mmx_emit_psubw (p, tmp, dest);

}

static void
mmx_rule_absl_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src == dest) {
    orc_mmx_emit_movq (p, src, tmp);
  } else {
    orc_mmx_emit_movq (p, src, tmp);
    orc_mmx_emit_movq (p, tmp, dest);
  }

  orc_mmx_emit_psrad_imm (p, 31, tmp);
  orc_mmx_emit_pxor (p, tmp, dest);
  orc_mmx_emit_psubd (p, tmp, dest);

}

static void
mmx_rule_shift (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int type = ORC_PTR_TO_INT (user);
  /* int imm_code1[] = { 0x71, 0x71, 0x71, 0x72, 0x72, 0x72, 0x73, 0x73 }; */
  /* int imm_code2[] = { 6, 2, 4, 6, 2, 4, 6, 2 }; */
  /* int reg_code[] = { 0xf1, 0xd1, 0xe1, 0xf2, 0xd2, 0xe2, 0xf3, 0xd3 }; */
  /* const char *code[] = { "psllw", "psrlw", "psraw", "pslld", "psrld", "psrad", "psllq", "psrlq" }; */
  const int opcodes[] = { ORC_X86_psllw, ORC_X86_psrlw, ORC_X86_psraw,
    ORC_X86_pslld, ORC_X86_psrld, ORC_X86_psrad, ORC_X86_psllq,
    ORC_X86_psrlq };
  const int opcodes_imm[] = { ORC_X86_psllw_imm, ORC_X86_psrlw_imm,
    ORC_X86_psraw_imm, ORC_X86_pslld_imm, ORC_X86_psrld_imm,
    ORC_X86_psrad_imm, ORC_X86_psllq_imm, ORC_X86_psrlq_imm };
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  if (p->vars[insn->src_args[1]].vartype == ORC_VAR_TYPE_CONST) {
    orc_x86_emit_cpuinsn_imm (p, opcodes_imm[type],
        p->vars[insn->src_args[1]].value.i, 0, dest);
  } else if (p->vars[insn->src_args[1]].vartype == ORC_VAR_TYPE_PARAM) {
    int tmp = orc_compiler_get_temp_reg (p);

    /* FIXME this is a gross hack to reload the register with a
     * 64-bit version of the parameter. */
    orc_x86_emit_mov_memoffset_mmx (p, 4,
        (int)ORC_STRUCT_OFFSET(OrcExecutor, params[insn->src_args[1]]),
        p->exec_reg, tmp, FALSE);

    orc_x86_emit_cpuinsn_size (p, opcodes[type], 16, tmp, dest);
  } else {
    orc_compiler_error (p, "code generation rule for %s only works with "
        "constant or parameter shifts", insn->opcode->name);
    p->result = ORC_COMPILE_RESULT_UNKNOWN_COMPILE;
  }
}

static void
mmx_rule_shlb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  if (p->vars[insn->src_args[1]].vartype == ORC_VAR_TYPE_CONST) {
    orc_mmx_emit_psllw_imm (p, p->vars[insn->src_args[1]].value.i, dest);
    const int tmp = orc_compiler_get_constant (p, 1,
        0xff & (0xff << p->vars[insn->src_args[1]].value.i));
    orc_mmx_emit_pand (p, tmp, dest);
  } else {
    orc_compiler_error (p, "code generation rule for %s only works with "
        "constant shifts", insn->opcode->name);
    p->result = ORC_COMPILE_RESULT_UNKNOWN_COMPILE;
  }
}

static void
mmx_rule_shrsb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (p->vars[insn->src_args[1]].vartype == ORC_VAR_TYPE_CONST) {
    orc_mmx_emit_movq (p, src, tmp);
    orc_mmx_emit_psllw_imm (p, 8, tmp);
    orc_mmx_emit_psraw_imm (p, p->vars[insn->src_args[1]].value.i, tmp);
    orc_mmx_emit_psrlw_imm (p, 8, tmp);

    if (src != dest) {
      orc_mmx_emit_movq (p, src, dest);
    }

    orc_mmx_emit_psraw_imm (p, 8 + p->vars[insn->src_args[1]].value.i, dest);
    orc_mmx_emit_psllw_imm (p, 8, dest);

    orc_mmx_emit_por (p, tmp, dest);
  } else {
    orc_compiler_error (p, "code generation rule for %s only works with "
        "constant shifts", insn->opcode->name);
    p->result = ORC_COMPILE_RESULT_UNKNOWN_COMPILE;
  }
}

static void
mmx_rule_shrub (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  if (p->vars[insn->src_args[1]].vartype == ORC_VAR_TYPE_CONST) {
    orc_mmx_emit_psrlw_imm (p, p->vars[insn->src_args[1]].value.i, dest);
    const int tmp = orc_compiler_get_constant (p, 1,
        (0xff >> p->vars[insn->src_args[1]].value.i));
    orc_mmx_emit_pand (p, tmp, dest);
  } else {
    orc_compiler_error (p, "code generation rule for %s only works with "
        "constant shifts", insn->opcode->name);
    p->result = ORC_COMPILE_RESULT_UNKNOWN_COMPILE;
  }
}

static void
mmx_rule_shrsq (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (p->vars[insn->src_args[1]].vartype == ORC_VAR_TYPE_CONST) {
#ifndef MMX
    orc_mmx_emit_pshufd (p, ORC_MMX_SHUF(3,3,1,1), src, tmp);
#else
    orc_mmx_emit_pshufw (p, ORC_MMX_SHUF(3,2,3,2), src, tmp);
#endif
    orc_mmx_emit_psrad_imm (p, 31, tmp);
    orc_mmx_emit_psllq_imm (p, 64-p->vars[insn->src_args[1]].value.i, tmp);

    if (src != dest) {
      orc_mmx_emit_movq (p, src, dest);
    }
    
    orc_mmx_emit_psrlq_imm (p, p->vars[insn->src_args[1]].value.i, dest);
    orc_mmx_emit_por (p, tmp, dest);
  } else {
    orc_compiler_error (p, "code generation rule for %s only works with "
        "constant shifts", insn->opcode->name);
    p->result = ORC_COMPILE_RESULT_UNKNOWN_COMPILE;
  }
}

static void
mmx_rule_convsbw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }
  /* values of dest are shifted away so don't matter */
  orc_mmx_emit_punpcklbw (p, src, dest);
  orc_mmx_emit_psraw_imm (p, 8, dest);
}

static void
mmx_rule_convubw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }
  orc_mmx_emit_pxor (p, tmp, tmp);
  orc_mmx_emit_punpcklbw (p, tmp, dest);
}

static void
mmx_rule_convssswb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }
  orc_mmx_emit_packsswb (p, src, dest);
}

static void
mmx_rule_convsuswb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }
  orc_mmx_emit_packuswb (p, src, dest);
}

static void
mmx_rule_convuuswb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  orc_mmx_emit_movq (p, src, tmp);
  orc_mmx_emit_movq (p, src, dest);
  orc_mmx_emit_psrlw_imm (p, 15, tmp);
  orc_mmx_emit_psllw_imm (p, 14, tmp);
  orc_mmx_emit_por (p, tmp, dest);
  orc_mmx_emit_psllw_imm (p, 1, tmp);
  orc_mmx_emit_pxor (p, tmp, dest);
  orc_mmx_emit_packuswb (p, dest, dest);
}

static void
mmx_rule_convwb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }
  orc_mmx_emit_psllw_imm (p, 8, dest);
  orc_mmx_emit_psrlw_imm (p, 8, dest);
  orc_mmx_emit_packuswb (p, dest, dest);
}

static void
mmx_rule_convhwb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }
  orc_mmx_emit_psrlw_imm (p, 8, dest);
  orc_mmx_emit_packuswb (p, dest, dest);
}

static void
mmx_rule_convswl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  /* values of dest are shifted away so don't matter */
  orc_mmx_emit_punpcklwd (p, src, dest);
  orc_mmx_emit_psrad_imm (p, 16, dest);
}

static void
mmx_rule_convuwl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }
  orc_mmx_emit_pxor (p, tmp, tmp);
  orc_mmx_emit_punpcklwd (p, tmp, dest);
}

static void
mmx_rule_convlw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }
  orc_mmx_emit_pslld_imm (p, 16, dest);
  orc_mmx_emit_psrad_imm (p, 16, dest);
  orc_mmx_emit_packssdw (p, dest, dest);
}

static void
mmx_rule_convhlw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }
  orc_mmx_emit_psrad_imm (p, 16, dest);
  orc_mmx_emit_packssdw (p, dest, dest);
}

static void
mmx_rule_convssslw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  orc_mmx_emit_packssdw (p, src, dest);
}

#ifndef MMX
static void
mmx_rule_convsuslw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  orc_mmx_emit_packusdw (p, src, dest);
}
#endif

static void
mmx_rule_convslq (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  orc_mmx_emit_movq (p, src, tmp);
  orc_mmx_emit_psrad_imm (p, 31, tmp);
  orc_mmx_emit_punpckldq (p, tmp, dest);
}

static void
mmx_rule_convulq (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_constant (p, 4, 0);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_punpckldq (p, tmp, dest);
}

static void
mmx_rule_convql (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

#ifndef MMX
  orc_mmx_emit_pshufd (p, ORC_MMX_SHUF(2,0,2,0), src, dest);
#else
  orc_mmx_emit_movq (p, src, dest);
#endif
}

static void
mmx_rule_splatw3q (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

#ifndef MMX
  orc_mmx_emit_pshuflw (p, ORC_MMX_SHUF(3,3,3,3), dest, dest);
  orc_mmx_emit_pshufhw (p, ORC_MMX_SHUF(3,3,3,3), dest, dest);
#else
  orc_mmx_emit_pshufw (p, ORC_MMX_SHUF(3,3,3,3), dest, dest);
#endif
}

static void
mmx_rule_splatbw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_punpcklbw (p, dest, dest);
}

static void
mmx_rule_splatbl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_punpcklbw (p, dest, dest);
  orc_mmx_emit_punpcklwd (p, dest, dest);
}

static void
mmx_rule_div255w (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmpc = orc_compiler_get_constant (p, 2, 0x8081);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_pmulhuw(p, tmpc, dest);
  orc_mmx_emit_psrlw_imm (p, 7, dest);
}

#if 1
static void
mmx_rule_divluw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  /* About 5.2 cycles per array member on ginger */
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int a = orc_compiler_get_temp_reg (p);
  const int j = orc_compiler_get_temp_reg (p);
  const int j2 = orc_compiler_get_temp_reg (p);
  const int l = orc_compiler_get_temp_reg (p);
  const int divisor = orc_compiler_get_temp_reg (p);
  const int tmp = orc_compiler_get_constant (p, 2, 0x8000);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_movq (p, src1, divisor);
  orc_mmx_emit_psllw_imm (p, 8, divisor);
  orc_mmx_emit_psrlw_imm (p, 1, divisor);

  orc_mmx_load_constant (p, a, 2, 0x00ff);
  orc_mmx_emit_movq (p, tmp, j);
  orc_mmx_emit_psrlw_imm (p, 8, j);

  orc_mmx_emit_pxor (p, tmp, dest);

  for (int i = 0; i < 7; i++) {
    orc_mmx_emit_movq (p, divisor, l);
    orc_mmx_emit_pxor (p, tmp, l);
    orc_mmx_emit_pcmpgtw (p, dest, l);
    orc_mmx_emit_movq (p, l, j2);
    orc_mmx_emit_pandn (p, divisor, l);
    orc_mmx_emit_psubw (p, l, dest);
    orc_mmx_emit_psrlw_imm (p, 1, divisor);

     orc_mmx_emit_pand (p, j, j2);
     orc_mmx_emit_pxor (p, j2, a);
     orc_mmx_emit_psrlw_imm (p, 1, j);
  }

  orc_mmx_emit_movq (p, divisor, l);
  orc_mmx_emit_pxor (p, tmp, l);
  orc_mmx_emit_pcmpgtw (p, dest, l);
  orc_mmx_emit_pand (p, j, l);
  orc_mmx_emit_pxor (p, l, a);

  orc_mmx_emit_movq (p, a, dest);
}
#else
static void
mmx_rule_divluw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  /* About 8.4 cycles per array member on ginger */
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int b = orc_compiler_get_temp_reg (p);
  const int a = orc_compiler_get_temp_reg (p);
  const int k = orc_compiler_get_temp_reg (p);
  const int j = orc_compiler_get_temp_reg (p);
  const int tmp = orc_compiler_get_constant (p, 2, 0x00ff);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_movq (p, dest, b);
  orc_mmx_emit_pand (p, tmp, src1);

  orc_mmx_emit_pxor (p, tmp, b);

  orc_mmx_emit_pxor (p, a, a);
  orc_mmx_emit_movq (p, tmp, j);
  orc_mmx_emit_psrlw_imm (p, 8, j);

  for (int i = 0; i < 8; i++) {
    orc_mmx_emit_por (p, j, a);
    orc_mmx_emit_movq (p, a, k);
    orc_mmx_emit_pmullw (p, src1, k);
    orc_mmx_emit_pxor (p, tmp, k);
    orc_mmx_emit_pcmpgtw (p, b, k);
    orc_mmx_emit_pand (p, j, k);
    orc_mmx_emit_pxor (p, k, a);
    orc_mmx_emit_psrlw_imm (p, 1, j);
  }

  orc_mmx_emit_movq (p, a, dest);
}
#endif

static void
mmx_rule_mulsbw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_punpcklbw (p, src1, tmp);
  orc_mmx_emit_psraw_imm (p, 8, tmp);
  orc_mmx_emit_punpcklbw (p, dest, dest);
  orc_mmx_emit_psraw_imm (p, 8, dest);
  orc_mmx_emit_pmullw (p, tmp, dest);
}

static void
mmx_rule_mulubw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_punpcklbw (p, src1, tmp);
  orc_mmx_emit_psrlw_imm (p, 8, tmp);
  orc_mmx_emit_punpcklbw (p, dest, dest);
  orc_mmx_emit_psrlw_imm (p, 8, dest);
  orc_mmx_emit_pmullw (p, tmp, dest);
}

static void
mmx_rule_mullb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);
  const int tmp2 = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_movq (p, dest, tmp);

  orc_mmx_emit_pmullw (p, src1, dest);
  orc_mmx_emit_psllw_imm (p, 8, dest);
  orc_mmx_emit_psrlw_imm (p, 8, dest);

  orc_mmx_emit_movq (p, src1, tmp2);
  orc_mmx_emit_psraw_imm (p, 8, tmp2);
  orc_mmx_emit_psraw_imm (p, 8, tmp);
  orc_mmx_emit_pmullw (p, tmp2, tmp);
  orc_mmx_emit_psllw_imm (p, 8, tmp);

  orc_mmx_emit_por (p, tmp, dest);
}

static void
mmx_rule_mulhsb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);
  const int tmp2 = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_movq (p, src1, tmp);
  orc_mmx_emit_movq (p, dest, tmp2);
  orc_mmx_emit_psllw_imm (p, 8, tmp);
  orc_mmx_emit_psraw_imm (p, 8, tmp);

  orc_mmx_emit_psllw_imm (p, 8, dest);
  orc_mmx_emit_psraw_imm (p, 8, dest);

  orc_mmx_emit_pmullw (p, tmp, dest);
  orc_mmx_emit_psrlw_imm (p, 8, dest);

  orc_mmx_emit_movq (p, src1, tmp);
  orc_mmx_emit_psraw_imm (p, 8, tmp);
  orc_mmx_emit_psraw_imm (p, 8, tmp2);
  orc_mmx_emit_pmullw (p, tmp, tmp2);
  orc_mmx_emit_psrlw_imm (p, 8, tmp2);
  orc_mmx_emit_psllw_imm (p, 8, tmp2);
  orc_mmx_emit_por (p, tmp2, dest);
}

static void
mmx_rule_mulhub (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);
  const int tmp2 = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_movq (p, src1, tmp);
  orc_mmx_emit_movq (p, dest, tmp2);
  orc_mmx_emit_psllw_imm (p, 8, tmp);
  orc_mmx_emit_psrlw_imm (p, 8, tmp);

  orc_mmx_emit_psllw_imm (p, 8, dest);
  orc_mmx_emit_psrlw_imm (p, 8, dest);

  orc_mmx_emit_pmullw (p, tmp, dest);
  orc_mmx_emit_psrlw_imm (p, 8, dest);

  orc_mmx_emit_movq (p, src1, tmp);
  orc_mmx_emit_psrlw_imm (p, 8, tmp);
  orc_mmx_emit_psrlw_imm (p, 8, tmp2);
  orc_mmx_emit_pmullw (p, tmp, tmp2);
  orc_mmx_emit_psrlw_imm (p, 8, tmp2);
  orc_mmx_emit_psllw_imm (p, 8, tmp2);
  orc_mmx_emit_por (p, tmp2, dest);
}

static void
mmx_rule_mulswl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_pmulhw (p, src1, tmp);
  orc_mmx_emit_pmullw (p, src1, dest);
  orc_mmx_emit_punpcklwd (p, tmp, dest);
}

static void
mmx_rule_muluwl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_pmulhuw (p, src1, tmp);
  orc_mmx_emit_pmullw (p, src1, dest);
  orc_mmx_emit_punpcklwd (p, tmp, dest);
}

static void
mmx_rule_mulll_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int offset = ORC_STRUCT_OFFSET (OrcExecutor, arrays[ORC_VAR_T1]);

  orc_x86_emit_mov_mmx_memoffset (p, ORC_REG_SIZE, p->vars[insn->src_args[0]].alloc,
      offset, p->exec_reg, FALSE, FALSE);
  orc_x86_emit_mov_mmx_memoffset (p, ORC_REG_SIZE, p->vars[insn->src_args[1]].alloc,
      offset + ORC_REG_SIZE, p->exec_reg, FALSE, FALSE);

  for (int i = 0; i < (1 << p->insn_shift); i++) {
     orc_x86_emit_mov_memoffset_reg (p, 4, offset + 4 * i, p->exec_reg,
         p->gp_tmpreg);
     orc_x86_emit_imul_memoffset_reg (p, 4, offset + ORC_REG_SIZE + 4 * i, p->exec_reg,
         p->gp_tmpreg);
     orc_x86_emit_mov_reg_memoffset (p, 4, p->gp_tmpreg, offset + 4 * i,
         p->exec_reg);
  }

  orc_x86_emit_mov_memoffset_mmx (p, ORC_REG_SIZE, offset, p->exec_reg,
      p->vars[insn->dest_args[0]].alloc, FALSE);
}

#ifndef MMX
static void
mmx_rule_mulhsl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);
  const int tmp2 = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_pshufd (p, ORC_MMX_SHUF(2,3,0,1), dest, tmp);
  orc_mmx_emit_pshufd (p, ORC_MMX_SHUF (2, 3, 0, 1), src1, tmp2);
  orc_mmx_emit_pmuldq (p, src1, dest);
  orc_mmx_emit_pmuldq (p, tmp, tmp2);
  orc_mmx_emit_pshufd (p, ORC_MMX_SHUF(2,0,3,1), dest, dest);
  orc_mmx_emit_pshufd (p, ORC_MMX_SHUF(2,0,3,1), tmp2, tmp2);
  orc_mmx_emit_punpckldq (p, tmp2, dest);
}
#endif

#ifndef MMX
static void
mmx_rule_mulhsl_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int regsize = p->is_64bit ? 8 : 4;
  const int offset = ORC_STRUCT_OFFSET (OrcExecutor, arrays[ORC_VAR_T1]);

  orc_x86_emit_mov_mmx_memoffset (p, 16, p->vars[insn->src_args[0]].alloc,
      offset, p->exec_reg, FALSE, FALSE);
  orc_x86_emit_mov_mmx_memoffset (p, 16, p->vars[insn->src_args[1]].alloc,
      offset + 16, p->exec_reg, FALSE, FALSE);
  orc_x86_emit_mov_reg_memoffset (p, regsize, X86_EAX, offset + 32,
      p->exec_reg);
  orc_x86_emit_mov_reg_memoffset (p, regsize, X86_EDX, offset + 40,
      p->exec_reg);

  for (int i = 0; i < (1 << p->insn_shift); i++) {
     orc_x86_emit_mov_memoffset_reg (p, 4, offset + 4 * i, p->exec_reg,
         X86_EAX);
     orc_x86_emit_cpuinsn_memoffset (p, ORC_X86_imul_rm, 4, offset + 16 + 4 * i,
         p->exec_reg);
     orc_x86_emit_mov_reg_memoffset (p, 4, X86_EDX, offset + 4 * i,
         p->exec_reg);
  }

  orc_x86_emit_mov_memoffset_mmx (p, 16, offset, p->exec_reg,
      p->vars[insn->dest_args[0]].alloc, FALSE);
  orc_x86_emit_mov_memoffset_reg (p, regsize, offset + 32, p->exec_reg, X86_EAX);
  orc_x86_emit_mov_memoffset_reg (p, regsize, offset + 40, p->exec_reg, X86_EDX);
}

static void
mmx_rule_mulhul (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);
  const int tmp2 = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_pshufd (p, ORC_MMX_SHUF(2,3,0,1), dest, tmp);
  orc_mmx_emit_pshufd (p, ORC_MMX_SHUF (2, 3, 0, 1), src1, tmp2);
  orc_mmx_emit_pmuludq (p, src1, dest);
  orc_mmx_emit_pmuludq (p, tmp, tmp2);
  orc_mmx_emit_pshufd (p, ORC_MMX_SHUF(2,0,3,1), dest, dest);
  orc_mmx_emit_pshufd (p, ORC_MMX_SHUF(2,0,3,1), tmp2, tmp2);
  orc_mmx_emit_punpckldq (p, tmp2, dest);
}
#endif

#ifndef MMX
static void
mmx_rule_mulslq (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_movq (p, src1, tmp);
  orc_mmx_emit_punpckldq (p, dest, dest);
  orc_mmx_emit_punpckldq (p, tmp, tmp);
  orc_mmx_emit_pmuldq (p, tmp, dest);
}

static void
mmx_rule_mulslq_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int regsize = p->is_64bit ? 8 : 4;
  const int offset = ORC_STRUCT_OFFSET (OrcExecutor, arrays[ORC_VAR_T1]);

  orc_x86_emit_mov_mmx_memoffset (p, 8, p->vars[insn->src_args[0]].alloc,
      offset, p->exec_reg, FALSE, FALSE);
  orc_x86_emit_mov_mmx_memoffset (p, 8, p->vars[insn->src_args[1]].alloc,
      offset + 8, p->exec_reg, FALSE, FALSE);
  orc_x86_emit_mov_reg_memoffset (p, regsize, X86_EAX, offset + 32,
      p->exec_reg);
  orc_x86_emit_mov_reg_memoffset (p, regsize, X86_EDX, offset + 40,
      p->exec_reg);

  for (int i = 0; i < (1 << p->insn_shift); i++) {
     orc_x86_emit_mov_memoffset_reg (p, 4, offset + 4 * i, p->exec_reg,
         X86_EAX);
     orc_x86_emit_cpuinsn_memoffset (p, ORC_X86_imul_rm, 4, offset + 8 + 4 * i,
         p->exec_reg);
     orc_x86_emit_mov_reg_memoffset (p, 4, X86_EAX, offset + 16 + 8 * i,
         p->exec_reg);
     orc_x86_emit_mov_reg_memoffset (p, 4, X86_EDX, offset + 16 + 8 * i + 4,
         p->exec_reg);
  }

  orc_x86_emit_mov_memoffset_mmx (p, 16, offset + 16, p->exec_reg,
      p->vars[insn->dest_args[0]].alloc, FALSE);
  orc_x86_emit_mov_memoffset_reg (p, regsize, offset + 32, p->exec_reg, X86_EAX);
  orc_x86_emit_mov_memoffset_reg (p, regsize, offset + 40, p->exec_reg, X86_EDX);
}

static void
mmx_rule_mululq (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_movq (p, src1, tmp);
  orc_mmx_emit_punpckldq (p, dest, dest);
  orc_mmx_emit_punpckldq (p, tmp, tmp);
  orc_mmx_emit_pmuludq (p, tmp, dest);
}
#endif

static void
mmx_rule_select0lw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
     orc_mmx_emit_movq (p, src, dest);
  }

  /* FIXME slow */
  /* same as convlw */

  orc_mmx_emit_pslld_imm (p, 16, dest);
  orc_mmx_emit_psrad_imm (p, 16, dest);
  orc_mmx_emit_packssdw (p, dest, dest);
}

static void
mmx_rule_select1lw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
     orc_mmx_emit_movq (p, src, dest);
  }

  /* FIXME slow */

  orc_mmx_emit_psrad_imm (p, 16, dest);
  orc_mmx_emit_packssdw (p, dest, dest);
}

static void
mmx_rule_select0ql (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  /* values of dest are shifted away so don't matter */

  /* same as convql */
#ifndef MMX
  orc_mmx_emit_pshufd (p, ORC_MMX_SHUF(2,0,2,0), src, dest);
#else
  orc_mmx_emit_movq (p, src, dest);
#endif
}

static void
mmx_rule_select1ql (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  /* values of dest are shifted away so don't matter */

  orc_mmx_emit_psrlq_imm (p, 32, dest);
#ifndef MMX
  orc_mmx_emit_pshufd (p, ORC_MMX_SHUF(2,0,2,0), src, dest);
#else
  orc_mmx_emit_movq (p, src, dest);
#endif
}

static void
mmx_rule_select0wb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
     orc_mmx_emit_movq (p, src, dest);
  }

  /* FIXME slow */
  /* same as convwb */

  orc_mmx_emit_psllw_imm (p, 8, dest);
  orc_mmx_emit_psraw_imm (p, 8, dest);
  orc_mmx_emit_packsswb (p, dest, dest);
}

static void
mmx_rule_select1wb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
     orc_mmx_emit_movq (p, src, dest);
  }

  /* FIXME slow */

  orc_mmx_emit_psraw_imm (p, 8, dest);
  orc_mmx_emit_packsswb (p, dest, dest);
}

static void
mmx_rule_splitql (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest1 = p->vars[insn->dest_args[0]].alloc;
  const int dest2 = p->vars[insn->dest_args[1]].alloc;
  const int zero = orc_compiler_get_constant (p, 4, 0);

  /* values of dest are shifted away so don't matter */

#ifndef MMX
  orc_mmx_emit_pshufd (p, ORC_MMX_SHUF (3, 1, 3, 1), src, dest1);
  orc_mmx_emit_punpcklqdq (p, zero, dest1);
  orc_mmx_emit_pshufd (p, ORC_MMX_SHUF (2, 0, 2, 0), src, dest2);
  orc_mmx_emit_punpcklqdq (p, zero, dest2);
#else
  orc_mmx_emit_movq (p, src, dest2);
  orc_mmx_emit_pshufw (p, ORC_MMX_SHUF (3, 2, 3, 2), src, dest1);
  orc_mmx_emit_punpckldq (p, zero, dest1);
  orc_mmx_emit_punpckldq (p, zero, dest2);
#endif
}

static void
mmx_rule_splitlw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest1 = p->vars[insn->dest_args[0]].alloc;
  const int dest2 = p->vars[insn->dest_args[1]].alloc;

  /* values of dest are shifted away so don't matter */

  /* FIXME slow */
  if (dest1 != src)
    orc_mmx_emit_movq (p, src, dest1);
  if (dest2 != src)
    orc_mmx_emit_movq (p, src, dest2);

  orc_mmx_emit_psrad_imm (p, 16, dest1);
  orc_mmx_emit_packssdw (p, dest1, dest1);

  orc_mmx_emit_pslld_imm (p, 16, dest2);
  orc_mmx_emit_psrad_imm (p, 16, dest2);
  orc_mmx_emit_packssdw (p, dest2, dest2);
}

static void
mmx_rule_splitwb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest1 = p->vars[insn->dest_args[0]].alloc;
  const int dest2 = p->vars[insn->dest_args[1]].alloc;
  const int tmp = orc_compiler_get_constant (p, 2, 0xff);

  /* values of dest are shifted away so don't matter */

  ORC_DEBUG ("got tmp %d", tmp);
  /* FIXME slow */

  if (dest1 != src)
    orc_mmx_emit_movq (p, src, dest1);
  if (dest2 != src)
    orc_mmx_emit_movq (p, src, dest2);

  orc_mmx_emit_psraw_imm (p, 8, dest1);
  orc_mmx_emit_packsswb (p, dest1, dest1);

#if 0
  orc_mmx_emit_psllw_imm (p, 8, dest2);
  orc_mmx_emit_psraw_imm (p, 8, dest2);
  orc_mmx_emit_packsswb (p, dest2, dest2);
#else
  orc_mmx_emit_pand (p, tmp, dest2);
  orc_mmx_emit_packuswb (p, dest2, dest2);
#endif
}

static void
mmx_rule_mergebw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_punpcklbw (p, src1, dest);
}

static void
mmx_rule_mergewl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_punpcklwd (p, src1, dest);
}

static void
mmx_rule_mergelq (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_punpckldq (p, src1, dest);
}

static void
mmx_rule_swapw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_movq (p, src, tmp);
  orc_mmx_emit_psllw_imm (p, 8, tmp);
  orc_mmx_emit_psrlw_imm (p, 8, dest);
  orc_mmx_emit_por (p, tmp, dest);
}

static void
mmx_rule_swapl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_movq (p, src, tmp);
  orc_mmx_emit_pslld_imm (p, 16, tmp);
  orc_mmx_emit_psrld_imm (p, 16, dest);
  orc_mmx_emit_por (p, tmp, dest);
  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_psllw_imm (p, 8, tmp);
  orc_mmx_emit_psrlw_imm (p, 8, dest);
  orc_mmx_emit_por (p, tmp, dest);
}

static void
mmx_rule_swapwl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_movq (p, src, tmp);
  orc_mmx_emit_pslld_imm (p, 16, tmp);
  orc_mmx_emit_psrld_imm (p, 16, dest);
  orc_mmx_emit_por (p, tmp, dest);
}

static void
mmx_rule_swapq (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_movq (p, src, tmp);
  orc_mmx_emit_psllq_imm (p, 32, tmp);
  orc_mmx_emit_psrlq_imm (p, 32, dest);
  orc_mmx_emit_por (p, tmp, dest);
  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_pslld_imm (p, 16, tmp);
  orc_mmx_emit_psrld_imm (p, 16, dest);
  orc_mmx_emit_por (p, tmp, dest);
  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_psllw_imm (p, 8, tmp);
  orc_mmx_emit_psrlw_imm (p, 8, dest);
  orc_mmx_emit_por (p, tmp, dest);
}

static void
mmx_rule_swaplq (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

#ifndef MMX
  orc_mmx_emit_pshufd (p, ORC_MMX_SHUF(2,3,0,1), dest, dest);
#else
  orc_mmx_emit_pshufw (p, ORC_MMX_SHUF(1,0,3,2), dest, dest);
#endif
}

#ifndef MMX
static void
mmx_rule_swapw_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_try_get_constant_long (p, 0x02030001, 0x06070405,
      0x0a0b0809, 0x0e0f0c0d);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  if (tmp != ORC_REG_INVALID) {
    orc_mmx_emit_pshufb (p, tmp, dest);
  } else {
    mmx_rule_swapw (p, user, insn);
  }
}

static void
mmx_rule_swapl_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_try_get_constant_long (p, 0x00010203, 0x04050607,
      0x08090a0b, 0x0c0d0e0f);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  if (tmp != ORC_REG_INVALID) {
    orc_mmx_emit_pshufb (p, tmp, dest);
  } else {
    mmx_rule_swapl (p, user, insn);
  }
}

static void
mmx_rule_swapwl_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_try_get_constant_long (p, 0x01000302, 0x05040706,
      0x09080b0a, 0x0d0c0f0e);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  if (tmp != ORC_REG_INVALID) {
    orc_mmx_emit_pshufb (p, tmp, dest);
  } else {
    mmx_rule_swapl (p, user, insn);
  }
}

static void
mmx_rule_swapq_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_try_get_constant_long (p, 0x04050607, 0x00010203,
      0x0c0d0e0f, 0x08090a0b);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  if (tmp != ORC_REG_INVALID) {
    orc_mmx_emit_pshufb (p, tmp, dest);
  } else {
    mmx_rule_swapq (p, user, insn);
  }
}

static void
mmx_rule_splitlw_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest1 = p->vars[insn->dest_args[0]].alloc;
  const int dest2 = p->vars[insn->dest_args[1]].alloc;
  const int tmp1 = orc_compiler_try_get_constant_long (p, 0x07060302,
      0x0f0e0b0a, 0x07060302, 0x0f0e0b0a);
  const int tmp2 = orc_compiler_try_get_constant_long (p, 0x05040100,
      0x0d0c0908, 0x05040100, 0x0d0c0908);

  if (src != dest1) {
    orc_mmx_emit_movq (p, src, dest1);
  }

  if (tmp1 != ORC_REG_INVALID && tmp2 != ORC_REG_INVALID) {
    orc_mmx_emit_pshufb (p, tmp1, dest1);
    if (dest2 != src)
      orc_mmx_emit_movq (p, src, dest2);
    orc_mmx_emit_pshufb (p, tmp2, dest2);
  } else {
    mmx_rule_splitlw (p, user, insn);
  }
}


static void
mmx_rule_splitwb_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest1 = p->vars[insn->dest_args[0]].alloc;
  const int dest2 = p->vars[insn->dest_args[1]].alloc;
  const int tmp1 = orc_compiler_try_get_constant_long (p, 0x07050301,
      0x0f0d0b09, 0x07050301, 0x0f0d0b09);
  const int tmp2 = orc_compiler_try_get_constant_long (p, 0x06040200,
      0x0e0c0a08, 0x06040200, 0x0e0c0a08);

  if (src != dest1) {
    orc_mmx_emit_movq (p, src, dest1);
  }

  if (tmp1 != ORC_REG_INVALID && tmp2 != ORC_REG_INVALID) {
    orc_mmx_emit_pshufb (p, tmp1, dest1);
    if (dest2 != src)
      orc_mmx_emit_movq (p, src, dest2);
    orc_mmx_emit_pshufb (p, tmp2, dest2);
  } else {
    mmx_rule_splitwb (p, user, insn);
  }
}

static void
mmx_rule_select0lw_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_try_get_constant_long (p, 0x05040100, 0x0d0c0908,
      0x05040100, 0x0d0c0908);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  if (tmp != ORC_REG_INVALID) {
    orc_mmx_emit_pshufb (p, tmp, dest);
  } else {
    mmx_rule_select0lw (p, user, insn);
  }
}

static void
mmx_rule_select1lw_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_try_get_constant_long (p, 0x07060302, 0x0f0e0b0a,
      0x07060302, 0x0f0e0b0a);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  if (tmp != ORC_REG_INVALID) {
    orc_mmx_emit_pshufb (p, tmp, dest);
  } else {
    mmx_rule_select1lw (p, user, insn);
  }
}

static void
mmx_rule_select0wb_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_try_get_constant_long (p, 0x06040200, 0x0e0c0a08,
      0x06040200, 0x0e0c0a08);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  if (tmp != ORC_REG_INVALID) {
    orc_mmx_emit_pshufb (p, tmp, dest);
  } else {
    mmx_rule_select0wb (p, user, insn);
  }
}

static void
mmx_rule_select1wb_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_try_get_constant_long (p, 0x07050301, 0x0f0d0b09,
      0x07050301, 0x0f0d0b09);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  if (tmp != ORC_REG_INVALID) {
    orc_mmx_emit_pshufb (p, tmp, dest);
  } else {
    mmx_rule_select1wb (p, user, insn);
  }
}
#endif

/* slow rules */

static void
mmx_rule_maxuw_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_constant (p, 2, 0x8000);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_pxor (p, tmp, src1);
  orc_mmx_emit_pxor(p, tmp, dest);
  orc_mmx_emit_pmaxsw (p, src1, dest);
  orc_mmx_emit_pxor (p, tmp, src1);
  orc_mmx_emit_pxor(p, tmp, dest);
}

static void
mmx_rule_minuw_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_constant (p, 2, 0x8000);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_pxor (p, tmp, src1);
  orc_mmx_emit_pxor(p, tmp, dest);
  orc_mmx_emit_pminsw (p, src1, dest);
  orc_mmx_emit_pxor (p, tmp, src1);
  orc_mmx_emit_pxor(p, tmp, dest);
}

static void
mmx_rule_avgsb_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_constant (p, 1, 0x80);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_pxor (p, tmp, src1);
  orc_mmx_emit_pxor(p, tmp, dest);
  orc_mmx_emit_pavgb (p, src1, dest);
  orc_mmx_emit_pxor (p, tmp, src1);
  orc_mmx_emit_pxor(p, tmp, dest);
}

static void
mmx_rule_avgsw_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_constant (p, 2, 0x8000);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_pxor (p, tmp, src1);
  orc_mmx_emit_pxor(p, tmp, dest);
  orc_mmx_emit_pavgw (p, src1, dest);
  orc_mmx_emit_pxor (p, tmp, src1);
  orc_mmx_emit_pxor(p, tmp, dest);
}

static void
mmx_rule_maxsb_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }
  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_pcmpgtb (p, src1, tmp);
  orc_mmx_emit_pand (p, tmp, dest);
  orc_mmx_emit_pandn (p, src1, tmp);
  orc_mmx_emit_por (p, tmp, dest);
}

static void
mmx_rule_minsb_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_movq (p, src1, tmp);
  orc_mmx_emit_pcmpgtb (p, dest, tmp);
  orc_mmx_emit_pand (p, tmp, dest);
  orc_mmx_emit_pandn (p, src1, tmp);
  orc_mmx_emit_por (p, tmp, dest);
}

static void
mmx_rule_maxsl_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_pcmpgtd (p, src1, tmp);
  orc_mmx_emit_pand (p, tmp, dest);
  orc_mmx_emit_pandn (p, src1, tmp);
  orc_mmx_emit_por (p, tmp, dest);
}

static void
mmx_rule_minsl_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_movq (p, src1, tmp);
  orc_mmx_emit_pcmpgtd (p, dest, tmp);
  orc_mmx_emit_pand (p, tmp, dest);
  orc_mmx_emit_pandn (p, src1, tmp);
  orc_mmx_emit_por (p, tmp, dest);
}

static void
mmx_rule_maxul_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);
  const int tmpc = orc_compiler_get_constant (p, 4, 0x80000000);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_pxor (p, tmpc, src1);
  orc_mmx_emit_pxor(p, tmpc, dest);

  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_pcmpgtd (p, src1, tmp);
  orc_mmx_emit_pand (p, tmp, dest);
  orc_mmx_emit_pandn (p, src1, tmp);
  orc_mmx_emit_por (p, tmp, dest);

  orc_mmx_emit_pxor (p, tmpc, src1);
  orc_mmx_emit_pxor(p, tmpc, dest);
}

static void
mmx_rule_minul_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);
  const int tmpc = orc_compiler_get_constant (p, 4, 0x80000000);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_pxor (p, tmpc, src1);
  orc_mmx_emit_pxor(p, tmpc, dest);

  orc_mmx_emit_movq (p, src1, tmp);
  orc_mmx_emit_pcmpgtd (p, dest, tmp);
  orc_mmx_emit_pand (p, tmp, dest);
  orc_mmx_emit_pandn (p, src1, tmp);
  orc_mmx_emit_por (p, tmp, dest);

  orc_mmx_emit_pxor (p, tmpc, src1);
  orc_mmx_emit_pxor(p, tmpc, dest);
}

static void
mmx_rule_avgsl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  /* (a+b+1) >> 1 = (a|b) - ((a^b)>>1) */

  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_pxor (p, src1, tmp);
  orc_mmx_emit_psrad_imm(p, 1, tmp);

  orc_mmx_emit_por (p, src1, dest);
  orc_mmx_emit_psubd(p, tmp, dest);
}

static void
mmx_rule_avgul (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  /* (a+b+1) >> 1 = (a|b) - ((a^b)>>1) */

  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_pxor (p, src1, tmp);
  orc_mmx_emit_psrld_imm(p, 1, tmp);

  orc_mmx_emit_por (p, src1, dest);
  orc_mmx_emit_psubd(p, tmp, dest);
}

static void
mmx_rule_addssl_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

#if 0
  int tmp2 = orc_compiler_get_temp_reg (p);
  int tmp3 = orc_compiler_get_temp_reg (p);

  orc_mmx_emit_movq (p, src1, tmp);
  orc_mmx_emit_pand (p, dest, tmp);

  orc_mmx_emit_movq (p, src1, tmp2);
  orc_mmx_emit_pxor (p, dest, tmp2);
  orc_mmx_emit_psrad_imm (p, 1, tmp2);
  orc_mmx_emit_paddd (p, tmp2, tmp);

  orc_mmx_emit_psrad (p, 30, tmp);
  orc_mmx_emit_pslld (p, 30, tmp);
  orc_mmx_emit_movq (p, tmp, tmp2);
  orc_mmx_emit_pslld_imm (p, 1, tmp2);
  orc_mmx_emit_movq (p, tmp, tmp3);
  orc_mmx_emit_pxor (p, tmp2, tmp3);
  orc_mmx_emit_psrad_imm (p, 31, tmp3);

  orc_mmx_emit_psrad_imm (p, 31, tmp2);
  tmp = orc_compiler_get_constant (p, 4, 0x80000000);
  orc_mmx_emit_pxor (p, tmp, tmp2); /*  clamped value */
  orc_mmx_emit_pand (p, tmp3, tmp2);

  orc_mmx_emit_paddd (p, src1, dest);
  orc_mmx_emit_pandn (p, dest, tmp3); /*  tmp is mask: ~0 is for clamping */
  orc_mmx_emit_movq (p, tmp3, dest);

  orc_mmx_emit_por (p, tmp2, dest);
#endif

  const int s = orc_compiler_get_temp_reg (p);
  const int t = orc_compiler_get_temp_reg (p);

  /*
     From Tim Terriberry: (slightly faster than above)

     m=0xFFFFFFFF;
     s=_a;
     t=_a;
     s^=_b;
     _a+=_b;
     t^=_a;
     t^=m;
     m>>=1;
     s|=t;
     t=_b;
     s>>=31;
     t>>=31;
     _a&=s;
     t^=m;
     s=~s&t;
     _a|=s; 
  */

  orc_mmx_emit_movq (p, dest, s);
  orc_mmx_emit_movq (p, dest, t);
  orc_mmx_emit_pxor (p, src1, s);
  orc_mmx_emit_paddd (p, src1, dest);
  orc_mmx_emit_pxor (p, dest, t);
  int tmp = orc_compiler_get_constant (p, 4, 0xffffffff);
  orc_mmx_emit_pxor (p, tmp, t);
  orc_mmx_emit_por (p, t, s);
  orc_mmx_emit_movq (p, src1, t);
  orc_mmx_emit_psrad_imm (p, 31, s);
  orc_mmx_emit_psrad_imm (p, 31, t);
  orc_mmx_emit_pand (p, s, dest);
  tmp = orc_compiler_get_constant (p, 4, 0x7fffffff);
  orc_mmx_emit_pxor (p, tmp, t);
  orc_mmx_emit_pandn (p, t, s);
  orc_mmx_emit_por (p, s, dest);
}

static void
mmx_rule_subssl_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = orc_compiler_get_temp_constant (p, 4, 0xffffffff);
  const int tmp2 = orc_compiler_get_temp_reg (p);
  const int tmp3 = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_pxor (p, src1, tmp);
  orc_mmx_emit_movq (p, tmp, tmp2);
  orc_mmx_emit_por (p, dest, tmp);

  orc_mmx_emit_pxor (p, dest, tmp2);
  orc_mmx_emit_psrad_imm (p, 1, tmp2);
  orc_mmx_emit_psubd (p, tmp2, tmp);

  orc_mmx_emit_psrad_imm (p, 30, tmp);
  orc_mmx_emit_pslld_imm (p, 30, tmp);
  orc_mmx_emit_movq (p, tmp, tmp2);
  orc_mmx_emit_pslld_imm (p, 1, tmp2);
  orc_mmx_emit_movq (p, tmp, tmp3);
  orc_mmx_emit_pxor (p, tmp2, tmp3);
  orc_mmx_emit_psrad_imm (p, 31, tmp3); /*  tmp3 is mask: ~0 is for clamping */

  orc_mmx_emit_psrad_imm (p, 31, tmp2);
  tmp = orc_compiler_get_constant (p, 4, 0x80000000);
  orc_mmx_emit_pxor (p, tmp, tmp2); /*  clamped value */
  orc_mmx_emit_pand (p, tmp3, tmp2);

  orc_mmx_emit_psubd (p, src1, dest);
  orc_mmx_emit_pandn (p, dest, tmp3);
  orc_mmx_emit_movq (p, tmp3, dest);

  orc_mmx_emit_por (p, tmp2, dest);

}

static void
mmx_rule_addusl_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);
  const int tmp2 = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

#if 0
  /* an alternate version.  slower. */
  /* Compute the bit that gets carried from bit 0 to bit 1 */
  orc_mmx_emit_movq (p, src1, tmp);
  orc_mmx_emit_pand (p, dest, tmp);
  orc_mmx_emit_pslld_imm (p, 31, tmp);
  orc_mmx_emit_psrld_imm (p, 31, tmp);

  /* Add in (src>>1) */
  orc_mmx_emit_movq (p, src1, tmp2);
  orc_mmx_emit_psrld_imm (p, 1, tmp2);
  orc_mmx_emit_paddd (p, tmp2, tmp);

  /* Add in (dest>>1) */
  orc_mmx_emit_movq (p, dest, tmp2);
  orc_mmx_emit_psrld_imm (p, 1, tmp2);
  orc_mmx_emit_paddd (p, tmp2, tmp);

  /* turn overflow bit into mask */
  orc_mmx_emit_psrad_imm (p, 31, tmp);

  /* compute the sum, then or over the mask */
  orc_mmx_emit_paddd (p, src1, dest);
  orc_mmx_emit_por (p, tmp, dest);
#endif

  orc_mmx_emit_movq (p, src1, tmp);
  orc_mmx_emit_pand (p, dest, tmp);

  orc_mmx_emit_movq (p, src1, tmp2);
  orc_mmx_emit_pxor (p, dest, tmp2);
  orc_mmx_emit_psrld_imm (p, 1, tmp2);
  orc_mmx_emit_paddd (p, tmp2, tmp);

  orc_mmx_emit_psrad_imm (p, 31, tmp);
  orc_mmx_emit_paddd (p, src1, dest);
  orc_mmx_emit_por (p, tmp, dest);
}

static void
mmx_rule_subusl_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);
  const int tmp2 = orc_compiler_get_temp_reg (p);

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_movq (p, src1, tmp2);
  orc_mmx_emit_psrld_imm (p, 1, tmp2);

  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_psrld_imm (p, 1, tmp);
  orc_mmx_emit_psubd (p, tmp, tmp2);

  /* turn overflow bit into mask */
  orc_mmx_emit_psrad_imm (p, 31, tmp2);

  /* compute the difference, then and over the mask */
  orc_mmx_emit_psubd (p, src1, dest);
  orc_mmx_emit_pand (p, tmp2, dest);

}

#ifndef MMX
/* float ops */

#define UNARY_F(opcode, insn_name, code) \
  static void mmx_rule_##opcode (OrcCompiler *p, void *user, \
      OrcInstruction *insn) \
  { \
    const int src0 = p->vars[insn->src_args[0]].alloc; \
    const int dest = p->vars[insn->dest_args[0]].alloc; \
\
    if (src0 != dest) { \
     orc_mmx_emit_movq (p, src0, dest); \
    } \
\
    orc_mmx_emit_##insn_name (p, src0, dest); \
  }

#define BINARY_F(opcode, insn_name, code) \
  static void mmx_rule_##opcode (OrcCompiler *p, void *user, \
      OrcInstruction *insn) \
  { \
    const int src0 = p->vars[insn->src_args[0]].alloc; \
    const int src1 = p->vars[insn->src_args[1]].alloc; \
    const int dest = p->vars[insn->dest_args[0]].alloc; \
\
    if (src0 != dest) { \
     orc_mmx_emit_movq (p, src0, dest); \
    } \
\
    orc_mmx_emit_##insn_name (p, src1, dest); \
  }

BINARY_F(addf, addps, 0x58)
BINARY_F(subf, subps, 0x5c)
BINARY_F(mulf, mulps, 0x59)
BINARY_F(divf, divps, 0x5e)
UNARY_F(sqrtf, sqrtps, 0x51)
BINARY_F(orf, orps, 0x56)
BINARY_F(andf, andps, 0x54)

#define UNARY_D(opcode,insn_name,code) \
static void \
mmx_rule_ ## opcode (OrcCompiler *p, void *user, OrcInstruction *insn) \
{ \
  orc_mmx_emit_ ## insn_name (p, \
      p->vars[insn->src_args[0]].alloc, \
      p->vars[insn->dest_args[0]].alloc); \
}

#define BINARY_D(opcode, insn_name, code) \
  static void mmx_rule_##opcode (OrcCompiler *p, void *user, \
      OrcInstruction *insn) \
  { \
    const int src0 = p->vars[insn->src_args[0]].alloc; \
    const int src1 = p->vars[insn->src_args[1]].alloc; \
    const int dest = p->vars[insn->dest_args[0]].alloc; \
\
    if (src0 != dest) { \
     orc_mmx_emit_movq (p, src0, dest); \
    } \
\
    orc_mmx_emit_##insn_name (p, src1, dest); \
  }

BINARY_D(addd, addpd, 0x58)
BINARY_D(subd, subpd, 0x5c)
BINARY_D(muld, mulpd, 0x59)
BINARY_D(divd, divpd, 0x5e)
UNARY_D(sqrtd, sqrtpd, 0x51)

static void
mmx_rule_minf (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  if (p->target_flags & ORC_TARGET_FAST_NAN) {
    orc_mmx_emit_minps (p, src1, dest);
  } else {
    const int tmp = orc_compiler_get_temp_reg (p);
    orc_mmx_emit_movq (p, src1, tmp);
    orc_mmx_emit_minps (p, src0, tmp);
    orc_mmx_emit_minps (p, src1, dest);
    orc_mmx_emit_por (p, tmp, dest);
  }
}

static void
mmx_rule_mind (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  if (p->target_flags & ORC_TARGET_FAST_NAN) {
    orc_mmx_emit_minpd (p, src1, dest);
  } else {
    const int tmp = orc_compiler_get_temp_reg (p);
    orc_mmx_emit_movq (p, src1, tmp);
    orc_mmx_emit_minpd (p, src0, tmp);
    orc_mmx_emit_minpd (p, src1, dest);
    orc_mmx_emit_por (p, tmp, dest);
  }
}

static void
mmx_rule_maxf (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  if (p->target_flags & ORC_TARGET_FAST_NAN) {
    orc_mmx_emit_maxps (p, src1, dest);
  } else {
    const int tmp = orc_compiler_get_temp_reg (p);
    orc_mmx_emit_movq (p, src1, tmp);
    orc_mmx_emit_maxps (p, src0, tmp);
    orc_mmx_emit_maxps (p, src1, dest);
    orc_mmx_emit_por (p, tmp, dest);
  }
}

static void
mmx_rule_maxd (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  if (p->target_flags & ORC_TARGET_FAST_NAN) {
    orc_mmx_emit_maxpd (p, src1, dest);
  } else {
    const int tmp = orc_compiler_get_temp_reg (p);
    orc_mmx_emit_movq (p, src1, tmp);
    orc_mmx_emit_maxpd (p, src0, tmp);
    orc_mmx_emit_maxpd (p, src1, dest);
    orc_mmx_emit_por (p, tmp, dest);
  }
}

static void
mmx_rule_cmpeqf (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_cmpeqps (p, src1, dest);
}

static void
mmx_rule_cmpeqd (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_cmpeqpd (p, src1, dest);
}


static void
mmx_rule_cmpltf (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_cmpltps (p, src1, dest);
}

static void
mmx_rule_cmpltd (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_cmpltpd (p, src1, dest);
}


static void
mmx_rule_cmplef (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_cmpleps (p, src1, dest);
}

static void
mmx_rule_cmpled (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src0 = p->vars[insn->src_args[0]].alloc;
  const int src1 = p->vars[insn->src_args[1]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  if (src0 != dest) {
    orc_mmx_emit_movq (p, src0, dest);
  }

  orc_mmx_emit_cmplepd (p, src1, dest);
}


static void
mmx_rule_convfl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);
  const int tmpc = orc_compiler_get_temp_constant (p, 4, 0x80000000);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_movq (p, src, tmp);
  orc_mmx_emit_cvttps2dq (p, src, dest);
  orc_mmx_emit_psrad_imm (p, 31, tmp);
  orc_mmx_emit_pcmpeqd (p, dest, tmpc);
  orc_mmx_emit_pandn (p, tmpc, tmp);
  orc_mmx_emit_paddd (p, tmp, dest);
}

static void
mmx_rule_convdl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmp = orc_compiler_get_temp_reg (p);
  const int tmpc = orc_compiler_get_temp_constant (p, 4, 0x80000000);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_pshufd (p, ORC_MMX_SHUF(3,1,3,1), src, tmp);
  orc_mmx_emit_cvttpd2dq (p, src, dest);
  orc_mmx_emit_psrad_imm (p, 31, tmp);
  orc_mmx_emit_pcmpeqd (p, dest, tmpc);
  orc_mmx_emit_pandn (p, tmpc, tmp);
  orc_mmx_emit_paddd (p, tmp, dest);
}

static void
mmx_rule_convwf (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  orc_mmx_emit_punpcklwd (p, src, dest);
  orc_mmx_emit_psrad_imm (p, 16, dest);
  orc_mmx_emit_cvtdq2ps (p, dest, dest);
}

static void
mmx_rule_convlf (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  orc_mmx_emit_cvtdq2ps (p,
      p->vars[insn->src_args[0]].alloc,
      p->vars[insn->dest_args[0]].alloc);
}

static void
mmx_rule_convld (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  orc_mmx_emit_cvtdq2pd (p,
      p->vars[insn->src_args[0]].alloc,
      p->vars[insn->dest_args[0]].alloc);
}

static void
mmx_rule_convfd (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  orc_mmx_emit_cvtps2pd (p,
      p->vars[insn->src_args[0]].alloc,
      p->vars[insn->dest_args[0]].alloc);
}

static void
mmx_rule_convdf (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  orc_mmx_emit_cvtpd2ps (p,
      p->vars[insn->src_args[0]].alloc,
      p->vars[insn->dest_args[0]].alloc);
}

#define UNARY_SSE41(opcode,insn_name) \
static void \
mmx_rule_ ## opcode ## _mmx41 (OrcCompiler *p, void *user, OrcInstruction *insn) \
{ \
  orc_mmx_emit_ ## insn_name (p, \
      p->vars[insn->src_args[0]].alloc, \
      p->vars[insn->dest_args[0]].alloc); \
}

UNARY_SSE41(convsbw,pmovsxbw);
UNARY_SSE41(convswl,pmovsxwd);
UNARY_SSE41(convslq,pmovsxdq);
UNARY_SSE41(convubw,pmovzxbw);
UNARY_SSE41(convuwl,pmovzxwd);
UNARY_SSE41(convulq,pmovzxdq);

static void
mmx_rule_convwf_mmx41 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;

  orc_mmx_emit_pmovsxwd (p, src, dest);
  orc_mmx_emit_cvtdq2ps (p, dest, dest);
}

static void
mmx_rule_convsssql_mmx41 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  const int src = p->vars[insn->src_args[0]].alloc;
  const int dest = p->vars[insn->dest_args[0]].alloc;
  const int tmpc_max = orc_compiler_get_temp_constant (p, 8, INT32_MAX);
  const int tmpc_min = orc_compiler_get_temp_constant (p, 8, INT32_MIN);
  const int src_backup = orc_compiler_get_temp_reg (p);
  const int tmp = orc_compiler_get_temp_reg (p);
  // Operate over tmp, because we don't know if src or dest are X86_MM0
  orc_mmx_emit_movq (p, src, tmp);
  if (src == X86_MM0) {
    orc_mmx_emit_movq (p, src, src_backup);
  } else {
    orc_mmx_emit_movq (p, X86_MM0, src_backup);
    orc_mmx_emit_movq (p, src, X86_MM0);
  }
  // Apply the same logic as in AVX, only that
  // BLENDVPD expects XMM0 to be the mask
  orc_mmx_emit_pcmpgtq (p, tmpc_max, X86_MM0);
  orc_mmx_emit_blendvpd (p, tmpc_max, tmp);
  orc_mmx_emit_movq (p, tmp, X86_MM0);
  orc_mmx_emit_pcmpgtq (p, tmpc_min, X86_MM0);
  orc_mmx_emit_blendvpd (p, tmp, tmpc_min);
  orc_mmx_emit_pshufd (p, ORC_MMX_SHUF (3, 1, 2, 0), tmpc_min, dest);
  // Undo the changes to src or X86_MM0 (if the latter is not dest)
  if (src == X86_MM0 && src != dest) {
    orc_mmx_emit_movq (p, src_backup, src);
  } else if (dest != X86_MM0) {
    orc_mmx_emit_movq (p, src_backup, X86_MM0);
  }
}
#endif

void
orc_compiler_mmx_register_rules (OrcTarget *target)
{
  OrcRuleSet *rule_set;

#define REG(x) \
  orc_rule_register (rule_set, #x , mmx_rule_ ## x, NULL)

  /* SSE 2 */
#ifndef MMX
  rule_set = orc_rule_set_new (orc_opcode_set_get("sys"), target,
      ORC_TARGET_MMX_MMXEXT);
#else
  rule_set = orc_rule_set_new (orc_opcode_set_get("sys"), target,
      ORC_TARGET_MMX_MMX);
#endif

  orc_rule_register (rule_set, "loadb", mmx_rule_loadX, NULL);
  orc_rule_register (rule_set, "loadw", mmx_rule_loadX, NULL);
  orc_rule_register (rule_set, "loadl", mmx_rule_loadX, NULL);
  orc_rule_register (rule_set, "loadq", mmx_rule_loadX, NULL);
  orc_rule_register (rule_set, "loadoffb", mmx_rule_loadoffX, NULL);
  orc_rule_register (rule_set, "loadoffw", mmx_rule_loadoffX, NULL);
  orc_rule_register (rule_set, "loadoffl", mmx_rule_loadoffX, NULL);
  orc_rule_register (rule_set, "loadupdb", mmx_rule_loadupdb, NULL);
  orc_rule_register (rule_set, "loadupib", mmx_rule_loadupib, NULL);
  orc_rule_register (rule_set, "loadpb", mmx_rule_loadpX, (void *)1);
  orc_rule_register (rule_set, "loadpw", mmx_rule_loadpX, (void *)2);
  orc_rule_register (rule_set, "loadpl", mmx_rule_loadpX, (void *)4);
  orc_rule_register (rule_set, "loadpq", mmx_rule_loadpX, (void *)8);
  orc_rule_register (rule_set, "ldresnearl", mmx_rule_ldresnearl, NULL);
  orc_rule_register (rule_set, "ldreslinl", mmx_rule_ldreslinl, NULL);

  orc_rule_register (rule_set, "storeb", mmx_rule_storeX, NULL);
  orc_rule_register (rule_set, "storew", mmx_rule_storeX, NULL);
  orc_rule_register (rule_set, "storel", mmx_rule_storeX, NULL);
  orc_rule_register (rule_set, "storeq", mmx_rule_storeX, NULL);

  REG(addb);
  REG(addssb);
  REG(addusb);
  REG(andb);
  REG(andnb);
  REG(avgub);
  REG(cmpeqb);
  REG(cmpgtsb);
  REG(maxub);
  REG(minub);
  REG(orb);
  REG(subb);
  REG(subssb);
  REG(subusb);
  REG(xorb);

  REG(addw);
  REG(addssw);
  REG(addusw);
  REG(andw);
  REG(andnw);
  REG(avguw);
  REG(cmpeqw);
  REG(cmpgtsw);
  REG(maxsw);
  REG(minsw);
  REG(mullw);
  REG(mulhsw);
  REG(mulhuw);
  REG(orw);
  REG(subw);
  REG(subssw);
  REG(subusw);
  REG(xorw);

  REG(addl);
  REG(andl);
  REG(andnl);
  REG(cmpeql);
  REG(cmpgtsl);
  REG(orl);
  REG(subl);
  REG(xorl);

  REG(andq);
  REG(andnq);
  REG(orq);
  REG(xorq);

  REG(select0ql);
  REG(select1ql);
  REG(select0lw);
  REG(select1lw);
  REG(select0wb);
  REG(select1wb);
  REG(mergebw);
  REG(mergewl);
  REG(mergelq);

  orc_rule_register (rule_set, "copyb", mmx_rule_copyx, NULL);
  orc_rule_register (rule_set, "copyw", mmx_rule_copyx, NULL);
  orc_rule_register (rule_set, "copyl", mmx_rule_copyx, NULL);
  orc_rule_register (rule_set, "copyq", mmx_rule_copyx, NULL);

  orc_rule_register (rule_set, "shlw", mmx_rule_shift, (void *)0);
  orc_rule_register (rule_set, "shruw", mmx_rule_shift, (void *)1);
  orc_rule_register (rule_set, "shrsw", mmx_rule_shift, (void *)2);
  orc_rule_register (rule_set, "shll", mmx_rule_shift, (void *)3);
  orc_rule_register (rule_set, "shrul", mmx_rule_shift, (void *)4);
  orc_rule_register (rule_set, "shrsl", mmx_rule_shift, (void *)5);
  orc_rule_register (rule_set, "shlq", mmx_rule_shift, (void *)6);
  orc_rule_register (rule_set, "shruq", mmx_rule_shift, (void *)7);
  orc_rule_register (rule_set, "shrsq", mmx_rule_shrsq, NULL);

  orc_rule_register (rule_set, "convsbw", mmx_rule_convsbw, NULL);
  orc_rule_register (rule_set, "convubw", mmx_rule_convubw, NULL);
  orc_rule_register (rule_set, "convssswb", mmx_rule_convssswb, NULL);
  orc_rule_register (rule_set, "convsuswb", mmx_rule_convsuswb, NULL);
  orc_rule_register (rule_set, "convuuswb", mmx_rule_convuuswb, NULL);
  orc_rule_register (rule_set, "convwb", mmx_rule_convwb, NULL);

  orc_rule_register (rule_set, "convswl", mmx_rule_convswl, NULL);
  orc_rule_register (rule_set, "convuwl", mmx_rule_convuwl, NULL);
  orc_rule_register (rule_set, "convssslw", mmx_rule_convssslw, NULL);

  orc_rule_register (rule_set, "convql", mmx_rule_convql, NULL);
  orc_rule_register (rule_set, "convslq", mmx_rule_convslq, NULL);
  orc_rule_register (rule_set, "convulq", mmx_rule_convulq, NULL);
  /* orc_rule_register (rule_set, "convsssql", mmx_rule_convsssql, NULL); */

  orc_rule_register (rule_set, "mulsbw", mmx_rule_mulsbw, NULL);
  orc_rule_register (rule_set, "mulubw", mmx_rule_mulubw, NULL);
  orc_rule_register (rule_set, "mulswl", mmx_rule_mulswl, NULL);
  orc_rule_register (rule_set, "muluwl", mmx_rule_muluwl, NULL);

  orc_rule_register (rule_set, "accw", mmx_rule_accw, NULL);
  orc_rule_register (rule_set, "accl", mmx_rule_accl, NULL);
  orc_rule_register (rule_set, "accsadubl", mmx_rule_accsadubl, NULL);

#ifndef MMX
  /* These require the SSE2 flag, although could be used with MMX.
     That flag is not yet handled. */
  orc_rule_register (rule_set, "mululq", mmx_rule_mululq, NULL);
  REG(addq);
  REG(subq);

  orc_rule_register (rule_set, "addf", mmx_rule_addf, NULL);
  orc_rule_register (rule_set, "subf", mmx_rule_subf, NULL);
  orc_rule_register (rule_set, "mulf", mmx_rule_mulf, NULL);
  orc_rule_register (rule_set, "divf", mmx_rule_divf, NULL);
  orc_rule_register (rule_set, "minf", mmx_rule_minf, NULL);
  orc_rule_register (rule_set, "maxf", mmx_rule_maxf, NULL);
  orc_rule_register (rule_set, "sqrtf", mmx_rule_sqrtf, NULL);
  orc_rule_register (rule_set, "cmpeqf", mmx_rule_cmpeqf, NULL);
  orc_rule_register (rule_set, "cmpltf", mmx_rule_cmpltf, NULL);
  orc_rule_register (rule_set, "cmplef", mmx_rule_cmplef, NULL);
  orc_rule_register (rule_set, "convfl", mmx_rule_convfl, NULL);
  orc_rule_register (rule_set, "convwf", mmx_rule_convwf, NULL);
  orc_rule_register (rule_set, "convlf", mmx_rule_convlf, NULL);
  orc_rule_register (rule_set, "orf", mmx_rule_orf, NULL);
  orc_rule_register (rule_set, "andf", mmx_rule_andf, NULL);

  orc_rule_register (rule_set, "addd", mmx_rule_addd, NULL);
  orc_rule_register (rule_set, "subd", mmx_rule_subd, NULL);
  orc_rule_register (rule_set, "muld", mmx_rule_muld, NULL);
  orc_rule_register (rule_set, "divd", mmx_rule_divd, NULL);
  orc_rule_register (rule_set, "mind", mmx_rule_mind, NULL);
  orc_rule_register (rule_set, "maxd", mmx_rule_maxd, NULL);
  orc_rule_register (rule_set, "sqrtd", mmx_rule_sqrtd, NULL);
  orc_rule_register (rule_set, "cmpeqd", mmx_rule_cmpeqd, NULL);
  orc_rule_register (rule_set, "cmpltd", mmx_rule_cmpltd, NULL);
  orc_rule_register (rule_set, "cmpled", mmx_rule_cmpled, NULL);
  orc_rule_register (rule_set, "convdl", mmx_rule_convdl, NULL);
  orc_rule_register (rule_set, "convld", mmx_rule_convld, NULL);

  orc_rule_register (rule_set, "convfd", mmx_rule_convfd, NULL);
  orc_rule_register (rule_set, "convdf", mmx_rule_convdf, NULL);
#endif

  /* slow rules */
  orc_rule_register (rule_set, "maxuw", mmx_rule_maxuw_slow, NULL);
  orc_rule_register (rule_set, "minuw", mmx_rule_minuw_slow, NULL);
  orc_rule_register (rule_set, "avgsb", mmx_rule_avgsb_slow, NULL);
  orc_rule_register (rule_set, "avgsw", mmx_rule_avgsw_slow, NULL);
  orc_rule_register (rule_set, "maxsb", mmx_rule_maxsb_slow, NULL);
  orc_rule_register (rule_set, "minsb", mmx_rule_minsb_slow, NULL);
  orc_rule_register (rule_set, "maxsl", mmx_rule_maxsl_slow, NULL);
  orc_rule_register (rule_set, "minsl", mmx_rule_minsl_slow, NULL);
  orc_rule_register (rule_set, "maxul", mmx_rule_maxul_slow, NULL);
  orc_rule_register (rule_set, "minul", mmx_rule_minul_slow, NULL);
  orc_rule_register (rule_set, "convlw", mmx_rule_convlw, NULL);
  orc_rule_register (rule_set, "signw", mmx_rule_signw_slow, NULL);
  orc_rule_register (rule_set, "absb", mmx_rule_absb_slow, NULL);
  orc_rule_register (rule_set, "absw", mmx_rule_absw_slow, NULL);
  orc_rule_register (rule_set, "absl", mmx_rule_absl_slow, NULL);
  orc_rule_register (rule_set, "swapw", mmx_rule_swapw, NULL);
  orc_rule_register (rule_set, "swapl", mmx_rule_swapl, NULL);
  orc_rule_register (rule_set, "swapwl", mmx_rule_swapwl, NULL);
  orc_rule_register (rule_set, "swapq", mmx_rule_swapq, NULL);
  orc_rule_register (rule_set, "swaplq", mmx_rule_swaplq, NULL);
  orc_rule_register (rule_set, "splitql", mmx_rule_splitql, NULL);
  orc_rule_register (rule_set, "splitlw", mmx_rule_splitlw, NULL);
  orc_rule_register (rule_set, "splitwb", mmx_rule_splitwb, NULL);
  orc_rule_register (rule_set, "avgsl", mmx_rule_avgsl, NULL);
  orc_rule_register (rule_set, "avgul", mmx_rule_avgul, NULL);
  orc_rule_register (rule_set, "shlb", mmx_rule_shlb, NULL);
  orc_rule_register (rule_set, "shrsb", mmx_rule_shrsb, NULL);
  orc_rule_register (rule_set, "shrub", mmx_rule_shrub, NULL);
  orc_rule_register (rule_set, "mulll", mmx_rule_mulll_slow, NULL);
#ifndef MMX
  orc_rule_register (rule_set, "mulhsl", mmx_rule_mulhsl_slow, NULL);
  orc_rule_register (rule_set, "mulhul", mmx_rule_mulhul, NULL);
  orc_rule_register (rule_set, "mulslq", mmx_rule_mulslq_slow, NULL);
#endif
  orc_rule_register (rule_set, "mullb", mmx_rule_mullb, NULL);
  orc_rule_register (rule_set, "mulhsb", mmx_rule_mulhsb, NULL);
  orc_rule_register (rule_set, "mulhub", mmx_rule_mulhub, NULL);
  orc_rule_register (rule_set, "addssl", mmx_rule_addssl_slow, NULL);
  orc_rule_register (rule_set, "subssl", mmx_rule_subssl_slow, NULL);
  orc_rule_register (rule_set, "addusl", mmx_rule_addusl_slow, NULL);
  orc_rule_register (rule_set, "subusl", mmx_rule_subusl_slow, NULL);
  orc_rule_register (rule_set, "convhwb", mmx_rule_convhwb, NULL);
  orc_rule_register (rule_set, "convhlw", mmx_rule_convhlw, NULL);
  orc_rule_register (rule_set, "splatw3q", mmx_rule_splatw3q, NULL);
  orc_rule_register (rule_set, "splatbw", mmx_rule_splatbw, NULL);
  orc_rule_register (rule_set, "splatbl", mmx_rule_splatbl, NULL);
  orc_rule_register (rule_set, "div255w", mmx_rule_div255w, NULL);
  orc_rule_register (rule_set, "divluw", mmx_rule_divluw, NULL);

  /* SSE 3 -- no rules */

  /* SSSE 3 */
  rule_set = orc_rule_set_new (orc_opcode_set_get("sys"), target,
      ORC_TARGET_MMX_SSSE3);

#ifndef MMX
  orc_rule_register (rule_set, "signb", mmx_rule_signX_ssse3, (void *)0);
  orc_rule_register (rule_set, "signw", mmx_rule_signX_ssse3, (void *)1);
  orc_rule_register (rule_set, "signl", mmx_rule_signX_ssse3, (void *)2);
#endif
  REG(absb);
  REG(absw);
  REG(absl);
#ifndef MMX
  orc_rule_register (rule_set, "swapw", mmx_rule_swapw_ssse3, NULL);
  orc_rule_register (rule_set, "swapl", mmx_rule_swapl_ssse3, NULL);
  orc_rule_register (rule_set, "swapwl", mmx_rule_swapwl_ssse3, NULL);
  orc_rule_register (rule_set, "swapq", mmx_rule_swapq_ssse3, NULL);
  orc_rule_register (rule_set, "splitlw", mmx_rule_splitlw_ssse3, NULL);
  orc_rule_register (rule_set, "splitwb", mmx_rule_splitwb_ssse3, NULL);
  orc_rule_register (rule_set, "select0lw", mmx_rule_select0lw_ssse3, NULL);
  orc_rule_register (rule_set, "select1lw", mmx_rule_select1lw_ssse3, NULL);
  orc_rule_register (rule_set, "select0wb", mmx_rule_select0wb_ssse3, NULL);
  orc_rule_register (rule_set, "select1wb", mmx_rule_select1wb_ssse3, NULL);
#endif

  /* SSE 4.1 */
  rule_set = orc_rule_set_new (orc_opcode_set_get("sys"), target,
      ORC_TARGET_MMX_SSE4_1);

#ifndef MMX
  REG(maxsb);
  REG(minsb);
  REG(maxuw);
  REG(minuw);
  REG(maxsl);
  REG(maxul);
  REG(minsl);
  REG(minul);
  REG(mulll);
  orc_rule_register (rule_set, "convsbw", mmx_rule_convsbw_mmx41, NULL);
  orc_rule_register (rule_set, "convswl", mmx_rule_convswl_mmx41, NULL);
  orc_rule_register (rule_set, "convslq", mmx_rule_convslq_mmx41, NULL);
  orc_rule_register (rule_set, "convubw", mmx_rule_convubw_mmx41, NULL);
  orc_rule_register (rule_set, "convuwl", mmx_rule_convuwl_mmx41, NULL);
  orc_rule_register (rule_set, "convulq", mmx_rule_convulq_mmx41, NULL);
  orc_rule_register (rule_set, "convwf", mmx_rule_convwf_mmx41, NULL);
  orc_rule_register (rule_set, "convsuslw", mmx_rule_convsuslw, NULL);
  orc_rule_register (rule_set, "mulslq", mmx_rule_mulslq, NULL);
  orc_rule_register (rule_set, "mulhsl", mmx_rule_mulhsl, NULL);
  orc_rule_register (rule_set, "convsssql", mmx_rule_convsssql_mmx41, NULL);
  REG(cmpeqq);
#endif

  /* SSE 4.2 -- no rules */
  rule_set = orc_rule_set_new (orc_opcode_set_get("sys"), target,
      ORC_TARGET_MMX_SSE4_2);

  REG(cmpgtsq);

  /* SSE 4a -- no rules */
}

