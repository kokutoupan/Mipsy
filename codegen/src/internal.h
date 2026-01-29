#pragma once
#include "../include/types.h" // パスは適宜調整
#include "mips_code.h"
#include "mips_reg.h"

// グローバル変数の共有
extern VarTable vartable;
extern CodeList codeList;

extern int opt_optimize;
extern int opt_debug;

// アドレス計算 (gen_addr.c)
MipsReg gen_addr(CodeList *out, Node *node, MipsReg reg);

// 式の評価 (gen_expr.c)
Operand get_operand(CodeList *out, Node *node, MipsReg reg);
Operand imm2reg(CodeList *out, Operand op, MipsReg reg);
void gen_expr(CodeList *out, Node *node);
void expr_eval(CodeList *out, Node *node, MipsReg reg);

// 文の生成 (gen_stmt.c)
void gen_stmt(CodeList *out, Node *node);
void gen_stmt_list(CodeList *out, Node *stmt_list);
void gen_assign(CodeList *out, Node *node);
void gen_if(CodeList *out, Node *node);
void gen_while(CodeList *out, Node *node);
Operand gen_call(CodeList *out, Node *node, MipsReg reg);

// --- generate.c ---
void def_variable(Node *node,int is_reg);
void def_var_array(Node *node);
int calc_stride(VarType *t);

int get_power_of_2(int x);
