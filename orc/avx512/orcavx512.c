#include "config.h"

#include <stdio.h>

#include <orc/orctarget.h>
#include <orc/orcx86.h>
#include <orc/orcx86insn.h>
#include <orc/orcx86-private.h>
#include <orc/avx512/orcavx512.h>
#include <orc/avx512/orcavx512-private.h>

#define XCR0_SUPPORT_XMM 1U << 1
#define XCR0_SUPPORT_YMM 1U << 2
#define XCR0_SUPPORT_K 1U << 5
#define XCR0_SUPPORT_ZMM0 1U << 6
#define XCR0_SUPPORT_ZMM16 1U << 7

#define XCR0_SUPPORT_AVX512 (\
  XCR0_SUPPORT_XMM |         \
  XCR0_SUPPORT_YMM |         \
  XCR0_SUPPORT_ZMM0 |        \
  XCR0_SUPPORT_ZMM16 |       \
  XCR0_SUPPORT_K             \
)

/* Found at EBX */
#define HAVE_AVX512F 1U << 16
#define HAVE_AVX512CD 1U << 28
#define HAVE_AVX512ER 1U << 27
#define HAVE_AVX512PF 1U << 26
#define HAVE_AVX512VL 1U << 31
#define HAVE_AVX512DQ 1U << 17
#define HAVE_AVX512BW 1U << 30
#define HAVE_AVX512_IFMA 1U << 21
/* Found at ECX */
#define HAVE_AVX512_VBMI 1U << 1
#define HAVE_AVX512_VNNI 1U << 11
#define HAVE_AVX512_BF16 1U << 22
#define HAVE_AVX512_VBMI2 1U << 6
#define HAVE_AVX512_BITALG 1U << 12
#define HAVE_AVX512_VPCLMULQDQ 1U << 10
#define HAVE_AVX512_GFNI 1U << 8
#define HAVE_AVX512_VAES 1U << 9
/* Found at EDX */
#define HAVE_AVX512_VP2INTERSECT 1U << 8
#define HAVE_AVX512_FP16 1U << 23

/* Check the documentation at Vol. 1, Ch. 15.2
 * DETECTION OF AVX-512 FOUNDATION INSTRUCTIONS
 */
unsigned int
orc_avx512_get_cpu_flags(void)
{
  unsigned int flags = 0;
#if defined(HAVE_AMD64) || defined(HAVE_I386)
  OrcTargetAVX512Flags flag;
  orc_uint32 eax, ebx, ecx, edx;

  orc_x86_cpu_detect (NULL, NULL);
  if (!orc_x86_cpu_is_xsave_enabled ())
    goto done;

  const orc_uint32 xcr0 = orc_x86_cpu_get_xcr0 ();
  /* Check the extended control register */
  if (!((xcr0 & XCR0_SUPPORT_AVX512) == XCR0_SUPPORT_AVX512))
    goto done;

  /* Check the instruction sets */
  orc_x86_cpu_get_cpuid (0x00000007, &eax, &ebx, &ecx, &edx);
  /* EBX case */
  if (ebx & HAVE_AVX512F)
    flags |= ORC_TARGET_AVX512_F;
  if (ebx & HAVE_AVX512CD)
    flags |= ORC_TARGET_AVX512_CD;
  if (ebx & HAVE_AVX512ER)
    flags |= ORC_TARGET_AVX512_ER;
  if (ebx & HAVE_AVX512PF)
    flags |= ORC_TARGET_AVX512_PF;
  if (ebx & HAVE_AVX512VL)
    flags |= ORC_TARGET_AVX512_VL;
  if (ebx & HAVE_AVX512DQ)
    flags |= ORC_TARGET_AVX512_DQ;
  if (ebx & HAVE_AVX512BW)
    flags |= ORC_TARGET_AVX512_BW;
  if (ebx & HAVE_AVX512_IFMA)
    flags |= ORC_TARGET_AVX512_IFMA;
  /* ECX case */
  if (ecx & HAVE_AVX512_VBMI)
    flags |= ORC_TARGET_AVX512_VBMI;
  if (ecx & HAVE_AVX512_VNNI)
    flags |= ORC_TARGET_AVX512_VNNI;
  if (ecx & HAVE_AVX512_BF16)
    flags |= ORC_TARGET_AVX512_BF16;
  if (ecx & HAVE_AVX512_VBMI2)
    flags |= ORC_TARGET_AVX512_VBMI2;
  if (ecx & HAVE_AVX512_BITALG)
    flags |= ORC_TARGET_AVX512_BITALG;
  if (ecx & HAVE_AVX512_VPCLMULQDQ)
    flags |= ORC_TARGET_AVX512_VPCLMULQDQ;
  if (ecx & HAVE_AVX512_GFNI)
    flags |= ORC_TARGET_AVX512_GFNI;
  if (ecx & HAVE_AVX512_VAES)
    flags |= ORC_TARGET_AVX512_VAES;
  /* EDX case */
  if (ecx & HAVE_AVX512_VP2INTERSECT)
    flags |= ORC_TARGET_AVX512_VP2INTERSECT;
  if (ecx & HAVE_AVX512_FP16)
    flags |= ORC_TARGET_AVX512_FP16;

  /* clear the flags based on the compiler flags */
  for (flag = ORC_TARGET_AVX512_F; flag <= ORC_TARGET_AVX512_FP16; flag <<= 1) {
    char strflag[40];

    snprintf (strflag, 40, "-%s", orc_avx512_get_flag_name (flag));
    if (orc_compiler_flag_check (strflag)) {
      flags &= ~flag;
    }
  }

done:
#endif
  return flags;
}

const char *
orc_avx512_get_regname (int i)
{
  static const char *regs[] = {
    "zmm0", "zmm1", "zmm2", "zmm3", "zmm4", "zmm5", "zmm6", "zmm7",
    "zmm8", "zmm9", "zmm10", "zmm11", "zmm12", "zmm13", "zmm14", "zmm15"
    "zmm16", "zmm17", "zmm18", "zmm19", "zmm20", "zmm21", "zmm22", "zmm23",
    "zmm24", "zmm25", "zmm26", "zmm27", "zmm28", "zmm29", "zmm30", "zmm31"
  };

  static const char *mregs[] = {
    "k0", "k1", "k2", "k3", "k4", "k5", "k6", "k7"
  };

  if (i >= ORC_AVX512_ZMM0 && i < ORC_AVX512_ZMM0 + 32) return regs[i - ORC_AVX512_ZMM0];
  if (i >= ORC_AVX512_K0 && i < ORC_AVX512_K0 + 8) return mregs[i - ORC_AVX512_K0];
  switch (i) {
    case 0:
      return "UNALLOCATED";
    case 1:
      return "direct";
    default:
      return "ERROR";
  }
}

const char *
orc_avx512_get_flag_name (OrcTargetAVX512Flags flag)
{
  switch (flag) {
    case ORC_TARGET_AVX512_FRAME_POINTER:
      return "frame_pointer";
    case ORC_TARGET_AVX512_SHORT_JUMPS:
      return "short_jumps";
    case ORC_TARGET_AVX512_64BIT:
      return "64bit";
    case ORC_TARGET_AVX512_F:
      return "avx512f";
    case ORC_TARGET_AVX512_CD:
      return "avx512cd";
    case ORC_TARGET_AVX512_ER:
      return "avx512er";
    case ORC_TARGET_AVX512_PF:
      return "avx512pf";
    case ORC_TARGET_AVX512_4FMAPS:
      return "avx512_4fmaps";
    case ORC_TARGET_AVX512_4VNNIW:
      return "avx512_4vnniw";
    case ORC_TARGET_AVX512_VPOPCNTDQ:
      return "avx512_vpopcntdq";
    case ORC_TARGET_AVX512_VL:
      return "avx512vl";
    case ORC_TARGET_AVX512_DQ:
      return "avx512dq";
    case ORC_TARGET_AVX512_BW:
      return "avx512bw";
    case ORC_TARGET_AVX512_IFMA:
      return "avx512ifma";
    case ORC_TARGET_AVX512_VBMI:
      return "avx512vbmi";
    case ORC_TARGET_AVX512_VNNI:
      return "avx512_vnni";
    case ORC_TARGET_AVX512_BF16:
      return "avx512_bf16";
    case ORC_TARGET_AVX512_VBMI2:
      return "avx512_vbmi2";
    case ORC_TARGET_AVX512_BITALG:
      return "avx512_bitalg";
    case ORC_TARGET_AVX512_VPCLMULQDQ:
      return "avx512_vpclmuldq";
    case ORC_TARGET_AVX512_GFNI:
      return "avx512_gfni";
    case ORC_TARGET_AVX512_VAES:
      return "avx512_vaes";
    case ORC_TARGET_AVX512_VP2INTERSECT:
      return "avx512_vp2intersect";
    case ORC_TARGET_AVX512_FP16:
      return "avx512_fp16";
    default:
      ORC_ERROR ("Unsupported flag %08x", flag);
      return NULL;
  }
}

void
orc_avx512_init (void)
{
  OrcTarget *t;
  t = orc_avx512_target_init ();
  orc_avx512_rules_init (t);
}
