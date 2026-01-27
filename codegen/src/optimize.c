#include "internal.h"
#include "mips_code.h"
#include <stdlib.h>

/* 分岐命令かどうか判定 */
static int is_branch(AsmCode code) {
  switch (code) {
  case ASM_BEQ:
  case ASM_BNE:
  case ASM_J:
  case ASM_JAL:
  case ASM_JR:
  case ASM_JALR:
    return 1;
  default:
    return 0;
  }
}

/* ロード命令かどうか判定 */
static int is_load(AsmCode code) {
  switch (code) {
  case ASM_LW:
  case ASM_LB:
    return 1;
  default:
    return 0;
  }
}

/* 命令が特定のレジスタを「読み取って」いるか判定する */
static int is_reg_used(AsmInst *insn, MipsReg reg) {
  if (reg == R_ZERO)
    return 0; // $zeroの読み取りは無視してよい

  switch (insn->code) {
    // --- 3オペランド (R形式): op1=dst, op2=src, op3=src ---
  case ASM_ADD:
  case ASM_ADDU:
  case ASM_SUB:
  case ASM_SUBU:
  case ASM_AND:
  case ASM_OR:
  case ASM_XOR:
  case ASM_NOR:
  case ASM_SLT:
  case ASM_SLTU:
  case ASM_MULT:
  case ASM_MULTU:
  case ASM_DIV:
  case ASM_DIVU:
  case ASM_SLLV:
  case ASM_SRLV:
  case ASM_SRAV:
    if (insn->op2.type == OP_REG && insn->op2.reg == reg)
      return 1;
    if (insn->op3.type == OP_REG && insn->op3.reg == reg)
      return 1;
    return 0;

    // --- 2オペランド (I形式): op1=dst, op2=src, op3=imm ---
    // addi rt, rs, imm -> op2 を読む
  case ASM_ADDI:
  case ASM_ADDIU:
  case ASM_ANDI:
  case ASM_ORI:
  case ASM_XORI:
  case ASM_SLTI:
  case ASM_SLTIU:
  case ASM_LW:
  case ASM_LB: // lw rt, off(base) -> base(op2) を読む
    if (insn->op2.type == OP_REG && insn->op2.reg == reg)
      return 1;
    return 0;

    // --- ストア命令: op1=src(val), op2=src(base) ---
    // sw rt, off(base) -> rt(op1) と base(op2) 両方を読む
  case ASM_SW:
  case ASM_SB:
    if (insn->op1.type == OP_REG && insn->op1.reg == reg)
      return 1;
    if (insn->op2.type == OP_REG && insn->op2.reg == reg)
      return 1;
    return 0;

    // --- 分岐命令: op1=src, op2=src ---
    // beq rs, rt, label
  case ASM_BEQ:
  case ASM_BNE:
    if (insn->op1.type == OP_REG && insn->op1.reg == reg)
      return 1;
    if (insn->op2.type == OP_REG && insn->op2.reg == reg)
      return 1;
    return 0;

    // --- ジャンプ (レジスタ指定) ---
    // jr rs -> op1 を読む
  case ASM_JR:
  case ASM_JALR:
    if (insn->op1.type == OP_REG && insn->op1.reg == reg)
      return 1;
    return 0;

    // --- シフト (即値): op1=dst, op2=src, op3=imm ---
    // sll rd, rt, shamt -> rt(op2) を読む
  case ASM_SLL:
  case ASM_SRL:
  case ASM_SRA:
    if (insn->op2.type == OP_REG && insn->op2.reg == reg)
      return 1;
    return 0;

  case ASM_SYSCALL:
    // syscallは $v0, $a0 等を暗黙に読む。
    return 1;

  default:
    return 1;
  }
}

/* NOP最適化 */
void optimize_nop(CodeList *list) {
  if (!list || !list->head)
    return;

  Code *prev_node = NULL;      // リスト操作用の1つ前のノード
  Code *last_insn_node = NULL; // 直近の「命令」ノード (ラベルはスキップ)
  Code *cur = list->head;

  while (cur != NULL) {
    int remove = 0;

    // 現在のノードが NOP 命令かどうか
    if (cur->kind == CODE_INSN && cur->insn.code == ASM_NOP) {
      remove = 1;

      if (last_insn_node != NULL) {
        // 1. 直前が分岐命令なら、遅延スロットなので消さない
        if (is_branch(last_insn_node->insn.code)) {
          remove = 0;
        }
        // 2. 直前がロード命令なら、依存関係をチェック
        else if (is_load(last_insn_node->insn.code)) {
          // ロード先レジスタ (LW rt, ... なので op1 が dst)
          MipsReg load_dst = last_insn_node->insn.op1.reg;

          // 次の命令(ラベルはスキップ)
          Code *next_insn = cur->next;
          while (next_insn != NULL && next_insn->kind == CODE_LABEL) {
            next_insn = next_insn->next;
          }

          if (next_insn != NULL && next_insn->kind == CODE_INSN) {
            // 次の命令がロード先を使っているなら、NOPが必要
            if (is_reg_used(&next_insn->insn, load_dst)) {
              remove = 0;
            }
          }
          // もし次の命令がなければ(関数末尾など)、NOPは不要なので remove=1
          // のまま
        }
      } else {
        // 直前の命令がない（関数の先頭など）なら、NOPは不要
        remove = 1;
      }
    }

    if (remove) {
      // リストから削除
      Code *next = cur->next;

      if (prev_node) {
        prev_node->next = next;
      } else {
        list->head = next;
      }

      if (cur == list->tail) {
        list->tail = prev_node;
      }

      free(cur);
      cur = next; // prev_node は更新しない
    } else {
      // 削除しない場合

      if (cur->kind == CODE_INSN) {
        last_insn_node = cur;
      }

      prev_node = cur;
      cur = cur->next;
    }
  }
}

/* レジスタがこの後で読まれるか */
static int is_reg_read_later(Code *start_node, MipsReg reg) {
  for (Code *cur = start_node; cur != NULL; cur = cur->next) {
    if (cur->kind != CODE_INSN)
      continue;

    // この命令で読まれているか？
    if (is_reg_used(&cur->insn, reg))
      return 1;

    // この命令で「上書き」されたら、そこから先は古い値は不要なので「読まれていない」と判定
    AsmInst *insn = &cur->insn;
    switch (insn->code) {
    case ASM_ADD:
    case ASM_ADDU:
    case ASM_SUB:
    case ASM_SUBU:
    case ASM_AND:
    case ASM_OR:
    case ASM_XOR:
    case ASM_NOR:
    case ASM_SLT:
    case ASM_SLTU:
    case ASM_SLLV:
    case ASM_SRLV:
    case ASM_SRAV:
      if (insn->op1.type == OP_REG && insn->op1.reg == reg)
        return 0; // 上書きされた
      break;
    // div/multなどは HI/LO への書き込みなので注意（ここでは無視）
    default:
      break;
    }

    // 分岐はめんどくさいから読まれるということで
    if (is_branch(insn->code))
      return 1;
  }
  return 0; // 最後まで読まれなかった
}

/* アドレス計算の統合: ADDIU rd, rs, imm -> LW/SW rt, 0(rd)  ==> LW/SW rt,
 * imm(rs) */
void optimize_address(CodeList *list) {
  if (!list || !list->head)
    return;

  Code *prev = NULL;
  Code *cur = list->head;

  while (cur != NULL) {
    Code *next = cur->next;

    // ラベル等はスキップして次の命令を探す
    while (next && next->kind != CODE_INSN) {
      next = next->next;
    }

    if (cur->kind == CODE_INSN && next != NULL && next->kind == CODE_INSN) {
      AsmInst *i1 = &cur->insn;
      AsmInst *i2 = &next->insn;

      // パターンマッチ:
      // 1. i1 が ADDIU (rd = rs + imm)
      // 2. i2 が LW/SW (rt = Mem[base + off])
      int is_addiu = (i1->code == ASM_ADDIU || i1->code == ASM_ADDI);
      int is_load_store = (i2->code == ASM_LW || i2->code == ASM_SW ||
                           i2->code == ASM_LB || i2->code == ASM_SB);

      if (is_addiu && is_load_store) {
        // レジスタの一致確認: i1の出力(op1) == i2のベース(op2)
        // i1: op1=rd, op2=rs, op3=imm
        // i2: op1=rt, op2=base, op3=offset

        if (i1->op1.type == OP_REG && i2->op2.type == OP_REG &&
            i1->op1.reg == i2->op2.reg) {

          MipsReg rd = i1->op1.reg; // 中間レジスタ ($t1など)

          // オフセット計算: i1のimm + i2のoffset
          int new_offset = i1->op3.imm + i2->op3.imm;

          // 16bitに収まるか？ (-32768 ~ 32767)
          if (new_offset >= -32768 && new_offset <= 32767) {

            int is_lw_addr_use = (i2->code == ASM_LW || i2->code == ASM_LB) &&
                                 next->next->kind == CODE_INSN &&
                                 next->next->insn.code == ASM_NOP;
            if (is_lw_addr_use || !is_reg_read_later(next->next, rd)) {

              // i2 (LW/SW) のベースを i1のrs ($fp) に書き換え
              i2->op2 = i1->op2;

              // i2 (LW/SW) のオフセットを 合計値 に書き換え
              i2->op3.imm = new_offset;

              // i1 (ADDIU) を削除
              Code *to_delete = cur;

              if (prev)
                prev->next = next; // prev -> i2
              else
                list->head = next;

              cur = next;

              free(to_delete);
              continue;
            }
          }
        }
      }
    }

    prev = cur;
    cur = cur->next;
  }
}
