#include "mips_code.h"
#include <stdlib.h>
#include <string.h>

int sizeof_type(VarType *t) {
  if (t == NULL) {
    fprintf(stderr, "sizeof_type: NULL type\n");
    exit(1);
  }
  if (t->kind == VarInt)
    return 4;
  return t->len * sizeof_type(t->base);
}

void var_table_init(VarTable *vt) {
  // 1. 全体をゼロクリア
  memset(vt, 0, sizeof(VarTable));

  // 2. グローバルスコープ (Stack Top = 0) の初期設定
  // 構造体 ScopeInfo のメンバを明示的に設定します
  vt->scope_top = 0;
  vt->scope_stack[0].id = 0;            // グローバルスコープID
  vt->scope_stack[0].saved_offset = 0;  // オフセット初期値 (0)
  vt->scope_stack[0].saved_var_idx = 0; // 変数登録数初期値 (0)

  // 3. 次のIDとオフセットの準備
  vt->next_scope_id = 1;

  vt->next_offset = 0;
}

void enter_scope(VarTable *vt) {
  int id = vt->next_scope_id++;

  vt->scope_top++;
  if (vt->scope_top >= MAX_SCOPES) {
    fprintf(stderr, "error: scope stack overflow\n");
    exit(1);
  }

  // 現在の状態（復元ポイント）を保存
  vt->scope_stack[vt->scope_top].id = id;
  vt->scope_stack[vt->scope_top].saved_offset = vt->next_offset;
  vt->scope_stack[vt->scope_top].saved_var_idx = vt->n_vars;
}

void leave_scope(VarTable *vt) {
  if (vt->scope_top < 0) {
    fprintf(stderr, "error: scope stack underflow\n");
    exit(1);
  }

  // 保存しておいた状態を取り出す
  int saved_offset = vt->scope_stack[vt->scope_top].saved_offset;
  int saved_var_idx = vt->scope_stack[vt->scope_top].saved_var_idx;

  vt->scope_top--;

  vt->next_offset = saved_offset; // オフセットを戻す（スタック領域再利用）
  vt->n_vars = saved_var_idx; // 変数リストの末尾ポインタを戻す（変数の破棄）
}

VarEntry *var_add(VarTable *vt, char *name) {
  // 1. 容量チェック
  if (vt->n_vars >= MAX_VARS) {
    fprintf(stderr, "error: too many vars\n");
    exit(1);
  }

  // 現在のスコープIDを取得
  int current_scope_id = vt->scope_stack[vt->scope_top].id;

  // 現在のスコープが始まった時の変数のインデックスを取得
  int start_idx = vt->scope_stack[vt->scope_top].saved_var_idx;

  // 2. 現在のスコープ内での重複チェック
  for (int i = start_idx; i < vt->n_vars; i++) {
    if (strcmp(vt->vars[i].name, name) == 0) {
      fprintf(stderr, "error: variable '%s' already exists in this scope\n",
              name);
      exit(1);
    }
  }

  // 3. 変数を末尾に追加（スタック方式）
  VarEntry *v = &vt->vars[vt->n_vars];

  // 型情報の作成（デフォルトで VarInt とする例）
  VarType *t = malloc(sizeof(VarType));
  t->kind = VarInt;
  t->len = 0;
  t->base = NULL;

  v->type = t;
  v->name = name;
  v->scope_id = current_scope_id;

  // オフセット決定
  v->offset = vt->next_offset;

  // 次のためにカウンタを進める
  vt->next_offset += 4; // intなら4バイト進める
  vt->n_vars++;         // 登録数 + 1

  return v;
}

// 配列変数を追加する関数
VarEntry *var_add_array(VarTable *vt, char *name, VarType *vartype, int size) {
  // 1. 容量チェック
  if (vt->n_vars >= MAX_VARS) {
    fprintf(stderr, "error: too many vars\n");
    exit(1);
  }

  // 現在のスコープ情報の取得
  int current_scope_id = vt->scope_stack[vt->scope_top].id;
  int start_idx = vt->scope_stack[vt->scope_top].saved_var_idx;

  // 2. 現在のスコープ内での重複チェック
  for (int i = start_idx; i < vt->n_vars; i++) {
    if (strcmp(vt->vars[i].name, name) == 0) {
      fprintf(stderr, "error: variable '%s' already exists in this scope\n",
              name);
      return NULL;
    }
  }

  // 3. 変数を末尾に追加
  VarEntry *v = &vt->vars[vt->n_vars];

  v->type = vartype;
  v->name = strdup(name); // 名前をコピーして保持
  v->scope_id = current_scope_id;
  v->offset = vt->next_offset;

  // 状態更新
  vt->next_offset += size; // 指定されたサイズ分(配列全体のサイズ)進める
  vt->n_vars++;            // 登録数 + 1

  return v;
}

// 変数を検索する関数
VarEntry *var_find(VarTable *vt, const char *name) {
  // スタックの後ろ（最新の変数）から順に探す
  // leave_scopeで n_vars
  // が巻き戻されるため、ここには「有効な変数」しか存在しない
  // 最初に見つかったものが「最も内側のスコープ」の変数となる（Shadowing対応）
  for (int i = vt->n_vars - 1; i >= 0; i--) {
    if (strcmp(vt->vars[i].name, name) == 0) {
      return &vt->vars[i];
    }
  }

  // 見つからなかった場合
  return NULL;
}

// 表示
void print_var_table(FILE *fp, const VarTable *vt) {
  fprintf(fp, "====== VarTable Debug ======\n");

  // 現在のスコープスタック表示
  fprintf(fp, "Scope Stack (top -> bottom): ");
  for (int i = vt->scope_top; i >= 0; i--) {
    fprintf(fp, "(%d %d)", vt->scope_stack[i].id,
            vt->scope_stack[i].saved_offset);
  }
  fprintf(fp, "\n");

  // デバッグ用に現在の変数登録数と次のオフセットも表示しておくと便利です
  fprintf(fp, "\nVariables (n_vars=%d, next_offset=%d):\n", vt->n_vars,
          vt->next_offset);
  fprintf(fp, "%-4s  %-20s  %-6s  %s\n", "Idx", "Name", "Scope", "Offset");
  fprintf(fp, "-----------------------------------------------\n");

  // スタック管理方式なので、0 から n_vars
  // まで回すだけで「有効な変数」になります
  for (int i = 0; i < vt->n_vars; i++) {
    const VarEntry *v = &vt->vars[i];

    fprintf(fp, "%-4d  %-20s  %-6d  %d\n", i, v->name ? v->name : "(null)",
            v->scope_id, v->offset);
  }

  fprintf(fp, "============================\n\n");
}

// 初期化
void init_code_list(CodeList *list) {
  list->head = NULL;
  list->tail = NULL;
}

// コードの追加
void append_code(CodeList *list, Code *node) {
  node->next = NULL;

  if (list->tail == NULL) {
    // 空のリスト
    list->head = node;
    list->tail = node;
  } else {
    list->tail->next = node;
    list->tail = node;
  }
}

// コードリストの連結
void concat_code_list(CodeList *dst, const CodeList *src) {
  dst->tail->next = src->head;
  dst->tail = src->tail;
}

// 命令形式ごとのコード生成
Code *new_code_r(AsmCode code, MipsReg rd, MipsReg rs, MipsReg rt) {
  Code *c = malloc(sizeof(Code));
  c->kind = CODE_INSN;
  c->insn.code = code;
  c->insn.op1 = (Operand){OP_REG, .reg = rd};
  c->insn.op2 = (Operand){OP_REG, .reg = rs};
  c->insn.op3 = (Operand){OP_REG, .reg = rt};
  c->next = NULL;
  return c;
}

Code *new_code_i(AsmCode code, MipsReg rt, MipsReg rs, int imm) {
  Code *c = malloc(sizeof(Code));
  c->kind = CODE_INSN;
  c->insn.code = code;
  c->insn.op1 = (Operand){OP_REG, .reg = rt};
  c->insn.op2 = (Operand){OP_REG, .reg = rs};
  c->insn.op3 = (Operand){OP_IMM, .imm = imm};
  c->next = NULL;
  return c;
}

Code *new_code_j(AsmCode code, const char *label) {
  Code *c = malloc(sizeof(Code));
  c->insn.code = code;
  c->insn.op1 = (Operand){OP_LABEL, .label = strdup(label)};
  c->next = NULL;
  return c;
}
Code *new_code0(AsmCode code) {
  Code *c = malloc(sizeof(Code));
  c->kind = CODE_INSN;
  c->insn.code = code;
  c->next = NULL;
  return c;
}

Code *new_code_branch(AsmCode code, MipsReg rs, MipsReg rt, const char *label) {
  Code *c = malloc(sizeof(Code));
  c->insn.code = code;

  c->insn.op1 = (Operand){OP_REG, .reg = rs};
  c->insn.op2 = (Operand){OP_REG, .reg = rt};
  c->insn.op3 = (Operand){OP_LABEL, .label = strdup(label)};

  c->next = NULL;
  return c;
}

Code *new_code(AsmCode code, Operand op1, Operand op2, Operand op3) {
  // --- J type ---
  // op1 = LABEL, op2/op3 unused
  if (op1.type == OP_LABEL) {
    return new_code_j(code, op1.label);
  }

  // --- R type ---
  // op1 = rd(REG), op2 = rs(REG), op3 = rt(REG)
  if (op1.type == OP_REG && op2.type == OP_REG && op3.type == OP_REG) {
    return new_code_r(code, op1.reg, op2.reg, op3.reg);
  }

  // --- I type ---
  // op1 = rt(REG), op2 = rs(REG), op3 = IMM
  if (op1.type == OP_REG && op2.type == OP_REG && op3.type == OP_IMM) {
    return new_code_i(code, op1.reg, op2.reg, op3.imm);
  }

  fprintf(stderr, "new_code: invalid operand combination for %d:o1:%dop2%d\n",
          code, op1.type, op2.type);
  exit(1);
}

char *new_label_pref(const char *prefix) {
  static int cnt = 0;
  char buf[32];
  snprintf(buf, sizeof(buf), "%s%d", prefix, cnt++);
  return strdup(buf);
}

Code *new_label(const char *name) {
  Code *c = malloc(sizeof(Code));
  c->kind = CODE_LABEL;
  c->label.name = strdup(name);
  c->next = NULL;
  return c;
}

// LA命令用: la rt, label
Code *new_code_la(MipsReg rt, const char *label) {
  Code *c = malloc(sizeof(Code));
  c->kind = CODE_INSN;
  c->insn.code = ASM_LA;

  // print_codeの case ASM_LA: ... op1(rt), op3(label) に対応
  c->insn.op1 = (Operand){OP_REG, .reg = rt};
  c->insn.op3 = (Operand){OP_LABEL, .label = strdup(label)};

  c->next = NULL;
  return c;
}

// 整数引数のディレクティブ用: .word 10, .space 40 など
Code *new_code_dir_i(AsmCode code, int imm) {
  Code *c = malloc(sizeof(Code));
  c->kind = CODE_INSN;
  c->insn.code = code;

  c->insn.op1 = (Operand){OP_IMM, .imm = imm};

  c->next = NULL;
  return c;
}

const char *asmcode_to_string(AsmCode code) {
  switch (code) {

  // arithmetic / logic
  case ASM_ADD:
    return "add";
  case ASM_ADDU:
    return "addu";
  case ASM_ADDI:
    return "addi";
  case ASM_ADDIU:
    return "addiu";
  case ASM_SUB:
    return "sub";
  case ASM_SUBU:
    return "subu";
  case ASM_MULT:
    return "mult";
  case ASM_MULTU:
    return "multu";
  case ASM_DIV:
    return "div";
  case ASM_DIVU:
    return "divu";
  case ASM_AND:
    return "and";
  case ASM_ANDI:
    return "andi";
  case ASM_OR:
    return "or";
  case ASM_ORI:
    return "ori";
  case ASM_XOR:
    return "xor";
  case ASM_XORI:
    return "xori";
  case ASM_NOR:
    return "nor";

  // compare
  case ASM_SLT:
    return "slt";
  case ASM_SLTU:
    return "sltu";
  case ASM_SLTI:
    return "slti";
  case ASM_SLTIU:
    return "sltiu";

  // load / store
  case ASM_LB:
    return "lb";
  case ASM_LW:
    return "lw";
  case ASM_SB:
    return "sb";
  case ASM_SW:
    return "sw";

  // shift
  case ASM_SLL:
    return "sll";
  case ASM_SRL:
    return "srl";
  case ASM_SRA:
    return "sra";
  case ASM_SLLV:
    return "sllv";
  case ASM_SRLV:
    return "srlv";
  case ASM_SRAV:
    return "srav";

  // branch
  case ASM_BEQ:
    return "beq";
  case ASM_BNE:
    return "bne";

  // jump
  case ASM_J:
    return "j";
  case ASM_JAL:
    return "jal";
  case ASM_JR:
    return "jr";
  case ASM_JALR:
    return "jalr";

  // constant operation / data transfer
  case ASM_LUI:
    return "lui";
  case ASM_MFHI:
    return "mfhi";
  case ASM_MFLO:
    return "mflo";

  // exception / interruption
  case ASM_SYSCALL:
    return "syscall";

  // pseudo instructions
  case ASM_LI:
    return "li";
  case ASM_LA:
    return "la";
  case ASM_NOP:
    return "nop";
  case ASM_d_TEXT:
    return ".text";
  case ASM_d_DATA:
    return ".data";
  case ASM_d_WORD:
    return ".word";
  case ASM_d_BYTE:
    return ".byte";
  case ASM_d_ASCII:
    return ".ascii";
  case ASM_d_ASCIIZ:
    return ".asciiz";
  case ASM_d_SPACE:
    return ".space";

  default:
    return "<unknown>";
  }
}
