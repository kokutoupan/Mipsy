#include "mips_print.h"
#include "mips_code.h"
#include <linux/limits.h>
#include <stdio.h>

void print_head(FILE *out) {

  fprintf(out, "    INITIAL_GP = 0x10008000         # initial value of global "
               "pointer\n");
  fprintf(out, "    INITIAL_SP = 0x7ffffffc         # initial value of stack "
               "pointer\n");
  fprintf(out, "    # system call service number\n");
  fprintf(out, "    stop_service = 99\n");
  fprintf(out, "\n");
  fprintf(out,
          "    .text                           # テキストセグメントの開始\n");
  fprintf(out, "init:\n");
  fprintf(out,
          "    # initialize $gp (global pointer) and $sp (stack pointer)\n");
  fprintf(out, "    la      $gp, INITIAL_GP         # $gp ← 0x10008000 "
               "(INITIAL_GP)\n");
  fprintf(out, "    la      $sp, INITIAL_SP         # $sp ← 0x7ffffffc "
               "(INITIAL_SP)\n");
  fprintf(out, "    jal     main                    # jump to `main'\n");
  fprintf(out, "    nop                             # (delay slot)\n");
  fprintf(out,
          "    li      $v0, stop_service       # $v0 ← 99 (stop_service)\n");
  fprintf(out, "    syscall                         # stop\n");
  fprintf(out, "    nop\n");
  fprintf(out, "    # not reach here\n");
  fprintf(out, "stop:                                   # if syscall return\n");
  fprintf(out, "    j       stop                    # infinite loop...\n");
  fprintf(out, "    nop                             # (delay slot)\n");
  fprintf(out, "\n");
  fprintf(out, "    .text   0x00001000              # "
               "以降のコードを0x00001000から配置\n");
}

// ---- Operand 出力 ----
static void print_operand(FILE *fp, Operand *o) {
  switch (o->type) {
  case OP_REG:
    fprintf(fp, "%s", reg_to_string(o->reg));
    break;
  case OP_IMM:
    fprintf(fp, "%d", o->imm);
    break;
  case OP_LABEL:
    fprintf(fp, "%s", o->label);
    break;
  }
}

void print_inst(FILE *fp, Code *code) {
  // 命令は一応インデントしといてやろう
  fprintf(fp, "    ");

  AsmInst *c = &(code->insn);
  const char *name = asmcode_to_string(c->code);

  switch (c->code) {

  // -------- R形式: op rd, rs, rt --------
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
    fprintf(fp, "%s ", name);
    print_operand(fp, &c->op1); // rd
    fprintf(fp, ", ");
    print_operand(fp, &c->op2); // rs
    fprintf(fp, ", ");
    print_operand(fp, &c->op3); // rt
    fprintf(fp, "\n");
    return;

  // mult, div は rs, rt のみ
  case ASM_MULT:
  case ASM_MULTU:
  case ASM_DIV:
  case ASM_DIVU:
    fprintf(fp, "%s ", name);
    print_operand(fp, &c->op2); // rs
    fprintf(fp, ", ");
    print_operand(fp, &c->op3); // rt
    fprintf(fp, "\n");
    return;

  // shift: op rd, rt, imm (または rs)
  case ASM_SLL:
  case ASM_SRL:
  case ASM_SRA:
    fprintf(fp, "%s ", name);
    print_operand(fp, &c->op1); // rd
    fprintf(fp, ", ");
    print_operand(fp, &c->op2); // rt
    fprintf(fp, ", ");
    print_operand(fp, &c->op3); // shamt (imm)
    fprintf(fp, "\n");
    return;

  // -------- I形式: op rt, rs, imm --------
  case ASM_ADDI:
  case ASM_ADDIU:
  case ASM_ANDI:
  case ASM_ORI:
  case ASM_XORI:
  case ASM_SLTI:
  case ASM_SLTIU:
    fprintf(fp, "%s ", name);
    print_operand(fp, &c->op1); // rt
    fprintf(fp, ", ");
    print_operand(fp, &c->op2); // rs
    fprintf(fp, ", ");
    print_operand(fp, &c->op3); // imm
    fprintf(fp, "\n");
    return;

  // load/store 形式: op rt, imm(rs)
  case ASM_LB:
  case ASM_LW:
  case ASM_SB:
  case ASM_SW:
    fprintf(fp, "%s ", name);
    print_operand(fp, &c->op1); // rt
    fprintf(fp, ", ");
    print_operand(fp, &c->op3); // offset
    fprintf(fp, "(");
    print_operand(fp, &c->op2); // base
    fprintf(fp, ")\n");
    return;

  // branch: op rs, rt, label
  case ASM_BEQ:
  case ASM_BNE:
    fprintf(fp, "%s ", name);
    print_operand(fp, &c->op1); // rs
    fprintf(fp, ", ");
    print_operand(fp, &c->op2); // rt
    fprintf(fp, ", ");
    print_operand(fp, &c->op3); // label
    fprintf(fp, "\n");
    return;

  // -------- J形式 --------
  case ASM_J:
  case ASM_JAL:
    fprintf(fp, "%s ", name);
    print_operand(fp, &c->op1); // label
    fprintf(fp, "\n");
    return;

  case ASM_JR:
  case ASM_JALR:
    fprintf(fp, "%s ", name);
    print_operand(fp, &c->op1); // reg
    fprintf(fp, "\n");
    return;

  // -------- 特殊 --------
  case ASM_LUI:
    fprintf(fp, "%s ", name);
    print_operand(fp, &c->op1); // rt
    fprintf(fp, ", ");
    print_operand(fp, &c->op3); // imm
    fprintf(fp, "\n");
    return;

  case ASM_MFHI:
  case ASM_MFLO:
    fprintf(fp, "%s ", name);
    print_operand(fp, &c->op1); // rd
    fprintf(fp, "\n");
    return;

  case ASM_SYSCALL:
    fprintf(fp, "syscall\n");
    return;

  // -------- 擬似命令 --------
  case ASM_LI:
    fprintf(fp, "li ");
    print_operand(fp, &c->op1); // rt
    fprintf(fp, ", ");
    print_operand(fp, &c->op3); // imm
    fprintf(fp, "\n");
    return;

  case ASM_LA:
    fprintf(fp, "la ");
    print_operand(fp, &c->op1); // rt
    fprintf(fp, ", ");
    print_operand(fp, &c->op3); // label
    fprintf(fp, "\n");
    return;

  case ASM_NOP:
    fprintf(fp, "nop\n");
    return;

  // -------- ディレクティブ --------
  case ASM_d_TEXT:
  case ASM_d_DATA:
    fprintf(fp, "\n%s\n", name);
    return;

  case ASM_d_WORD:
  case ASM_d_BYTE:
  case ASM_d_ASCII:
  case ASM_d_ASCIIZ:
  case ASM_d_SPACE:
    fprintf(fp, "%s ", name);
    print_operand(fp, &c->op1);
    fprintf(fp, "\n");
    return;

  default:
    fprintf(fp, "# [unknown asm]\n");
    return;
  }
}
// ---- 命令 1行を出力 ----
void print_code(FILE *fp, Code *code) {
  switch (code->kind) {
  case CODE_LABEL:
    fprintf(fp, "%s:\n", code->label.name);
    return;
  case CODE_INSN:
    print_inst(fp, code);
    return;
  case CODE_FUNC_ENTER:
    fprintf(fp, "# FUNCTION START %s:\n", code->label.name);
    return;
  case CODE_PROLOGUE_END:
    fprintf(fp, "# PROLOGUE END\n");
    return;
  case CODE_EPILOGUE_START:
    fprintf(fp, "# EPILOGUE START\n");
    return;
  case CODE_FUNC_LEAVE:
    fprintf(fp, "# FUNCTION END\n");
    return;
  }
}

// ---- リスト全体を出力 ----
void print_code_list(FILE *fp, Code *head) {
  print_head(fp);

  // ループで出力
  while (head) {
    print_code(fp, head);
    head = head->next;
  }
}
