#include "internal.h"
#include "mips_code.h"
#include "mips_print.h"
#include "mips_reg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* その命令が reg を「読む（使用する）」か判定 */
static int is_reg_used(AsmInst *insn, MipsReg reg) {
  if (reg == R_ZERO)
    return 0;

  switch (insn->code) {
  // 3オペランド (R形式): op2, op3 を読む
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

  // 2オペランド (I形式): op2 を読む
  case ASM_ADDI:
  case ASM_ADDIU:
  case ASM_ANDI:
  case ASM_ORI:
  case ASM_XORI:
  case ASM_SLTI:
  case ASM_SLTIU:
  case ASM_LW:
  case ASM_LB:
    if (insn->op2.type == OP_REG && insn->op2.reg == reg)
      return 1;
    return 0;

  // ストア: op1(値), op2(アドレス) 両方を読む
  case ASM_SW:
  case ASM_SB:
    if (insn->op1.type == OP_REG && insn->op1.reg == reg)
      return 1;
    if (insn->op2.type == OP_REG && insn->op2.reg == reg)
      return 1;
    return 0;

  // 分岐: op1, op2 を読む
  case ASM_BEQ:
  case ASM_BNE:
    if (insn->op1.type == OP_REG && insn->op1.reg == reg)
      return 1;
    if (insn->op2.type == OP_REG && insn->op2.reg == reg)
      return 1;
    return 0;

  // ジャンプ(Register): op1 を読む
  case ASM_JR:
  case ASM_JALR:
    if (insn->op1.type == OP_REG && insn->op1.reg == reg)
      return 1;
    return 0;

  // シフト(Imm): op2 を読む
  case ASM_SLL:
  case ASM_SRL:
  case ASM_SRA:
    if (insn->op2.type == OP_REG && insn->op2.reg == reg)
      return 1;
    return 0;

  // 即値ロード系 & NOP & Jump: レジスタを読まない
  case ASM_LI:
  case ASM_LUI:
  case ASM_LA:
  case ASM_NOP:
  case ASM_J:
  case ASM_JAL:
    return 0;

  case ASM_SYSCALL:
    return 1;

  default:
    return 1; // 未知の命令は安全側に倒す
  }
}

/* 命令が書き込むレジスタを取得 (-1 if none) */
static MipsReg get_def_reg(AsmInst *insn) {
  if (insn->op1.type == OP_REG) {
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
    case ASM_ADDI:
    case ASM_ADDIU:
    case ASM_ANDI:
    case ASM_ORI:
    case ASM_XORI:
    case ASM_SLTI:
    case ASM_SLTIU:
    case ASM_LW:
    case ASM_LB:
    case ASM_LUI:
    case ASM_LA:
    case ASM_LI:
    case ASM_MFHI:
    case ASM_MFLO:
    case ASM_SLL:
    case ASM_SRL:
    case ASM_SRA:
      return insn->op1.reg;
    default:
      return -1;
    }
  }
  return -1;
}

/* 遅延スロット充填 (Delay Slot Filling)
 */
void optimize_delay_slot(CodeList *list) {
  if (!list || !list->head)
    return;

  Code *second_last_insn = NULL; // 2つ前の「命令」
  Code *last_insn = NULL;        // 1つ前の「命令」

  Code *prev = NULL; // リスト上の直前のノード (ラベル含む)
  Code *cur = list->head;

  while (cur != NULL) {
    int processed = 0;

    // Branch かつ 直後が NOP かチェック
    if (cur->kind == CODE_INSN && is_branch(cur->insn.code)) {
      Code *delay_slot = cur->next;

      if (delay_slot && delay_slot->kind == CODE_INSN &&
          delay_slot->insn.code == ASM_NOP) {

        // 直前のノード(prev)が命令であり、かつそれが last_insn と一致するか？
        if (prev && prev->kind == CODE_INSN && prev == last_insn) {
          AsmInst *b_insn = &cur->insn;  // Branch
          AsmInst *p_insn = &prev->insn; // Prev

          // 1. Prev は分岐・ジャンプ命令であってはならない
          if (!is_branch(p_insn->code) && p_insn->code != ASM_SYSCALL) {

            // 2. 依存関係チェック
            MipsReg p_def = get_def_reg(p_insn);
            int safe = 1;

            // Check A: Prev が Branch の条件レジスタを書き換えていないか？
            if (p_def != -1) {
              if (is_reg_used(b_insn, p_def))
                safe = 0;
            }

            // Check B: JAL/JALR の場合
            if (b_insn->code == ASM_JAL || b_insn->code == ASM_JALR) {
              // Prev が $ra を読んでいないか？
              if (is_reg_used(p_insn, R_RA))
                safe = 0;
            }

            // Check C: Load Delay (Prevがロードなら移動禁止)
            if (is_load(p_insn->code))
              safe = 0;

            // Check D: Prev
            // が既に誰か(second_last_insn)の遅延スロットなら移動禁止
            if (second_last_insn && is_branch(second_last_insn->insn.code)) {
              safe = 0;
            }

            if (safe) {
              // 入れ替え実行
              // 構造: [PrevPrev] -> [Prev] -> [Branch(cur)] -> [NOP] -> [Next]

              // 1. delay_slot (NOP) に Prev の内容をコピー
              delay_slot->insn = prev->insn;

              // 2. Prev を NOP にする
              prev->insn.code = ASM_NOP;

              second_last_insn = cur; // Branch
              last_insn = delay_slot; // Prev(Moved)

              prev = delay_slot;
              cur = delay_slot->next;
              processed = 1;
            }
          }
        }
      }
    }

    if (!processed) {
      if (cur->kind == CODE_INSN) {
        second_last_insn = last_insn;
        last_insn = cur;
      }
      prev = cur;
      cur = cur->next;
    }
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

/* NOP最適化 */
int optimize_nop_range(Code *start, Code *end) {
  if (!start || !end)
    return 0;

  int is_change = 0;
  Code *prev_node = NULL;      // リスト操作用の1つ前のノード
  Code *last_insn_node = NULL; // 直近の「命令」ノード (ラベルはスキップ)
  Code *cur = start;

  while (cur != NULL && cur != end) {
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
      }

      free(cur);
      cur = next; // prev_node は更新しない
      is_change = 1;
    } else {
      // 削除しない場合

      if (cur->kind == CODE_INSN) {
        last_insn_node = cur;
      }

      prev_node = cur;
      cur = cur->next;
    }
  }
  return is_change;
}

// 加算(ADDIU/ADDI/ADDU+zero)なら値を返す。それ以外ならフラグを下ろす
static int get_add_imm(AsmInst *insn, int *is_valid) {
  if (insn->code == ASM_ADDIU || insn->code == ASM_ADDI) {
    *is_valid = 1;
    return insn->op3.imm;
  }
  if (insn->code == ASM_ADDU && insn->op3.type == OP_REG &&
      insn->op3.reg == R_ZERO) {
    *is_valid = 1;
    return 0; // +0 とみなす
  }
  *is_valid = 0;
  return 0;
};

static int is_control_flow(Code *c) {
  if (c->kind == CODE_LABEL || c->kind == CODE_FUNC_ENTER ||
      c->kind == CODE_FUNC_LEAVE || c->kind == CODE_PROLOGUE_END ||
      c->kind == CODE_EPILOGUE_START)
    return 1;

  if (c->kind == CODE_INSN) {
    AsmCode code = c->insn.code;
    // 分岐、ジャンプ、関数呼び出し、リターン
    if (code == ASM_BEQ || code == ASM_BNE || code == ASM_J ||
        code == ASM_JAL || code == ASM_JALR || code == ASM_JR) {
      return 1;
    }
  }
  return 0;
}

/* 指定範囲内でレジスタが使用（リード）されているか判定
 */
static int is_reg_read_in_range(Code *start, Code *end, MipsReg reg) {
  // $t0-$t7 (Caller Saved) かどうか
  int is_stric = !(reg >= R_T0 && reg <= R_T7);

  // FPとかは、だめ
  if (reg == R_FP || reg == R_SP) {
    return 1;
  }

  for (Code *cur = start; cur != end && cur != NULL; cur = cur->next) {

    // エピローグの開始に到達したら、そこでスコープ終了とみなす（読まれない）
    if (cur->kind == CODE_EPILOGUE_START) {
      return 0;
    }

    // ラベルは無視して次の命令へ（リニアスキャンを継続）
    if (cur->kind == CODE_LABEL || cur->kind == CODE_FUNC_ENTER ||
        cur->kind == CODE_FUNC_LEAVE || cur->kind == CODE_PROLOGUE_END) {
      continue;
    }

    // 命令以外はスキップ
    if (cur->kind != CODE_INSN) {
      continue;
    }

    // この命令で読まれているか？
    if (is_reg_used(&cur->insn, reg)) {
      return 1;
    }

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
    case ASM_ADDI:
    case ASM_ADDIU:
    case ASM_ANDI:
    case ASM_ORI:
    case ASM_XORI:
    case ASM_SLTI:
    case ASM_SLTIU:
    case ASM_LW:
    case ASM_LB:
    case ASM_MFLO:
    case ASM_MFHI:
      // op1 が上書き対象
      if (insn->op1.type == OP_REG && insn->op1.reg == reg)
        return 0;
      break;

    // div/mult, syscall などはここでは上書きとみなさない（安全側）
    default:
      break;
    }

    // 分岐・ジャンプの判定
    if (is_branch(insn->code)) {
      // ジャンプ命令群 (J, JAL, JALR, JR)
      int is_jump = (insn->code == ASM_J || insn->code == ASM_JAL ||
                     insn->code == ASM_JALR || insn->code == ASM_JR);

      // 一時レジスタ($t系)かつジャンプなら、その先では破棄されるとみなす -> 0
      // それ以外（条件分岐や、$s系レジスタ）は、合流先で読まれる可能性あり -> 1
      // (Unknown/Unsafe)
      if (!is_stric && is_jump) {
        return 0;
      } else {
        return 1;
      }
    }
  }

  return 0; // 最後まで読まれなかった
}

/* アドレス計算の統合: ADDIU rd, rs, imm -> LW/SW rt, 0(rd)  ==> LW/SW rt,
 * imm(rs) */
int optimize_address_range(Code *start, Code *end) {
  if (!start || !end)
    return 0;

  Code *prev = NULL;
  Code *cur = start;

  int is_change = 0;
  while (cur != NULL && cur != end) {
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
        // さらに,7版より上(= T0から)

        if (i1->op1.type == OP_REG && i2->op2.type == OP_REG &&
            i1->op1.reg == i2->op2.reg && i1->op1.reg > 7) {

          MipsReg rd = i1->op1.reg; // 中間レジスタ ($t1など)

          // オフセット計算: i1のimm + i2のoffset
          int new_offset = i1->op3.imm + i2->op3.imm;

          // 16bitに収まるか？ (-32768 ~ 32767)
          if (new_offset >= -32768 && new_offset <= 32767) {

            int is_lw_addr_use =
                is_load(i2->code) && (i2->op2.reg == i2->op1.reg) &&
                next->next != NULL && next->next->kind == CODE_INSN &&
                next->next->insn.code == ASM_NOP;
            if (is_lw_addr_use || !is_reg_read_in_range(next->next, end, rd)) {

              // i2 (LW/SW) のベースを i1のrs ($fp) に書き換え
              i2->op2 = i1->op2;

              // i2 (LW/SW) のオフセットを 合計値 に書き換え
              i2->op3.imm = new_offset;

              // i1 (ADDIU) を削除
              Code *to_delete = cur;

              prev->next = cur->next;

              free(to_delete);
              is_change = 1;
              continue;
            }
          }
        }
      }
    }

    prev = cur;
    cur = cur->next;
  }

  return is_change;
}

/* Load -> Move の統合
 * LW rt, off(base)
 * NOP
 * ADDU rd, rt, $zero (move)
 *
 * ==> LW rd, off(base)
 */
int optimize_load_move_range(Code *start, Code *end) {
  if (!start || !end)
    return 0;

  Code *cur = start;
  int is_change = 0;

  while (cur != NULL) {
    // LW
    if (cur->kind == CODE_INSN && is_load(cur->insn.code)) {

      Code *nop = cur->next;
      Code *move = (nop) ? nop->next : NULL;

      // NOP,MOVE判定
      if (nop && nop->kind == CODE_INSN && nop->insn.code == ASM_NOP && move &&
          move->kind == CODE_INSN) {

        AsmInst *i_lw = &cur->insn;
        AsmInst *i_mv = &move->insn;

        // ムーブ命令か判定 (ADDU rd, rs, $zero とか)
        int is_move = 0;
        MipsReg dst_reg, src_reg;

        // addu rd, rs, zero
        if (i_mv->code == ASM_ADDU && i_mv->op3.reg == R_ZERO) {
          dst_reg = i_mv->op1.reg;
          src_reg = i_mv->op2.reg;
          is_move = 1;
        }
        // or rd, rs, zero
        else if (i_mv->code == ASM_OR && i_mv->op3.reg == R_ZERO) {
          dst_reg = i_mv->op1.reg;
          src_reg = i_mv->op2.reg;
          is_move = 1;
        }

        if (is_move && i_lw->op1.reg == src_reg) {

          // MoveのdstがLWのbaseでない場合のみ適用
          int base_reg_conflict =
              (i_lw->op2.type == OP_REG && i_lw->op2.reg == dst_reg);

          if (!base_reg_conflict &&
              !is_reg_read_in_range(move->next, end, src_reg)) {

            // LW の出力先を Moveのdst に書き換え
            i_lw->op1.reg = dst_reg;

            // Move命令を削除
            nop->next = move->next;
            free(move);

            cur = nop;
            is_change = 1;
            continue;
          }
        }
      }
    }
    cur = cur->next;
  }
  return is_change;
}

/* 連続する即値加算の統合 (Chain Optimization)
 * 1. ADDIU rd, rs, imm1
 * 2. ADDIU rd, rd, imm2
 * ==> ADDIU rd, rs, (imm1 + imm2)
 */
int optimize_addiu_chain_range(Code *head, Code *end) {
  if (!head || !end)
    return 0;

  Code *prev = NULL;
  Code *cur = head;

  int is_change = 0;

  while (cur != NULL && cur != end) {
    Code *next = cur->next;

    // cur と next が共に ADDIU (または ADDI) かチェック
    if (cur->kind == CODE_INSN && next != NULL && next->kind == CODE_INSN) {
      AsmInst *i1 = &cur->insn;
      AsmInst *i2 = &next->insn;

      int valid1, valid2;
      int imm1 = get_add_imm(i1, &valid1);
      int imm2 = get_add_imm(i2, &valid2);

      if (valid1 && valid2) {
        // パターンマッチ:
        // i1: rd = rs + imm1
        // i2: rd = rd + imm2(or $zero)

        MipsReg rd1 = i1->op1.reg;
        // i1 の出力先(rd1) を i2 が 入出力(rd2, rs2) として使っているか
        if (i2->op2.reg == rd1 &&
            (i2->op1.reg == rd1 ||
             !is_reg_read_in_range(next->next, end, rd1))) {

          int new_imm = imm1 + imm2;

          // 16bit オーバーフローチェック
          if (new_imm >= -32768 && new_imm <= 32767) {
            if (i1->code == ASM_ADDU) {
              i1->code = ASM_ADDIU;
              i1->op3.type = OP_IMM; // レジスタオペランドを即値オペランドに変更
            }

            i1->op3.imm = new_imm;

            // 出力に合わせる
            i1->op1.reg = i2->op1.reg;

            // next (i2) をリストから削除
            cur->next = next->next;
            free(next);

            is_change = 1;
            continue;
          }
        }
      }
    }

    // 統合が起きなかった場合のみ、ポインタを進める
    prev = cur;
    cur = cur->next;
  }

  return is_change;
}

/* 定数分岐の畳み込み
 * 1. LI t0, 1
 * 2. LI t1, 1
 * 3. BEQ t0, t1, Label
 */
void optimize_branch(CodeList *list) {
  if (!list || !list->head)
    return;

  Code *prev2 = NULL; // 2つ前の命令
  Code *prev1 = NULL; // 1つ前の命令
  Code *cur = list->head;

  while (cur != NULL) {

    // 現在が条件分岐 (BEQ, BNE) かチェック
    if (cur->kind == CODE_INSN &&
        (cur->insn.code == ASM_BEQ || cur->insn.code == ASM_BNE)) {

      // 直前2つが命令であり、定数セット(ADDI reg, zero, imm)かチェック
      if (prev1 && prev1->kind == CODE_INSN && prev2 &&
          prev2->kind == CODE_INSN) {

        AsmInst *b_insn = &cur->insn; // Branch
        AsmInst *i1 = &prev1->insn;   // 直前 ($t1 = 1)
        AsmInst *i2 = &prev2->insn;   // 2個前 ($t0 = 1)

        // Branchのオペランド (op1, op2)
        MipsReg b_r1 = b_insn->op1.reg;
        MipsReg b_r2 = b_insn->op2.reg;

        // 定数ロード
        int is_const1 = (i1->code == ASM_ADDI || i1->code == ASM_ADDIU) &&
                        i1->op2.reg == R_ZERO;
        int is_const2 = (i2->code == ASM_ADDI || i2->code == ASM_ADDIU) &&
                        i2->op2.reg == R_ZERO;

        if (is_const1 && is_const2) {

          int val1 = 0, val2 = 0;
          int matched = 0;

          if (i1->op1.reg == b_r1 && i2->op1.reg == b_r2) {
            val1 = i1->op3.imm; // b_r1の値
            val2 = i2->op3.imm; // b_r2の値
            matched = 1;
          } else if (i1->op1.reg == b_r2 && i2->op1.reg == b_r1) {
            val1 = i2->op3.imm; // b_r1の値
            val2 = i1->op3.imm; // b_r2の値
            matched = 1;
          }

          if (matched) {
            int cond = 0;
            if (b_insn->code == ASM_BEQ)
              cond = (val1 == val2);
            if (b_insn->code == ASM_BNE)
              cond = (val1 != val2);

            if (cond) {

              char *label = b_insn->op3.label; // BEQのラベル

              b_insn->code = ASM_J;
              b_insn->op1.type = OP_LABEL;
              b_insn->op1.label = label;
              // op2, op3 は不要になる
              b_insn->op2.type = OP_REG;
              b_insn->op2.reg = R_ZERO; // 安全のためクリア
            } else {
              // 条件不成立 ->  命令削除
              Code *next = cur->next;
              prev1->next = next;
              if (cur == list->tail)
                list->tail = prev1;
              free(cur);

              cur = next;
              continue;
            }
          }
        }
      }
    }

    prev2 = prev1;
    prev1 = cur;
    cur = cur->next;
  }
}

// 指定範囲内でレジスタへの書き込み(clobber)があるかチェック
static int is_reg_clobbered(Code *start, Code *end, MipsReg reg) {
  Code *cur = start;
  while (cur != end && cur != NULL) {
    if (cur->kind == CODE_INSN) {
      AsmInst *insn = &cur->insn;

      // 1. 関数呼び出し (JAL) があるか?
      // 呼び出し規約上、$a0-$a3, $t0-$t9, $v0-$v1, $ra は破壊されるとみなす
      if (insn->code == ASM_JAL || insn->code == ASM_JALR) {
        if (reg >= R_A0 && reg <= R_A3)
          return 1;
        if (reg >= R_T0 && reg <= R_T9)
          return 1;
        if (reg >= R_V0 && reg <= R_V1)
          return 1;
        if (reg == R_RA)
          return 1;
      }

      // 2. 明示的な書き込み (rd = reg)
      int is_write = 0;

      // ストア命令や分岐命令は「レジスタへの書き込み」ではない
      if (insn->code == ASM_SW || insn->code == ASM_SB ||
          insn->code == ASM_BEQ || insn->code == ASM_BNE ||
          insn->code == ASM_JR) {
        is_write = 0;
      }
      // それ以外の演算命令・ロード命令は op1 が書き込み先
      else if (insn->op1.type == OP_REG && insn->op1.reg == reg) {
        is_write = 1;
      }

      if (is_write) {
        // 例外: "move reg, reg" (自分への代入) は値が変わらないので無視可能
        // ADDU reg, reg, $zero
        if (insn->code == ASM_ADDU && insn->op2.type == OP_REG &&
            insn->op2.reg == reg && insn->op3.type == OP_REG &&
            insn->op3.reg == R_ZERO) {
          // 安全
        } else {
          return 1; // 上書きあり
        }
      }
    }
    cur = cur->next;
  }
  return 0; // 上書きなし
}

/* Move伝播 (Window探索版)
 * start から end までの範囲で、Move命令の値を前方に伝播させる
 */
int optimize_move_propagation(Code *start, Code *end) {
  int changed = 0;
  Code *cur = start;
  int WINDOW_SIZE = 16; // 探索ウィンドウサイズ

  while (cur != end && cur != NULL) {
    if (cur->kind == CODE_INSN) {
      AsmInst *insn = &cur->insn;

      // Move命令か判定 (ADDU, OR, ADDIU のゼロ加算/論理和)
      int is_move = 0;
      MipsReg dst, src;

      if (insn->code == ASM_ADDU && insn->op3.type == OP_REG &&
          insn->op3.reg == R_ZERO) {
        is_move = 1;
        dst = insn->op1.reg;
        src = insn->op2.reg;
      } else if (insn->code == ASM_OR && insn->op3.type == OP_REG &&
                 insn->op3.reg == R_ZERO) {
        is_move = 1;
        dst = insn->op1.reg;
        src = insn->op2.reg;
      } else if ((insn->code == ASM_ADDIU || insn->code == ASM_ADDI) &&
                 insn->op3.type == OP_IMM && insn->op3.imm == 0) {
        is_move = 1;
        dst = insn->op1.reg;
        src = insn->op2.reg;
      }

      if (is_move && dst != src) {
        // 前方スキャンして dst を src に置換
        Code *scan = cur->next;
        int dist = 0;
        int src_clobbered = 0;
        int dst_clobbered = 0;

        while (scan != end && scan != NULL && dist < WINDOW_SIZE) {
          // 制御フローに当たったら安全のため打ち切り
          if (is_control_flow(scan))
            break;

          if (scan->kind == CODE_INSN) {
            AsmInst *s_insn = &scan->insn;

            // 1. 置換チャンス: dstを使っているか？
            // (src/dstがまだ上書きされていない場合のみ)
            if (!src_clobbered && !dst_clobbered) {
              int replaced = 0;

              // op1 check (SW/SB/BEQ/BNE では入力)
              if (s_insn->op1.type == OP_REG && s_insn->op1.reg == dst) {
                if (s_insn->code == ASM_SW || s_insn->code == ASM_SB ||
                    s_insn->code == ASM_BEQ || s_insn->code == ASM_BNE) {
                  s_insn->op1.reg = src;
                  replaced = 1;
                }
              }
              // op2 check
              if (s_insn->op2.type == OP_REG && s_insn->op2.reg == dst) {
                s_insn->op2.reg = src;
                replaced = 1;
              }
              // op3 check
              if (s_insn->op3.type == OP_REG && s_insn->op3.reg == dst) {
                s_insn->op3.reg = src;
                replaced = 1;
              }

              if (replaced)
                changed = 1;
            }

            // 2. 生存チェック: src や dst が書き換えられるか？
            // op1 (出力) をチェック
            if (s_insn->op1.type == OP_REG) {
              int is_write = 1;
              if (s_insn->code == ASM_SW || s_insn->code == ASM_SB ||
                  s_insn->code == ASM_BEQ || s_insn->code == ASM_BNE ||
                  s_insn->code == ASM_JR || s_insn->code == ASM_NOP) {
                is_write = 0;
              }

              if (is_write) {
                if (s_insn->op1.reg == src)
                  src_clobbered = 1;
                if (s_insn->op1.reg == dst)
                  dst_clobbered = 1;
              }
            }
          }
          scan = scan->next;
          dist++;
        }
      }
    }
    cur = cur->next;
  }
  return changed;
}

/* デッドコード削除 (Dead Definition)
 */
int optimize_dead_def(Code *start, Code *end) {
  int changed = 0;
  Code *cur = start;

  while (cur != end && cur != NULL) {
    if (cur->kind == CODE_INSN) {
      AsmInst *insn = &cur->insn;

      // 書き込みを行う命令か？
      // (ADD, LW, MOVEなど。SWやBRANCH、副作用のある命令は除外)
      int is_def = 0;
      MipsReg target_reg;

      if (insn->op1.type == OP_REG) {
        if (insn->code != ASM_SW && insn->code != ASM_SB &&
            insn->code != ASM_BEQ && insn->code != ASM_BNE &&
            insn->code != ASM_JR && insn->code != ASM_JAL &&
            insn->code != ASM_JALR && insn->code != ASM_SYSCALL &&
            insn->code != ASM_NOP) {
          is_def = 1;
          target_reg = insn->op1.reg;
        }
      }

      if (is_def) {
        // 後続で使われているかチェック
        if (target_reg != R_V0 && target_reg != R_V1 &&
            !is_reg_read_in_range(cur->next, end, target_reg)) {
          // 削除 (NOP化)
          insn->code = ASM_NOP;
          changed = 1;
        }
      }
    }
    cur = cur->next;
  }
  return changed;
}

// ジャンプ最適化
int optimize_jump_range(Code *start, Code *end) {
  if (!start || !end)
    return 0;

  int is_change = 0;
  Code *cur = start;

  while (cur != NULL && cur != end) {
    if (cur->kind == CODE_INSN && cur->insn.code == ASM_J) {
      char *target_label = cur->insn.op1.label;

      // 飛び先ラベルを探す
      Code *target = start;
      while (target && target != end) {
        if (target->kind == CODE_LABEL &&
            strcmp(target->label.name, target_label) == 0) {
          break;
        }
        target = target->next;
      }

      if (target && target != end) {
        // パターン1: Fallthrough
        Code *scan = cur->next;
        int is_fallthrough = 0;
        while (scan && scan != end) {
          if (scan == target) {
            is_fallthrough = 1;
            break;
          }
          if (scan->kind == CODE_INSN && scan->insn.code != ASM_NOP) {
            break; // 間に命令があるのでFallthrough不可
          }
          if (scan->kind == CODE_LABEL) {
            break; // 別のラベルが挟まっている場合も安全のため不可
          }
          scan = scan->next;
        }

        if (is_fallthrough) {
          // ジャンプ不要なので削除 (NOP化)
          cur->insn.code = ASM_NOP;
          is_change = 1;
        } else {
          // 飛び先の次の命令を取得（ラベルやNOPをスキップ）
          Code *tgt_inst1 = target->next;
          while (tgt_inst1 && tgt_inst1 != end &&
                 (tgt_inst1->kind == CODE_LABEL ||
                  (tgt_inst1->kind == CODE_INSN &&
                   tgt_inst1->insn.code == ASM_NOP))) {
            tgt_inst1 = tgt_inst1->next;
          }

          if (tgt_inst1 && tgt_inst1 != end && tgt_inst1->kind == CODE_INSN) {

            // パターン2: 飛び先がいきなりジャンプ (Label: j LabelB)
            if (tgt_inst1->insn.code == ASM_J) {
              cur->insn.op1.label = tgt_inst1->insn.op1.label;
              is_change = 1;
            }
            // パターン3: 飛び先が「命令 + ジャンプ」 (Label: Inst; j LabelB)
            else {
              Code *tgt_inst2 = tgt_inst1->next;
              // tgt_inst2 がジャンプ命令か確認
              while (tgt_inst2 && tgt_inst2 != end &&
                     tgt_inst2->kind == CODE_INSN &&
                     tgt_inst2->insn.code == ASM_NOP) {
                tgt_inst2 = tgt_inst2->next;
              }

              if (tgt_inst2 && tgt_inst2 != end &&
                  tgt_inst2->kind == CODE_INSN &&
                  tgt_inst2->insn.code == ASM_J) {

                if (!(tgt_inst2->next->kind == CODE_INSN &&
                      tgt_inst2->next->insn.code == ASM_NOP)) {
                  cur = cur->next;
                  continue;
                }

                // 自分の遅延スロットがNOPか確認
                Code *my_delay = cur->next;
                if (my_delay && my_delay != end &&
                    my_delay->kind == CODE_INSN &&
                    my_delay->insn.code == ASM_NOP) {
                  // コピーする命令が安全か確認 (分岐やSyscallでないこと)
                  AsmCode c = tgt_inst1->insn.code;
                  if (!is_control_flow(tgt_inst1)) {
                    Code *cur_copy =
                        new_code_j(ASM_J, tgt_inst2->insn.op1.label);

                    cur->insn = tgt_inst1->insn;
                    cur_copy->next = cur->next;
                    cur->next = cur_copy;

                    cur = cur_copy;
                    is_change = 1;
                  }
                }
              }
            }
          }
        }
      }
    }
    cur = cur->next;
  }
  return is_change;
}

// 分岐反転最適化 (if( __ ){break;}とか
int optimize_branch_inversion(Code *start, Code *end) {
  if (!start || !end)
    return 0;

  int changed = 0;
  Code *cur = start;
  while (cur && cur != end) {
    if (cur->kind == CODE_INSN &&
        (cur->insn.code == ASM_BEQ || cur->insn.code == ASM_BNE)) {

      AsmInst *branch = &cur->insn;
      char *skip_label = branch->op3.label; // BEQ/BNEの飛び先 (SkipLabel)

      // 1. BranchのDelay Slot
      Code *delay1 = cur->next;
      if (delay1 && delay1 != end && delay1->kind == CODE_INSN) {

        // 2. 直後が Jump 命令か？
        Code *jump_node = delay1->next;
        if (jump_node && jump_node != end && jump_node->kind == CODE_INSN &&
            jump_node->insn.code == ASM_J) {

          // 3. JumpのDelay Slot
          Code *delay2 = jump_node->next;
          if (delay2 && delay2 != end && delay2->kind == CODE_INSN &&
              delay2->insn.code == ASM_NOP) {

            // 4. Jumpの直後に SkipLabel があるか確認
            // (Fallthrough先がSkipLabelか)
            Code *scan = delay2->next;
            int found = 0;

            // 間のNOPや他のラベルをスキップして探す
            while (scan && scan != end) {
              if (scan->kind == CODE_LABEL) {
                if (strcmp(scan->label.name, skip_label) == 0) {
                  found = 1;
                  break;
                }
                scan = scan->next;
                continue;
              }
              if (scan->kind == CODE_INSN && scan->insn.code == ASM_NOP) {
                scan = scan->next;
                continue;
              }
              // それ以外の命令が来たら不一致
              break;
            }

            if (found) {
              // A. 分岐条件を反転 (BEQ <-> BNE)
              if (branch->code == ASM_BEQ)
                branch->code = ASM_BNE;
              else
                branch->code = ASM_BEQ;
              // B. 分岐先をJumpのターゲットに変更
              branch->op3.label = jump_node->insn.op1.label;

              // C. Jump命令をNOP化
              jump_node->insn.code = ASM_NOP;

              changed = 1;
            }
          }
        }
      }
    }
    cur = cur->next;
  }
  return changed;
}

int optimize_unreachable(Code *start, Code *end) {
  if (!start || !end) {
    return 0;
  }
  int is_change = 0;

  Code *cur = start;
  int reach = 1;

  while (cur && cur != end) {

    if (!reach) {
      if (cur->kind == CODE_LABEL) {
        reach = 1;
      } else if (cur->kind == CODE_INSN) {
        // 到達しない
        cur->insn.code = ASM_NOP;
        is_change = 1;
      }
    }

    if (cur->kind == CODE_INSN && cur->insn.code == ASM_J) {
      reach = 0;
      // jumpの次は除外
      cur = cur->next;
    }
    cur = cur->next;
  }

  return is_change;
}

static int is_label_used(char **used_list, int count, char *target) {
  for (int i = 0; i < count; ++i) {
    if (strcmp(used_list[i], target) == 0) {
      return 1;
    }
  }
  return 0;
}

int optimize_unused_label(Code *start, Code *end) {
  if (!start || !end) {
    return 0;
  }
  int is_change = 0;
  char *used_labels[1024];
  int used_count = 0;

  // ラベルの登録
  for (Code *cur = start; cur && cur != end; cur = cur->next) {
    if (cur->kind != CODE_INSN && is_branch(cur->insn.code)) {
      continue;
    }
    AsmInst *insn = &cur->insn;

    // j
    if (insn->op1.type == OP_LABEL) {
      if (used_count < 1024) {
        used_labels[used_count++] = insn->op1.label;
      }
    }

    // bneとか
    if (insn->op3.type == OP_LABEL) {
      if (used_count < 1024) {
        used_labels[used_count++] = insn->op3.label;
      }
    }
  }

  for (Code *cur = start; cur && cur != end; cur = cur->next) {
    if (cur->kind == CODE_LABEL && cur->label.name[0] == '$' &&
        !is_label_used(used_labels, used_count, cur->label.name)) {
      free(cur->label.name);
      cur->kind = CODE_INSN;
      cur->insn.code = ASM_NOP;
      is_change = 1;
    }
  }

  return is_change;
}

/* フレームポインタ省略最適化
 * 関数内で $sp が変更されない場合、$fp を $sp に置換し、
 * プロローグ/エピローグの $fp 保存/復帰処理を削除する。
 */
int optimize_frame_pointer_omission(Code *func_start, Code *func_end) {
  if (!func_start || !func_end)
    return 0;

  Code *prologue_end = NULL;
  Code *epilogue_start = NULL;

  // 1. プロローグ・エピローグの境界を探す
  for (Code *cur = func_start; cur != func_end; cur = cur->next) {
    if (cur->kind == CODE_PROLOGUE_END)
      prologue_end = cur;
    if (cur->kind == CODE_EPILOGUE_START)
      epilogue_start = cur;
  }

  if (!prologue_end || !epilogue_start)
    return 0;

  // 2. 関数本体で $sp が変更されているかチェック
  for (Code *cur = prologue_end->next; cur && cur != epilogue_start;
       cur = cur->next) {
    if (cur->kind == CODE_INSN) {
      AsmInst *insn = &cur->insn;

      // $sp への書き込みがあるかチェック
      int is_write = 1;
      // 書き込みを行わない命令（判定用）
      if (insn->code == ASM_SW || insn->code == ASM_SB ||
          insn->code == ASM_BEQ || insn->code == ASM_BNE ||
          insn->code == ASM_JR || insn->code == ASM_J ||
          insn->code == ASM_NOP || insn->code == ASM_SYSCALL) {
        is_write = 0;
      } else if (insn->code == ASM_JAL || insn->code == ASM_JALR) {
        // JAL系は $ra 等を壊すが $sp は壊さない
        is_write = 0;
      }

      if (is_write && insn->op1.type == OP_REG && insn->op1.reg == R_SP) {
        // 本体で $sp が変更されている -> オフセットが狂うので最適化不可
        return 0;
      }
    }
  }

  int changed = 0;

  // 3. 全命令の $fp を $sp に置換
  for (Code *cur = func_start; cur != func_end; cur = cur->next) {
    if (cur->kind == CODE_INSN) {
      AsmInst *insn = &cur->insn;
      if (insn->op1.type == OP_REG && insn->op1.reg == R_FP)
        insn->op1.reg = R_SP;
      if (insn->op2.type == OP_REG && insn->op2.reg == R_FP)
        insn->op2.reg = R_SP;
      if (insn->op3.type == OP_REG && insn->op3.reg == R_FP)
        insn->op3.reg = R_SP;
    }
  }

  // 4. プロローグ・エピローグの不要命令削除

  // --- プロローグ側 ---
  for (Code *cur = func_start; cur != prologue_end; cur = cur->next) {
    if (cur->kind == CODE_INSN) {
      AsmInst *insn = &cur->insn;

      // (元 sw $fp, ...) -> sw $sp, offset($sp) になっている
      // 保存命令
      if (insn->code == ASM_SW && insn->op1.type == OP_REG &&
          insn->op1.reg == R_SP) {
        insn->code = ASM_NOP;
        changed = 1;
      }

      // (元 ori $fp, $sp, 0) -> ori $sp, $sp, 0 になっている
      if (insn->code == ASM_ORI && insn->op1.type == OP_REG &&
          insn->op1.reg == R_SP && insn->op2.type == OP_REG &&
          insn->op2.reg == R_SP && insn->op3.imm == 0) {
        insn->code = ASM_NOP;
        changed = 1;
      }
    }
  }

  // --- エピローグ側 ---
  for (Code *cur = epilogue_start; cur != func_end; cur = cur->next) {
    if (cur->kind == CODE_INSN) {
      AsmInst *insn = &cur->insn;

      // (元 ori $sp, $fp, 0) -> ori $sp, $sp, 0 になっている
      if (insn->code == ASM_ORI && insn->op1.type == OP_REG &&
          insn->op1.reg == R_SP && insn->op2.type == OP_REG &&
          insn->op2.reg == R_SP && insn->op3.imm == 0) {
        insn->code = ASM_NOP;
        changed = 1;
      }

      // (元 lw $fp, ...) -> lw $sp, offset($sp) になっている
      if (insn->code == ASM_LW && insn->op1.type == OP_REG &&
          insn->op1.reg == R_SP) {
        insn->code = ASM_NOP;
        changed = 1;
      }
    }
  }

  return changed;
}

/* 関数単位ドライバ
 * リストから関数ブロックを切り出し、収束するまで最適化を回す
 */
void optimize_per_function(CodeList *list) {
  if (!list || !list->head)
    return;

  Code *cur = list->head;
  while (cur != NULL) {
    if (cur->kind == CODE_FUNC_ENTER) {
      Code *func_start = cur;
      Code *func_end = cur->next;

      // 関数の終わりを探す
      while (func_end != NULL && func_end->kind != CODE_FUNC_LEAVE) {
        func_end = func_end->next;
      }

      if (func_end) {
        // --- 最適化ループ ---
        int changed;
        int loop_count = 0;
        optimize_frame_pointer_omission(func_start, func_end);
        do {
          changed = 0;

          changed |= optimize_unused_label(func_start, func_end);
          changed |= optimize_unreachable(func_start, func_end);

          changed |= optimize_nop_range(func_start, func_end);

          // 1. Move伝播 (Window探索)
          changed |= optimize_move_propagation(func_start, func_end);

          changed |= optimize_load_move_range(func_start, func_end);
          // 2. 定数・アドレス計算の畳み込み
          changed |= optimize_addiu_chain_range(func_start, func_end);

          changed |= optimize_address_range(func_start, func_end);

          // 3. デッドコード削除
          changed |= optimize_dead_def(func_start, func_end);

          changed |= optimize_branch_inversion(func_start, func_end);
          changed |= optimize_jump_range(func_start, func_end);

          loop_count++;
        } while (changed && loop_count < 10); // 無限ループ防止で上限を設ける

        cur = func_end;
      }
    }
    cur = cur->next;
  }
}
