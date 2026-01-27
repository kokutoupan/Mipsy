#include "mips_code.h"

void print_head(FILE *out);
static void print_operand(FILE *fp, Operand *o);
void print_code(FILE *fp, Code *c);
void print_code_list(FILE *fp, Code *head);
