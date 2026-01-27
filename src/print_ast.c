#include "print_ast.h"

#include <stdio.h>
#include <stdlib.h>

// 演算子の文字列表記用ヘルパー
const char *get_arith_op_str(int op) {
  switch (op) {
  case OP_ADD:
    return "+";
  case OP_SUB:
    return "-";
  case OP_MUL:
    return "*";
  case OP_DIV:
    return "/";
  default:
    return "???";
  }
}

const char *get_comp_op_str(int op) {
  switch (op) {
  case OP_EQ:
    return "==";
  case OP_LT:
    return "<";
  case OP_GT:
    return ">";
  case OP_LEQ:
    return "<=";
  case OP_GEQ:
    return ">=";
  default:
    return "???";
  }
}

void shift_view(int depth) {
  // インデント表示
  for (int i = 0; i < depth; i++)
    printf("  ");
}

// 再帰的にASTを表示する関数
void print_ast(Node *node, int depth) {
  if (node == NULL)
    return;

  shift_view(depth);

  switch (node->id) {
  case ND_IDENT:
    printf("IDENT: %s\n", node->str);
    break;
  case ND_DEF:
    printf("DEF:\n");
    print_ast(node->node0, depth + 1);
    break;

  case ND_DEFARRAY:
    printf("ARRAY_DEF:\n");
    print_ast(node->node0, depth + 1);
    print_ast(node->node1, depth + 1);
    break;

  case ND_NUM:
    printf("VAR: %d\n", node->extra);
    break;

  case ND_REF:
    printf("REF:\n");
    print_ast(node->node0, depth + 1);
    print_ast(node->node1, depth + 1);
    break;

  case ND_ASSIGN:
    printf("ASSIGN (=)\n");
    print_ast(node->node0, depth + 1); // 左辺
    print_ast(node->node1, depth + 1); // 右辺
    break;

  case ND_ARITH:
    printf("OP_ARITH (%s)\n", get_arith_op_str(node->extra));
    print_ast(node->node0, depth + 1);
    print_ast(node->node1, depth + 1);
    break;

  case ND_COMP:
    printf("OP_COMP (%s)\n", get_comp_op_str(node->extra));
    print_ast(node->node0, depth + 1);
    print_ast(node->node1, depth + 1);
    break;

  case ND_IF:
    printf("IF\n");
    print_ast(node->node0, depth + 1); // 条件
    shift_view(depth);
    printf("(THEN)\n"); // 見やすくするためのラベル（手動インデント）
    print_ast(node->node1, depth + 1); // ブロック
    break;

  case ND_IF_ELSE:
    printf("IF-ELSE\n");
    print_ast(node->node0, depth + 1); // 条件

    shift_view(depth);
    printf("(THEN)\n");
    print_ast(node->node1, depth + 1); // IFブロック

    shift_view(depth);
    printf("(ELSE)\n");
    print_ast(node->node2, depth + 1); // ELSEブロック
    break;

  case ND_WHILE:
    printf("WHILE\n");
    print_ast(node->node0, depth + 1); // 条件
    print_ast(node->node1, depth + 1); // ループ本体
    break;

  case ND_BLOCK:
    printf("BLOCK\n");
    print_ast(node->node0, depth + 1);
    print_ast(node->node1, depth + 1);
    break;

  case ND_DECLS:
    printf("DECLARATIONS\n");
    print_ast(node->node0, depth + 1);
    print_ast(node->node1, depth + 1);
    break;
  case ND_LIST:
    printf("LISTNODE\n");
    print_ast(node->node0, depth + 1);
    print_ast(node->node1, depth + 1);
    break;

  case ND_FUNC:
    printf("FUNCNODE\n");
    print_ast(node->node0, depth + 1);
    print_ast(node->node1, depth + 1);
    break;

  default:
    printf("UNKNOWN NODE (id=%d)\n", node->id);
    break;
  }
}

// 再帰的にASTを表示する関数
void print_ast_json(Node *node, int depth) {
  shift_view(depth);

  if (!node) {
    printf("null");
    return;
  }

  printf("{\n");

  // type
  shift_view(depth + 1);
  printf("\"type\": \"");

  switch (node->id) {
  case ND_IDENT:
    printf("IDENT\",\n");
    shift_view(depth + 1);
    printf("\"value\": \"%s\"\n", node->str);
    break;

  case ND_NUM:
    printf("NUM\",\n");
    shift_view(depth + 1);
    printf("\"value\": \"%s\"\n", node->str);
    break;

  case ND_DEF:
    printf("DEF\",\n");
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2);
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_DEFARRAY:
    printf("DEFARRAY\",\n");
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2);
    printf(",\n");
    print_ast_json(node->node1, depth + 2);
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_REF:
    printf("REF\",\n");
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2);
    printf(",\n");
    print_ast_json(node->node1, depth + 2);
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_ASSIGN:
    printf("ASSIGN\",\n");
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2);
    printf(",\n");
    print_ast_json(node->node1, depth + 2);
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_ARITH:
    printf("ARITH\",\n");
    shift_view(depth + 1);
    printf("\"op\": \"%s\",\n", get_arith_op_str(node->extra));
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2);
    printf(",\n");
    print_ast_json(node->node1, depth + 2);
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_COMP:
    printf("COMP\",\n");
    shift_view(depth + 1);
    printf("\"op\": \"%s\",\n", get_comp_op_str(node->extra));
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2);
    printf(",\n");
    print_ast_json(node->node1, depth + 2);
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_IF:
    printf("IF\",\n");
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2); // 条件
    printf(",\n");
    print_ast_json(node->node1, depth + 2); // THEN ブロック
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_IF_ELSE:
    printf("IF_ELSE\",\n");
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2); // 条件
    printf(",\n");
    print_ast_json(node->node1, depth + 2); // THEN
    printf(",\n");
    print_ast_json(node->node2, depth + 2); // ELSE
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_WHILE:
    printf("WHILE\",\n");
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2); // 条件
    printf(",\n");
    print_ast_json(node->node1, depth + 2); // 本体
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_BLOCK:
  case ND_DECLS:
  case ND_LIST:
  case ND_FUNC:
    // すべて node0, node1 をリストとして扱う
    printf("%s\",\n", (node->id == ND_BLOCK)   ? "BLOCK"
                      : (node->id == ND_DECLS) ? "DECLS"
                      : (node->id == ND_FUNC)  ? "FUNC"
                                               : "LIST");
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2);
    printf(",\n");
    print_ast_json(node->node1, depth + 2);
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  default:
    printf("UNKNOWN\",\n");
    shift_view(depth + 1);
    printf("\"value\": %d\n", node->id);
    break;
  }

  shift_view(depth);
  printf("}");
}
