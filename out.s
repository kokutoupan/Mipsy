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
stop:                                           # if syscall return        
        j       stop                    # infinite loop...                 
        nop                             # (delay slot)                     
                                                                           
        .text   0x00001000              # 以降のコードを0x00001000から配置 
quicksort:
    addiu $sp, $sp, -36
    sw $ra, 32($sp)
    sw $fp, 28($sp)
    ori $fp, $sp, 0
    sw $a0, 0($fp)
    sw $a1, 4($fp)
    sw $a2, 8($fp)
    lw $t0, 8($fp)
    lw $t1, 4($fp)
    nop
    subu $t0, $t1, $t0
    sra $t0, $t0, 31
    bne $t0, $zero, IF_T0
    nop
    j IF_END1
    nop
IF_T0:
    lw $t0, 0($fp)
    lw $t1, 8($fp)
    nop
    sll $t1, $t1, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    nop
    sw $t0, 12($fp)
    lw $t0, 4($fp)
    nop
    addiu $t0, $t0, -1
    sw $t0, 16($fp)
    lw $t0, 8($fp)
    nop
    sw $t0, 20($fp)
    j loop_cond2
    nop
loop_head3:
    lw $t0, 16($fp)
    nop
    addiu $t0, $t0, 1
    sw $t0, 16($fp)
    j loop_cond5
    nop
loop_head6:
    lw $t0, 16($fp)
    nop
    addiu $t0, $t0, 1
    sw $t0, 16($fp)
loop_cond5:
    lw $t0, 0($fp)
    lw $t1, 16($fp)
    nop
    sll $t1, $t1, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    lw $t1, 12($fp)
    nop
    subu $t0, $t0, $t1
    sra $t0, $t0, 31
    bne $t0, $zero, loop_head6
    nop
loop_end7:
    lw $t0, 20($fp)
    nop
    addiu $t0, $t0, -1
    sw $t0, 20($fp)
    j loop_cond8
    nop
loop_head9:
    lw $t0, 20($fp)
    lw $t1, 4($fp)
    nop
    beq $t0, $t1, IF_T11
    nop
    j IF_END12
    nop
IF_T11:
    j loop_end10
    nop
IF_END12:
    lw $t0, 20($fp)
    nop
    addiu $t0, $t0, -1
    sw $t0, 20($fp)
loop_cond8:
    lw $t0, 0($fp)
    lw $t1, 20($fp)
    nop
    sll $t1, $t1, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    lw $t1, 12($fp)
    nop
    subu $t0, $t1, $t0
    sra $t0, $t0, 31
    bne $t0, $zero, loop_head9
    nop
loop_end10:
    lw $t0, 16($fp)
    lw $t1, 20($fp)
    nop
    subu $t0, $t0, $t1
    sra $t0, $t0, 31
    beq $t0, $zero, IF_T13
    nop
    j IF_END14
    nop
IF_T13:
    j loop_end4
    nop
IF_END14:
    lw $t0, 0($fp)
    lw $t1, 16($fp)
    nop
    sll $t1, $t1, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    nop
    sw $t0, 24($fp)
    lw $t0, 0($fp)
    lw $t1, 20($fp)
    nop
    sll $t1, $t1, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    lw $t1, 0($fp)
    lw $t2, 16($fp)
    nop
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    lw $t0, 24($fp)
    lw $t1, 0($fp)
    lw $t2, 20($fp)
    nop
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
loop_cond2:
    addi $t0, $zero, 1
    addi $t1, $zero, 1
    beq $t0, $t1, loop_head3
    nop
loop_end4:
    lw $t0, 0($fp)
    lw $t1, 16($fp)
    nop
    sll $t1, $t1, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    nop
    sw $t0, 24($fp)
    lw $t0, 0($fp)
    lw $t1, 8($fp)
    nop
    sll $t1, $t1, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    lw $t1, 0($fp)
    lw $t2, 16($fp)
    nop
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    lw $t0, 24($fp)
    lw $t1, 0($fp)
    lw $t2, 8($fp)
    nop
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    lw $t0, 0($fp)
    nop
    addu $a0, $t0, $zero
    lw $t0, 4($fp)
    nop
    addu $a1, $t0, $zero
    lw $t0, 16($fp)
    nop
    addiu $t0, $t0, -1
    addu $a2, $t0, $zero
    jal quicksort
    nop
    lw $t0, 0($fp)
    nop
    addu $a0, $t0, $zero
    lw $t0, 16($fp)
    nop
    addiu $t0, $t0, 1
    addu $a1, $t0, $zero
    lw $t0, 8($fp)
    nop
    addu $a2, $t0, $zero
    jal quicksort
    nop
IF_END1:
    ori $sp, $fp, 0
    lw $fp, 28($sp)
    lw $ra, 32($sp)
    addiu $sp, $sp, 36
    jr $ra
    nop
main:
    addiu $sp, $sp, -48
    sw $ra, 44($sp)
    sw $fp, 40($sp)
    ori $fp, $sp, 0
    addi $t0, $zero, 10
    sw $t0, 0($fp)
    addi $t0, $zero, 4
    sw $t0, 4($fp)
    addi $t0, $zero, 2
    sw $t0, 8($fp)
    addi $t0, $zero, 7
    sw $t0, 12($fp)
    addi $t0, $zero, 3
    sw $t0, 16($fp)
    addi $t0, $zero, 5
    sw $t0, 20($fp)
    addi $t0, $zero, 9
    sw $t0, 24($fp)
    addi $t0, $zero, 10
    sw $t0, 28($fp)
    addi $t0, $zero, 1
    sw $t0, 32($fp)
    addi $t0, $zero, 8
    sw $t0, 36($fp)
    addiu $t0, $fp, 0
    addu $a0, $t0, $zero
    addi $a1, $zero, 0
    addi $a2, $zero, 9
    jal quicksort
    nop
    ori $sp, $fp, 0
    lw $fp, 40($sp)
    lw $ra, 44($sp)
    addiu $sp, $sp, 48
    jr $ra
    nop
    
.data
