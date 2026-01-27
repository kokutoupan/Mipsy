#pragma once
#include "types.h"

const char *get_arith_op_str(int op);
const char *get_comp_op_str(int op);

void print_ast(Node *node, int depth);
void print_ast_json(Node *node, int depth);
