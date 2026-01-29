#ifndef ORC_AVX512_INSN_H_
#define ORC_AVX512_INSN_H_

ORC_BEGIN_DECLS

#ifdef ORC_ENABLE_UNSTABLE_API

/* The format is the following:
 * Each opcode has or either the name of the opcode, or a suffix to
 * differentiate from other variants of the same opcode
 * {opcode}_[k,r,m]_[k,r,m,i]_[k,r,m,i]_[k,r,m,i]
 * Depending on the number of operands and the type
 * r: register
 * rm: register/memory
 * m: memory
 * i: immediate
 * k: mask
 * km: mask/memory
 * If the opcode requires further disambiguation, r can be replaced by
 * sse, avx or avx512.
 *
 * Each opcode has a macro to use it with the pattern
 * orc_avx512_insn_[avx,sse]_emit_{opcode}
 * In case it requires further disambiguation, use above rules to dsccribe
 * the operands.
 *
 * TODO
 * Add a _full_ macro that will receive the size to work on, and the macro will
 * call the corresponding SSE, AVX or AVX512 variant
 */

typedef enum _OrcAVX512Insn {
  ORC_AVX512_INSN_SSE_vpshufb,
  ORC_AVX512_INSN_AVX_vpshufb,
  ORC_AVX512_INSN_vpshufb,
  ORC_AVX512_INSN_AVX_vpermq_r_rm_i,
  ORC_AVX512_INSN_vpermq_r_rm_i,
  ORC_AVX512_INSN_SSE_vpmaddubsw,
  ORC_AVX512_INSN_AVX_vpmaddubsw,
  ORC_AVX512_INSN_vpmaddubsw,
  ORC_AVX512_INSN_SSE_vpmulhrsw,
  ORC_AVX512_INSN_AVX_vpmulhrsw,
  ORC_AVX512_INSN_vpmulhrsw,
  ORC_AVX512_INSN_SSE_vpalignr,
  ORC_AVX512_INSN_AVX_vpalignr,
  ORC_AVX512_INSN_vpalignr,
  ORC_AVX512_INSN_vpextrb,
  ORC_AVX512_INSN_vpextrw,
  ORC_AVX512_INSN_vmovhps_r_r_m,
  ORC_AVX512_INSN_vmovhps_m_r,
  ORC_AVX512_INSN_SSE_vpabsb,
  ORC_AVX512_INSN_AVX_vpabsb,
  ORC_AVX512_INSN_vpabsb,
  ORC_AVX512_INSN_SSE_vpabsw,
  ORC_AVX512_INSN_AVX_vpabsw,
  ORC_AVX512_INSN_vpabsw,
  ORC_AVX512_INSN_SSE_vpabsd,
  ORC_AVX512_INSN_AVX_vpabsd,
  ORC_AVX512_INSN_vpabsd,
  ORC_AVX512_INSN_SSE_vpcmpeqd,
  ORC_AVX512_INSN_AVX_vpcmpeqd,
  ORC_AVX512_INSN_vpcmpeqd,
  ORC_AVX512_INSN_SSE_vpcmpltd,
  ORC_AVX512_INSN_AVX_vpcmpltd,
  ORC_AVX512_INSN_vpcmpltd,
  ORC_AVX512_INSN_SSE_vpcmpled,
  ORC_AVX512_INSN_AVX_vpcmpled,
  ORC_AVX512_INSN_vpcmpled,
  ORC_AVX512_INSN_SSE_vpcmpneqd,
  ORC_AVX512_INSN_AVX_vpcmpneqd,
  ORC_AVX512_INSN_vpcmpneqd,
  ORC_AVX512_INSN_SSE_vpcmpnltd,
  ORC_AVX512_INSN_AVX_vpcmpnltd,
  ORC_AVX512_INSN_vpcmpnltd,
  ORC_AVX512_INSN_SSE_vpcmpnled,
  ORC_AVX512_INSN_AVX_vpcmpnled,
  ORC_AVX512_INSN_vpcmpnled,
  ORC_AVX512_INSN_vpinsrb,
  ORC_AVX512_INSN_SSE_vpmovsxbw,
  ORC_AVX512_INSN_AVX_vpmovsxbw,
  ORC_AVX512_INSN_vpmovsxbw,
  ORC_AVX512_INSN_SSE_vpmovsxbd,
  ORC_AVX512_INSN_AVX_vpmovsxbd,
  ORC_AVX512_INSN_vpmovsxbd,
  ORC_AVX512_INSN_SSE_vpmovsxbq,
  ORC_AVX512_INSN_AVX_vpmovsxbq,
  ORC_AVX512_INSN_vpmovsxbq,
  ORC_AVX512_INSN_vpinsrd,
  ORC_AVX512_INSN_SSE_vpmovsxwd,
  ORC_AVX512_INSN_AVX_vpmovsxwd,
  ORC_AVX512_INSN_vpmovsxwd,
  ORC_AVX512_INSN_SSE_vpmovsxwq,
  ORC_AVX512_INSN_AVX_vpmovsxwq,
  ORC_AVX512_INSN_vpmovsxwq,
  ORC_AVX512_INSN_SSE_vpmovsxdq,
  ORC_AVX512_INSN_AVX_vpmovsxdq,
  ORC_AVX512_INSN_vpmovsxdq,
  ORC_AVX512_INSN_SSE_vpternlogd,
  ORC_AVX512_INSN_AVX_vpternlogd,
  ORC_AVX512_INSN_vpternlogd,
  ORC_AVX512_INSN_SSE_vpternlogq,
  ORC_AVX512_INSN_AVX_vpternlogq,
  ORC_AVX512_INSN_vpternlogq,
  ORC_AVX512_INSN_SSE_vpmuldq,
  ORC_AVX512_INSN_AVX_vpmuldq,
  ORC_AVX512_INSN_vpmuldq,
  ORC_AVX512_INSN_SSE_vpmovm2b,
  ORC_AVX512_INSN_AVX_vpmovm2b,
  ORC_AVX512_INSN_vpmovm2b,
  ORC_AVX512_INSN_SSE_vpmovm2w,
  ORC_AVX512_INSN_AVX_vpmovm2w,
  ORC_AVX512_INSN_vpmovm2w,
  ORC_AVX512_INSN_SSE_vpcmpeqq,
  ORC_AVX512_INSN_AVX_vpcmpeqq,
  ORC_AVX512_INSN_vpcmpeqq,
  ORC_AVX512_INSN_SSE_vpmovb2m,
  ORC_AVX512_INSN_AVX_vpmovb2m,
  ORC_AVX512_INSN_vpmovb2m,
  ORC_AVX512_INSN_SSE_vpmovw2m,
  ORC_AVX512_INSN_AVX_vpmovw2m,
  ORC_AVX512_INSN_vpmovw2m,
  ORC_AVX512_INSN_SSE_vpackusdw,
  ORC_AVX512_INSN_AVX_vpackusdw,
  ORC_AVX512_INSN_vpackusdw,
  ORC_AVX512_INSN_SSE_vpmovzxbw,
  ORC_AVX512_INSN_AVX_vpmovzxbw,
  ORC_AVX512_INSN_vpmovzxbw,
  ORC_AVX512_INSN_kshiftrw,
  ORC_AVX512_INSN_kshiftrb,
  ORC_AVX512_INSN_SSE_vpmovzxbd,
  ORC_AVX512_INSN_AVX_vpmovzxbd,
  ORC_AVX512_INSN_vpmovzxbd,
  ORC_AVX512_INSN_kshiftrq,
  ORC_AVX512_INSN_kshiftrd,
  ORC_AVX512_INSN_SSE_vpmovzxbq,
  ORC_AVX512_INSN_AVX_vpmovzxbq,
  ORC_AVX512_INSN_vpmovzxbq,
  ORC_AVX512_INSN_kshiftlb,
  ORC_AVX512_INSN_kshiftlw,
  ORC_AVX512_INSN_SSE_vpmovzxwd,
  ORC_AVX512_INSN_AVX_vpmovzxwd,
  ORC_AVX512_INSN_vpmovzxwd,
  ORC_AVX512_INSN_kshiftld,
  ORC_AVX512_INSN_kshiftlq,
  ORC_AVX512_INSN_SSE_vpmovzxwq,
  ORC_AVX512_INSN_AVX_vpmovzxwq,
  ORC_AVX512_INSN_vpmovzxwq,
  ORC_AVX512_INSN_SSE_vpmovzxdq,
  ORC_AVX512_INSN_AVX_vpmovzxdq,
  ORC_AVX512_INSN_vpmovzxdq,
  ORC_AVX512_INSN_SSE_vpmovqd,
  ORC_AVX512_INSN_AVX_vpmovqd,
  ORC_AVX512_INSN_vpmovqd,
  ORC_AVX512_INSN_AVX_vpermd,
  ORC_AVX512_INSN_vpermd,
  ORC_AVX512_INSN_AVX_vpermq,
  ORC_AVX512_INSN_vpermq,
  ORC_AVX512_INSN_SSE_vpcmpgtq,
  ORC_AVX512_INSN_AVX_vpcmpgtq,
  ORC_AVX512_INSN_vpcmpgtq,
  ORC_AVX512_INSN_SSE_vpminsb,
  ORC_AVX512_INSN_AVX_vpminsb,
  ORC_AVX512_INSN_vpminsb,
  ORC_AVX512_INSN_AVX_vinserti32x4,
  ORC_AVX512_INSN_vinserti32x4,
  ORC_AVX512_INSN_AVX_vinserti64x2,
  ORC_AVX512_INSN_vinserti64x2,
  ORC_AVX512_INSN_SSE_vpmovm2d,
  ORC_AVX512_INSN_AVX_vpmovm2d,
  ORC_AVX512_INSN_vpmovm2d,
  ORC_AVX512_INSN_SSE_vpmovm2q,
  ORC_AVX512_INSN_AVX_vpmovm2q,
  ORC_AVX512_INSN_vpmovm2q,
  ORC_AVX512_INSN_SSE_vpminsd,
  ORC_AVX512_INSN_AVX_vpminsd,
  ORC_AVX512_INSN_vpminsd,
  ORC_AVX512_INSN_AVX_vextracti32x4,
  ORC_AVX512_INSN_vextracti32x4,
  ORC_AVX512_INSN_AVX_vextracti64x2,
  ORC_AVX512_INSN_vextracti64x2,
  ORC_AVX512_INSN_SSE_vpmovd2m,
  ORC_AVX512_INSN_AVX_vpmovd2m,
  ORC_AVX512_INSN_vpmovd2m,
  ORC_AVX512_INSN_SSE_vpminuw,
  ORC_AVX512_INSN_AVX_vpminuw,
  ORC_AVX512_INSN_vpminuw,
  ORC_AVX512_INSN_vinserti32x8,
  ORC_AVX512_INSN_vinserti64x4,
  ORC_AVX512_INSN_SSE_vpminud,
  ORC_AVX512_INSN_AVX_vpminud,
  ORC_AVX512_INSN_vpminud,
  ORC_AVX512_INSN_AVX_vextracti32x8,
  ORC_AVX512_INSN_vextracti64x4,
  ORC_AVX512_INSN_SSE_vpmaxsb,
  ORC_AVX512_INSN_AVX_vpmaxsb,
  ORC_AVX512_INSN_vpmaxsb,
  ORC_AVX512_INSN_SSE_vpmaxsd,
  ORC_AVX512_INSN_AVX_vpmaxsd,
  ORC_AVX512_INSN_vpmaxsd,
  ORC_AVX512_INSN_SSE_vpmaxuw,
  ORC_AVX512_INSN_AVX_vpmaxuw,
  ORC_AVX512_INSN_vpmaxuw,
  ORC_AVX512_INSN_SSE_vpmaxud,
  ORC_AVX512_INSN_AVX_vpmaxud,
  ORC_AVX512_INSN_vpmaxud,
  ORC_AVX512_INSN_SSE_vpcmpeqb,
  ORC_AVX512_INSN_AVX_vpcmpeqb,
  ORC_AVX512_INSN_vpcmpeqb,
  ORC_AVX512_INSN_SSE_vpcmpltb,
  ORC_AVX512_INSN_AVX_vpcmpltb,
  ORC_AVX512_INSN_vpcmpltb,
  ORC_AVX512_INSN_SSE_vpcmpleb,
  ORC_AVX512_INSN_AVX_vpcmpleb,
  ORC_AVX512_INSN_vpcmpleb,
  ORC_AVX512_INSN_SSE_vpcmpneqb,
  ORC_AVX512_INSN_AVX_vpcmpneqb,
  ORC_AVX512_INSN_vpcmpneqb,
  ORC_AVX512_INSN_SSE_vpcmpnltb,
  ORC_AVX512_INSN_AVX_vpcmpnltb,
  ORC_AVX512_INSN_vpcmpnltb,
  ORC_AVX512_INSN_SSE_vpcmpnleb,
  ORC_AVX512_INSN_AVX_vpcmpnleb,
  ORC_AVX512_INSN_vpcmpnleb,
  ORC_AVX512_INSN_SSE_vpcmpeqw,
  ORC_AVX512_INSN_AVX_vpcmpeqw,
  ORC_AVX512_INSN_vpcmpeqw,
  ORC_AVX512_INSN_SSE_vpcmpltw,
  ORC_AVX512_INSN_AVX_vpcmpltw,
  ORC_AVX512_INSN_vpcmpltw,
  ORC_AVX512_INSN_SSE_vpcmplew,
  ORC_AVX512_INSN_AVX_vpcmplew,
  ORC_AVX512_INSN_vpcmplew,
  ORC_AVX512_INSN_SSE_vpcmpneqw,
  ORC_AVX512_INSN_AVX_vpcmpneqw,
  ORC_AVX512_INSN_vpcmpneqw,
  ORC_AVX512_INSN_SSE_vpcmpnltw,
  ORC_AVX512_INSN_AVX_vpcmpnltw,
  ORC_AVX512_INSN_vpcmpnltw,
  ORC_AVX512_INSN_SSE_vpcmpnlew,
  ORC_AVX512_INSN_AVX_vpcmpnlew,
  ORC_AVX512_INSN_vpcmpnlew,
  ORC_AVX512_INSN_SSE_vpmulld,
  ORC_AVX512_INSN_AVX_vpmulld,
  ORC_AVX512_INSN_vpmulld,
  ORC_AVX512_INSN_kxnorw,
  ORC_AVX512_INSN_kxnorb,
  ORC_AVX512_INSN_kxnorq,
  ORC_AVX512_INSN_kxnord,
  ORC_AVX512_INSN_SSE_vsqrtps,
  ORC_AVX512_INSN_AVX_vsqrtps,
  ORC_AVX512_INSN_vsqrtps,
  ORC_AVX512_INSN_SSE_vsqrtpd,
  ORC_AVX512_INSN_AVX_vsqrtpd,
  ORC_AVX512_INSN_vsqrtpd,
  ORC_AVX512_INSN_SSE_vandps,
  ORC_AVX512_INSN_AVX_vandps,
  ORC_AVX512_INSN_vandps,
  ORC_AVX512_INSN_SSE_vorps,
  ORC_AVX512_INSN_AVX_vorps,
  ORC_AVX512_INSN_vorps,
  ORC_AVX512_INSN_SSE_vorpd,
  ORC_AVX512_INSN_AVX_vorpd,
  ORC_AVX512_INSN_vorpd,
  ORC_AVX512_INSN_SSE_vaddps,
  ORC_AVX512_INSN_AVX_vaddps,
  ORC_AVX512_INSN_vaddps,
  ORC_AVX512_INSN_SSE_vaddpd,
  ORC_AVX512_INSN_AVX_vaddpd,
  ORC_AVX512_INSN_vaddpd,
  ORC_AVX512_INSN_SSE_vpbroadcastd,
  ORC_AVX512_INSN_AVX_vpbroadcastd,
  ORC_AVX512_INSN_vpbroadcastd,
  ORC_AVX512_INSN_SSE_vmulps,
  ORC_AVX512_INSN_AVX_vmulps,
  ORC_AVX512_INSN_vmulps,
  ORC_AVX512_INSN_SSE_vmulpd,
  ORC_AVX512_INSN_AVX_vmulpd,
  ORC_AVX512_INSN_vmulpd,
  ORC_AVX512_INSN_SSE_vpbroadcastq,
  ORC_AVX512_INSN_AVX_vpbroadcastq,
  ORC_AVX512_INSN_vpbroadcastq,
  ORC_AVX512_INSN_SSE_vcvtps2pd,
  ORC_AVX512_INSN_AVX_vcvtps2pd,
  ORC_AVX512_INSN_vcvtps2pd,
  ORC_AVX512_INSN_SSE_vcvtpd2ps,
  ORC_AVX512_INSN_AVX_vcvtpd2ps,
  ORC_AVX512_INSN_vcvtpd2ps,
  ORC_AVX512_INSN_SSE_vcvtdq2ps,
  ORC_AVX512_INSN_AVX_vcvtdq2ps,
  ORC_AVX512_INSN_vcvtdq2ps,
  ORC_AVX512_INSN_SSE_vcvtps2dq,
  ORC_AVX512_INSN_AVX_vcvtps2dq,
  ORC_AVX512_INSN_vcvtps2dq,
  ORC_AVX512_INSN_SSE_vcvttps2dq,
  ORC_AVX512_INSN_AVX_vcvttps2dq,
  ORC_AVX512_INSN_vcvttps2dq,
  ORC_AVX512_INSN_SSE_vsubps,
  ORC_AVX512_INSN_AVX_vsubps,
  ORC_AVX512_INSN_vsubps,
  ORC_AVX512_INSN_SSE_vsubpd,
  ORC_AVX512_INSN_AVX_vsubpd,
  ORC_AVX512_INSN_vsubpd,
  ORC_AVX512_INSN_SSE_vminps,
  ORC_AVX512_INSN_AVX_vminps,
  ORC_AVX512_INSN_vminps,
  ORC_AVX512_INSN_SSE_vminpd,
  ORC_AVX512_INSN_AVX_vminpd,
  ORC_AVX512_INSN_vminpd,
  ORC_AVX512_INSN_SSE_vdivps,
  ORC_AVX512_INSN_AVX_vdivps,
  ORC_AVX512_INSN_vdivps,
  ORC_AVX512_INSN_SSE_vdivpd,
  ORC_AVX512_INSN_AVX_vdivpd,
  ORC_AVX512_INSN_vdivpd,
  ORC_AVX512_INSN_SSE_vmaxps,
  ORC_AVX512_INSN_AVX_vmaxps,
  ORC_AVX512_INSN_vmaxps,
  ORC_AVX512_INSN_SSE_vmaxpd,
  ORC_AVX512_INSN_AVX_vmaxpd,
  ORC_AVX512_INSN_vmaxpd,
  ORC_AVX512_INSN_SSE_vpunpcklbw,
  ORC_AVX512_INSN_AVX_vpunpcklbw,
  ORC_AVX512_INSN_vpunpcklbw,
  ORC_AVX512_INSN_SSE_vpunpcklwd,
  ORC_AVX512_INSN_AVX_vpunpcklwd,
  ORC_AVX512_INSN_vpunpcklwd,

  ORC_AVX512_INSN_SSE_vpunpckldq,
  ORC_AVX512_INSN_AVX_vpunpckldq,
  ORC_AVX512_INSN_vpunpckldq,

  ORC_AVX512_INSN_SSE_vpacksswb,
  ORC_AVX512_INSN_AVX_vpacksswb,
  ORC_AVX512_INSN_vpacksswb,

  ORC_AVX512_INSN_SSE_vpcmpgtb,
  ORC_AVX512_INSN_AVX_vpcmpgtb,
  ORC_AVX512_INSN_vpcmpgtb,

  ORC_AVX512_INSN_SSE_vpblendmd,
  ORC_AVX512_INSN_AVX_vpblendmd,
  ORC_AVX512_INSN_vpblendmd,

  ORC_AVX512_INSN_SSE_vpblendmq,
  ORC_AVX512_INSN_AVX_vpblendmq,
  ORC_AVX512_INSN_vpblendmq,

  ORC_AVX512_INSN_SSE_vpcmpgtw,
  ORC_AVX512_INSN_AVX_vpcmpgtw,
  ORC_AVX512_INSN_vpcmpgtw,

  ORC_AVX512_INSN_SSE_vpblendmb,
  ORC_AVX512_INSN_AVX_vpblendmb,
  ORC_AVX512_INSN_vpblendmb,
  ORC_AVX512_INSN_SSE_vpblendmw,
  ORC_AVX512_INSN_AVX_vpblendmw,
  ORC_AVX512_INSN_vpblendmw,
  ORC_AVX512_INSN_SSE_vpcmpgtd,
  ORC_AVX512_INSN_AVX_vpcmpgtd,
  ORC_AVX512_INSN_vpcmpgtd,
  ORC_AVX512_INSN_SSE_vpackuswb,
  ORC_AVX512_INSN_AVX_vpackuswb,
  ORC_AVX512_INSN_vpackuswb,
  ORC_AVX512_INSN_SSE_vpunpckhbw,
  ORC_AVX512_INSN_AVX_vpunpckhbw,
  ORC_AVX512_INSN_vpunpckhbw,
  ORC_AVX512_INSN_SSE_vpunpckhwd,
  ORC_AVX512_INSN_AVX_vpunpckhwd,
  ORC_AVX512_INSN_vpunpckhwd,
  ORC_AVX512_INSN_SSE_vpunpckhdq,
  ORC_AVX512_INSN_AVX_vpunpckhdq,
  ORC_AVX512_INSN_vpunpckhdq,
  ORC_AVX512_INSN_SSE_vpackssdw,
  ORC_AVX512_INSN_AVX_vpackssdw,
  ORC_AVX512_INSN_vpackssdw,
  ORC_AVX512_INSN_SSE_vpunpcklqdq,
  ORC_AVX512_INSN_AVX_vpunpcklqdq,
  ORC_AVX512_INSN_vpunpcklqdq,
  ORC_AVX512_INSN_SSE_vpunpckhqdq,
  ORC_AVX512_INSN_AVX_vpunpckhqdq,
  ORC_AVX512_INSN_vpunpckhqdq,
  ORC_AVX512_INSN_vmovd_r_rm,
  ORC_AVX512_INSN_vmovq_sse_rm,
  ORC_AVX512_INSN_SSE_vmovdqa32_r_rm,
  ORC_AVX512_INSN_AVX_vmovdqa32_r_rm,
  ORC_AVX512_INSN_vmovdqa32_r_rm,
  ORC_AVX512_INSN_SSE_vmovdqa64_r_rm,
  ORC_AVX512_INSN_AVX_vmovdqa64_r_rm,
  ORC_AVX512_INSN_vmovdqa64_r_rm,
  ORC_AVX512_INSN_SSE_vmovdqu8_r_rm,
  ORC_AVX512_INSN_AVX_vmovdqu8_r_rm,
  ORC_AVX512_INSN_vmovdqu8_r_rm,
  ORC_AVX512_INSN_SSE_vmovdqu16_r_rm,
  ORC_AVX512_INSN_AVX_vmovdqu16_r_rm,
  ORC_AVX512_INSN_vmovdqu16_r_rm,
  ORC_AVX512_INSN_SSE_vmovdqu32_r_rm,
  ORC_AVX512_INSN_AVX_vmovdqu32_r_rm,
  ORC_AVX512_INSN_vmovdqu32_r_rm,
  ORC_AVX512_INSN_SSE_vmovdqu64_r_rm,
  ORC_AVX512_INSN_AVX_vmovdqu64_r_rm,
  ORC_AVX512_INSN_vmovdqu64_r_rm,
  ORC_AVX512_INSN_SSE_vpshufd,
  ORC_AVX512_INSN_AVX_vpshufd,
  ORC_AVX512_INSN_vpshufd,
  ORC_AVX512_INSN_SSE_vpshuflw,
  ORC_AVX512_INSN_AVX_vpshuflw,
  ORC_AVX512_INSN_vpshuflw,
  ORC_AVX512_INSN_SSE_vpshufhw,
  ORC_AVX512_INSN_AVX_vpshufhw,
  ORC_AVX512_INSN_vpshufhw,
  ORC_AVX512_INSN_SSE_vpsrlw_r_rm_i,
  ORC_AVX512_INSN_AVX_vpsrlw_r_rm_i,
  ORC_AVX512_INSN_vpsrlw_r_rm_i,
  ORC_AVX512_INSN_SSE_vpsraw_r_rm_i,
  ORC_AVX512_INSN_AVX_vpsraw_r_rm_i,
  ORC_AVX512_INSN_vpsraw_r_rm_i,
  ORC_AVX512_INSN_SSE_vpsllw_r_rm_i,
  ORC_AVX512_INSN_AVX_vpsllw_r_rm_i,
  ORC_AVX512_INSN_vpsllw_r_rm_i,
  ORC_AVX512_INSN_SSE_vpsrld_r_rm_i,
  ORC_AVX512_INSN_AVX_vpsrld_r_rm_i,
  ORC_AVX512_INSN_vpsrld_r_rm_i,
  ORC_AVX512_INSN_SSE_vpsrad_r_rm_i,
  ORC_AVX512_INSN_AVX_vpsrad_r_rm_i,
  ORC_AVX512_INSN_vpsrad_r_rm_i,
  ORC_AVX512_INSN_SSE_vpsraq_r_rm_i,
  ORC_AVX512_INSN_AVX_vpsraq_r_rm_i,
  ORC_AVX512_INSN_vpsraq_r_rm_i,
  ORC_AVX512_INSN_SSE_vpslld_r_rm_i,
  ORC_AVX512_INSN_AVX_vpslld_r_rm_i,
  ORC_AVX512_INSN_vpslld_r_rm_i,
  ORC_AVX512_INSN_SSE_vpsrlq_r_rm_i,
  ORC_AVX512_INSN_AVX_vpsrlq_r_rm_i,
  ORC_AVX512_INSN_vpsrlq_r_rm_i,
  ORC_AVX512_INSN_SSE_vpsrldq_r_rm_i,
  ORC_AVX512_INSN_AVX_vpsrldq_r_rm_i,
  ORC_AVX512_INSN_vpsrldq_r_rm_i,
  ORC_AVX512_INSN_SSE_vpsllq_r_rm_i,
  ORC_AVX512_INSN_AVX_vpsllq_r_rm_i,
  ORC_AVX512_INSN_vpsllq_r_rm_i,
  ORC_AVX512_INSN_SSE_vpslldq_r_rm_i,
  ORC_AVX512_INSN_AVX_vpslldq_r_rm_i,
  ORC_AVX512_INSN_vpslldq_r_rm_i,
  ORC_AVX512_INSN_SSE_vpcmpeqb_k_r_rm,
  ORC_AVX512_INSN_AVX_vpcmpeqb_k_r_rm,
  ORC_AVX512_INSN_vpcmpeqb_k_r_rm,
  ORC_AVX512_INSN_SSE_vpcmpeqw_k_r_rm,
  ORC_AVX512_INSN_AVX_vpcmpeqw_k_r_rm,
  ORC_AVX512_INSN_vpcmpeqw_k_r_rm,
  /* next block */
  ORC_AVX512_INSN_SSE_vpbroadcastb,
  ORC_AVX512_INSN_AVX_vpbroadcastb,
  ORC_AVX512_INSN_vpbroadcastb,
  ORC_AVX512_INSN_SSE_vpbroadcastw,
  ORC_AVX512_INSN_AVX_vpbroadcastw,
  ORC_AVX512_INSN_vpbroadcastw,
  ORC_AVX512_INSN_SSE_X86_vpbroadcastb,
  ORC_AVX512_INSN_AVX_X86_vpbroadcastb,
  ORC_AVX512_INSN_X86_vpbroadcastb,
  ORC_AVX512_INSN_SSE_X86_vpbroadcastw,
  ORC_AVX512_INSN_AVX_X86_vpbroadcastw,
  ORC_AVX512_INSN_X86_vpbroadcastw,
  ORC_AVX512_INSN_SSE_X86_vpbroadcastd,
  ORC_AVX512_INSN_AVX_X86_vpbroadcastd,
  ORC_AVX512_INSN_X86_vpbroadcastd,
  ORC_AVX512_INSN_SSE_X86_vpbroadcastq,
  ORC_AVX512_INSN_AVX_X86_vpbroadcastq,
  ORC_AVX512_INSN_X86_vpbroadcastq,
  ORC_AVX512_INSN_SSE_vpermt2b,
  ORC_AVX512_INSN_AVX_vpermt2b,
  ORC_AVX512_INSN_vpermt2b,
  ORC_AVX512_INSN_SSE_vpermt2w,
  ORC_AVX512_INSN_AVX_vpermt2w,
  ORC_AVX512_INSN_vpermt2w,
  ORC_AVX512_INSN_vmovd_rm_r,
  ORC_AVX512_INSN_SSE_vpermt2d,
  ORC_AVX512_INSN_AVX_vpermt2d,
  ORC_AVX512_INSN_vpermt2d,
  ORC_AVX512_INSN_SSE_vpermt2q,
  ORC_AVX512_INSN_AVX_vpermt2q,
  ORC_AVX512_INSN_vpermt2q,
  ORC_AVX512_INSN_vmovq_sse_ssem,
  ORC_AVX512_INSN_SSE_vmovdqa32_rm_r,
  ORC_AVX512_INSN_AVX_vmovdqa32_rm_r,
  ORC_AVX512_INSN_vmovdqa32_rm_r,
  ORC_AVX512_INSN_SSE_vmovdqa64_rm_r,
  ORC_AVX512_INSN_AVX_vmovdqa64_rm_r,
  ORC_AVX512_INSN_vmovdqa64_rm_r,
  ORC_AVX512_INSN_SSE_vmovdqu8_rm_r,
  ORC_AVX512_INSN_AVX_vmovdqu8_rm_r,
  ORC_AVX512_INSN_vmovdqu8_rm_r,
  ORC_AVX512_INSN_SSE_vmovdqu16_rm_r,
  ORC_AVX512_INSN_AVX_vmovdqu16_rm_r,
  ORC_AVX512_INSN_vmovdqu16_rm_r,
  ORC_AVX512_INSN_SSE_vmovdqu32_rm_r,
  ORC_AVX512_INSN_AVX_vmovdqu32_rm_r,
  ORC_AVX512_INSN_vmovdqu32_rm_r,
  ORC_AVX512_INSN_SSE_vmovdqu64_rm_r,
  ORC_AVX512_INSN_AVX_vmovdqu64_rm_r,
  ORC_AVX512_INSN_vmovdqu64_rm_r,
  ORC_AVX512_INSN_SSE_vpermb,
  ORC_AVX512_INSN_AVX_vpermb,
  ORC_AVX512_INSN_vpermb,
  ORC_AVX512_INSN_SSE_vpermw,
  ORC_AVX512_INSN_AVX_vpermw,
  ORC_AVX512_INSN_vpermw,
  ORC_AVX512_INSN_kmovw_k_km,
  ORC_AVX512_INSN_kmovq_k_km,
  ORC_AVX512_INSN_kmovb_k_km,
  ORC_AVX512_INSN_kmovd_k_km,
  ORC_AVX512_INSN_kmovw_m_k,
  ORC_AVX512_INSN_kmovq_m_k,
  ORC_AVX512_INSN_kmovb_m_k,
  ORC_AVX512_INSN_kmovd_m_k,
  ORC_AVX512_INSN_kmovw_k_r,
  ORC_AVX512_INSN_kmovq_k_r,
  ORC_AVX512_INSN_kmovb_k_r,
  ORC_AVX512_INSN_kmovd_k_r,
  ORC_AVX512_INSN_kmovw_r_k,
  ORC_AVX512_INSN_kmovq_r_k,
  ORC_AVX512_INSN_kmovb_r_k,
  ORC_AVX512_INSN_kmovd_r_k,
  ORC_AVX512_INSN_SSE_vcmpeqps,
  ORC_AVX512_INSN_AVX_vcmpeqps,
  ORC_AVX512_INSN_vcmpeqps,
  ORC_AVX512_INSN_SSE_vcmpltqps,
  ORC_AVX512_INSN_AVX_vcmpltps,
  ORC_AVX512_INSN_vcmpltps,
  ORC_AVX512_INSN_SSE_vcmpleqps,
  ORC_AVX512_INSN_AVX_vcmpleps,
  ORC_AVX512_INSN_vcmpleps,
  ORC_AVX512_INSN_SSE_vcmpeqpd,
  ORC_AVX512_INSN_AVX_vcmpeqpd,
  ORC_AVX512_INSN_vcmpeqpd,
  ORC_AVX512_INSN_SSE_vcmpltqpd,
  ORC_AVX512_INSN_AVX_vcmpltpd,
  ORC_AVX512_INSN_vcmpltpd,
  ORC_AVX512_INSN_SSE_vcmpleqpd,
  ORC_AVX512_INSN_AVX_vcmplepd,
  ORC_AVX512_INSN_vcmplepd,
  ORC_AVX512_INSN_vpinsrw,
  /* next block */
  ORC_AVX512_INSN_SSE_vpsrlw_r_r_rm,
  ORC_AVX512_INSN_AVX_vpsrlw_r_r_rm,
  ORC_AVX512_INSN_vpsrlw_r_r_rm,
  ORC_AVX512_INSN_SSE_vpsrld_r_r_rm,
  ORC_AVX512_INSN_AVX_vpsrld_r_r_rm,
  ORC_AVX512_INSN_vpsrld_r_r_rm,
  ORC_AVX512_INSN_SSE_vpsrlq_r_r_rm,
  ORC_AVX512_INSN_AVX_vpsrlq_r_r_rm,
  ORC_AVX512_INSN_vpsrlq_r_r_rm,
  ORC_AVX512_INSN_SSE_vpaddq,
  ORC_AVX512_INSN_AVX_vpaddq,
  ORC_AVX512_INSN_vpaddq,
  ORC_AVX512_INSN_SSE_vpmullw,
  ORC_AVX512_INSN_AVX_vpmullw,
  ORC_AVX512_INSN_vpmullw,
  ORC_AVX512_INSN_vmovq_ssem_sse,
  ORC_AVX512_INSN_SSE_vpsubusb,
  ORC_AVX512_INSN_AVX_vpsubusb,
  ORC_AVX512_INSN_vpsubusb,
  ORC_AVX512_INSN_SSE_vpsubusw,
  ORC_AVX512_INSN_AVX_vpsubusw,
  ORC_AVX512_INSN_vpsubusw,
  /* next block */
  ORC_AVX512_INSN_SSE_vpminub,
  ORC_AVX512_INSN_AVX_vpminub,
  ORC_AVX512_INSN_vpminub,
  ORC_AVX512_INSN_SSE_vpandd,
  ORC_AVX512_INSN_AVX_vpandd,
  ORC_AVX512_INSN_vpandd,
  ORC_AVX512_INSN_SSE_vpandq,
  ORC_AVX512_INSN_AVX_vpandq,
  ORC_AVX512_INSN_vpandq,
  ORC_AVX512_INSN_SSE_vpaddusb,
  ORC_AVX512_INSN_AVX_vpaddusb,
  ORC_AVX512_INSN_vpaddusb,
  ORC_AVX512_INSN_SSE_vpaddusw,
  ORC_AVX512_INSN_AVX_vpaddusw,
  ORC_AVX512_INSN_vpaddusw,
  ORC_AVX512_INSN_SSE_vpmaxub,
  ORC_AVX512_INSN_AVX_vpmaxub,
  ORC_AVX512_INSN_vpmaxub,
  ORC_AVX512_INSN_SSE_vpandnd,
  ORC_AVX512_INSN_AVX_vpandnd,
  ORC_AVX512_INSN_vpandnd,
  ORC_AVX512_INSN_SSE_vpandnq,
  ORC_AVX512_INSN_AVX_vpandnq,
  ORC_AVX512_INSN_vpandnq,
  ORC_AVX512_INSN_SSE_vpavgb,
  ORC_AVX512_INSN_AVX_vpavgb,
  ORC_AVX512_INSN_vpavgb,
  ORC_AVX512_INSN_SSE_vpsraw_r_r_rm,
  ORC_AVX512_INSN_AVX_vpsraw_r_r_rm,
  ORC_AVX512_INSN_vpsraw_r_r_rm,
  ORC_AVX512_INSN_SSE_vpsrad_r_r_rm,
  ORC_AVX512_INSN_AVX_vpsrad_r_r_rm,
  ORC_AVX512_INSN_vpsrad_r_r_rm,
  ORC_AVX512_INSN_SSE_vpsraq_r_r_rm,
  ORC_AVX512_INSN_AVX_vpsraq_r_r_rm,
  ORC_AVX512_INSN_vpsraq_r_r_rm,
  ORC_AVX512_INSN_SSE_vpavgw,
  ORC_AVX512_INSN_AVX_vpavgw,
  ORC_AVX512_INSN_vpavgw,
  ORC_AVX512_INSN_SSE_vpmulhuw,
  ORC_AVX512_INSN_AVX_vpmulhuw,
  ORC_AVX512_INSN_vpmulhuw,
  ORC_AVX512_INSN_SSE_vpmulhw,
  ORC_AVX512_INSN_AVX_vpmulhw,
  ORC_AVX512_INSN_vpmulhw,
  ORC_AVX512_INSN_SSE_vcvtdq2pd,
  ORC_AVX512_INSN_AVX_vcvtdq2pd,
  ORC_AVX512_INSN_vcvtdq2pd,
  ORC_AVX512_INSN_SSE_vcvtpd2dq,
  ORC_AVX512_INSN_AVX_vcvtpd2dq,
  ORC_AVX512_INSN_vcvtpd2dq,
  ORC_AVX512_INSN_SSE_vcvttpd2dq,
  ORC_AVX512_INSN_AVX_vcvttpd2dq,
  ORC_AVX512_INSN_vcvttpd2dq,
  /* next block */
  ORC_AVX512_INSN_SSE_vmovntdq,
  ORC_AVX512_INSN_AVX_vmovntdq,
  ORC_AVX512_INSN_vmovntdq,
  /* next block */
  ORC_AVX512_INSN_SSE_vpsubsb,
  ORC_AVX512_INSN_AVX_vpsubsb,
  ORC_AVX512_INSN_vpsubsb,
  ORC_AVX512_INSN_SSE_vpsubsw,
  ORC_AVX512_INSN_AVX_vpsubsw,
  ORC_AVX512_INSN_vpsubsw,

  ORC_AVX512_INSN_SSE_vpminsw,
  ORC_AVX512_INSN_AVX_vpminsw,
  ORC_AVX512_INSN_vpminsw,
  ORC_AVX512_INSN_SSE_vpord,
  ORC_AVX512_INSN_AVX_vpord,
  ORC_AVX512_INSN_vpord,
  ORC_AVX512_INSN_SSE_vporq,
  ORC_AVX512_INSN_AVX_vporq,
  ORC_AVX512_INSN_vporq,

  ORC_AVX512_INSN_SSE_vpaddsb,
  ORC_AVX512_INSN_AVX_vpaddsb,
  ORC_AVX512_INSN_vpaddsb,
  ORC_AVX512_INSN_SSE_vpaddsw,
  ORC_AVX512_INSN_AVX_vpaddsw,
  ORC_AVX512_INSN_vpaddsw,
  ORC_AVX512_INSN_SSE_vpmaxsw,
  ORC_AVX512_INSN_AVX_vpmaxsw,
  ORC_AVX512_INSN_vpmaxsw,
  ORC_AVX512_INSN_SSE_vpxord,
  ORC_AVX512_INSN_AVX_vpxord,
  ORC_AVX512_INSN_vpxord,
  ORC_AVX512_INSN_SSE_vpxorq,
  ORC_AVX512_INSN_AVX_vpxorq,
  ORC_AVX512_INSN_vpxorq,
  ORC_AVX512_INSN_SSE_vpsllw_r_r_rm,
  ORC_AVX512_INSN_AVX_vpsllw_r_r_rm,
  ORC_AVX512_INSN_vpsllw_r_r_rm,
  ORC_AVX512_INSN_SSE_vpslld_r_r_rm,
  ORC_AVX512_INSN_AVX_vpslld_r_r_rm,
  ORC_AVX512_INSN_vpslld_r_r_rm,
  ORC_AVX512_INSN_SSE_vpsllq_r_r_rm,
  ORC_AVX512_INSN_AVX_vpsllq_r_r_rm,
  ORC_AVX512_INSN_vpsllq_r_r_rm,
  ORC_AVX512_INSN_SSE_vpmuludq,
  ORC_AVX512_INSN_AVX_vpmuludq,
  ORC_AVX512_INSN_vpmuludq,
  ORC_AVX512_INSN_SSE_vpsadbw,
  ORC_AVX512_INSN_AVX_vpsadbw,
  ORC_AVX512_INSN_vpsadbw,
  /* next block */
  ORC_AVX512_INSN_SSE_vpsubb,
  ORC_AVX512_INSN_AVX_vpsubb,
  ORC_AVX512_INSN_vpsubb,
  ORC_AVX512_INSN_SSE_vpsubw,
  ORC_AVX512_INSN_AVX_vpsubw,
  ORC_AVX512_INSN_vpsubw,
  ORC_AVX512_INSN_SSE_vpsubd,
  ORC_AVX512_INSN_AVX_vpsubd,
  ORC_AVX512_INSN_vpsubd,
  ORC_AVX512_INSN_SSE_vpsubq,
  ORC_AVX512_INSN_AVX_vpsubq,
  ORC_AVX512_INSN_vpsubq,
  ORC_AVX512_INSN_SSE_vpaddb,
  ORC_AVX512_INSN_AVX_vpaddb,
  ORC_AVX512_INSN_vpaddb,
  ORC_AVX512_INSN_SSE_vpaddw,
  ORC_AVX512_INSN_AVX_vpaddw,
  ORC_AVX512_INSN_vpaddw,
  ORC_AVX512_INSN_SSE_vpaddd,
  ORC_AVX512_INSN_AVX_vpaddd,
  ORC_AVX512_INSN_vpaddd,
} OrcAVX512Insn;

#define orc_avx512_insn_emit_vpshufb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpshufb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpermq_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_vpermq_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpermq_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_AVX_vpermq_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpabsb(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpabsb, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpabsb(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpabsb, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_sse_emit_vpabsb(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpabsb, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_vpabsw(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpabsw, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpabsw(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpabsw, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_sse_emit_vpabsw(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpabsw, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_vpabsd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpabsd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpabsd(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpabsd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_sse_emit_vpabsd(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpabsd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_vpcmpeqd(c, s0, s1, d, m) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpcmpeqd, s0, s1, ORC_REG_INVALID, d, m, FALSE)

#define orc_avx512_insn_avx_emit_vpcmpeqd(c, s0, s1, d, m) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpcmpeqd, s0, s1, ORC_REG_INVALID, d, m, FALSE)

#define orc_avx512_insn_sse_emit_vpcmpeqd(c, s0, s1, d, m) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpcmpeqd, s0, s1, ORC_REG_INVALID, d, m, FALSE)

#define orc_avx512_insn_emit_vpcmpeqq(c, s0, s1, d, m) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpcmpeqq, s0, s1, ORC_REG_INVALID, d, m, FALSE)

#define orc_avx512_insn_avx_emit_vpcmpeqq(c, s0, s1, d, m) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpcmpeqq, s0, s1, ORC_REG_INVALID, d, m, FALSE)

#define orc_avx512_insn_sse_emit_vpcmpeqq(c, s0, s1, d, m) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpcmpeqq, s0, s1, ORC_REG_INVALID, d, m, FALSE)

#define orc_avx512_insn_emit_vpinsrb_r_r_m_i(c, i, o, s0, s1, d) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_vpinsrb, i, o, s0, s1, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_vpmovsxbw(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovsxbw, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpmovsxbw(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmovsxbw, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpmovsxbw(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmovsxbw, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpmovsxbd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovsxbd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpmovsxbq(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovsxbq, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpinsrd_r_r_m_i(c, i, o, s0, s1, d) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_vpinsrd, i, o, s0, s1, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_vpmovsxwd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovsxwd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpmovsxwd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmovsxwd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpmovsxwd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmovsxwd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpmovsxdq(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovsxdq, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpmovsxdq(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmovsxdq, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpmovsxdq(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmovsxdq, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpternlogd(c, i, s0, s1, d, m, z) \
  orc_avx512_insn_emit_imm (c, ORC_AVX512_INSN_vpternlogd, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, s1, d, m, z)

#define orc_avx512_insn_emit_vpternlogq(c, i, s0, s1, d, m, z) \
  orc_avx512_insn_emit_imm (c, ORC_AVX512_INSN_vpternlogq, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, s1, d, m, z)

#define orc_avx512_insn_emit_vpmuldq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmuldq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpmovm2b(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovm2b, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_vpmovm2w(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovm2w, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_vpmovb2m(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovb2m, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_vpmovw2m(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovw2m, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_vpmovzxbw(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovzxbw, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpmovzxbw(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmovzxbw, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpmovzxbw(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmovzxbw, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_kshiftrw(c, i, s0, d) \
  orc_avx512_insn_emit_imm (c, ORC_AVX512_INSN_kshiftrw, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_kshiftrb(c, i, s0, d) \
  orc_avx512_insn_emit_imm (c, ORC_AVX512_INSN_kshiftrb, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_vpmovzxbd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovzxbd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_kshiftrq(c, i, s0, d) \
  orc_avx512_insn_emit_imm (c, ORC_AVX512_INSN_kshiftrq, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_kshiftrd(c, i, s0, d) \
  orc_avx512_insn_emit_imm (c, ORC_AVX512_INSN_kshiftrd, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)


#define orc_avx512_insn_emit_vpmovzxbq(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovzxbq, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpmovzxwd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovzxwd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vpmovzxwd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmovzxwd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_sse_emit_vpmovzxwd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmovzxwd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpmovzxdq(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovzxdq, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vpmovzxdq(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmovzxdq, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_sse_emit_vpmovzxdq(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmovzxdq, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpmovqd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovqd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vpmovqd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmovqd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_sse_emit_vpmovqd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmovqd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpermq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpermq, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpermd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpermd, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpminsb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpminsb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpminsb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpminsb, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpminsb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpminsb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vinserti32x4(c, i, s0, s1, d, m, z) \
  orc_avx512_insn_emit_imm (c, ORC_AVX512_INSN_AVX_vinserti32x4, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, s1, d, m, z)

#define orc_avx512_insn_emit_vinserti32x4(c, i, s0, s1, d, m, z) \
  orc_avx512_insn_emit_imm (c, ORC_AVX512_INSN_vinserti32x4, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, s1, d, m, z)

#define orc_avx512_insn_emit_vpmovm2d(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovm2d, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_avx_emit_vpmovm2d(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmovm2d, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_sse_emit_vpmovm2d(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmovm2d, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_vpmovm2q(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovm2q, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_avx_emit_vpmovm2q(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmovm2q, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_sse_emit_vpmovm2q(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmovm2q, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_vpminsd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpminsd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpminsd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpminsd, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpminsd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpminsd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vextracti32x4(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm (c, ORC_AVX512_INSN_AVX_vextracti32x4, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vextracti32x4(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm (c, ORC_AVX512_INSN_vextracti32x4, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vextracti64x2(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm (c, ORC_AVX512_INSN_vextracti64x2, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpmovd2m(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmovd2m, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_vpminuw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpminuw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpminuw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpminuw, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpminuw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpminuw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vinserti32x8(c, i, s0, s1, d, m, z) \
  orc_avx512_insn_emit_imm (c, ORC_AVX512_INSN_vinserti32x8, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, s1, d, m, z)

#define orc_avx512_insn_emit_vinserti64x4(c, i, s0, s1, d, m, z) \
  orc_avx512_insn_emit_imm (c, ORC_AVX512_INSN_vinserti64x4, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, s1, d, m, z)

#define orc_avx512_insn_emit_vpminud(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpminud, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpminud(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpminud, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpminud(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpminud, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vextracti32x8(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm (c, ORC_AVX512_INSN_vextracti32x8, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vextracti64x4(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm (c, ORC_AVX512_INSN_vextracti64x4, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpmaxsb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmaxsb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpmaxsb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmaxsb, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpmaxsb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmaxsb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpmaxsd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmaxsd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpmaxsd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmaxsd, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpmaxsd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmaxsd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpmaxuw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmaxuw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpmaxuw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmaxuw, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpmaxuw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmaxuw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpmaxud(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmaxud, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpmaxud(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmaxud, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpmaxud(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmaxud, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpcmpeqb(c, s0, s1, d, m) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpcmpeqb, s0, s1, ORC_REG_INVALID, d, m, FALSE)

#define orc_avx512_insn_emit_vpcmpltb(c, s0, s1, d, m) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpcmpltb, s0, s1, ORC_REG_INVALID, d, m, FALSE)

#define orc_avx512_insn_emit_vpcmpleb(c, s0, s1, d, m) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpcmpleb, s0, s1, ORC_REG_INVALID, d, m, FALSE)

#define orc_avx512_insn_emit_vpcmpneqb(c, s0, s1, d, m) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpcmpneqb, s0, s1, ORC_REG_INVALID, d, m, FALSE)

#define orc_avx512_insn_emit_vpcmpnltb(c, s0, s1, d, m) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpcmpnltb, s0, s1, ORC_REG_INVALID, d, m, FALSE)

#define orc_avx512_insn_emit_vpcmpnleb(c, s0, s1, d, m) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpcmpnleb, s0, s1, ORC_REG_INVALID, d, m, FALSE)

#define orc_avx512_insn_emit_vpmulld(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmulld, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpmulld(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmulld, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpmulld(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmulld, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_kxnorw(c, s0, s1, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_kxnorw, s0, s1, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_kxnorb(c, s0, s1, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_kxnorb, s0, s1, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_kxnorq(c, s0, s1, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_kxnorq, s0, s1, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_kxnord(c, s0, s1, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_kxnord, s0, s1, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_vsqrtps(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vsqrtps, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vsqrtpd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vsqrtpd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vsqrtpd(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vsqrtpd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_sse_emit_vsqrtpd(c, s0, d) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vsqrtpd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_emit_vandps(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vandps, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vorps(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vorps, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vorps(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vorps, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vorps(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vorps, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vorpd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vorpd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vaddps(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vaddps, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vaddpd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vaddpd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vaddpd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vaddpd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vaddpd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vaddpd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpbroadcastd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpbroadcastd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vmulps(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vmulps, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vmulpd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vmulpd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vmulpd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vmulpd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vmulpd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vmulpd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpbroadcastq(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpbroadcastq, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vcvtps2pd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vcvtps2pd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vcvtps2pd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vcvtps2pd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vcvtps2pd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vcvtps2pd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vcvtpd2ps(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vcvtpd2ps, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vcvtdq2ps(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vcvtdq2ps, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vcvtps2dq(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vcvtps2dq, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vcvttps2dq(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vcvttps2dq, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vsubps(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vsubps, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vsubpd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vsubpd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vsubpd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vsubpd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vsubpd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vsubpd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vminps(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vminps, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vminpd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vminpd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vdivps(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vdivps, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vdivpd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vdivpd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vmaxps(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vmaxps, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vmaxpd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vmaxpd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpunpcklbw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpunpcklbw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpunpcklbw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpunpcklbw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpunpcklbw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpunpcklbw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpunpcklwd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpunpcklwd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpunpcklwd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpunpcklwd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpunpcklwd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpunpcklwd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpunpckldq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpunpckldq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpunpckldq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpunpckldq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpunpckldq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpunpckldq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpunpckhdq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpunpckhdq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpunpckhdq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpunpckhdq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpunpckhdq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpunpckhdq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpacksswb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpacksswb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpacksswb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpacksswb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpacksswb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpacksswb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpcmpgtb(c, s0, s1, d, m) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpcmpgtb, s0, s1, ORC_REG_INVALID, d, m, FALSE)

#define orc_avx512_insn_emit_vpblendmd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpblendmd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpblendmq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpblendmq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpcmpgtw(c, s0, s1, d, m) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpcmpgtw, s0, s1, ORC_REG_INVALID, d, m, FALSE)

#define orc_avx512_insn_emit_vpblendmb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpblendmb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpblendmw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpblendmw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpcmpgtd(c, s0, s1, d, m) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpcmpgtd, s0, s1, ORC_REG_INVALID, d, m, FALSE)

#define orc_avx512_insn_emit_vpcmpgtq(c, s0, s1, d, m) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpcmpgtq, s0, s1, ORC_REG_INVALID, d, m, FALSE)

#define orc_avx512_insn_emit_vpackuswb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpackuswb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpackuswb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpackuswb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpackuswb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpackuswb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpackssdw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpackssdw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpackssdw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpackssdw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpackssdw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpackssdw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpackusdw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpackusdw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpunpckhbw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpunpckhbw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpunpckhbw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpunpckhbw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpunpckhbw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpunpckhbw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpunpckhwd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpunpckhwd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpunpckhwd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpunpckhwd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpunpckhwd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpunpckhwd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vmovd_r_m(c, o, s0, d) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_vmovd_r_rm, 0, o, s0, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE);

#define orc_avx512_insn_emit_vmovq_sse_r(c, s0, d) \
  orc_avx512_insn_emit_size (c, ORC_AVX512_INSN_vmovq_sse_rm, 8, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE)

#define orc_avx512_insn_sse_emit_vmovdqa32_r_m(c, o, s0, d, m, z) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_SSE_vmovdqa32_r_rm, 0, o, s0, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vmovdqa32_r_m(c, o, s0, d, m, z) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_AVX_vmovdqa32_r_rm, 0, o, s0, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vmovdqa32_r_m(c, o, s0, d, m, z) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_vmovdqa32_r_rm, 0, o, s0, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vmovdqa32_r_r(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vmovdqa32_r_rm, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_sse_emit_vmovdqu32_r_m(c, o, s0, d, m, z) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_SSE_vmovdqu32_r_rm, 0, o, s0, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vmovdqu32_r_m(c, o, s0, d, m, z) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_AVX_vmovdqu32_r_rm, 0, o, s0, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vmovdqu32_r_m(c, o, s0, d, m, z) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_vmovdqu32_r_rm, 0, o, s0, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpextrb_m_r_i(c, i, o, s0, d) \
  orc_avx512_insn_emit_store_memoffset (c, ORC_AVX512_INSN_vpextrb, i, o, s0, d, ORC_REG_INVALID, FALSE);

#define orc_avx512_insn_emit_vpextrw_m_r_i(c, i, o, s0, d) \
  orc_avx512_insn_emit_store_memoffset (c, ORC_AVX512_INSN_vpextrw, i, o, s0, d, ORC_REG_INVALID, FALSE);

#define orc_avx512_insn_sse_emit_vpshufd(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_SSE_vpshufd, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpshufd(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_AVX_vpshufd, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpshufd(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_vpshufd, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpshuflw(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_SSE_vpshuflw, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpshuflw(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_AVX_vpshuflw, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpshuflw(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_vpshuflw, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpshufhw(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_vpshufhw, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsrlw_r_m_i(c, i, o, s0, d, m, z) \
  orc_avx512_insn_emit_load_memoffset(c, ORC_AVX512_INSN_vpsrlw_r_rm_i, i, o, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsrlw_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_vpsrlw_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsraw_r_m_i(c, i, o, s0, d, m, z) \
  orc_avx512_insn_emit_load_memoffset(c, ORC_AVX512_INSN_vpsraw_r_rm_i, i, o, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsraw_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_vpsraw_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpsraw_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_AVX_vpsraw_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpsraw_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_SSE_vpsraw_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsllw_r_m_i(c, i, o, s0, d, m, z) \
  orc_avx512_insn_emit_load_memoffset(c, ORC_AVX512_INSN_vpsllw_r_rm_i, i, o, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsllw_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_vpsllw_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsrld_r_m_i(c, i, o, s0, d, m, z) \
  orc_avx512_insn_emit_load_memoffset(c, ORC_AVX512_INSN_vpsrld_r_rm_i, i, o, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsrld_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_vpsrld_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpsrld_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_AVX_vpsrld_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpsrld_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_SSE_vpsrld_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsrad_r_m_i(c, i, o, s0, d, m, z) \
  orc_avx512_insn_emit_load_memoffset(c, ORC_AVX512_INSN_vpsrad_r_rm_i, i, o, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsrad_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_vpsrad_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpsrad_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_AVX_vpsrad_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpsrad_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_SSE_vpsrad_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsraq_r_m_i(c, i, o, s0, d, m, z) \
  orc_avx512_insn_emit_load_memoffset(c, ORC_AVX512_INSN_vpsraq_r_rm_i, i, o, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsraq_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_vpsraq_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpsraq_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_AVX_vpsraq_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpsraq_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_SSE_vpsraq_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpslld_r_m_i(c, i, o, s0, d, m, z) \
  orc_avx512_insn_emit_load_memoffset(c, ORC_AVX512_INSN_vpslld_r_rm_i, i, o, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpslld_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_vpslld_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpslld_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_AVX_vpslld_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpslld_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_SSE_vpslld_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsrlq_r_m_i(c, i, o, s0, d, m, z) \
  orc_avx512_insn_emit_load_memoffset(c, ORC_AVX512_INSN_vpsrlq_r_rm_i, i, o, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsrlq_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_vpsrlq_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsllq_r_m_i(c, i, o, s0, d, m, z) \
  orc_avx512_insn_emit_load_memoffset(c, ORC_AVX512_INSN_vpsllq_r_rm_i, i, o, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsllq_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_vpsllq_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpslldq_r_m_i(c, i, o, s0, d, m, z) \
  orc_avx512_insn_emit_load_memoffset(c, ORC_AVX512_INSN_vpslldq_r_rm_i, i, o, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpslldq_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_vpslldq_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpslldq_r_r_i(c, i, s0, d, m, z) \
  orc_avx512_insn_emit_imm(c, ORC_AVX512_INSN_SSE_vpslldq_r_rm_i, ORC_X86_INSN_OPERAND_SIZE_NONE, i, s0, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpbroadcastb(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpbroadcastb, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpbroadcastw(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpbroadcastw, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_x86_vpbroadcastb(c, s0, d, m, z) \
  orc_avx512_insn_emit_size (c, ORC_AVX512_INSN_X86_vpbroadcastb, 4, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_x86_vpbroadcastw(c, s0, d, m, z) \
  orc_avx512_insn_emit_size (c, ORC_AVX512_INSN_X86_vpbroadcastw, 4, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_x86_vpbroadcastd(c, s0, d, m, z) \
  orc_avx512_insn_emit_size (c, ORC_AVX512_INSN_X86_vpbroadcastd, 4, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_x86_vpbroadcastq(c, s0, d, m, z) \
  orc_avx512_insn_emit_size (c, ORC_AVX512_INSN_X86_vpbroadcastq, 8, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpermt2b(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpermt2b, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpermt2w(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpermt2w, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vmovd_m_r(c, o, s0, d) \
  orc_avx512_insn_emit_store_memoffset (c, ORC_AVX512_INSN_vmovd_rm_r, 0, o, s0, d, ORC_REG_INVALID, FALSE);

#define orc_avx512_insn_emit_vpermt2d(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpermt2d, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_sse_emit_vpermt2q(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpermt2q, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vpermt2q(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpermt2q, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpermt2q(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpermt2q, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vmovq_sse_m(c, o, s0, d) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_vmovq_sse_ssem, 0, o, s0, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE);

#define orc_avx512_insn_sse_emit_vmovdqa32_m_r(c, o, s0, d, m, z) \
  orc_avx512_insn_emit_store_memoffset (c, ORC_AVX512_INSN_SSE_vmovdqa32_rm_r, 0, o, s0, d, m, z);

#define orc_avx512_insn_avx_emit_vmovdqa32_m_r(c, o, s0, d, m, z) \
  orc_avx512_insn_emit_store_memoffset (c, ORC_AVX512_INSN_AVX_vmovdqa32_rm_r, 0, o, s0, d, m, z);

#define orc_avx512_insn_emit_vmovdqa32_m_r(c, o, s0, d, m, z) \
  orc_avx512_insn_emit_store_memoffset (c, ORC_AVX512_INSN_vmovdqa32_rm_r, 0, o, s0, d, m, z);

#define orc_avx512_insn_sse_emit_vmovdqu32_m_r(c, o, s0, d, m, z) \
  orc_avx512_insn_emit_store_memoffset (c, ORC_AVX512_INSN_SSE_vmovdqu32_rm_r, 0, o, s0, d, m, z);

#define orc_avx512_insn_avx_emit_vmovdqu32_m_r(c, o, s0, d, m, z) \
  orc_avx512_insn_emit_store_memoffset (c, ORC_AVX512_INSN_AVX_vmovdqu32_rm_r, 0, o, s0, d, m, z);

#define orc_avx512_insn_emit_vmovdqu32_m_r(c, o, s0, d, m, z) \
  orc_avx512_insn_emit_store_memoffset (c, ORC_AVX512_INSN_vmovdqu32_rm_r, 0, o, s0, d, m, z);

#define orc_avx512_insn_emit_vpermb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpermb, s0, s1, ORC_REG_INVALID, d, m, z);
#define orc_avx512_insn_avx_emit_vpermb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpermb, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpermb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpermb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpermw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpermw, s0, s1, ORC_REG_INVALID, d, m, z);
#define orc_avx512_insn_avx_emit_vpermw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpermw, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpermw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpermw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vcmpeqps(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vcmpeqps, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vcmpltps(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vcmpltps, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vcmpleps(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vcmpleps, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vcmpeqpd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vcmpeqpd, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vcmpltpd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vcmpltpd, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vcmplepd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vcmplepd, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpinsrw_r_r_m_i(c, i, o, s0, s1, d) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_vpinsrw, i, o, s0, s1, d, ORC_REG_INVALID, FALSE);

#define orc_avx512_insn_emit_vpsrlw_r_r_m(c, o, s0, s1, d, m, z) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_vpsrlw_r_r_rm, 0, o, s0, s1, d, m, z);

#define orc_avx512_insn_emit_vpsrlw_r_r_r(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsrlw_r_r_rm, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpsrld_r_r_m(c, o, s0, s1, d, m, z) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_vpsrld_r_r_rm, 0, o, s0, s1, d, m, z);

#define orc_avx512_insn_emit_vpsrld_r_r_r(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsrld_r_r_rm, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpsrlq_r_r_m(c, o, s0, s1, d, m, z) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_vpsrlq_r_r_rm, 0, o, s0, s1, d, m, z);

#define orc_avx512_insn_emit_vpsrlq_r_r_r(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsrlq_r_r_rm, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpaddq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpaddq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpaddq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpaddq, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpaddq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpaddq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpmullw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmullw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpmullw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmullw, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpmullw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmullw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vmovq_m_r(c, o, s0, d) \
  orc_avx512_insn_emit_store_memoffset (c, ORC_AVX512_INSN_vmovq_ssem_sse, 0, o, s0, d, ORC_REG_INVALID, FALSE);

#define orc_avx512_insn_emit_vpsubusb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsubusb, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vpsubusb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpsubusb, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpsubusb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpsubusb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsubusw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsubusw, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vpsubusw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpsubusw, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpsubusw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpsubusw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpminub(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpminub, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vpminub(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpminub, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpminub(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpminub, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpandd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpandd, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vpandd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpandd, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpandd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpandd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpaddusb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpaddusb, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vpaddusb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpaddusb, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpaddusb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpaddusb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpaddusw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpaddusw, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vpaddusw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpaddusw, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpaddusw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpaddusw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpmaxub(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmaxub, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vpmaxub(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmaxub, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpmaxub(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmaxub, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpandnd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpandnd, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vpandnd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpandnd, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpandnd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpandnd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpandnq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpandnq, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vpandnq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpandnq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpandnq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpandnq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpavgb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpavgb, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vpavgb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpavgb, s0, s1, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE);

#define orc_avx512_insn_sse_emit_vpavgb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpavgb, s0, s1, ORC_REG_INVALID, d, ORC_REG_INVALID, FALSE);

#define orc_avx512_insn_emit_vpsraw_r_r_m(c, o, s0, s1, d, m, z) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_vpsraw_r_r_rm, 0, o, s0, s1, d, m, z);

#define orc_avx512_insn_sse_emit_vpsraw_r_r_r(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpsraw_r_r_rm, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vpsraw_r_r_r(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpsraw_r_r_rm, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpsraw_r_r_r(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsraw_r_r_rm, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpsrad_r_r_m(c, o, s0, s1, d, m, z) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_vpsrad_r_r_rm, 0, o, s0, s1, d, m, z);

#define orc_avx512_insn_emit_vpsrad_r_r_r(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsrad_r_r_rm, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpsraq_r_r_m(c, o, s0, s1, d, m, z) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_vpsraq_r_r_rm, 0, o, s0, s1, d, m, z);

#define orc_avx512_insn_emit_vpsraq_r_r_r(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsraq_r_r_rm, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpavgw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpavgw, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_avx_emit_vpavgw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpavgw, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpavgw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpavgw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpmulhuw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmulhuw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpmulhuw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmulhuw, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpmulhuw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmulhuw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpmulhw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmulhw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpmulhw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmulhw, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpmulhw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmulhw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vcvtdq2pd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vcvtdq2pd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vcvtdq2pd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vcvtdq2pd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vcvtdq2pd(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vcvtdq2pd, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vcvtpd2dq(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vcvtpd2dq, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vcvttpd2dq(c, s0, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vcvttpd2dq, s0, ORC_REG_INVALID, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vmovntdq(c, o, s0, d) \
  orc_avx512_insn_emit_store_memoffset (c, ORC_AVX512_INSN_SSE_vmovntdq, 0, o, s0, d, ORC_REG_INVALID, FALSE);

#define orc_avx512_insn_avx_emit_vmovntdq(c, o, s0, d) \
  orc_avx512_insn_emit_store_memoffset (c, ORC_AVX512_INSN_AVX_vmovntdq, 0, o, s0, d, ORC_REG_INVALID, FALSE);

#define orc_avx512_insn_emit_vmovntdq(c, o, s0, d) \
  orc_avx512_insn_emit_store_memoffset (c, ORC_AVX512_INSN_vmovntdq, 0, o, s0, d, ORC_REG_INVALID, FALSE);

#define orc_avx512_insn_emit_vpxord_sse(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpxord, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpxord_avx(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpxord, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsubsb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsubsb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpsubsb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpsubsb, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpsubsb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpsubsb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsubsw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsubsw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpsubsw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpsubsw, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpsubsw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpsubsw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpminsw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpminsw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpminsw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpminsw, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpminsw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpminsw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpord(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpord, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpord(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpord, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpord(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpord, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vporq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vporq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpaddsb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpaddsb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpaddsb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpaddsb, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpaddsb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpaddsb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpaddsw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpaddsw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpaddsw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpaddsw, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpaddsw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpaddsw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpmaxsw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmaxsw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpmaxsw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpmaxsw, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpmaxsw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpmaxsw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpxord(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpxord, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpxord(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpxord, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpxord(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpxord, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpxorq_sse(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpxorq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpxorq_avx(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpxorq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpxorq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpxorq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsllw_r_r_m(c, o, s0, s1, d, m, z) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_vpsllw_r_r_rm, 0, o, s0, s1, d, ORC_REG_INVALID, m, z);

#define orc_avx512_insn_emit_vpsllw_r_r_r(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsllw_r_r_rm, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpslld_r_r_m(c, o, s0, s1, d, m, z) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_vpslld_r_r_rm, 0, o, s0, s1, d, ORC_REG_INVALID, m, z);

#define orc_avx512_insn_emit_vpslld_r_r_r(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpslld_r_r_rm, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpsllq_r_r_m(c, o, s0, s1, d, m, z) \
  orc_avx512_insn_emit_load_memoffset (c, ORC_AVX512_INSN_vpsllq_r_r_rm, 0, o, s0, s1, d, ORC_REG_INVALID, m, z);

#define orc_avx512_insn_emit_vpsllq_r_r_r(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsllq, s0, s1, ORC_REG_INVALID, d, m, z);

#define orc_avx512_insn_emit_vpmuludq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpmuludq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsubb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsubb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpsubb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpsubb, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpsubb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpsubb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsubw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsubw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpsubw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpsubw, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpsubw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpsubw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsubd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsubd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpsubd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpsubd, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpsubd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpsubd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsubq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsubq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpsubq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpsubq, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpsubq(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpsubq, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpaddb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpaddb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpaddb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpaddb, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpaddb(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpaddb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpaddw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpaddw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpaddw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpaddw, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpaddw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpaddw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpaddd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpaddd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_avx_emit_vpaddd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_vpaddd, s0, s1, ORC_REG_INVALID, d, m, z)
#define orc_avx512_insn_sse_emit_vpaddd(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpaddd, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_emit_vpsadbw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_vpsadbw, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_sse_emit_vpsadbw(c, s0, s1, d, m, z) \
  orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_vpsadbw, s0, s1, ORC_REG_INVALID, d, m, z)

ORC_API void orc_avx512_insn_emit_mov_memoffset_reg (OrcCompiler *c, int size,
    int offset, int s0, int d, int is_aligned);

ORC_API void orc_avx512_insn_emit_mov_reg_memoffset (OrcCompiler *c, int size,
    int s0, int offset, int d, int aligned, int uncached);

ORC_API void orc_avx512_insn_emit_avx512 (OrcCompiler *c, OrcAVX512Insn i,
    int s0, int s1, int s2, int d, int m, orc_bool z);

ORC_API void orc_avx512_insn_emit_store_memoffset (OrcCompiler *c,
    OrcAVX512Insn i, int imm, int offset, int s0, int d, int m, orc_bool z);

ORC_API void orc_avx512_insn_emit_load_memoffset (OrcCompiler *c,
    OrcAVX512Insn i, int imm, int offset, int s0, int s1, int d, int m,
    orc_bool z);
ORC_API void orc_avx512_insn_emit_imm (OrcCompiler *c, OrcAVX512Insn i,
    int size, int imm, int src0, int src1, int dest, int m, orc_bool z);
ORC_API void orc_avx512_insn_emit_size (OrcCompiler *c, OrcAVX512Insn i,
    int size, int src0, int src1, int src2, int dest, int m, orc_bool z);

/* Generic adaptive dispatcher for AVX512_AVX512_AVX512 type instructions */
#define ORC_AVX512_INSN_EMIT_ADAPTIVE(c, size, opcode, s0, s1, s2, d, m, z) \
  do { \
    if ((size) >= 64) { \
      orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_##opcode, s0, s1, s2, d, m, z); \
    } else if ((size) >= 32) { \
      orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_AVX_##opcode, \
          ORC_AVX512_AVX_REG (s0), \
          ORC_AVX512_AVX_REG (s1), \
          ORC_AVX512_AVX_REG (s2), \
          ORC_AVX512_AVX_REG (d), m, z); \
    } else { \
      orc_avx512_insn_emit_avx512 (c, ORC_AVX512_INSN_SSE_##opcode, \
          ORC_AVX512_SSE_REG (s0), \
          ORC_AVX512_SSE_REG (s1), \
          ORC_AVX512_SSE_REG (s2), \
          ORC_AVX512_SSE_REG (d), m, z); \
    } \
  } while (0)

/* Adaptive emit macros - select instruction variant based on size */
#define orc_avx512_insn_adaptive_emit_vpermb(c, size, s0, s1, d, m, z) \
  ORC_AVX512_INSN_EMIT_ADAPTIVE(c, size, vpermb, s0, s1, ORC_REG_INVALID, d, m, z)

#define orc_avx512_insn_adaptive_emit_vpermw(c, size, s0, s1, d, m, z) \
  ORC_AVX512_INSN_EMIT_ADAPTIVE(c, size, vpermw, s0, s1, ORC_REG_INVALID, d, m, z)
#endif

ORC_END_DECLS

#endif
