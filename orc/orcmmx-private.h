#ifndef ORC_MMX_PRIVATE_H
#define ORC_MMX_PRIVATE_H

ORC_BEGIN_DECLS

/* orcmmxinsn.c */
typedef enum _OrcMMXInsnOperandFlag {
  /* Size embedded on the type of register */
  ORC_MMX_INSN_OPERAND_OP1_MM = (1 << 0),
  ORC_MMX_INSN_OPERAND_OP2_MM = (1 << 1),
} OrcMMXInsnOperandFlag;

#define ORC_MMX_INSN_OPERAND_FLAG_LAST 1

ORC_END_DECLS

#endif

