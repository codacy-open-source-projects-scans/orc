#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <orc/orc.h>
#include <orc/orcinternal.h>
#include <orc/avx512/orcavx512.h>

/*
 * TODO
 * To further optimize the compiler we might need to:
 * 1. Have a way to generate mask constants
 */ 

/**
 * orc_avx512_compiler_get_mask_reg:
 *
 * Get a temporary mask register from the pool of registers
 *
 * @c: The #OrcCompiler to get the register from
 * @mask_op: In case the register is going to be used as a mask in an instruction
 * instead of a source or destination register. If #TRUE the potential range will
 * start from #ORC_AVX512_K1, #ORC_AVX512_K0 otherwise
 */
int
orc_avx512_compiler_get_mask_reg (OrcCompiler *c, orc_bool mask_op)
{
  int i = ORC_AVX512_K0;

  if (mask_op)
    i = ORC_AVX512_K1;

  for (; i < ORC_AVX512_K7; i++) {
    if (c->temp_regs[i])
      continue;

    c->temp_regs[i] = TRUE;
    return i;
  }

  ORC_ERROR ("No available mask register");
  return ORC_REG_INVALID;
}

/**
 * orc_avx512_compiler_release_mask_reg:
 *
 * Release a temporary mask register previously allocated with
 * orc_avx512_compiler_get_mask_reg()
 *
 * @c: The #OrcCompiler to release the register
 * @reg: The register to release
 */
void
orc_avx512_compiler_release_mask_reg (OrcCompiler *c, int reg)
{
  if (reg < ORC_AVX512_K0 || reg > ORC_AVX512_K7) {
    ORC_WARNING ("Trying to release a non mask register");
    return;
  }

  ORC_DEBUG ("Releasing mask register %d", reg);
  c->temp_regs[reg] = FALSE;
}
