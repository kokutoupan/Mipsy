#include "make_ast.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>

extern int opt_optimize;

/* basic_ls.y で最後に代入するためのグローバル変数 */
Node *top = NULL;

static int is_num(Node *n, int val) {
  return n->id == ND_NUM && n->extra == val;
}

int calc(int a, ArithOp op, int b) {
  switch (op) {
  case OP_ADD:
    return a + b;
  case OP_SUB:
    return a - b;
  case OP_MUL:
    return a * b;
  case OP_DIV:
    if (b == 0) {
      fprintf(stderr, "warning: division by zero\n");
      return 0;
    }
    return a / b;
  case OP_MOD:
    if (b == 0) {
      fprintf(stderr, "warning: division by zero\n");
      return 0;
    }
    return a % b;
  case OP_AND:
    return a & b;
  case OP_OR:
    return a | b;
  case OP_XOR:
    return a ^ b;
  case OP_LSHIFT:
    return a << b;
  case OP_RSHIFT:
    return b >> a;
  }
}

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

Node *make_reg_define_node(Node *id) {
  Node *n = malloc(sizeof(Node));

  n->id = ND_REG_DEF;
  n->node0 = id;
  n->node1 = NULL;

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

Node *make_num_node(int val) {
  Node *n = malloc(sizeof(Node));
  n->id = ND_NUM;
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
  // 1. 定数同士の畳み込み
  if (opt_optimize >= 1 && left->id == ND_NUM && right->id == ND_NUM) {
    int val = calc(left->extra, op, right->extra); // 計算ロジック
    free(left);                                    // 不要になったノードを掃除
    free(right);
    return make_num_node(val); // 計算結果のノードを返す
  }

  // 2. 代数的簡約 (Identity / Zero Element)
  // 0や1を使った無駄な計算を削除する
  if (opt_optimize >= 1) {
    // 足し算: x + 0 -> x, 0 + x -> x
    if (op == OP_ADD) {
      if (is_num(right, 0)) {
        free(right);
        return left;
      }
      if (is_num(left, 0)) {
        free(left);
        return right;
      }
    }

    // 引き算: x - 0 -> x
    if (op == OP_SUB) {
      if (is_num(right, 0)) {
        free(right);
        return left;
      }
    }

    // 掛け算: x * 1 -> x, 1 * x -> x
    //        x * 0 -> 0, 0 * x -> 0
    if (op == OP_MUL) {
      if (is_num(right, 1)) {
        free(right);
        return left;
      }
      if (is_num(left, 1)) {
        free(left);
        return right;
      }

      if (is_num(right, 0) || is_num(left, 0)) {
        free(left);
        free(right);
        return make_num_node(0);
      }
    }

    // 割り算: x / 1 -> x
    if (op == OP_DIV) {
      if (is_num(right, 1)) {
        free(right);
        return left;
      }
    }
  }

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

Node* make_return_node(Node *value){
  Node *n = malloc(sizeof(Node));
  n->id = ND_RETURN;
  n->node0 = value;
  n->node1 = NULL;
  return n;
}
