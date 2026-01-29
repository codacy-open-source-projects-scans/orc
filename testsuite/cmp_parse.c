#include "config.h"

#include <stdio.h>

#define ORC_ENABLE_UNSTABLE_API

#include <orc/orc.h>
#include <orc-test/orctest.h>
#include <orc/orcparse.h>

#include "common.h"

int
main (int argc, char *argv[])
{
  OrcProgram **programs;
  char *code;
  int i, n, s;
  const char *filename = NULL;

  orc_test_init();
  orc_init();

  if (argc >= 2) {
    filename = argv[1];
  }

  code = read_file (filename);
  if (!code) {
    printf("cmp_parse <file.orc>\n");
    exit(1);
  }

  s = orc_parse (code, &programs);
  n = orc_target_n_get ();

  for (i = 0; i < s; i++) {
    OrcProgram *p = programs[i];
    int j;
    double prev_perf, perf;

    printf ("program_%s\n", p->name);
    /* TODO run the C version first */
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
