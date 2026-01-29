#include "internal.h"
#include "mips_code.h"
#include <stdio.h>
#include <stdlib.h>

/* x が 2^n なら n を返す。そうでなければ -1 を返す */
int get_power_of_2(int x) {
  if (x <= 0)
    return -1;
  // ビットが1つだけ立っているかチェック: (x & (x-1)) == 0
  if ((x & (x - 1)) != 0)
    return -1;

  int n = 0;
  while ((x & 1) == 0) {
    x >>= 1;
    n++;
  }
  return n;
}

// ノードの評価に必要なレジスタ数（重み）を計算
int get_weight(Node *node) {
  if (!node)
    return 0;

  // 1. 即値・レジスタ変数は計算済みとみなす (重み0)
  if (node->id == ND_NUM)
    return 0;
  if (node->id == ND_IDENT) {
    VarEntry *ent = var_find(&vartable, node->str);
    if (ent && ent->reg_idx != -1)
      return 0; // レジスタ変数
  }

  // 2. 演算式: 左右の重みを比較
  if (node->id == ND_ARITH) {
    int w1 = get_weight(node->node0); // 左
    int w2 = get_weight(node->node1); // 右

    // Sethi-Ullmanの基本ルール:
    if (w1 == w2)
      return w1 + 1;
    return (w1 > w2) ? w1 : w2;
  }

  // 3. その他（メモリロードが必要な変数など）は最低1つ必要
  return 1;
}

Operand imm2reg(CodeList *out, Operand op, MipsReg reg) {
  if (op.type == OP_REG)
    return op; // 既にレジスタならそのまま
  append_code(out, new_code_i(ASM_ADDI, reg, R_ZERO, op.imm));
  return (Operand){OP_REG, reg};
}

// ノードを評価してオペランド(レジスタor即値)として返す
Operand get_operand(CodeList *out, Node *node, MipsReg reg) {
  Operand op = {OP_REG, reg};

  // 1. 即値 (数値リテラルなど)
  if (node->id == ND_NUM) {
    op.type = OP_IMM;
    op.imm = node->extra;
    return op;
  }

  // 2. アドレス取得 (@a)
  if (node->id == ND_ADDR) {
    // gen_addr は「アドレス」を計算してレジスタに入れる関数
    // @a は「aのアドレス」そのものが値なので、gen_addrの結果をそのまま使う
    op.reg = gen_addr(out, node->node0, reg);
    return op;
  }

  if (node->id == ND_IDENT) {
    VarEntry *ent = var_find(&vartable, node->str);
    if (ent && ent->reg_idx != -1) {
      // MOVE reg, $sX  (ADDU reg, $sX, $zero)
      // append_code(out, new_code_r(ASM_ADDU, reg, ent->reg_idx, R_ZERO));
      // return op;
      return (Operand){OP_REG, ent->reg_idx};
    }
  }

  // 3. 左辺値 (変数, 配列, ポインタ参照) の「値」のロード
  if (node->id == ND_IDENT || node->id == ND_REF || node->id == ND_DEREF) {
    // アドレスを計算
    MipsReg addr = gen_addr(out, node, reg);
    // そのアドレスから値をロード
    append_code(out, new_code_i(ASM_LW, reg, addr, 0));
    append_code(out, new_code0(ASM_NOP)); // 遅延スロット対策
    return op;
  }

  // 4. 演算式
  if (node->id == ND_ARITH) {
    expr_eval(out, node, reg);
    return op;
  }

  fprintf(stderr, "get_operand: unknown node type %d\n", node->id);
  exit(1);
}

// 演算式の評価
void expr_eval(CodeList *out, Node *node, MipsReg reg) {

  if (reg > R_T7) {
    fprintf(stderr, "Fatal Error: Register exhausted ($t0-$t7 used up).\n");
    fprintf(stderr, "Expression is too complex to compile without spilling.\n");
    exit(1);
  }

  // 重みを計算
  int w0 = get_weight(node->node0); // 左
  int w1 = get_weight(node->node1); // 右

  Operand op1, op2;

  if (w0 < w1) {

    // 1. 右辺を先に評価 (reg を使用)
    op2 = get_operand(out, node->node1, reg);
    if (op2.type == OP_IMM)
      op2 = imm2reg(out, op2, reg);

    // 2. 左辺を評価 (reg + 1 を使用)
    op1 = get_operand(out, node->node0, reg + 1);
    if (op1.type == OP_IMM)
      op1 = imm2reg(out, op1, reg + 1);
  } else {
    // 左辺
    op1 = get_operand(out, node->node0, reg);
    if (op1.type == OP_IMM)
      op1 = imm2reg(out, op1, reg);

    // 右辺 (レジスタを1つずらして評価)
    op2 = get_operand(out, node->node1, reg + 1);
  }

  // 結果の格納先レジスタ
  Operand dest = {OP_REG, reg};

  AsmCode asmc;
  int shift;

  switch (node->extra) {
  case OP_ADD:
    if (op2.type == OP_REG)
      asmc = ASM_ADDU;
    else
      asmc = ASM_ADDIU;
    append_code(out, new_code(asmc, dest, op1, op2));
    break;

  case OP_SUB:
    if (op2.type == OP_REG) {
      asmc = ASM_SUBU;
    } else {
      asmc = ASM_ADDIU;
      op2.imm *= -1; // SUBIはないのでADDIで負数を足す
    }
    append_code(out, new_code(asmc, dest, op1, op2));
    break;

  case OP_MUL:
    if (op2.type == OP_IMM) {
      if ((shift = get_power_of_2(op2.imm)) > 0) {
        append_code(out, new_code(ASM_SLL, dest, op1, op2));
      }
      op2 = imm2reg(out, op2, reg + 1);
    }
    append_code(out, new_code(ASM_MULTU, op1, op1, op2));
    append_code(out, new_code_r(ASM_MFLO, reg, reg, reg));
    break;

  case OP_DIV:
    if (op2.type == OP_IMM)
      op2 = imm2reg(out, op2, reg + 1);
    append_code(out, new_code(ASM_DIVU, op1, op1, op2));
    append_code(out, new_code_r(ASM_MFLO, reg, reg, reg));
    break;
  case OP_MOD: // 剰余 (%)
    if (op2.type == OP_IMM)
      op2 = imm2reg(out, op2, reg + 1);
    append_code(out, new_code(ASM_DIVU, op1, op1, op2));
    append_code(
        out, new_code_r(ASM_MFHI, reg, reg, reg)); // Hiレジスタから余りを取得
    break;

  case OP_AND: // &
    if (op2.type == OP_IMM)
      op2 = imm2reg(out, op2, reg + 1);
    append_code(out, new_code(ASM_AND, dest, op1, op2));
    break;

  case OP_OR: // |
    if (op2.type == OP_IMM)
      op2 = imm2reg(out, op2, reg + 1);
    append_code(out, new_code(ASM_OR, dest, op1, op2));
    break;

  case OP_XOR: // ^
    if (op2.type == OP_IMM)
      op2 = imm2reg(out, op2, reg + 1);
    append_code(out, new_code(ASM_XOR, dest, op1, op2));
    break;

  case OP_LSHIFT: // <<
    if (op2.type == OP_IMM)
      op2 = imm2reg(out, op2, reg + 1);
    append_code(out, new_code(ASM_SLLV, dest, op1, op2));
    break;

  case OP_RSHIFT: // >> (算術右シフト)
    if (op2.type == OP_IMM)
      op2 = imm2reg(out, op2, reg + 1);
    append_code(out, new_code(ASM_SRAV, dest, op1, op2));
    break;

  default:
    fprintf(stderr, "expr_eval: unknown op %d\n", node->extra);
    exit(1);
  }
}

// 汎用エントリポイント (結果は $t0 固定)
void gen_expr(CodeList *out, Node *node) {
  if (node->id == ND_ARITH) {
    expr_eval(out, node, R_T0);
  } else {
    // 単なる変数の場合などは get_operand で $t0 にロード
    Operand op = get_operand(out, node, R_T0);
    if (op.type == OP_IMM)
      imm2reg(out, op, R_T0);
    else if (op.reg != R_T0) {
      // もし違うレジスタに入ってたらmove
      append_code(out, new_code_r(ASM_ADDU, R_T0, op.reg, R_ZERO));
    }
  }
}
