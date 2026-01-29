#pragma once

#include "types.h"

Node *make_node(NodeType type);
Node *make_ident_node(char *str);
Node *make_define_node(Node *id);
Node *make_reg_define_node(Node *id);
Node *make_array_node(Node *id, Node *num);
Node *make_array_ref_node(Node *id, Node *num);

Node *make_num_node(int val);

Node *make_cond_node(Node *left, CompOp op, Node *right);
Node *make_if_node(Node *cond, Node *t);
Node *make_if_else_node(Node *cond, Node *t, Node *f);

Node *make_loop_node(Node *cond, Node *loop);

Node *make_arith_node(Node *left, ArithOp op, Node *right);

Node *make_assign_node(Node *left, Node *right);

Node *make_concat_node(Node *f, Node *s, int extra);

Node *make_unary_node(NodeType type, Node *one);

Node *make_function_node(Node *name, Node *locals, Node *body, Node *params);

Node *make_call_node(Node *name, Node *args);

Node *make_return_node(Node *value);
