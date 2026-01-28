#pragma once
#include "../include/types.h"
#include "mips_reg.h"
#include <stdio.h>

typedef enum {
  // 数式とか op(dst, $1,$2)
  ASM_ADD,
  ASM_ADDU,
  ASM_ADDI,
  ASM_ADDIU,
  ASM_SUB,
  ASM_SUBU,
  ASM_MULT,
  ASM_MULTU,
  ASM_DIV,
  ASM_DIVU,
  ASM_AND,
  ASM_ANDI,
  ASM_OR,
  ASM_ORI,
  ASM_XOR,
  ASM_XORI,
  ASM_NOR,

  // 比較演算
  ASM_SLT,
  ASM_SLTU,
  ASM_SLTI,
  ASM_SLTIU,

  // (load/store)
  ASM_LB,
  ASM_LW,
  ASM_SB,
  ASM_SW,

  // (shift)
  ASM_SLL,
  ASM_SRL,
  ASM_SRA,
  ASM_SLLV,
  ASM_SRLV,
  ASM_SRAV,

  // (branch)
  ASM_BEQ,
  ASM_BNE,

  // (jump)
  ASM_J,
  ASM_JAL,
  ASM_JR,
  ASM_JALR,

  // (constant operation/data transfer)
  ASM_LUI,
  ASM_MFHI,
  ASM_MFLO,

  // (exception/interruption)
  ASM_SYSCALL,

  // 疑似命令
  ASM_LI,
  ASM_LA,
  ASM_NOP,
  ASM_d_TEXT,
  ASM_d_DATA,
  ASM_d_WORD,
  ASM_d_BYTE,
  ASM_d_ASCII,
  ASM_d_ASCIIZ,
  ASM_d_SPACE

} AsmCode;

typedef enum {
  OP_REG,  // レジスタ
  OP_IMM,  // 即値
  OP_LABEL // ラベル（ジャンプ先）
} OperandType;

typedef struct {
  OperandType type;

  union {
    MipsReg reg; // type == OP_REG
    int imm;     // type == OP_IMM
    char *label; // type == OP_LABEL
  };
} Operand;

typedef enum {
  CODE_INSN,
  CODE_LABEL,
  CODE_FUNC_ENTER,     // 関数の開始
  CODE_PROLOGUE_END,   // プロローグ終了（＝本体開始）
  CODE_EPILOGUE_START, // エピローグ開始（＝本体終了）
  CODE_FUNC_LEAVE      // 関数の終了
} CodeKind;

typedef struct {
  AsmCode code;
  Operand op1, op2, op3;
} AsmInst;

typedef struct code_node {
  CodeKind kind;

  union {
    AsmInst insn;

    struct {
      char *name;
    } label;
  };

  struct code_node *next;
} Code;

typedef struct code_list {
  Code *head;
  Code *tail;
} CodeList;

int sizeof_type(VarType *t);
void var_table_init(VarTable *vt);
VarEntry *var_add(VarTable *vt, char *name);
VarEntry *var_add_array(VarTable *vt, char *name, VarType *vartype, int size);

void enter_scope(VarTable *vt);

void leave_scope(VarTable *vt);

VarEntry *var_find(VarTable *vt, const char *name);
// 表示
void print_var_table(FILE *fp, const VarTable *vt);

// 初期化
void init_code_list(CodeList *list);
// コードの追加
void append_code(CodeList *list, Code *node);

// コードリストの結合
void concat_code_list(CodeList *dst, const CodeList *src);

// 各形式の命令を作成する関数
Code *new_code_r(AsmCode code, MipsReg rd, MipsReg rs, MipsReg rt);
Code *new_code_i(AsmCode code, MipsReg rt, MipsReg rs, int imm);
Code *new_code_j(AsmCode code, const char *label);
Code *new_code_branch(AsmCode code, MipsReg rs, MipsReg rt, const char *label);

Code *new_code0(AsmCode code);

// ラッパー関数
Code *new_code(AsmCode code, Operand op1, Operand op2, Operand op3);

char *new_label_pref(const char *prefix);
Code *new_label(const char *name);

Code *new_code_la(MipsReg rt, const char *label);
Code *new_code_dir_i(AsmCode code, int imm);

Code *new_code_func_enter(char *name);

Code *new_code_func_leave();

Code *new_code_prologue_end();

Code *new_code_epilogue_start();

// アセンブリのコードを文字列へ変換する関数
const char *asmcode_to_string(AsmCode code);
