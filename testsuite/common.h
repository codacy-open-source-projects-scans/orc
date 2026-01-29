#ifndef _COMMON_H_
#define _COMMON_H_

char * read_file (const char *filename);
double gain (double prev, double curr);
void print_gain (const char *name, double prev, double curr);

#endif
