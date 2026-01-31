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
#include <orc/orcvariable.h>

#include <orc/orcneon.h>
#include <orc/orcneon-private.h>
#include <orc/orcarminsn.h>
#include <orc/orcarm64insn.h>

void orc_neon64_compiler_init (OrcCompiler *compiler);
void orc_neon64_compile (OrcCompiler *compiler);

static void orc_neon64_emit_loop (OrcCompiler *compiler, int unroll_index);
static void orc_neon64_short_unaligned_loop (OrcCompiler *compiler);
static void orc_neon64_save_accumulators (OrcCompiler *compiler);
static void orc_neon64_add_strides (OrcCompiler *compiler);
static void orc_neon64_emit_prologue (OrcCompiler *compiler);
static void orc_neon64_emit_epilogue (OrcCompiler *compiler);
static void orc_neon64_load_constants_inner (OrcCompiler *compiler);
static void orc_neon64_set_region_counters (OrcCompiler *compiler);
static void orc_neon64_loop_caches (OrcCompiler *compiler);
static void orc_neon64_emit_inc_pointers (OrcCompiler *compiler);
static void orc_neon64_load_constants_outer (OrcCompiler *compiler);

#define ORC_NEON_LONG_PROGRAM_CUTOFF 5

#define orc_neon64_loop_shift_remainder(compiler,counter,label_loop,label_skip) \
{ \
  int save_loop_shift = compiler->loop_shift; \
  compiler->loop_shift = 0; \
  orc_arm64_emit_load_reg (compiler, 32, ORC_ARM64_IP0, compiler->exec_reg, \
      (int)ORC_STRUCT_OFFSET(OrcExecutor,counter)); \
  orc_arm64_emit_cmp_imm (compiler, 32, ORC_ARM64_IP0, 0); \
  orc_arm64_emit_branch (compiler, ORC_ARM_COND_EQ, label_skip); \
  orc_arm_emit_label (compiler, label_loop); \
  orc_arm64_emit_subs_imm (compiler, 32, ORC_ARM64_IP0, ORC_ARM64_IP0, 1); \
  orc_neon64_emit_loop (compiler, -1); \
  orc_arm64_emit_branch (compiler, ORC_ARM_COND_NE, label_loop); \
  orc_arm_emit_label (compiler, label_skip); \
  compiler->loop_shift = save_loop_shift; \
}


/* VV PUBLIC VV */

void
orc_neon64_compiler_init (OrcCompiler *compiler)
{
  int i;

  compiler->is_64bit = TRUE;

  /** AArch64
   * 31 64-bit generic-purpose registers (R0-R30) and SP
   * 32 128-bit vector registers (do not overlap multiple registers in a narrower view)
   * Note that PC is not a generic-purpose register in AArch64
   */
  for(i=ORC_GP_REG_BASE;i<ORC_GP_REG_BASE+32;i++){
    compiler->valid_regs[i] = 1;
  }
  for(i=ORC_VEC_REG_BASE+0;i<ORC_VEC_REG_BASE+32;i++){
    compiler->valid_regs[i] = 1;
  }

  compiler->valid_regs[ORC_ARM64_IP0] = 0;
  compiler->valid_regs[ORC_ARM64_IP1] = 0;

  compiler->valid_regs[ORC_ARM64_FP] = 0;
  compiler->valid_regs[ORC_ARM64_LR] = 0;
  compiler->valid_regs[ORC_ARM64_SP] = 0;

  /** r19 to r29 are callee-saved */
  for(i=19;i<29;i++) {
    compiler->save_regs[ORC_GP_REG_BASE+i] = 1;
  }

  compiler->tmpreg = ORC_VEC_REG_BASE + 0;
  compiler->tmpreg2 = ORC_VEC_REG_BASE + 1;

  orc_neon_compiler_init_common (compiler);
}

void
orc_neon64_compile (OrcCompiler *compiler)
{
  const enum RegionFlags region_flags = orc_compiler_neon_calc_regions (compiler);
  const orc_bool region1 = region_flags & FLAG_REGION1;
  const orc_bool region2 = region_flags & FLAG_REGION2;
  const orc_bool region3 = region_flags & FLAG_REGION3;
  const orc_bool nunroll = region_flags & FLAG_NUNROLL;
  int align_var;

  align_var = orc_neon_get_align_var (compiler);
  if (compiler->error) return;

  ORC_DEBUG ("Neon compiler regions = [ %s %s %s %s ]"
      , region1 ? "region1":"-------"
      , region2 ? "region2":"-------"
      , region3 ? "region3":"-------"
      , nunroll ? "nunroll":"-------"
  );

  compiler->vars[align_var].is_aligned = FALSE;

  orc_neon64_emit_prologue (compiler);

  orc_neon64_load_constants_outer (compiler);

  if (compiler->program->is_2d) {
    if (compiler->program->constant_m > 0) {
      orc_arm64_emit_mov_imm (compiler, 32, ORC_ARM64_IP1, compiler->program->constant_m);
      orc_arm64_emit_store_reg (compiler, 32, ORC_ARM64_IP1, compiler->exec_reg,
          (int)ORC_STRUCT_OFFSET(OrcExecutor,params[ORC_VAR_A2]));
    } else {
      orc_arm64_emit_load_reg (compiler, 32, ORC_ARM64_IP1, compiler->exec_reg,
          (int)ORC_STRUCT_OFFSET(OrcExecutor, params[ORC_VAR_A1]));
      orc_arm64_emit_store_reg (compiler, 32, ORC_ARM64_IP1, compiler->exec_reg,
          (int)ORC_STRUCT_OFFSET(OrcExecutor, params[ORC_VAR_A2]));
    }

    orc_arm_emit_label (compiler, LABEL_OUTER_LOOP);
  }

  if (!region1 && nunroll) {
    const int constant_n = compiler->program->constant_n;
    const int num_loops = constant_n >> compiler->loop_shift;
    const int remaining = constant_n & ((1 << compiler->loop_shift) - 1);
    ORC_DEBUG("unrolled loops     = %d", num_loops);
    ORC_DEBUG("remaining elements = %d", remaining);

    orc_neon64_load_constants_inner (compiler);

    for (int i = 0; i < num_loops; i++) {
      orc_neon64_emit_loop (compiler, i);
    }

    if (region3) {
      orc_arm64_emit_mov_imm (compiler, ORC_ARM64_REG_32, ORC_ARM64_IP0, remaining);
      orc_arm_emit_label (compiler, LABEL_REGION3_LOOP);
      orc_arm64_emit_subs_imm (compiler, ORC_ARM64_REG_32, ORC_ARM64_IP0, ORC_ARM64_IP0, 1);

      int save_loop_shift = compiler->loop_shift; // Ugly, but...uff
      compiler->loop_shift = 0;
      orc_neon64_emit_loop (compiler, -1);
      compiler->loop_shift = save_loop_shift;

      orc_arm64_emit_branch (compiler, ORC_ARM_COND_NE, LABEL_REGION3_LOOP);
    }
  } else if (compiler->loop_shift > 0) {
    if (compiler->n_insns < ORC_NEON_LONG_PROGRAM_CUTOFF)
      orc_neon64_short_unaligned_loop (compiler);

    orc_neon64_set_region_counters (compiler);

    orc_neon64_load_constants_inner (compiler);

    orc_neon64_loop_shift_remainder (compiler, counter1,
        LABEL_REGION1_LOOP, LABEL_REGION1_SKIP);
    compiler->vars[align_var].is_aligned = TRUE;

    orc_arm64_emit_load_reg (compiler, 32, ORC_ARM64_IP0, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,counter2));

    orc_neon64_loop_caches (compiler);

    compiler->vars[align_var].is_aligned = FALSE;
    orc_neon64_loop_shift_remainder (compiler, counter3,
        LABEL_REGION3_LOOP, LABEL_REGION3_SKIP);
  } else {
    orc_neon64_load_constants_inner (compiler);

    orc_arm64_emit_load_reg (compiler, 32, ORC_ARM64_IP0, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,n));

    orc_neon64_loop_caches (compiler);
  }

  if (compiler->program->is_2d) {
    orc_neon64_add_strides (compiler);

    orc_arm64_emit_load_reg (compiler, 32, ORC_ARM64_IP1, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor, params[ORC_VAR_A2]));
    orc_arm64_emit_subs_imm (compiler, 32, ORC_ARM64_IP1, ORC_ARM64_IP1, 1);
    orc_arm64_emit_store_reg (compiler, 32, ORC_ARM64_IP1, compiler->exec_reg,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,params[ORC_VAR_A2]));
    orc_arm64_emit_branch (compiler, ORC_ARM_COND_NE, LABEL_OUTER_LOOP);
  }

  orc_neon64_save_accumulators (compiler);

  orc_neon64_emit_epilogue (compiler);

  orc_arm64_emit_align (compiler, 4);

  orc_arm_do_fixups (compiler);
}


/* VV PRIVATE VV */

static void
orc_neon64_emit_loop (OrcCompiler *compiler, int unroll_index)
{
  orc_neon_emit_loop_common (compiler, unroll_index);
  orc_neon64_emit_inc_pointers (compiler);
}

static void
orc_neon64_short_unaligned_loop (OrcCompiler *compiler)
{
  /** Get the number of loops (N) from OrcExecutor */
  orc_arm64_emit_load_reg (compiler, 32, ORC_ARM64_R2, compiler->exec_reg,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,n));

  /** if N > ORC_NEON_ALIGNED_DEST_CUTOFF, go to LABEL_REGION0_SKIP */
  orc_arm64_emit_cmp_imm (compiler, 32, ORC_ARM64_R2, ORC_NEON_ALIGNED_DEST_CUTOFF);
  orc_arm64_emit_branch (compiler, ORC_ARM_COND_GT, LABEL_REGION0_SKIP);

  /** counter2 = N >> loop shift */
  orc_arm64_emit_asr_imm (compiler, 32, ORC_ARM64_R1, ORC_ARM64_R2,
      compiler->loop_shift);
  orc_arm64_emit_store_reg (compiler, 32, ORC_ARM64_R1, compiler->exec_reg,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,counter2));

  /** counter3 = N & loop shift */
  orc_arm64_emit_and_imm (compiler, 32, ORC_ARM64_R2, ORC_ARM64_R2,
      (1<<compiler->loop_shift)-1);
  orc_arm64_emit_store_reg (compiler, 32, ORC_ARM64_R2, compiler->exec_reg,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,counter3));

  /** load function arguments */
  orc_neon64_load_constants_inner (compiler);

  /** if counter2 == zero, go to LABEL_REGION2_SKIP */
  orc_arm64_emit_load_reg (compiler, 32, ORC_ARM64_IP0, compiler->exec_reg,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,counter2));
  orc_arm64_emit_cmp_imm (compiler, 32, ORC_ARM64_IP0, 0);
  orc_arm64_emit_branch (compiler, ORC_ARM_COND_EQ, LABEL_REGION2_SKIP);

  /** vector calculation loop */
  compiler->size_region = 0;
  orc_arm_emit_label (compiler, LABEL_REGION0_LOOP);
  orc_arm64_emit_subs_imm (compiler, 32, ORC_ARM64_IP0, ORC_ARM64_IP0, 1);

  /** vector instructions: @todo port to aarch64 */
  orc_neon64_emit_loop (compiler, -1);

  /** if counter2 != zero, repeat loop */
  orc_arm64_emit_branch (compiler, ORC_ARM_COND_NE, LABEL_REGION0_LOOP);
  /** else go to LABEL_REGION2_SKIP */
  orc_arm64_emit_branch (compiler, ORC_ARM_COND_AL, LABEL_REGION2_SKIP);
  orc_arm_emit_label (compiler, LABEL_REGION0_SKIP);
}

static void
orc_neon64_save_accumulators (OrcCompiler *compiler)
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
        orc_arm64_emit_add_imm (compiler, 64, compiler->gp_tmpreg,
            compiler->exec_reg,
            ORC_STRUCT_OFFSET(OrcExecutor, accumulators[i-ORC_VAR_A1]));
        switch (var->size) {
          case 2:
            if (compiler->loop_shift > 0) {
              ORC_ASM_CODE(compiler,"  addv %s, %s, %s\n",
                  orc_neon64_reg_name_vector (src, 8, 0),
                  orc_neon64_reg_name_vector (src, 8, 0),
                  orc_neon64_reg_name_vector (src, 8, 0));
              code = 0x0e71b800;
              code |= (src&0x1f)<<5;
              code |= (src&0x1f);
              orc_arm_emit (compiler, code);
            }

            ORC_ASM_CODE(compiler,"  st1 %s, [%s]\n",
                orc_neon64_reg_name_vector (src, 8, 0),
                orc_arm64_reg_name (compiler->gp_tmpreg, 64));
            code = 0x0d004000;
            code |= (compiler->gp_tmpreg&0x1f) << 5;
            code |= (src&0x1f) << 0;
            orc_arm_emit (compiler, code);
            break;
          case 4:
            if (compiler->loop_shift > 0) {
              ORC_ASM_CODE(compiler,"  addp %s, %s, %s\n",
                  orc_neon64_reg_name_vector (src, 8, 0),
                  orc_neon64_reg_name_vector (src, 8, 0),
                  orc_neon64_reg_name_vector (src, 8, 0));
              code = 0x0ea0bc00;
              code |= (src&0x1f)<<16;
              code |= (src&0x1f)<<5;
              code |= (src&0x1f);
              orc_arm_emit (compiler, code);
            }

            ORC_ASM_CODE(compiler,"  st1 {%s}[0], [%s]\n",
                orc_neon64_reg_name_vector (src, 8, 0),
                orc_arm64_reg_name (compiler->gp_tmpreg, 64));
            code = 0x0d008000;
            code |= (compiler->gp_tmpreg&0x1f) << 5;
            code |= (src&0x1f) << 0;
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

void
orc_neon64_add_strides (OrcCompiler *compiler)
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
        orc_arm64_emit_load_reg (compiler, 32, ORC_ARM64_IP1, compiler->exec_reg,
            (int)ORC_STRUCT_OFFSET(OrcExecutor, arrays[i]));
        orc_arm64_emit_load_reg (compiler, 32, ORC_ARM64_R18, compiler->exec_reg,
            (int)ORC_STRUCT_OFFSET(OrcExecutor, params[i]));
        orc_arm64_emit_add (compiler, 32, ORC_ARM64_IP1, ORC_ARM64_IP1, ORC_ARM64_R18);
        orc_arm64_emit_store_reg (compiler, 32, ORC_ARM64_IP1, compiler->exec_reg,
            (int)ORC_STRUCT_OFFSET(OrcExecutor,arrays[i]));
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
orc_neon64_emit_prologue (OrcCompiler *compiler)
{
  unsigned int regs = 0;
  orc_uint32 vregs = 0;

  orc_compiler_append_code(compiler,".global %s\n", compiler->program->name);
  orc_compiler_append_code(compiler,"%s:\n", compiler->program->name);

  regs = orc_neon_get_callee_saved_regs (compiler, ORC_GP_REG_BASE, 32);
  vregs = orc_neon_get_callee_saved_regs (compiler, ORC_VEC_REG_BASE, 32);

  orc_arm64_emit_push (compiler, regs, vregs);
}

static void
orc_neon64_emit_epilogue (OrcCompiler *compiler)
{
  unsigned int regs = 0;
  orc_uint32 vregs = 0;

  regs = orc_neon_get_callee_saved_regs (compiler, ORC_GP_REG_BASE, 32);
  vregs = orc_neon_get_callee_saved_regs (compiler, ORC_VEC_REG_BASE, 32);

  orc_arm64_emit_pop (compiler, regs, vregs);
  orc_arm64_emit_ret (compiler, ORC_ARM64_LR);
}

static void
orc_neon64_load_constants_inner (OrcCompiler *compiler)
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
        orc_arm64_emit_load_reg (compiler, 64,
            compiler->vars[i].ptr_register,
            compiler->exec_reg, ORC_STRUCT_OFFSET(OrcExecutor, arrays[i]));
        if (compiler->vars[i].ptr_offset)
            orc_arm64_emit_eor(compiler, 64,
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
orc_neon64_set_region_counters (OrcCompiler *compiler)
{
  int align_var;
  int var_size_shift;
  int align_shift = 4;

  align_var = orc_neon_get_align_var (compiler);
  if (compiler->error) return;
  orc_variable_get_shift (&compiler->vars[align_var], &var_size_shift);

  /** IP0 = 1 << align_shift */
  orc_arm64_emit_mov_imm (compiler, 32, ORC_ARM64_IP0, 1<<align_shift);

  /** r1 == ORC_VAR_D1 */
  orc_arm64_emit_load_reg (compiler, 32, ORC_ARM64_R1, compiler->exec_reg,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,arrays[align_var]));
  /** IP0 = IP0 - r1 */
  orc_arm64_emit_sub (compiler, 32, ORC_ARM64_IP0, ORC_ARM64_IP0, ORC_ARM64_R1);
  /** IP0 = IP0 & ((1 << aligned_shift) -1) */
  orc_arm64_emit_and_imm (compiler, 32, ORC_ARM64_IP0, ORC_ARM64_IP0,
      (1<<align_shift)-1);
  if (var_size_shift > 0) {
    /** IP0 = IP0 >> var_size_shift */
    orc_arm64_emit_asr_imm (compiler, 32, ORC_ARM64_IP0, ORC_ARM64_IP0, var_size_shift);
  }

  /** r2 = N */
  orc_arm64_emit_load_reg (compiler, 32, ORC_ARM64_R2, compiler->exec_reg,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,n));
  /** N <= IP0, go to LABEL_ONE_REGION */
  orc_arm64_emit_cmp (compiler, 32, ORC_ARM64_R2, ORC_ARM64_IP0);
  orc_arm64_emit_branch (compiler, ORC_ARM_COND_LE, LABEL_ONE_REGION);

  /** counter1 = IP0 */
  orc_arm64_emit_store_reg (compiler, 32, ORC_ARM64_IP0, compiler->exec_reg,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,counter1));
  /** r1 = r2 - IP0 */
  orc_arm64_emit_sub (compiler, 32, ORC_ARM64_R1, ORC_ARM64_R2, ORC_ARM64_IP0);

  /** r2 = r1 >> (loop_shift + unroll_shift) */
  orc_arm64_emit_asr_imm (compiler, 32, ORC_ARM64_R2, ORC_ARM64_R1,
      compiler->loop_shift + compiler->unroll_shift);
  /** counter2 = r2 */
  orc_arm64_emit_store_reg (compiler, 32, ORC_ARM64_R2, compiler->exec_reg,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,counter2));

  /** r2 = r1 & ((1<<(loop_shift + unroll_shift))-1) */
  orc_arm64_emit_and_imm (compiler, 32, ORC_ARM64_R2, ORC_ARM64_R1,
      (1<<(compiler->loop_shift + compiler->unroll_shift))-1);
  /** counter3 = r2 */
  orc_arm64_emit_store_reg (compiler, 32, ORC_ARM64_R2, compiler->exec_reg,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,counter3));

  /** go to LABEL_ONE_REGION_AFTER */
  orc_arm64_emit_branch (compiler, ORC_ARM_COND_AL, LABEL_ONE_REGION_AFTER);
  orc_arm_emit_label (compiler, LABEL_ONE_REGION);

  /** counter1 = r2 */
  orc_arm64_emit_store_reg (compiler, 32, ORC_ARM64_R2, compiler->exec_reg,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,counter1));
  /** counter2 = counter3 = 0 */
  orc_arm64_emit_mov_uimm (compiler, 32, ORC_ARM64_R2, 0);
  orc_arm64_emit_store_reg (compiler, 32, ORC_ARM64_R2, compiler->exec_reg,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,counter2));
  orc_arm64_emit_store_reg (compiler, 32, ORC_ARM64_R2, compiler->exec_reg,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,counter3));

  orc_arm_emit_label (compiler, LABEL_ONE_REGION_AFTER);
}

static void
orc_neon64_loop_caches (OrcCompiler *compiler)
{
  int align_var;
  int var_size_shift;
  int i;

  align_var = orc_neon_get_align_var (compiler);
  if (compiler->error) return;
  orc_variable_get_shift (&compiler->vars[align_var], &var_size_shift);

  /** if IP0 == 0, go to LABEL_REGION2_SKIP */
  orc_arm64_emit_cmp_imm (compiler, 32, ORC_ARM64_IP0, 0);
  orc_arm64_emit_branch (compiler, ORC_ARM_COND_EQ, LABEL_REGION2_SKIP);

  /** r1 = IP0 >> (17 + var_size_shift - compiler->loop_shift - compiler->unroll_shift) */
  orc_arm64_emit_asr_imm (compiler, 32, compiler->gp_tmpreg, ORC_ARM64_IP0,
      17 + var_size_shift - compiler->loop_shift - compiler->unroll_shift);

  /** if r1 == 0, go to LABEL_REGION2_MEDIUM */
  orc_arm64_emit_cmp_imm (compiler, 32, compiler->gp_tmpreg, 0);
  orc_arm64_emit_branch (compiler, ORC_ARM_COND_EQ, LABEL_REGION2_MEDIUM);

  /** N is larger than L2 cache size */
  compiler->size_region = 3;
  orc_arm_emit_label (compiler, LABEL_REGION2_LOOP_LARGE);
  orc_arm64_emit_subs_imm (compiler, 32, ORC_ARM64_IP0, ORC_ARM64_IP0, 1);
  for(i=0;i<(1<<compiler->unroll_shift);i++){
    orc_neon64_emit_loop (compiler, i);
  }
  orc_arm64_emit_branch (compiler, ORC_ARM_COND_NE, LABEL_REGION2_LOOP_LARGE);
  /** DONE, let's finish */
  orc_arm64_emit_branch (compiler, ORC_ARM_COND_AL, LABEL_REGION2_SKIP);

  orc_arm_emit_label (compiler, LABEL_REGION2_MEDIUM);
  orc_arm64_emit_asr_imm (compiler, 32, compiler->gp_tmpreg, ORC_ARM64_IP0,
      13 + var_size_shift - compiler->loop_shift - compiler->unroll_shift);
  orc_arm64_emit_cmp_imm (compiler, 32, compiler->gp_tmpreg, 0);
  orc_arm64_emit_branch (compiler, ORC_ARM_COND_EQ, LABEL_REGION2_SMALL);

  /* N is smaller than L2 cache size */
  compiler->size_region = 2;
  orc_arm_emit_label (compiler, LABEL_REGION2_LOOP_MEDIUM);
  orc_arm64_emit_subs_imm (compiler, 32, ORC_ARM64_IP0, ORC_ARM64_IP0, 1);
  for(i=0;i<(1<<compiler->unroll_shift);i++){
    orc_neon64_emit_loop (compiler, i);
  }
  orc_arm64_emit_branch (compiler, ORC_ARM_COND_NE, LABEL_REGION2_LOOP_MEDIUM);
  orc_arm64_emit_branch (compiler, ORC_ARM_COND_AL, LABEL_REGION2_SKIP);

  orc_arm_emit_label (compiler, LABEL_REGION2_SMALL);
  /* N is smaller than L1 cache size */
  compiler->size_region = 1;
  orc_arm_emit_label (compiler, LABEL_REGION2_LOOP_SMALL);
  orc_arm64_emit_subs_imm (compiler, 32, ORC_ARM64_IP0, ORC_ARM64_IP0, 1);
  for(i=0;i<(1<<compiler->unroll_shift);i++){
    orc_neon64_emit_loop (compiler, i);
  }
  orc_arm64_emit_branch (compiler, ORC_ARM_COND_NE, LABEL_REGION2_LOOP_SMALL);

  orc_arm_emit_label (compiler, LABEL_REGION2_SKIP);
}

static void
orc_neon64_emit_inc_pointers (OrcCompiler *compiler)
{
  for(int k=0;k<ORC_N_COMPILER_VARIABLES;k++){
    if (compiler->vars[k].name == NULL) continue;
    if (compiler->vars[k].vartype == ORC_VAR_TYPE_SRC ||
        compiler->vars[k].vartype == ORC_VAR_TYPE_DEST) {
      if (compiler->vars[k].ptr_offset) {
        orc_arm64_emit_add_imm (compiler, 64,
            compiler->vars[k].ptr_offset,
            compiler->vars[k].ptr_offset,
            compiler->vars[k].size << compiler->loop_shift);
      } else if (compiler->vars[k].ptr_register) {
        orc_arm64_emit_add_imm (compiler, 64,
            compiler->vars[k].ptr_register,
            compiler->vars[k].ptr_register,
            compiler->vars[k].size << compiler->loop_shift);
      }
    }
  }
}

static void
orc_neon64_load_constants_outer (OrcCompiler *compiler)
{
  for(int i = 0; i < ORC_N_COMPILER_VARIABLES; i++) {
    if (compiler->vars[i].name == NULL) continue;
    if (compiler->vars[i].vartype != ORC_VAR_TYPE_ACCUMULATOR) continue;

    orc_neon64_emit_loadil (compiler, &(compiler->vars[i]), 0);
  }

  orc_compiler_emit_invariants (compiler);
}
