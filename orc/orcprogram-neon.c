
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

#define SIZE 65536
#define MAX_UNROLL 16

static unsigned int orc_compiler_neon_get_default_flags (void);
static void orc_neon_compiler_init (OrcCompiler *compiler);
static void orc_neon_compile (OrcCompiler *compiler);

unsigned int
orc_neon_get_callee_saved_regs (const OrcCompiler *compiler, int base, int n)
{
  int regs = 0;

  for(int i = 0; i < n; i++)
    if (compiler->used_regs[base + i] && compiler->save_regs[base + i])
      regs |= (1 << i);

  return regs;
}

static OrcTarget neon_target = {
  "neon",
#if defined(HAVE_ARM) || defined(HAVE_AARCH64)
  TRUE,
#else
  FALSE,
#endif
  ORC_VEC_REG_BASE,
  orc_compiler_neon_get_default_flags,
  orc_neon_compiler_init,
  orc_neon_compile,
  { { 0 } }, 0,
  NULL,
  NULL,
  NULL,
  orc_arm_flush_cache,
  NULL,
  NULL,
  { 0 },
  16
};

void
orc_neon_init (void)
{
#if defined(HAVE_ARM) || defined(HAVE_AARCH64)
  if (!(orc_arm_get_cpu_flags () & ORC_TARGET_NEON_NEON)) {
    ORC_INFO("marking neon backend non-executable");
    neon_target.executable = FALSE;
  }
#endif

  orc_target_register (&neon_target);

  orc_compiler_neon_register_rules (&neon_target);
}

static unsigned int
orc_compiler_neon_get_default_flags (void)
{
  unsigned int flags = 0;

#if defined(HAVE_AARCH64)
  flags |= ORC_TARGET_NEON_64BIT;
#endif
  flags |= ORC_TARGET_NEON_NEON;

  return flags;
}

enum RegionFlags
orc_compiler_neon_calc_regions (const OrcCompiler *compiler)
{
  enum RegionFlags flags = 0;

  // NOTE: Assume aligned if we know the input size
  const int constant_n        = compiler->program->constant_n;
  const orc_bool vectorizable = compiler->loop_shift > 0;
  const int num_loops         = constant_n >> compiler->loop_shift;
  const orc_bool n_unrollable = constant_n && (num_loops < MAX_UNROLL);
  const int elm_per_loop      = 1 << compiler->loop_shift;
  const int remaining         = constant_n & (elm_per_loop - 1);
  const orc_bool aligned      = compiler->has_iterator_opcode || (constant_n > 0);
  const orc_bool enough_n     = constant_n >= elm_per_loop;

  if (!vectorizable)
    return FLAG_REGION2; // Only region 2

  orc_bool region1 = !aligned;
  orc_bool region2 = !constant_n || enough_n;
  orc_bool region3 = !n_unrollable || remaining;

  if (region1)
    flags |= FLAG_REGION1;

  if (region2)
    flags |= FLAG_REGION2;

  if (region3)
    flags |= FLAG_REGION3;

  if (n_unrollable)
    flags |= FLAG_NUNROLL;

  return flags;
}

void
orc_neon_compiler_init_common (OrcCompiler *compiler)
{
  int i;
  int loop_shift;

  /** Both architectures have 8 callee-saved SIMD registers (v8-v15) */
  for(i=8;i<16;i++) {
    compiler->save_regs[ORC_VEC_REG_BASE+i] = 1;
  }

  for(i=0;i<ORC_N_REGS;i++){
    compiler->alloc_regs[i] = 0;
    compiler->used_regs[i] = 0;
  }

  compiler->exec_reg = ORC_ARM_A1;
  compiler->gp_tmpreg = ORC_ARM_A2;
  compiler->valid_regs[compiler->exec_reg] = 0;
  compiler->valid_regs[compiler->gp_tmpreg] = 0;
  compiler->valid_regs[compiler->tmpreg] = 0;
  compiler->valid_regs[compiler->tmpreg2] = 0;

  loop_shift = 0;
  
  /* Calculate initial loop_shift based on max_var_size */
  if (!orc_compiler_get_max_loop_shift (compiler, &compiler->loop_shift)) {
    ORC_ERROR("unhandled max var size %d", compiler->max_var_size);
  }

  /* Constrain by max_array_size */
  switch (orc_program_get_max_array_size (compiler->program)) {
    case 0:
    case 1:
      loop_shift = 4;
      break;
    case 2:
      loop_shift = 3;
      break;
    case 4:
      loop_shift = 2;
      break;
    case 8:
      loop_shift = 1;
      break;
    default:
      ORC_ERROR("unhandled max array size %d",
          orc_program_get_max_array_size (compiler->program));
      break;
  }
  if (loop_shift < compiler->loop_shift) {
    compiler->loop_shift = loop_shift;
  }

  switch (orc_program_get_max_accumulator_size (compiler->program)) {
    case 0:
      loop_shift = 4;
      break;
    case 1:
      loop_shift = 3;
      break;
    case 2:
      loop_shift = 2;
      break;
    case 4:
      loop_shift = 1;
      break;
    case 8:
      loop_shift = 0;
      break;
    default:
      ORC_ERROR("unhandled max accumulator size %d",
          orc_program_get_max_accumulator_size (compiler->program));
      break;
  }
  if (loop_shift < compiler->loop_shift) {
    compiler->loop_shift = loop_shift;
  }

  /* Unrolling isn't helpful until neon gets an instruction
   * scheduler.  This decreases the raw amount of code generated
   * while still keeping the feature active. */
  if (compiler->n_insns < 5) {
    compiler->unroll_shift = 0;
  }

  for(i=0;i<compiler->n_insns;i++){
    OrcInstruction *insn = compiler->insns + i;
    OrcStaticOpcode *opcode = insn->opcode;

    if (strcmp (opcode->name, "loadupdb") == 0) {
      compiler->vars[insn->src_args[0]].need_offset_reg = TRUE;
    }
  }

  if (0) {
    compiler->need_mask_regs = TRUE;
  }
}

static void
orc_neon_compiler_init (OrcCompiler *compiler)
{
  if (compiler->target_flags & ORC_TARGET_NEON_64BIT)
    orc_neon64_compiler_init (compiler);
  else
    orc_neon32_compiler_init (compiler);
}

#if 0
void
orc_neon_emit_load_src (OrcCompiler *compiler, OrcVariable *var, int unroll_index)
{
  int ptr_reg;
  int update;

  if (var->ptr_register == 0) {
    int i;
    i = var - compiler->vars;
    /* arm_emit_mov_memoffset_reg (compiler, arm_ptr_size, */
    /*     (int)ORC_STRUCT_OFFSET(OrcExecutor, arrays[i]), */
    /*     p->exec_reg, X86_ECX); */
    ptr_reg = ORC_ARM_PC;
  } else {
    ptr_reg = var->ptr_register;
  }
  if (var->vartype == ORC_VAR_TYPE_DEST) {
    update = FALSE;
  } else {
    update = TRUE;
  }
  switch (var->size) {
    case 1:
      orc_neon_loadb (compiler, var, update);
      break;
    case 2:
      orc_neon_loadw (compiler, var, update);
      break;
    case 4:
      orc_neon_loadl (compiler, var, update);
      break;
    case 8:
      orc_neon_loadq (compiler, var->alloc, ptr_reg, update, var->is_aligned);
      break;
    default:
      ORC_ERROR("bad size");
  }
  
  if (unroll_index == 0) {
  switch (compiler->size_region) {
    case 0:
    case 1:
      orc_neon_preload (compiler, var, FALSE, 208);
      break;
    case 2:
    case 3:
      orc_neon_preload (compiler, var, FALSE, 208);
      break;
  }
  }
}

void
orc_neon_emit_store_dest (OrcCompiler *compiler, OrcVariable *var)
{
  int ptr_reg;
  if (var->ptr_register == 0) {
    /* arm_emit_mov_memoffset_reg (compiler, arm_ptr_size, */
    /*     var->ptr_offset, p->exec_reg, X86_ECX); */
    ptr_reg = ORC_ARM_PC;
  } else {
    ptr_reg = var->ptr_register;
  }
  switch (var->size) {
    case 1:
      orc_neon_storeb (compiler, ptr_reg, TRUE, var->alloc, var->is_aligned);
      break;
    case 2:
      orc_neon_storew (compiler, ptr_reg, TRUE, var->alloc, var->is_aligned);
      break;
    case 4:
      orc_neon_storel (compiler, ptr_reg, TRUE, var->alloc, var->is_aligned);
      break;
    case 8:
      orc_neon_storeq (compiler, ptr_reg, TRUE, var->alloc, var->is_aligned);
      break;
    default:
      ORC_ERROR("bad size");
  }

  switch (compiler->size_region) {
    case 0:
      break;
    case 1:
      /* assume hot cache, see below */
      break;
    case 2:
      /* This is only useful for cold cache and for memset-like operations,
         which isn't the usual case, thus it's disabled. */
#if 0
      orc_neon_preload (compiler, var, FALSE, 208);
#endif
      break;
    case 3:
      /* none */
      break;
  }
}
#endif

int
orc_neon_get_align_var (OrcCompiler *compiler)
{
  if (compiler->vars[ORC_VAR_D1].size) return ORC_VAR_D1;
  if (compiler->vars[ORC_VAR_S1].size) return ORC_VAR_S1;

  ORC_PROGRAM_ERROR(compiler, "could not find alignment variable");

  return -1;
}

static void
orc_neon_compile (OrcCompiler *compiler)
{
  if (compiler->is_64bit)
    orc_neon64_compile (compiler);
  else
    orc_neon32_compile (compiler);
}

void
orc_neon_emit_loop_common (OrcCompiler *compiler, int unroll_index)
{
  int j;
  OrcInstruction *insn;
  OrcStaticOpcode *opcode;
  OrcRule *rule;

  orc_compiler_append_code(compiler,"# LOOP shift %d (%d)\n",
      compiler->loop_shift, unroll_index);
  for(j=0;j<compiler->n_insns;j++){
    compiler->insn_index = j;
    insn = compiler->insns + j;
    opcode = insn->opcode;

    if (insn->flags & ORC_INSN_FLAG_INVARIANT) continue;

    orc_compiler_append_code(compiler,"# %d: %s", j, insn->opcode->name);

    orc_compiler_append_code(compiler,"\n");

    compiler->insn_shift = compiler->loop_shift;
    if (insn->flags & ORC_INSTRUCTION_FLAG_X2) {
      compiler->insn_shift += 1;
    }
    if (insn->flags & ORC_INSTRUCTION_FLAG_X4) {
      compiler->insn_shift += 2;
    }

    rule = insn->rule;
    if (rule && rule->emit) {
      rule->emit (compiler, rule->emit_user, insn);
    } else {
      orc_compiler_append_code(compiler,"No rule for: %s\n", opcode->name);
    }
  }
}
