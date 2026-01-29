#ifndef _ORC_AVX512_H_
#define _ORC_AVX512_H_

#include <orc/orc.h>
#include <orc/orcx86.h>
#include <orc/orcx86insn.h>
#include <orc/orcavx.h> /* To know the X86_YMM0 register */
#include <orc/orcsse.h> /* To know the X86_XMM0 register */

ORC_BEGIN_DECLS

#ifdef ORC_ENABLE_UNSTABLE_API

/* AVX512 has:
 * - 32 general registers starting at 112, therefore ending at 143.
 * - 8 mask registers
 * Orc has a start point of registers at 64 with a total of 32*4 = 128
 * We need to expand Orc register number. Now at 48*4 = 192
 * ORC_AVX512_ZMM0 is 64 + 48 = 112 
 * ORC_AVX512_ZMM31 is 112 + 31 = 143
 * ORC_AVX512_K0 is starting at 176 to leave 64 more potential registers in
 * case another intel extension happens.
 */
typedef enum _OrcAVX512Register {
  ORC_AVX512_ZMM0 = ORC_VEC_REG_BASE + 48,
  ORC_AVX512_ZMM1,
  ORC_AVX512_ZMM2,
  ORC_AVX512_ZMM3,
  ORC_AVX512_ZMM4,
  ORC_AVX512_ZMM5,
  ORC_AVX512_ZMM6,
  ORC_AVX512_ZMM7,
  ORC_AVX512_ZMM8,
  ORC_AVX512_ZMM9,
  ORC_AVX512_ZMM10,
  ORC_AVX512_ZMM11,
  ORC_AVX512_ZMM12,
  ORC_AVX512_ZMM13,
  ORC_AVX512_ZMM14,
  ORC_AVX512_ZMM15,
  ORC_AVX512_ZMM16,
  ORC_AVX512_ZMM17,
  ORC_AVX512_ZMM18,
  ORC_AVX512_ZMM19,
  ORC_AVX512_ZMM20,
  ORC_AVX512_ZMM21,
  ORC_AVX512_ZMM22,
  ORC_AVX512_ZMM23,
  ORC_AVX512_ZMM24,
  ORC_AVX512_ZMM25,
  ORC_AVX512_ZMM26,
  ORC_AVX512_ZMM27,
  ORC_AVX512_ZMM28,
  ORC_AVX512_ZMM29,
  ORC_AVX512_ZMM30,
  ORC_AVX512_ZMM31,
  /* K (Opmask) rgisters */
  ORC_AVX512_K0 = 176,
  ORC_AVX512_K1,
  ORC_AVX512_K2,
  ORC_AVX512_K3,
  ORC_AVX512_K4,
  ORC_AVX512_K5,
  ORC_AVX512_K6,
  ORC_AVX512_K7,
} OrcAVX512Register;

#define ORC_AVX512_SSE_REG(i) (i == ORC_REG_INVALID ? ORC_REG_INVALID : X86_XMM0 + (i - ORC_AVX512_ZMM0))
#define ORC_AVX512_AVX_REG(i) (i == ORC_REG_INVALID ? ORC_REG_INVALID : X86_YMM0 + (i - ORC_AVX512_ZMM0))

#define ORC_AVX512_SHUF(a, b, c, d) \
  ((((a)&3) << 6) | (((b)&3) << 4) | (((c)&3) << 2) | (((d)&3) << 0))

ORC_API const char * orc_avx512_get_regname (int i);
ORC_API unsigned int orc_avx512_get_cpu_flags (void);
#endif

ORC_END_DECLS

#endif
