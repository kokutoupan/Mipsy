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
  case OP_MOD:
    return "%";
  case OP_AND:
    return "&";
  case OP_OR:
    return "|";
  case OP_XOR:
    return "^";
  case OP_LSHIFT:
    return "<<";
  case OP_RSHIFT:
    return ">>";
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
  case OP_NEQ:
    return "!=";
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

  case ND_NUM:
    printf("NUM: %d\n", node->extra);
    break;

  case ND_DEF:
    printf("DEF (define):\n");
    print_ast(node->node0, depth + 1);
    break;

  case ND_REG_DEF:
    printf("DEF (reg):\n");
    print_ast(node->node0, depth + 1);
    break;

  case ND_DEFARRAY:
    printf("DEFARRAY:\n");
    print_ast(node->node0, depth + 1); // 名前
    print_ast(node->node1, depth + 1); // サイズ/次元
    break;

  case ND_REF:
    printf("ARRAY_REF ([]):\n");
    print_ast(node->node0, depth + 1); // ベース
    print_ast(node->node1, depth + 1); // インデックス
    break;

  case ND_ADDR:
    printf("ADDR_OF (@)\n");
    print_ast(node->node0, depth + 1);
    break;

  case ND_DEREF:
    printf("DEREF ($)\n");
    print_ast(node->node0, depth + 1);
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
    printf("(THEN)\n");
    print_ast(node->node1, depth + 1); // THENブロック
    break;

  case ND_IF_ELSE:
    printf("IF-ELSE\n");
    print_ast(node->node0, depth + 1); // 条件
    shift_view(depth);
    printf("(THEN)\n");
    print_ast(node->node1, depth + 1); // THENブロック
    shift_view(depth);
    printf("(ELSE)\n");
    print_ast(node->node2, depth + 1); // ELSEブロック
    break;

  case ND_WHILE:
    printf("WHILE\n");
    print_ast(node->node0, depth + 1); // 条件
    print_ast(node->node1, depth + 1); // 本体
    break;

  case ND_BREAK:
    printf("BREAK\n");
    break;

  case ND_RETURN:
    printf("RETURN\n");
    print_ast(node->node0, depth + 1); // 戻り値
    break;

  case ND_FUNC_CALL:
    printf("CALL: %s\n", node->str);
    print_ast(node->node0, depth + 1); // 引数(ND_ARGS or Single Node)
    break;

  case ND_ARGS:
    printf("ARGS\n");
    print_ast(node->node0, depth + 1);
    print_ast(node->node1, depth + 1);
    break;

  case ND_FUNC:
    printf("FUNC: %s\n", node->str ? node->str : "(no name)");
    shift_view(depth);
    printf("(PARAMS)\n");
    print_ast(node->node2, depth + 1); // 引数定義
    shift_view(depth);
    printf("(LOCALS)\n");
    print_ast(node->node0, depth + 1); // ローカル変数
    shift_view(depth);
    printf("(BODY)\n");
    print_ast(node->node1, depth + 1); // 本体
    break;

  // リスト構造系 (再帰的に表示)
  case ND_BLOCK:
  case ND_DECLS:
  case ND_LIST:
    // リストの種類を表示
    if (node->id == ND_BLOCK)
      printf("BLOCK\n");
    else if (node->id == ND_DECLS)
      printf("DECLS\n");
    else
      printf("LIST\n");

    print_ast(node->node0, depth + 1);
    print_ast(node->node1, depth + 1);
    break;

  default:
    printf("UNKNOWN NODE (id=%d)\n", node->id);
    break;
  }
}

// JSON形式でASTを表示する関数
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
    printf("\"value\": %d\n", node->extra);
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

  case ND_REG_DEF:
    printf("REG_DEF\",\n");
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
    print_ast_json(node->node0, depth + 2); // Name
    printf(",\n");
    print_ast_json(node->node1, depth + 2); // Size/Dims
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_REF:
    printf("ARRAY_REF\",\n");
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2); // Base
    printf(",\n");
    print_ast_json(node->node1, depth + 2); // Index
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_ADDR:
    printf("ADDR_OF\",\n");
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2);
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_DEREF:
    printf("DEREF\",\n");
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2);
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
    print_ast_json(node->node0, depth + 2); // Cond
    printf(",\n");
    print_ast_json(node->node1, depth + 2); // Then
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_IF_ELSE:
    printf("IF_ELSE\",\n");
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2); // Cond
    printf(",\n");
    print_ast_json(node->node1, depth + 2); // Then
    printf(",\n");
    print_ast_json(node->node2, depth + 2); // Else
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_WHILE:
    printf("WHILE\",\n");
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2); // Cond
    printf(",\n");
    print_ast_json(node->node1, depth + 2); // Body
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_BREAK:
    printf("BREAK\"\n");
    // No children
    break;

  case ND_RETURN:
    printf("RETURN\",\n");
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2);
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_FUNC_CALL:
    printf("CALL\",\n");
    shift_view(depth + 1);
    printf("\"name\": \"%s\",\n", node->str);
    shift_view(depth + 1);
    printf("\"children\": [\n");
    print_ast_json(node->node0, depth + 2); // Args
    printf("\n");
    shift_view(depth + 1);
    printf("]\n");
    break;

  case ND_FUNC:
    printf("FUNC\",\n");
    shift_view(depth + 1);
    printf("\"name\": \"%s\",\n", node->str ? node->str : "(no name)");
    shift_view(depth + 1);
    printf("\"params\": \n");
    print_ast_json(node->node2, depth + 2);
    printf(",\n");
    shift_view(depth + 1);
    printf("\"locals\": \n");
    print_ast_json(node->node0, depth + 2);
    printf(",\n");
    shift_view(depth + 1);
    printf("\"body\": \n");
    print_ast_json(node->node1, depth + 2);
    printf("\n");
    break;

  case ND_BLOCK:
  case ND_DECLS:
  case ND_LIST:
  case ND_ARGS:
    // リスト構造
    if (node->id == ND_BLOCK)
      printf("BLOCK\",\n");
    else if (node->id == ND_DECLS)
      printf("DECLS\",\n");
    else if (node->id == ND_ARGS)
      printf("ARGS\",\n");
    else
      printf("LIST\",\n");

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
    printf("\"id\": %d\n", node->id);
    break;
  }

  shift_view(depth);
  printf("}");
}
