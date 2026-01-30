    INITIAL_GP = 0x10008000         # initial value of global pointer
    INITIAL_SP = 0x7ffffffc         # initial value of stack pointer
    # system call service number
    stop_service = 99

    .text                           # テキストセグメントの開始
init:
    # initialize $gp (global pointer) and $sp (stack pointer)
    la      $gp, INITIAL_GP         # $gp ← 0x10008000 (INITIAL_GP)
    la      $sp, INITIAL_SP         # $sp ← 0x7ffffffc (INITIAL_SP)
    jal     main                    # jump to `main'
    nop                             # (delay slot)
    li      $v0, stop_service       # $v0 ← 99 (stop_service)
    syscall                         # stop
    nop
    # not reach here
stop:                                   # if syscall return
    j       stop                    # infinite loop...
    nop                             # (delay slot)

    .text   0x00001000              # 以降のコードを0x00001000から配置
# FUNCTION START main:
main:
    addiu $sp, $sp, -68
    sw $ra, 64($sp)
    sw $fp, 60($sp)
    sw $s0, 48($sp)
    sw $s1, 52($sp)
    sw $s2, 56($sp)
    ori $fp, $sp, 0
# PROLOGUE END
    addi $t0, $zero, 1
    sw $t0, 0($fp)
    addi $t0, $zero, 2
    sw $t0, 4($fp)
    addi $t0, $zero, 3
    sw $t0, 8($fp)
    addi $t0, $zero, 4
    sw $t0, 12($fp)
    addi $t0, $zero, 5
    sw $t0, 16($fp)
    addi $t0, $zero, 6
    sw $t0, 20($fp)
    addi $t0, $zero, 7
    sw $t0, 24($fp)
    addi $t0, $zero, 8
    sw $t0, 28($fp)
    j $loop_cond1
    addu $s0, $zero, $zero
$loop_head2:
    j $loop_cond4
    addu $s1, $zero, $zero
$loop_head5:
    addiu $t1, $fp, 32
    sll $t2, $s0, 3
    addu $t1, $t1, $t2
    sll $t2, $s1, 2
    addu $t1, $t1, $t2
    sw $zero, 0($t1)
    addiu $s1, $s1, 1
$loop_cond4:
    slti $t0, $s1, 2
    bne $t0, $zero, $loop_head5
    nop
    addiu $s0, $s0, 1
$loop_cond1:
    slti $t0, $s0, 2
    bne $t0, $zero, $loop_head2
    nop
    j $loop_cond7
    addu $s0, $zero, $zero
$loop_head8:
    j $loop_cond10
    addu $s1, $zero, $zero
$loop_head11:
    j $loop_cond13
    addu $s2, $zero, $zero
$loop_head14:
    sll $t1, $s0, 3
    addu $t0, $fp, $t1
    sll $t1, $s2, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    addiu $t1, $fp, 16
    sll $t2, $s2, 3
    addu $t1, $t1, $t2
    sll $t2, $s1, 2
    addu $t1, $t1, $t2
    lw $t1, 0($t1)
    nop
    multu $t0, $t1
    mflo $t0
    addiu $t1, $fp, 32
    sll $t2, $s0, 3
    addu $t1, $t1, $t2
    sll $t2, $s1, 2
    addu $t1, $t1, $t2
    lw $t1, 0($t1)
    nop
    addu $t0, $t1, $t0
    addiu $t1, $fp, 32
    sll $t2, $s0, 3
    addu $t1, $t1, $t2
    sll $t2, $s1, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addiu $s2, $s2, 1
$loop_cond13:
    slti $t0, $s2, 2
    bne $t0, $zero, $loop_head14
    nop
    addiu $s1, $s1, 1
$loop_cond10:
    slti $t0, $s1, 2
    bne $t0, $zero, $loop_head11
    nop
    addiu $s0, $s0, 1
$loop_cond7:
    slti $t0, $s0, 2
    bne $t0, $zero, $loop_head8
    nop
# EPILOGUE START
    ori $sp, $fp, 0
    lw $s0, 48($sp)
    lw $s1, 52($sp)
    lw $s2, 56($sp)
    lw $ra, 64($sp)
    lw $fp, 60($sp)
    jr $ra
    addiu $sp, $sp, 68
# FUNCTION END
    
.data
