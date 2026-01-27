#include "make_ast.h"
#include "types.h"
#include <stdlib.h>

/* basic_ls.y で最後に代入するためのグローバル変数 */
Node *top = NULL;

Node *make_node(NodeType type) {
  Node *n = malloc(sizeof(Node));
  n->id = type;
  n->node0 = NULL;
  return n;
}

Node *make_ident_node(char *str) {
  Node *n = malloc(sizeof(Node));
  n->id = ND_IDENT;
  n->str = str;
  n->node0 = NULL;

  return n;
}

Node *make_define_node(Node *id) {
  Node *n = malloc(sizeof(Node));

  n->id = ND_DEF;
  n->node0 = id;
  n->node1 = NULL;

  return n;
}

Node *make_array_node(Node *id, Node *num) {
  Node *n = malloc(sizeof(Node));

  n->id = ND_DEFARRAY;
  n->node0 = id;
  n->node1 = num;
  n->node2 = NULL;
  return n;
}

Node *make_array_ref_node(Node *id, Node *num) {
  Node *n = malloc(sizeof(Node));
  n->id = ND_REF;
  n->node0 = id;
  n->node1 = num;
  n->node2 = NULL;
  return n;
}

Node *make_val_id_node(int val) {
  Node *n = malloc(sizeof(Node));
  n->id = ND_VAR;
  n->extra = val;
  n->node0 = NULL;
  return n;
}

Node *make_cond_node(Node *left, CompOp op, Node *right) {
  Node *n = malloc(sizeof(Node));
  n->id = ND_COMP;
  n->node0 = left;
  n->node1 = right;
  n->node2 = NULL;
  n->extra = op;
  return n;
}

Node *make_if_node(Node *cond, Node *t) {
  Node *n = malloc(sizeof(Node));
  n->id = ND_IF;
  n->node0 = cond;
  n->node1 = t;
  n->node2 = NULL;
  return n;
}

Node *make_if_else_node(Node *cond, Node *t, Node *f) {
  Node *n = malloc(sizeof(Node));
  n->id = ND_IF_ELSE;
  n->node0 = cond;
  n->node1 = t;
  n->node2 = f;
  return n;
}

Node *make_loop_node(Node *cond, Node *loop) {
  Node *n = malloc(sizeof(Node));
  n->id = ND_WHILE;
  n->node0 = cond;
  n->node1 = loop;
  n->node2 = NULL;
  return n;
}

Node *make_arith_node(Node *left, ArithOp op, Node *right) {
  Node *n = malloc(sizeof(Node));
  n->id = ND_ARITH;
  n->node0 = left;
  n->node1 = right;
  n->node2 = NULL;
  n->extra = op;
  return n;
}

Node *make_assign_node(Node *left, Node *right) {
  Node *n = malloc(sizeof(Node));
  n->id = ND_ASSIGN;
  n->node0 = left;
  n->node1 = right;
  n->node2 = NULL;
  return n;
}

Node *make_unary_node(NodeType type, Node *one) {
  Node *n = malloc(sizeof(Node));
  n->id = type;
  n->node0 = one;
  n->node1 = NULL;
  return n;
}

/* extra引数で BLOCK か DECLS かを決めるようにする */
Node *make_concat_node(Node *f, Node *s, int extra) {
  Node *n = malloc(sizeof(Node));
  /* extra: 2->ND_DECLS, 3->ND_BLOCK と仮定してマッピング */
  if (extra == 1)
    n->id = ND_LIST;
  else if (extra == 2)
    n->id = ND_DECLS;
  else if (extra == 3)
    n->id = ND_BLOCK;
  else
    n->id = extra;

  n->node0 = f;
  n->node1 = s;
  n->node2 = NULL;
  return n;
}

Node *make_function_node(Node *name, Node *locals, Node *body, Node *params) {
  Node *n = malloc(sizeof(Node));
  n->id = ND_FUNC;
  n->str = name->str; // 関数名
  n->node0 = locals;  // ローカル変数宣言 (ValDeffineSection)
  n->node1 = body;    // 関数本体 (SentenceSet)
  n->node2 = params;  // 引数
  return n;
}

Node *make_call_node(Node *name, Node *args) {
  Node *n = malloc(sizeof(Node));
  n->id = ND_FUNC_CALL;
  n->str = name->str;
  n->node0 = args;
  return n;
}
