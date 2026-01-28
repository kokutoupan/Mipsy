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
      if (ent->scope_id == 0) {
        // グローバルポインタ:
        // 1. ポインタ変数自体の場所を取得 (la)
        // 2. その中身（指し先のアドレス）をロード (lw)
        append_code(out, new_code_la(reg, ent->name)); // reg = &p
        append_code(out, new_code0(ASM_NOP));
        append_code(out, new_code_i(ASM_LW, reg, reg, 0)); // reg = *p
        append_code(out, new_code0(ASM_NOP));
      } else {
        // ローカルポインタ:
        // スタック上の変数の中身をロード or レジスタ参照
        if (ent->reg_idx == -1) {
          append_code(out, new_code_i(ASM_LW, reg, R_FP, ent->offset));
          append_code(out, new_code0(ASM_NOP));
        } else {
          append_code(out, new_code_r(ASM_ADDU, reg, ent->reg_idx, R_ZERO));
        }
      }
    }

    // 定数のオフセット
    int const_offset = 0;
    // インデックス加算ループ
    for (int k = depth - 1; k >= 0; k--) {
      Node *ref_node = refs[k];
      Node *idx_node = ref_node->node1; // インデックスのノード

      // --- ストライド計算と型更新 ---
      int stride;
      if (type->kind == VarArray) {
        stride = calc_stride(type);
        type = type->base;
      } else {
        stride = 4;
      }

      // 定数(ND_NUM)なら、コード生成せずにオフセットに足し込むだけ！
      if (idx_node->id == ND_NUM) {
        const_offset += idx_node->extra * stride;
        continue; // 次の次元へ
      }

      // 変数の場合はコード生成して加算
      Operand idx = get_operand(out, idx_node, reg + 1);
      if (idx.type == OP_IMM) {
        const_offset += idx.imm * stride;
        continue;
      }

      // 2の累乗最適化 (Shift vs Mult)
      int shift = get_power_of_2(stride);
      if (shift >= 0) {
        if (shift > 0) {
          append_code(out, new_code_i(ASM_SLL, reg + 1, idx.reg, shift));
        }
        if (shift == 0 && idx.reg != reg + 1) {
          append_code(out, new_code_r(ASM_ADDU, reg + 1, idx.reg, R_ZERO));
        }
      } else {
        append_code(out, new_code_i(ASM_ORI, reg + 2, R_ZERO, stride));
        append_code(out, new_code_r(ASM_MULT, idx.reg, idx.reg, reg + 2));
        append_code(out, new_code_r(ASM_MFLO, reg + 1, reg + 1, reg + 1));
      }

      // ベースに加算
      append_code(out, new_code_r(ASM_ADDU, reg, reg, reg + 1));
    }

    // 最後に累積した定数オフセットを加算
    if (const_offset != 0) {
      // 16bit即値に収まるなら ADDIU
      if (const_offset >= -32768 && const_offset <= 32767) {
        append_code(out, new_code_i(ASM_ADDIU, reg, reg, const_offset));
      } else {
        // 大きすぎる場合は LI を使う
        append_code(out, new_code_i(ASM_LI, reg + 1, R_ZERO, const_offset));
        append_code(out, new_code_r(ASM_ADDU, reg, reg, reg + 1));
      }
    }

    return reg;
  }

  fprintf(stderr, "error: not an lvalue %d\n", node->id);
  exit(1);
}
