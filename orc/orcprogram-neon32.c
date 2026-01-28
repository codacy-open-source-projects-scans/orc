#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>

#include <orc/orcprogram.h>
#include <orc/orcarm.h>
#include <orc/orcutils.h>
#include <orc/orcdebug.h>
#include <orc/orcinternal.h>

#include <orc/orcneon.h>
#include <orc/orcneon-private.h>
#include <orc/orcarminsn.h>
#include <orc/orcarm64insn.h>

void orc_neon32_compiler_init (OrcCompiler *compiler);
void orc_neon32_compile (OrcCompiler *compiler);

static void orc_neon32_emit_loop (OrcCompiler *compiler, int unroll_index);
static void orc_neon32_save_accumulators (OrcCompiler *compiler);
static void orc_neon32_add_strides (OrcCompiler *compiler);
static void orc_neon32_emit_prologue (OrcCompiler *compiler);
static void orc_neon32_emit_epilogue (OrcCompiler *compiler);
static void orc_neon32_emit_inc_pointers (OrcCompiler *compiler);
static void orc_neon32_load_constants_inner (OrcCompiler *compiler);
static void orc_neon32_load_constants_outer (OrcCompiler *compiler);

#define NEON_BINARY(code,a,b,c) \
  ((code) | \
   (((a)&0xf)<<12) | \
   ((((a)>>4)&0x1)<<22) | \
   (((b)&0xf)<<16) | \
   ((((b)>>4)&0x1)<<7) | \
   (((c)&0xf)<<0) | \
   ((((c)>>4)&0x1)<<5))


/* VV PUBLIC VV */

void
orc_neon32_compiler_init (OrcCompiler *compiler)
{
  int i;

  /** AArch32
   * 16 32-bit generic-purpose registers (R0-R15)
   * 32 64-bit vector registers (smaller registers are packed into larger ones)
   */
  for(i=ORC_GP_REG_BASE;i<ORC_GP_REG_BASE+16;i++){
    compiler->valid_regs[i] = 1;
  }
  for(i=ORC_VEC_REG_BASE+0;i<ORC_VEC_REG_BASE+32;i+=2){
    compiler->valid_regs[i] = 1;
  }
  /* compiler->valid_regs[ORC_ARM_SB] = 0; */
  compiler->valid_regs[ORC_ARM_IP] = 0;
  compiler->valid_regs[ORC_ARM_SP] = 0;
  compiler->valid_regs[ORC_ARM_LR] = 0;
  compiler->valid_regs[ORC_ARM_PC] = 0;

  for(i=4;i<12;i++) {
    compiler->save_regs[ORC_GP_REG_BASE+i] = 1;
  }

  compiler->tmpreg = ORC_VEC_REG_BASE + 0;
  compiler->tmpreg2 = ORC_VEC_REG_BASE + 2;

  orc_neon_compiler_init_common (compiler);
}

void
orc_neon32_compile (OrcCompiler *compiler)
{
  int align_var;
  int align_shift;
  int var_size_shift;
  int i;
  int set_fpscr = FALSE;

  align_var = orc_neon_get_align_var (compiler);
  if (compiler->error) return;

  var_size_shift = orc_neon_get_shift (compiler->vars[align_var].size);
  align_shift = 4;

  compiler->vars[align_var].is_aligned = FALSE;

  orc_neon32_emit_prologue (compiler);

  if (orc_compiler_has_float (compiler)) {
    set_fpscr = TRUE;
    ORC_ASM_CODE (compiler,"  vmrs %s, fpscr\n", orc_arm_reg_name (compiler->gp_tmpreg));
    orc_arm_emit (compiler, 0xeef10a10 | ((compiler->gp_tmpreg&0xf)<<12));
    ORC_ASM_CODE (compiler,"  push %s\n", orc_arm_reg_name (compiler->gp_tmpreg));
    orc_arm_emit (compiler, 0xe52d0004 | ((compiler->gp_tmpreg&0xf)<<12));

    orc_arm_emit_load_imm (compiler, compiler->gp_tmpreg, 1<<24);
    ORC_ASM_CODE (compiler,"  vmsr fpscr, %s\n", orc_arm_reg_name (compiler->gp_tmpreg));
    orc_arm_emit (compiler, 0xeee10a10 | ((compiler->gp_tmpreg&0xf)<<12));
  }

  orc_neon32_load_constants_outer (compiler);

  if (compiler->program->is_2d) {
    if (compiler->program->constant_m > 0) {
      orc_arm_emit_load_imm (compiler, ORC_ARM_A3, compiler->program->constant_m);
      orc_arm_emit_store_reg (compiler, ORC_ARM_A3, compiler->exec_reg,
          (int)ORC_STRUCT_OFFSET(OrcExecutor,params[ORC_VAR_A2]));
    } else {
      orc_arm_emit_load_reg (compiler, ORC_ARM_A3, compiler->exec_reg,
          (int)ORC_STRUCT_OFFSET(OrcExecutor, params[ORC_VAR_A1]));
      orc_arm_emit_store_reg (compiler, ORC_ARM_A3, compiler->exec_reg,
          (int)ORC_STRUCT_OFFSET(OrcExecutor,params[ORC_VAR_A2]));
    }

    orc_arm_emit_label (compiler, LABEL_OUTER_LOOP);
  }

  if (compiler->loop_shift > 0 && compiler->n_insns < 5) {
    orc_arm_emit_load_reg (compiler, ORC_ARM_A3, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,n));

    orc_arm_emit_cmp_imm (compiler, ORC_ARM_A3, ORC_NEON_ALIGNED_DEST_CUTOFF);
    orc_arm_emit_branch (compiler, ORC_ARM_COND_GT, LABEL_REGION0_SKIP);

    orc_arm_emit_asr_imm (compiler, ORC_ARM_A2, ORC_ARM_A3,
        compiler->loop_shift);
    orc_arm_emit_store_reg (compiler, ORC_ARM_A2, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,counter2));

    orc_arm_emit_and_imm (compiler, ORC_ARM_A3, ORC_ARM_A3,
        (1<<compiler->loop_shift)-1);
    orc_arm_emit_store_reg (compiler, ORC_ARM_A3, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,counter3));

    orc_neon32_load_constants_inner (compiler);
    orc_arm_emit_load_reg (compiler, ORC_ARM_IP, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,counter2));
    orc_arm_emit_cmp_imm (compiler, ORC_ARM_IP, 0);
    orc_arm_emit_branch (compiler, ORC_ARM_COND_EQ, LABEL_REGION2_SKIP);

    compiler->size_region = 0;
    orc_arm_emit_label (compiler, LABEL_REGION0_LOOP);

    orc_neon32_emit_loop (compiler, -1);
    orc_arm_emit_sub_imm (compiler, ORC_ARM_IP, ORC_ARM_IP, 1, TRUE);
    orc_arm_emit_branch (compiler, ORC_ARM_COND_NE, LABEL_REGION0_LOOP);
    orc_arm_emit_branch (compiler, ORC_ARM_COND_AL, LABEL_REGION2_SKIP);
    orc_arm_emit_label (compiler, LABEL_REGION0_SKIP);
  }

  if (compiler->loop_shift > 0) {
    orc_arm_emit_load_imm (compiler, ORC_ARM_IP, 1<<align_shift);

    orc_arm_emit_load_reg (compiler, ORC_ARM_A2, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,arrays[align_var]));
    orc_arm_emit_sub (compiler, ORC_ARM_IP, ORC_ARM_IP, ORC_ARM_A2);
    orc_arm_emit_and_imm (compiler, ORC_ARM_IP, ORC_ARM_IP,
        (1<<align_shift)-1);
    if (var_size_shift > 0) {
      orc_arm_emit_asr_imm (compiler, ORC_ARM_IP, ORC_ARM_IP, var_size_shift);
    }

    orc_arm_emit_load_reg (compiler, ORC_ARM_A3, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,n));
    orc_arm_emit_cmp (compiler, ORC_ARM_A3, ORC_ARM_IP);
    orc_arm_emit_branch (compiler, ORC_ARM_COND_LE, LABEL_ONE_REGION);

    orc_arm_emit_store_reg (compiler, ORC_ARM_IP, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,counter1));
    orc_arm_emit_sub (compiler, ORC_ARM_A2, ORC_ARM_A3, ORC_ARM_IP);

    orc_arm_emit_asr_imm (compiler, ORC_ARM_A3, ORC_ARM_A2,
        compiler->loop_shift + compiler->unroll_shift);
    orc_arm_emit_store_reg (compiler, ORC_ARM_A3, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,counter2));

    orc_arm_emit_and_imm (compiler, ORC_ARM_A3, ORC_ARM_A2,
        (1<<(compiler->loop_shift + compiler->unroll_shift))-1);
    orc_arm_emit_store_reg (compiler, ORC_ARM_A3, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,counter3));

    orc_arm_emit_branch (compiler, ORC_ARM_COND_AL, LABEL_ONE_REGION_AFTER);
    orc_arm_emit_label (compiler, LABEL_ONE_REGION);

    orc_arm_emit_store_reg (compiler, ORC_ARM_A3, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,counter1));

    orc_arm_emit_load_imm (compiler, ORC_ARM_A3, 0);
    orc_arm_emit_store_reg (compiler, ORC_ARM_A3, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,counter2));
    orc_arm_emit_store_reg (compiler, ORC_ARM_A3, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,counter3));

    orc_arm_emit_label (compiler, LABEL_ONE_REGION_AFTER);
  }

  orc_neon32_load_constants_inner (compiler);

  if (compiler->loop_shift > 0) {
    int save_loop_shift = compiler->loop_shift;
    compiler->loop_shift = 0;

    orc_arm_emit_load_reg (compiler, ORC_ARM_IP, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,counter1));

    orc_arm_emit_cmp_imm (compiler, ORC_ARM_IP, 0);
    orc_arm_emit_branch (compiler, ORC_ARM_COND_EQ, LABEL_REGION1_SKIP);

    orc_arm_emit_label (compiler, LABEL_REGION1_LOOP);
    orc_neon32_emit_loop (compiler, -1);
    orc_arm_emit_sub_imm (compiler, ORC_ARM_IP, ORC_ARM_IP, 1, TRUE);
    orc_arm_emit_branch (compiler, ORC_ARM_COND_NE, LABEL_REGION1_LOOP);
    orc_arm_emit_label (compiler, LABEL_REGION1_SKIP);

    compiler->loop_shift = save_loop_shift;
    compiler->vars[align_var].is_aligned = TRUE;
  }

  if (compiler->loop_shift > 0) {
    orc_arm_emit_load_reg (compiler, ORC_ARM_IP, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,counter2));
  } else {
    orc_arm_emit_load_reg (compiler, ORC_ARM_IP, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,n));
  }

  orc_arm_emit_cmp_imm (compiler, ORC_ARM_IP, 0);
  orc_arm_emit_branch (compiler, ORC_ARM_COND_EQ, LABEL_REGION2_SKIP);

  orc_arm_emit_asr_imm (compiler, compiler->gp_tmpreg, ORC_ARM_IP,
      17 + var_size_shift - compiler->loop_shift - compiler->unroll_shift);
  orc_arm_emit_cmp_imm (compiler, compiler->gp_tmpreg, 0);
  orc_arm_emit_branch (compiler, ORC_ARM_COND_EQ, LABEL_REGION2_MEDIUM);

  /* N is larger than L2 cache size */
  compiler->size_region = 3;
  orc_arm_emit_label (compiler, LABEL_REGION2_LOOP_LARGE);
  for(i=0;i<(1<<compiler->unroll_shift);i++){
    orc_neon32_emit_loop (compiler, i);
  }
  orc_arm_emit_sub_imm (compiler, ORC_ARM_IP, ORC_ARM_IP, 1, TRUE);
  orc_arm_emit_branch (compiler, ORC_ARM_COND_NE, LABEL_REGION2_LOOP_LARGE);
  orc_arm_emit_branch (compiler, ORC_ARM_COND_AL, LABEL_REGION2_SKIP);

  orc_arm_emit_label (compiler, LABEL_REGION2_MEDIUM);
  orc_arm_emit_asr_imm (compiler, compiler->gp_tmpreg, ORC_ARM_IP,
      13 + var_size_shift - compiler->loop_shift - compiler->unroll_shift);
  orc_arm_emit_cmp_imm (compiler, compiler->gp_tmpreg, 0);
  orc_arm_emit_branch (compiler, ORC_ARM_COND_EQ, LABEL_REGION2_SMALL);

  /* N is smaller than L2 cache size */
  compiler->size_region = 2;
  orc_arm_emit_label (compiler, LABEL_REGION2_LOOP_MEDIUM);
  for(i=0;i<(1<<compiler->unroll_shift);i++){
    orc_neon32_emit_loop (compiler, i);
  }
  orc_arm_emit_sub_imm (compiler, ORC_ARM_IP, ORC_ARM_IP, 1, TRUE);
  orc_arm_emit_branch (compiler, ORC_ARM_COND_NE, LABEL_REGION2_LOOP_MEDIUM);
  orc_arm_emit_branch (compiler, ORC_ARM_COND_AL, LABEL_REGION2_SKIP);

  orc_arm_emit_label (compiler, LABEL_REGION2_SMALL);
  /* N is smaller than L2 cache size */
  compiler->size_region = 1;
  orc_arm_emit_label (compiler, LABEL_REGION2_LOOP_SMALL);
  for(i=0;i<(1<<compiler->unroll_shift);i++){
    orc_neon32_emit_loop (compiler, i);
  }
  orc_arm_emit_sub_imm (compiler, ORC_ARM_IP, ORC_ARM_IP, 1, TRUE);
  orc_arm_emit_branch (compiler, ORC_ARM_COND_NE, LABEL_REGION2_LOOP_SMALL);

  orc_arm_emit_label (compiler, LABEL_REGION2_SKIP);

  if (compiler->loop_shift > 0) {
    int save_loop_shift = compiler->loop_shift;

    compiler->loop_shift = 0;

    compiler->vars[align_var].is_aligned = FALSE;

    orc_arm_emit_load_reg (compiler, ORC_ARM_IP, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,counter3));

    orc_arm_emit_cmp_imm (compiler, ORC_ARM_IP, 0);
    orc_arm_emit_branch (compiler, ORC_ARM_COND_EQ, LABEL_REGION3_SKIP);

    orc_arm_emit_label (compiler, LABEL_REGION3_LOOP);
    orc_neon32_emit_loop (compiler, -1);
    orc_arm_emit_sub_imm (compiler, ORC_ARM_IP, ORC_ARM_IP, 1, TRUE);
    orc_arm_emit_branch (compiler, ORC_ARM_COND_NE, LABEL_REGION3_LOOP);
    orc_arm_emit_label (compiler, LABEL_REGION3_SKIP);

    compiler->loop_shift = save_loop_shift;
  }

  if (compiler->program->is_2d) {
    orc_neon32_add_strides (compiler);

    orc_arm_emit_load_reg (compiler, ORC_ARM_A3, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor, params[ORC_VAR_A2]));
    orc_arm_emit_sub_imm (compiler, ORC_ARM_A3, ORC_ARM_A3, 1, TRUE);
    orc_arm_emit_store_reg (compiler, ORC_ARM_A3, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,params[ORC_VAR_A2]));
    orc_arm_emit_branch (compiler, ORC_ARM_COND_NE, LABEL_OUTER_LOOP);
  }

  orc_neon32_save_accumulators (compiler);

  if (set_fpscr) {
    ORC_ASM_CODE (compiler,"  pop %s\n", orc_arm_reg_name (compiler->gp_tmpreg));
    orc_arm_emit (compiler, 0xe49d0004 | ((compiler->gp_tmpreg&0xf)<<12));

    ORC_ASM_CODE (compiler,"  vmsr fpscr, %s\n", orc_arm_reg_name (compiler->gp_tmpreg));
    orc_arm_emit (compiler, 0xeee10a10 | ((compiler->gp_tmpreg&0xf)<<12));
  }

  orc_neon32_emit_epilogue (compiler);

  orc_arm_emit_align (compiler, 4);

  orc_arm_emit_label (compiler, 20);
  orc_arm_emit_data (compiler, 0x07060706);
  orc_arm_emit_data (compiler, 0x07060706);
  orc_arm_emit_data (compiler, 0x0f0e0f0e);
  orc_arm_emit_data (compiler, 0x0f0e0f0e);

  orc_arm_do_fixups (compiler);
}


/* VV PRIVATE VV */

static void
orc_neon32_emit_loop (OrcCompiler *compiler, int unroll_index)
{
  orc_neon_emit_loop_common (compiler, unroll_index);
  orc_neon32_emit_inc_pointers (compiler);
}

static void
orc_neon32_save_accumulators (OrcCompiler *compiler)
{
  int i;
  int src;
  unsigned int code;

  for(i=0;i<ORC_N_COMPILER_VARIABLES;i++){
    OrcVariable *var = compiler->vars + i;

    if (compiler->vars[i].name == NULL) continue;
    switch (compiler->vars[i].vartype) {
      case ORC_VAR_TYPE_ACCUMULATOR:
        src = compiler->vars[i].alloc;
        orc_arm_emit_load_imm (compiler, compiler->gp_tmpreg,
            ORC_STRUCT_OFFSET(OrcExecutor, accumulators[i-ORC_VAR_A1]));
        orc_arm_emit_add (compiler, compiler->gp_tmpreg,
            compiler->gp_tmpreg, compiler->exec_reg);
        switch (var->size) {
          case 2:
            if (compiler->loop_shift > 0) {
              ORC_ASM_CODE(compiler,"  vpaddl.u16 %s, %s\n",
                  orc_neon32_reg_name (src),
                  orc_neon32_reg_name (src));
              code = 0xf3b40280;
              code |= (src&0xf) << 12;
              code |= ((src>>4)&0x1) << 22;
              code |= (src&0xf) << 0;
              orc_arm_emit (compiler, code);

              ORC_ASM_CODE(compiler,"  vpaddl.u32 %s, %s\n",
                  orc_neon32_reg_name (src),
                  orc_neon32_reg_name (src));
              code = 0xf3b80280;
              code |= (src&0xf) << 12;
              code |= ((src>>4)&0x1) << 22;
              code |= (src&0xf) << 0;
              orc_arm_emit (compiler, code);
            }

            ORC_ASM_CODE(compiler,"  vst1.16 %s[%d], [%s]\n",
                orc_neon32_reg_name (src), 0,
                orc_arm_reg_name (compiler->gp_tmpreg));
            code = 0xf480040f;
            code |= (compiler->gp_tmpreg&0xf) << 16;
            code |= (src&0xf) << 12;
            code |= ((src>>4)&0x1) << 22;
            orc_arm_emit (compiler, code);
            break;
          case 4:
            if (compiler->loop_shift > 0) {
              ORC_ASM_CODE(compiler,"  vpadd.u32 %s, %s, %s\n",
                  orc_neon32_reg_name (src),
                  orc_neon32_reg_name (src),
                  orc_neon32_reg_name (src));
              code = NEON_BINARY(0xf2200b10, src, src, src);
              orc_arm_emit (compiler, code);
            }

            ORC_ASM_CODE(compiler,"  vst1.32 %s[%d], [%s]\n",
                orc_neon32_reg_name (src), 0,
                orc_arm_reg_name (compiler->gp_tmpreg));
            code = 0xf480080f;
            code |= (compiler->gp_tmpreg&0xf) << 16;
            code |= (src&0xf) << 12;
            code |= ((src>>4)&0x1) << 22;
            orc_arm_emit (compiler, code);
            break;
          default:
            ORC_ERROR("bad size");
        }

        break;
      default:
        break;
    }
  }
}

static void
orc_neon32_add_strides (OrcCompiler *compiler)
{
  int i;

  for(i=0;i<ORC_N_COMPILER_VARIABLES;i++){
    if (compiler->vars[i].name == NULL) continue;
    switch (compiler->vars[i].vartype) {
      case ORC_VAR_TYPE_CONST:
        break;
      case ORC_VAR_TYPE_PARAM:
        break;
      case ORC_VAR_TYPE_SRC:
      case ORC_VAR_TYPE_DEST:
        orc_arm_emit_load_reg (compiler, ORC_ARM_A3, compiler->exec_reg,
            (int)ORC_STRUCT_OFFSET(OrcExecutor, arrays[i]));
        orc_arm_emit_load_reg (compiler, ORC_ARM_A2, compiler->exec_reg,
            (int)ORC_STRUCT_OFFSET(OrcExecutor, params[i]));
        orc_arm_emit_add (compiler, ORC_ARM_A3, ORC_ARM_A3, ORC_ARM_A2);
        orc_arm_emit_store_reg (compiler, ORC_ARM_A3, compiler->exec_reg,
            (int)ORC_STRUCT_OFFSET(OrcExecutor, arrays[i]));
        break;
      case ORC_VAR_TYPE_ACCUMULATOR:
        break;
      case ORC_VAR_TYPE_TEMP:
        break;
      default:
        ORC_COMPILER_ERROR(compiler,"bad vartype");
        break;
    }
  }
}

static void
orc_neon32_emit_prologue (OrcCompiler *compiler)
{
  unsigned int regs = 0;
  orc_uint32 vregs = 0;

  orc_compiler_append_code(compiler,".global %s\n", compiler->program->name);
  orc_compiler_append_code(compiler,"%s:\n", compiler->program->name);

  regs = orc_neon_get_callee_saved_regs (compiler, ORC_GP_REG_BASE, 16);
  vregs = orc_neon_get_callee_saved_regs (compiler, ORC_VEC_REG_BASE, 32);

  orc_arm_emit_push (compiler, regs, vregs);
}

static void
orc_neon32_emit_epilogue (OrcCompiler *compiler)
{
  unsigned int regs = 0;
  orc_uint32 vregs = 0;

  regs = orc_neon_get_callee_saved_regs (compiler, ORC_GP_REG_BASE, 16);
  vregs = orc_neon_get_callee_saved_regs (compiler, ORC_VEC_REG_BASE, 32);

  orc_arm_emit_pop (compiler, regs, vregs);
  orc_arm_emit_bx_lr (compiler);
}

static void
orc_neon32_emit_inc_pointers (OrcCompiler *compiler)
{
  for(int k=0;k<ORC_N_COMPILER_VARIABLES;k++){
    if (compiler->vars[k].name == NULL) continue;
    if (compiler->vars[k].vartype == ORC_VAR_TYPE_SRC ||
        compiler->vars[k].vartype == ORC_VAR_TYPE_DEST) {
      if (compiler->vars[k].ptr_offset) {
        orc_arm_emit_add_imm (compiler,
            compiler->vars[k].ptr_offset,
            compiler->vars[k].ptr_offset,
            compiler->vars[k].size << compiler->loop_shift);
      } else if (compiler->vars[k].ptr_register) {
        orc_arm_emit_add_imm (compiler,
            compiler->vars[k].ptr_register,
            compiler->vars[k].ptr_register,
            compiler->vars[k].size << compiler->loop_shift);
      }
    }
  }
}

static void
orc_neon32_load_constants_inner (OrcCompiler *compiler)
{
  int i;
  for(i=0;i<ORC_N_COMPILER_VARIABLES;i++){
    if (compiler->vars[i].name == NULL) continue;

    switch (compiler->vars[i].vartype) {
      case ORC_VAR_TYPE_CONST:
        break;
      case ORC_VAR_TYPE_PARAM:
        break;
      case ORC_VAR_TYPE_SRC:
      case ORC_VAR_TYPE_DEST:
        orc_arm_emit_load_reg (compiler,
            compiler->vars[i].ptr_register,
            compiler->exec_reg, ORC_STRUCT_OFFSET(OrcExecutor, arrays[i]));
        if (compiler->vars[i].ptr_offset)
            orc_arm_emit_eor_r(compiler, ORC_ARM_COND_AL, 0,
                compiler->vars[i].ptr_offset,
                compiler->vars[i].ptr_offset,
                compiler->vars[i].ptr_offset);
        break;
      case ORC_VAR_TYPE_ACCUMULATOR:
        break;
      case ORC_VAR_TYPE_TEMP:
        break;
      default:
        ORC_PROGRAM_ERROR(compiler,"bad vartype");
        break;
    }
  }
}

static void
orc_neon32_load_constants_outer (OrcCompiler *compiler)
{
  for(int i = 0; i < ORC_N_COMPILER_VARIABLES; i++) {
    if (compiler->vars[i].name == NULL) continue;
    if (compiler->vars[i].vartype != ORC_VAR_TYPE_ACCUMULATOR) continue;

    orc_neon32_emit_loadil (compiler, &(compiler->vars[i]), 0);
  }

  orc_compiler_emit_invariants (compiler);
}
