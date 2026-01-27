#pragma once

/* ノードの種類を識別するためのタグ */
typedef enum {
  ND_DEF,      // 定義
  ND_DEFARRAY, // ARRAYの定義
  ND_REF,      // 参照
  ND_NUM,      // 変数・数値
  ND_ASSIGN,   // 代入 (=)
  ND_IF,       // IF
  ND_IF_ELSE,  // IF-ELSE
  ND_WHILE,    // WHILE
  ND_BLOCK,    // 文の並び (SentenceSet)
  ND_FUNC,     // 関数(初期はProgramも)
  ND_FUNC_CALL,
  ND_ARGS,
  ND_DECLS, // 定義の並び (ValDeffineSection)
  ND_ARITH, // 算術演算 (+, -, *, /)
  ND_COMP,  // 比較演算 (==, <, >)
  ND_ADDR,  // @
  ND_DEREF, // ^
  ND_IDENT, // 識別子
  ND_LIST,
  ND_BREAK,
} NodeType;

typedef enum { OP_EQ, OP_LT, OP_GT, OP_LEQ, OP_GEQ } CompOp;
typedef enum { OP_ADD, OP_SUB, OP_MUL, OP_DIV } ArithOp;

// -- 変数関係 --
#define MAX_VARS 512
#define MAX_SCOPES 64

typedef struct vartype {
  enum { VarArray, VarInt } kind;
  int len;
  struct vartype *base;
} VarType;

typedef struct {
  VarType *type;
  char *name;   // 変数名
  int offset;   // スタックのオフセット or 任意の値
  int scope_id; // 属するスコープ
  int in_use;   // 使用中 = 1
} VarEntry;

// スコープ情報を管理する構造体を追加
typedef struct {
  int id;
  int saved_offset;  // このスコープに入ったときの next_offset
  int saved_var_idx; // このスコープに入ったときの変数の登録数
} ScopeInfo;

typedef struct {
  VarEntry vars[MAX_VARS];
  int n_vars; // 現在登録されている変数の総数（末尾ポインタ）

  ScopeInfo scope_stack[MAX_SCOPES]; // int配列から構造体配列に変更
  int scope_top;
  int next_scope_id;

  int next_offset;
} VarTable;

// -- ASTのノード --

typedef struct node {
  NodeType id; /* int id を NodeType id に変更 */
  char *str;
  int extra; /* 演算子などが入る */
  struct node *node0;
  struct node *node1;
  struct node *node2;

  VarType *type;
} Node;
