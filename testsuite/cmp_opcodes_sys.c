#include "config.h"

#include <stdio.h>

#define ORC_ENABLE_UNSTABLE_API

#include <orc/orc.h>
#include <orc-test/orctest.h>

#include "common.h"

int
main (int argc, char *argv[])
{
  OrcOpcodeSet *opcode_set;
  int i, n;

  orc_test_init();
  orc_init();

  n = orc_target_n_get ();
  opcode_set = orc_opcode_set_get ("sys");

  for (i = 0; i < opcode_set->n_opcodes; i++) {
    OrcStaticOpcode *opcode = &opcode_set->opcodes[i];
    OrcProgram *p;
    int j;
    double prev_perf, perf;

    printf ("opcode_%s:\n", opcode->name);
    /* TODO run the C version */
    p = orc_test_get_program_for_opcode (opcode);
    perf = prev_perf = 0;

    for (j = 0; j < n; j++) {
      OrcTarget *t = orc_target_get_by_idx (j);
  
      if (!orc_target_is_executable (t))
        continue;
      perf = orc_test_performance_full (p, 0, orc_target_get_name (t));
      print_gain (orc_target_get_name (t), prev_perf, perf);
      prev_perf = perf;
    }
    printf ("\n");  
    orc_program_free (p);
  }
}
