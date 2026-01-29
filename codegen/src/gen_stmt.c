#include "internal.h"
#include "mips_reg.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>

static char *current_break_label = NULL;

// 条件式の評価
void cond_eval(CodeList *out, Node *node, char *t_label, MipsReg reg,
               int invert) {
  // 左辺
  Operand op1 = get_operand(out, node->node0, reg);
  if (op1.type == OP_IMM)
    op1 = imm2reg(out, op1, reg);

  // 右辺 (reg+1 を作業用に使用)
  Operand op2 = get_operand(out, node->node1, reg + 1);

  // == の最適化 (BEQ命令)
  if (node->extra == OP_EQ || node->extra == OP_NEQ) {
    if (op2.type == OP_IMM)
      op2 = imm2reg(out, op2, reg + 1);

    AsmCode code;
    if (node->extra == OP_EQ)
      code = (invert) ? ASM_BNE : ASM_BEQ;
    else
      code = (invert) ? ASM_BEQ : ASM_BNE;

    append_code(out, new_code_branch(code, op1.reg, op2.reg, t_label));
    append_code(out, new_code0(ASM_NOP));
    return;
  }

  // 大小比較 (<, <=, >, >=)
  // slt rd, rs, rt  =>  if (rs < rt) rd=1 else rd=0

  Operand slt_op1 = op1;
  Operand slt_op2 = op2;
  int check_eq_zero = 0; // 結果が0(偽)なら分岐するか、1(真)なら分岐するか

  /*
   * 条件   | SLTでの表現      | 真のときSLT結果 | 偽でJmp(inv=1) |
   * -----------------------------------------------------------------------
   * A < B  | slt t, A, B     | 1               | beq t, zero    | bne t, zero
   * A > B  | slt t, B, A     | 1               | beq t, zero    | bne t, zero
   * A <= B | ! (B < A)       | 0 (B<Aが偽)     | bne t, zero    | beq t, zero
   * A >= B | ! (A < B)       | 0 (A<Bが偽)     | bne t, zero    | beq t, zero
   */

  switch (node->extra) {
  case OP_LT: // <
    check_eq_zero = (invert) ? 1 : 0;
    break;
  case OP_GT: // >
    slt_op1 = op2;
    slt_op2 = op1;
    check_eq_zero = (invert) ? 1 : 0;
    break;
  case OP_LEQ: // <=
    slt_op1 = op2;
    slt_op2 = op1;
    check_eq_zero = (invert) ? 0 : 1;
    break;
  case OP_GEQ: // >=
    check_eq_zero = (invert) ? 0 : 1;
    break;
  }

  // 即値対応 (SLTI)
  if (slt_op2.type == OP_IMM) {
    append_code(out, new_code_i(ASM_SLTI, reg, slt_op1.reg, slt_op2.imm));
  } else {
    append_code(out,
                new_code(ASM_SLT, (Operand){OP_REG, reg}, slt_op1, slt_op2));
  }

  // 分岐生成
  AsmCode branch_code = (check_eq_zero) ? ASM_BEQ : ASM_BNE;
  append_code(out, new_code_branch(branch_code, reg, R_ZERO, t_label));
  append_code(out, new_code0(ASM_NOP));
}

void gen_if(CodeList *out, Node *node) {
  if (node->id == ND_IF_ELSE) {
    char *e_label = new_label_pref("IF_ELSE");
    char *end_label = new_label_pref("IF_END");

    // 偽なら Else へ
    cond_eval(out, node->node0, e_label, R_T0, 1);

    // true
    gen_stmt_list(out, node->node1);
    append_code(out, new_code_j(ASM_J, end_label));
    append_code(out, new_code0(ASM_NOP));

    // Else Block
    append_code(out, new_label(e_label));
    gen_stmt_list(out, node->node2);

    append_code(out, new_label(end_label));

  } else {
    // Elseなし
    char *end_label = new_label_pref("IF_END");

    // 偽なら End へ
    cond_eval(out, node->node0, end_label, R_T0, 1);

    // Then Block
    gen_stmt_list(out, node->node1);

    append_code(out, new_label(end_label));
  }
}

void gen_break(CodeList *out, Node *node) {
  if (current_break_label == NULL) {
    fprintf(stderr, "error: break statement not within loop\n");
    exit(1);
  }
  // 脱出先へジャンプ
  append_code(out, new_code_j(ASM_J, current_break_label));
  append_code(out, new_code0(ASM_NOP));
}

void gen_while(CodeList *out, Node *node) {
  char *loop_cond = new_label_pref("loop_cond");
  char *loop_head = new_label_pref("loop_head");
  char *loop_end = new_label_pref("loop_end"); //  脱出先ラベル

  // まず条件判定へジャンプ
  append_code(out, new_code_j(ASM_J, loop_cond));
  append_code(out, new_code0(ASM_NOP));

  // ループ先頭ラベル
  append_code(out, new_label(loop_head));

  char *prev_break = current_break_label;
  current_break_label = loop_end;
  // 本文
  gen_stmt_list(out, node->node1);

  // 条件判定
  append_code(out, new_label(loop_cond));
  cond_eval(out, node->node0, loop_head, R_T0, 0);
  append_code(out, new_label(loop_end));
  current_break_label = prev_break;
}

void gen_assign(CodeList *out, Node *node) {
  if (node == NULL || node->id != ND_ASSIGN)
    return;

  // 左辺が変数(IDENT)かどうか
  if (node->node0->id == ND_IDENT) {
    VarEntry *ent = var_find(&vartable, node->node0->str);
    // レジスタ
    if (ent && ent->reg_idx != -1) {
      // 右辺を評価して $t0 へ
      Operand rhs = get_operand(out, node->node1, R_T0);
      if (rhs.type == OP_IMM)
        imm2reg(out, rhs, R_T0);
      else if (rhs.reg != R_T0) {
        append_code(out, new_code_r(ASM_ADDU, R_T0, rhs.reg, R_ZERO));
      }

      // MOVE $sX, $t0
      append_code(out, new_code_r(ASM_ADDU, ent->reg_idx, R_T0, R_ZERO));
      return;
    }
  }

  // 1. 右辺値を評価 ($t0)
  Operand rhs = get_operand(out, node->node1, R_T0);
  if (rhs.type == OP_IMM)
    imm2reg(out, rhs, R_T0);
  else if (rhs.reg != R_T0) {
    // 右辺が他のレジスタにある場合、$t0に持ってくる
    append_code(out, new_code_r(ASM_ADDU, R_T0, rhs.reg, R_ZERO));
  }

  // 2. 左辺値のアドレスを計算 ($t1)
  // gen_addr はアドレス計算だけを行い、そのアドレスが入ったレジスタを返す
  MipsReg addr = gen_addr(out, node->node0, R_T1);

  // 3. ストア (SW $t0, 0($t1))
  append_code(out, new_code_i(ASM_SW, R_T0, addr, 0));
}

void gen_call(CodeList *out, Node *node, MipsReg reg) {
  // node->str : 関数名
  // node->node0 : 引数リスト (ND_ARGS または 単体)

  // 引数レジスタ $a0 ~ $a3
  MipsReg arg_regs[] = {R_A0, R_A1, R_A2, R_A3};
  int arg_idx = 0;

  Node *cur = node->node0; // 引数の先頭

  // 引数リストを走査して $a0... にセット
  while (cur != NULL) {
    if (arg_idx >= 4) {
      fprintf(stderr, "error: too many arguments (max 4)\n");
      exit(1);
    }

    // 引数ノードを取り出す
    Node *arg_expr = NULL;
    if (cur->id == ND_ARGS) {
      arg_expr = cur->node0; // ND_ARGSなら左側が実引数
      // cur->node1 が次のリスト
    } else {
      // リストの最後、または単体の引数
      arg_expr = cur;
    }

    // 引数を評価して、一旦 reg (作業用) に入れる
    // いきなり $a0 に入れないのは、式の評価中に $a0 を使う可能性があるため
    Operand op = get_operand(out, arg_expr, reg);

    if (op.type == OP_IMM) {
      // 即値ならロード: li $a0, 10
      append_code(out, new_code_i(ASM_ADDI, arg_regs[arg_idx], R_ZERO, op.imm));
    } else {
      // レジスタなら移動: move $a0, reg
      append_code(out, new_code_r(ASM_ADDU, arg_regs[arg_idx], op.reg, R_ZERO));
    }

    arg_idx++;

    // 次の引数へ
    if (cur->id == ND_ARGS) {
      cur = cur->node1;
    } else {
      break;
    }
  }

  // 関数呼び出し (JAL label)
  append_code(out, new_code_j(ASM_JAL, node->str));
  append_code(out, new_code0(ASM_NOP)); // 遅延スロット
}

// 文の生成
void gen_stmt(CodeList *out, Node *node) {
  if (node == NULL)
    return;

  switch (node->id) {
  case ND_ASSIGN:
    gen_assign(out, node);
    break;
  case ND_IF:
  case ND_IF_ELSE:
    gen_if(out, node);
    break;
  case ND_WHILE:
    gen_while(out, node);
    break;
  case ND_BLOCK:
    gen_stmt_list(out, node);
    break;
  case ND_FUNC_CALL:
    gen_call(out, node, R_T0);
    break;
  case ND_BREAK:
    gen_break(out, node);
    break;
  default:
    fprintf(stderr, "unknown stmt id: %d\n", node->id);
    break;
  }
}

void gen_stmt_list(CodeList *out, Node *stmt_list) {
  if (stmt_list == NULL)
    return;

  if (stmt_list->id != ND_BLOCK) {
    gen_stmt(out, stmt_list);
    return;
  }

  gen_stmt(out, stmt_list->node0);
  gen_stmt_list(out, stmt_list->node1);
}
