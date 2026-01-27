#include "internal.h"
#include "mips_code.h"
#include <stdio.h>
#include <stdlib.h>

// ノードのアドレスを計算し、そのアドレスが入ったレジスタ番号を返す
// reg: アドレス計算に使ってよいレジスタの開始番号
MipsReg gen_addr(CodeList *out, Node *node, MipsReg reg) {

  // 1. 単純な変数 (IDENT)
  if (node->id == ND_IDENT) {
    VarEntry *ent = var_find(&vartable, node->str);
    if (!ent) {
      fprintf(stderr, "error: undefined variable %s\n", node->str);
      exit(1);
    }
    if (ent->scope_id == 0) {
      // グローバル変数: LA命令を追加
      append_code(out, new_code_la(reg, ent->name));
      append_code(out, new_code0(ASM_NOP));
    } else {
      // ローカル変数
      append_code(out, new_code_i(ASM_ADDIU, reg, R_FP, ent->offset));
    }
    return reg;
  }

  // 2. ポインタの参照 (ptr^)
  if (node->id == ND_DEREF) {
    // 中身（ポインタ値＝アドレス）を評価するだけ
    Operand op = get_operand(out, node->node0, reg);
    if (op.type == OP_IMM) {
      imm2reg(out, op, reg);
      return reg;
    }
    return op.reg;
  }

  // 3. 配列参照 (REF)
  if (node->id == ND_REF) {
    // スタックに積んで内側から処理
    Node *refs[20];
    int depth = 0;
    Node *cur = node;
    while (cur->id == ND_REF) {
      refs[depth++] = cur;
      cur = cur->node0;
    }

    // ベース変数のアドレスを取得
    if (cur->id != ND_IDENT) {
      fprintf(stderr, "Array base must be IDENT\n");
      exit(1);
    }

    VarEntry *ent = var_find(&vartable, cur->str);
    if (!ent) {
      fprintf(stderr, "error: undefined variable %s\n", cur->str);
      exit(1);
    }
    VarType *type = ent->type;

    // --- ベースアドレスの決定 ---
    if (type->kind == VarArray) {
      if (ent->scope_id == 0) {
        // グローバル変数: LA命令を追加
        append_code(out, new_code_la(reg, ent->name));
        append_code(out, new_code0(ASM_NOP));
      } else {
        // ローカル配列: 実体がスタックにある -> FP + offset が配列の先頭
        append_code(out, new_code_i(ASM_ADDIU, reg, R_FP, ent->offset));
      }
    } else {
      // ポインタ(VarInt):
      // 実体は別の場所にあり、スタックにはアドレスが入っている
      // -> FP + offset の中身(アドレス)をロードする
      append_code(out, new_code_i(ASM_LW, reg, R_FP, ent->offset));
      append_code(out, new_code0(ASM_NOP));
    }

    // インデックス加算
    for (int k = depth - 1; k >= 0; k--) {
      Node *ref_node = refs[k];

      // インデックス値を reg+1 に計算
      Operand idx = get_operand(out, ref_node->node1, reg + 1);
      if (idx.type == OP_IMM)
        imm2reg(out, idx, reg + 1);

      // --- ストライド（幅）の決定 ---
      int stride;
      if (type->kind == VarArray) {
        stride = calc_stride(type);
        type = type->base; // 型を一段階掘り下げる
      } else {
        stride = 4;
      }

      // reg = reg + idx * stride
      if (stride == 4) {
        append_code(out, new_code_i(ASM_SLL, reg + 1, reg + 1, 2));
      } else {
        append_code(out, new_code_i(ASM_ORI, reg + 2, R_ZERO, stride));
        append_code(out, new_code_r(ASM_MULT, reg + 1, reg + 1, reg + 2));
        append_code(out, new_code_r(ASM_MFLO, reg + 1, reg + 1, reg + 1));
      }
      append_code(out, new_code_r(ASM_ADDU, reg, reg, reg + 1));
    }
    return reg;
  }

  fprintf(stderr, "error: not an lvalue %d\n", node->id);
  exit(1);
}
