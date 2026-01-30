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
    addiu $sp, $sp, -28
    sw $s0, 16($sp)
# PROLOGUE END
    sw $zero, 0($sp)
    sw $zero, 4($sp)
    sw $zero, 8($sp)
    sw $zero, 12($sp)
    j $loop_cond1
    addi $s0, $zero, 1
$loop_head2:
    addi $t1, $zero, 15
    divu $s0, $t1
    mfhi $t0
    addi $t1, $zero, 0
    bne $t0, $t1, $IF_ELSE4
    nop
    lw $t0, 8($sp)
    nop
    addiu $t0, $t0, 1
    j $IF_END5
    sw $t0, 8($sp)
$IF_ELSE4:
    addi $t1, $zero, 3
    divu $s0, $t1
    mfhi $t0
    addi $t1, $zero, 0
    bne $t0, $t1, $IF_ELSE6
    nop
    lw $t0, 0($sp)
    nop
    addiu $t0, $t0, 1
    j $IF_END7
    sw $t0, 0($sp)
$IF_ELSE6:
    addi $t1, $zero, 5
    divu $s0, $t1
    mfhi $t0
    addi $t1, $zero, 0
    bne $t0, $t1, $IF_ELSE8
    nop
    lw $t0, 4($sp)
    nop
    addiu $t0, $t0, 1
    j $IF_END9
    sw $t0, 4($sp)
$IF_ELSE8:
    lw $t0, 12($sp)
    nop
    addiu $t0, $t0, 1
    addiu $t1, $sp, 12
    sw $t0, 0($t1)
$IF_END9:
$IF_END7:
$IF_END5:
    addiu $s0, $s0, 1
$loop_cond1:
    slti $t0, $s0, 31
    bne $t0, $zero, $loop_head2
    nop
# EPILOGUE START
    lw $s0, 16($sp)
    jr $ra
    addiu $sp, $sp, 28
# FUNCTION END
    
.data
