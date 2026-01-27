#include "internal.h"
#include "mips_code.h"
#include <stdio.h>
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

/* レジスタがこの後で読まれるか */
static int is_reg_read_later(Code *start_node, MipsReg reg) {

  for (Code *cur = start_node; cur != NULL; cur = cur->next) {
    if (cur->kind != CODE_INSN)
      continue;

    // この命令で読まれているか？
    if (is_reg_used(&cur->insn, reg)) {

      // fprintf(stderr, "use : [%d][%d],[%d]\n", cur->insn.code,
      //         cur->insn.op1.reg, cur->insn.op2.reg);
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
      if (insn->op1.type == OP_REG && insn->op1.reg == reg)
        return 0; // 上書きされた
      break;
    // div/multなどは HI/LO への書き込みなので注意（ここでは無視）
    default:
      break;
    }

    // 関数呼び出し、終了は良しとする
    if (insn->code != ASM_JAL && insn->code != ASM_JALR) {
      return 0;
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

/* Load -> Move の統合
 * LW rt, off(base)
 * NOP
 * ADDU rd, rt, $zero (move)
 *
 * ==> LW rd, off(base)
 */
void optimize_load_move(CodeList *list) {
  if (!list || !list->head)
    return;

  Code *cur = list->head;

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

          if (!base_reg_conflict && !is_reg_read_later(move->next, src_reg)) {

            // LW の出力先を Moveのdst に書き換え
            i_lw->op1.reg = dst_reg;

            // Move命令を削除
            nop->next = move->next;
            if (move == list->tail)
              list->tail = nop;
            free(move);

            cur = nop;
            continue;
          }
        }
      }
    }
    cur = cur->next;
  }
}

/* 連続する即値加算の統合 (Chain Optimization)
 * 1. ADDIU rd, rs, imm1
 * 2. ADDIU rd, rd, imm2
 * ==> ADDIU rd, rs, (imm1 + imm2)
 */
void optimize_addiu_chain(CodeList *list) {
  if (!list || !list->head)
    return;

  Code *prev = NULL;
  Code *cur = list->head;

  while (cur != NULL) {
    Code *next = cur->next;

    // cur と next が共に ADDIU (または ADDI) かチェック
    if (cur->kind == CODE_INSN && next != NULL && next->kind == CODE_INSN) {
      AsmInst *i1 = &cur->insn;
      AsmInst *i2 = &next->insn;

      int is_addi1 = (i1->code == ASM_ADDIU || i1->code == ASM_ADDI);
      int is_addi2 = (i2->code == ASM_ADDIU || i2->code == ASM_ADDI);

      if (is_addi1 && is_addi2) {
        // パターンマッチ:
        // i1: rd = rs + imm1
        // i2: rd = rd + imm2  (i1の出力と同じレジスタに加算しているか)

        MipsReg rd1 = i1->op1.reg;
        MipsReg rs1 = i1->op2.reg;
        // i1 の出力先(rd1) を i2 が 入出力(rd2, rs2) として使っているか
        if (i2->op1.reg == rd1 && i2->op2.reg == rd1) {

          int imm1 = i1->op3.imm;
          int imm2 = i2->op3.imm;
          int new_imm = imm1 + imm2;

          // 16bit オーバーフローチェック
          if (new_imm >= -32768 && new_imm <= 32767) {

            i1->op3.imm = new_imm;

            // next (i2) をリストから削除
            cur->next = next->next; // curの次を nextの次へ飛ばす
            if (next == list->tail)
              list->tail = cur;
            free(next);

            continue;
          }
        }
      }
    }

    // 統合が起きなかった場合のみ、ポインタを進める
    prev = cur;
    cur = cur->next;
  }
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
