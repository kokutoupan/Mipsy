#include "generate_code.h" // 公開ヘッダ
#include "internal.h"
#include "mips_code.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>

// グローバル変数の実体
VarTable vartable;
CodeList codeList;
static int used_s_regs = 0;
VarType *make_type_from_dims(Node *dimlist);

// 型のサイズ計算（配列のストライド計算用）
int calc_stride(VarType *t) {
  if (t == NULL)
    return 4;
  if (t->base == NULL)
    return 4; // int base
  return sizeof_type(t->base);
}

void gen_global_vars(Node *node, CodeList *codeList) {
  if (node == NULL)
    return;

  if (node->id == ND_DECLS) {
    gen_global_vars(node->node0, codeList);
    gen_global_vars(node->node1, codeList);
    return;
  }

  // 変数定義 (define x;)
  if (node->id == ND_DEF) {
    char *name = node->node0->str;

    append_code(codeList, new_label(name));               // ラベル:
    append_code(codeList, new_code_dir_i(ASM_d_WORD, 0)); // .word 0

    var_add(&vartable, name);
  }
  // 配列定義 (array arr[10];)
  else if (node->id == ND_DEFARRAY) {
    char *name = node->node0->str;
    int size = node->node1->extra;

    // 次元リストから型情報を生成 (多次元対応)
    VarType *type = make_type_from_dims(node->node1);

    int byte_size = sizeof_type(type);

    append_code(codeList, new_label(name));                        // ラベル:
    append_code(codeList, new_code_dir_i(ASM_d_SPACE, byte_size)); // .space 40

    // 型情報の登録
    var_add_array(&vartable, name, type, byte_size);
  }
}

// 次元リストから型情報を生成
VarType *make_type_from_dims(Node *dimlist) {
  VarType *t = malloc(sizeof(VarType));
  t->kind = VarInt;
  t->base = NULL;

  // 後ろから積む
  if (dimlist->id == ND_LIST) {
    // 右から先に (再帰)
    t = make_type_from_dims(dimlist->node1);

    VarType *arr = malloc(sizeof(VarType));
    arr->kind = VarArray;
    arr->len = dimlist->node0->extra;
    arr->base = t;
    return arr;
  }

  // 単一次元 (最内側)
  VarType *arr = malloc(sizeof(VarType));
  arr->kind = VarArray;
  arr->len = dimlist->extra;
  arr->base = t;
  return arr;
}

// 配列変数の定義
void def_var_array(Node *node) {
  // node->node0 : IDENT
  // node->node1 : dim list

  VarType *type = make_type_from_dims(node->node1);
  int size = sizeof_type(type);

  var_add_array(&vartable, node->node0->str, type, size);
}

// 通常変数の定義
void def_variable(Node *node, int is_reg) {
  if (node == NULL)
    return;

  if (node->id == ND_IDENT) {
    // 変数テーブルに登録（この時点では初期化のみ）
    VarEntry *ent = var_add(&vartable, node->str);

    // 「reg指定あり」かつ「まだ$sレジスタ(8個)に空きがある」場合
    if (is_reg && used_s_regs < 8) {
      ent->reg_idx = used_s_regs++; // 0, 1, 2... と割り当て
      ent->offset = 0;              // スタックオフセットは使わない
      vartable.next_offset -= 4; // 打ち消し

    } else {
      // 従来通りスタックに割り当て
      ent->reg_idx = -1;
    }
    return;
  }

  if (node->id == ND_LIST) {
    // 左の要素（IDENT または LIST の先頭）を処理
    def_variable(node->node0, is_reg);
    // 右側（次の要素へ進む）
    def_variable(node->node1, is_reg);
    return;
  }
}

// 変数宣言セクションの処理 (再帰)
void variable_declaration(Node *node) {
  if (node == NULL)
    return;

  // 1. 単体の変数定義 (ND_DEF) の場合
  if (node->id == ND_DEF) {
    def_variable(node->node0, 0);
    return;
  }

  if (node->id == ND_REG_DEF) {
    def_variable(node->node0, 1);
    return;
  }

  // 2. 単体の配列定義 (ND_DEFARRAY) の場合
  if (node->id == ND_DEFARRAY) {
    def_var_array(node);
    return;
  }

  // 3. 宣言リスト (ND_DECLS) の場合
  if (node->id == ND_DECLS) {
    variable_declaration(node->node0); // 左側を処理
    variable_declaration(node->node1); // 右側を処理
    return;
  }

  // それ以外（本来は来ないはずだがガードしておく）
  fprintf(stderr, "unknown node in declaration: %d\n", node->id);
}

// 関数プロローグ (スタック確保)
void func_head_code(CodeList *out, int size, int n_regs) {

  int save_area_size = 8 + (n_regs * 4);
  // SPの確保 (ローカル変数サイズ + $fp(4) + $ra(4))
  int frame_size = size + save_area_size;

  append_code(out, new_code_i(ASM_ADDIU, R_SP, R_SP, -frame_size));

  // $raの退避 (sp + frame_size - 8 )
  append_code(out, new_code_i(ASM_SW, R_RA, R_SP, frame_size - 4));

  // $fpの退避 (sp + size)
  append_code(out, new_code_i(ASM_SW, R_FP, R_SP, frame_size - 8));

  // $s レジスタの退避
  // スタックの空いた隙間 (size の直後) に詰めていく
  for (int i = 0; i < n_regs; i++) {
    // R_S0 は mips_reg.h で定義されている定数 ($16)
    append_code(out, new_code_i(ASM_SW, R_S0 + i, R_SP, size + (i * 4)));
  }

  // $fp <- $sp (新しいフレームポインタの設定)
  append_code(out, new_code_i(ASM_ORI, R_FP, R_SP, 0));
}

// 関数エピローグ (スタック解放・復帰)
void func_bottom_code(CodeList *out, int size, int n_regs) {
  int save_area_size = 8 + (n_regs * 4);
  int frame_size = size + save_area_size;

  // $sp <- $fp (スタックポインタを戻す)
  append_code(out, new_code_i(ASM_ORI, R_SP, R_FP, 0));

  // ★ $s レジスタの復帰
  for (int i = 0; i < n_regs; i++) {
    append_code(out, new_code_i(ASM_LW, R_S0 + i, R_SP, size + (i * 4)));
  }

  // $fpの復元
  append_code(out, new_code_i(ASM_LW, R_FP, R_SP, size + save_area_size - 8));

  // $raの復元
  append_code(out, new_code_i(ASM_LW, R_RA, R_SP, size + save_area_size - 4));

  // スタック領域の解放
  append_code(out, new_code_i(ASM_ADDIU, R_SP, R_SP, frame_size));

  // jr $ra (リターン)
  // new_code0 は op1 を設定しないため、手動で op1=RA を設定したCodeを作る
  Code *c = malloc(sizeof(Code));
  c->kind = CODE_INSN;
  c->insn.code = ASM_JR;
  c->insn.op1 = (Operand){OP_REG, .reg = R_RA};
  c->next = NULL;
  append_code(out, c);

  append_code(out, new_code0(ASM_NOP)); // 遅延スロット
}

void gen_function(Node *func_node) {
  if (func_node->id != ND_FUNC)
    return;

  used_s_regs = 0;

  // 1. 関数ラベルの生成
  append_code(&codeList, new_label(func_node->str));

  // 2. 変数テーブル（スコープ）の初期化
  // 関数ごとにローカル変数はリセットされる
  // var_table_init(&vartable);
  vartable.next_offset = 0;
  enter_scope(&vartable);

  // 3. 引数の登録
  struct {
    int offset;
    MipsReg reg;
  } param_saves[4];
  int param_count = 0;
  MipsReg arg_regs[] = {R_A0, R_A1, R_A2, R_A3};

  Node *params = func_node->node2; // Params (Idents list)

  while (params != NULL) {
    if (param_count >= 4) {
      fprintf(stderr, "error: too many parameters (max 4)\n");
      exit(1);
    }

    Node *ident_node = NULL;
    // Paramsは ND_IDENT 単体か、ND_CONCAT(1) によるリスト構造
    if (params->id == ND_IDENT) {
      ident_node = params;
      params = NULL; // リスト終了
    } else {
      ident_node = params->node0;
      params = params->node1;
    }

    // VarTableに登録 (すべて4バイトintとして扱う)
    VarEntry *ent = var_add(&vartable, ident_node->str);

    // 「$aX を ent->offset に保存する」という情報を記録
    param_saves[param_count].offset = ent->offset;
    param_saves[param_count].reg = arg_regs[param_count];

    param_count++;
  }

  // 4. ローカル変数宣言の処理
  // node0: Locals (ValDeffineSection)
  if (func_node->node0 != NULL)
    variable_declaration(func_node->node0);

  // 5. プロローグ生成
  int locals_size = (vartable.next_offset);
  func_head_code(&codeList, locals_size,used_s_regs);

  // 6. 引数の値を変数に保存
  // プロローグで $fp が設定された直後に実行する
  for (int i = 0; i < param_count; i++) {
    append_code(&codeList, new_code_i(ASM_SW, param_saves[i].reg, R_FP,
                                      param_saves[i].offset));
  }

  // 7. 関数本体の生成
  // node1: Body (SentenceSet)
  gen_stmt_list(&codeList, func_node->node1);

  // 8. エピローグ生成
  func_bottom_code(&codeList, locals_size,used_s_regs);

  // デバッグ(変数の表示)
  if (opt_debug)
    print_var_table(stderr, &vartable);

  leave_scope(&vartable);
}

CodeList *generate_code(Node *node) {
  init_code_list(&codeList);

  var_table_init(&vartable);

  CodeList global_data;
  init_code_list(&global_data);

  if (node->node0 != NULL) {

    fprintf(stderr, "use global data\n");
    gen_global_vars(node->node0, &global_data);

    if (opt_debug)
      print_var_table(stderr, &vartable);
  }

  // ルートはグローバルと関数たちでできている
  // ルートノード (Program) は関数のリスト (ND_CONCATで繋がっている)
  Node *cur = node->node1;
  while (cur != NULL) {
    if (cur->id == ND_FUNC) {
      // 単一の関数定義の場合
      gen_function(cur);
      break;
    } else if (cur->id == ND_LIST) {
      // リストの場合: 左(node0)が関数、右(node1)が次のリスト
      gen_function(cur->node0);
      cur = cur->node1;
    } else {
      fprintf(stderr, "Unexpected node in top level: %d\n", cur->id);
      break;
    }
  }

  append_code(&codeList, new_code0(ASM_d_DATA));

  concat_code_list(&codeList, &global_data);

  return &codeList;
}
