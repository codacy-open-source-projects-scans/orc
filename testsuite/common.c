#include <stdio.h>
#include <stdlib.h>

char *
read_file (const char *filename)
{
  FILE *file = NULL;
  char *contents = NULL;
  long size;
  int ret;

  if (filename == NULL) {
    filename = getenv ("testfile");
  }
  if (filename == NULL) {
    filename = "test.orc";
  }

  file = fopen (filename, "rb");
  if (file == NULL) return NULL;

  ret = fseek (file, 0, SEEK_END);
  if (ret < 0) goto bail;

  size = ftell (file);
  if (size < 0) goto bail;

  ret = fseek (file, 0, SEEK_SET);
  if (ret < 0) goto bail;

  contents = malloc (size + 1);
  if (contents == NULL) goto bail;

  ret = fread (contents, size, 1, file);
  if (ret < 0) goto bail;

  contents[size] = 0;

  fclose (file);

  return contents;
bail:
  /* something failed */
  if (file) fclose (file);
  if (contents) free (contents);

  return NULL;
}

double
gain (double prev, double curr)
{
  double ret;

  if (!prev || !curr)
    return 0;

  if (curr < prev) {
    ret = ((prev - curr) / prev) * 100;
  } else {
    ret = ((curr - prev) / curr) * -100;
  }
  return ret;
}

void
print_gain (const char *name, double prev, double curr)
{
  if (prev && curr) {
    double g = gain (prev, curr);
    printf ("%-10s: %#0g (%+.2g%%)%s\n", name, curr,
      g, g < 0 ? " *" : "");
  } else {
    printf ("%-10s: %#0g (N/A)\n", name, curr);
  }
}


