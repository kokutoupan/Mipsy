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
    addiu $t0, $fp, 8
    lw $t0, 0($t0)
    nop
    addiu $t1, $fp, 4
    lw $t1, 0($t1)
    nop
    subu $t0, $t1, $t0
    sra $t0, $t0, 31
    bne $t0, $zero, IF_T0
    nop
    j IF_END1
    nop
IF_T0:
    lw $t0, 0($fp)
    nop
    addiu $t1, $fp, 8
    lw $t1, 0($t1)
    nop
    sll $t1, $t1, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    nop
    addiu $t1, $fp, 12
    sw $t0, 0($t1)
    addiu $t0, $fp, 4
    lw $t0, 0($t0)
    nop
    addiu $t0, $t0, -1
    addiu $t1, $fp, 16
    sw $t0, 0($t1)
    addiu $t0, $fp, 8
    lw $t0, 0($t0)
    nop
    addiu $t1, $fp, 20
    sw $t0, 0($t1)
    j loop_cond2
    nop
loop_head3:
    addiu $t0, $fp, 16
    lw $t0, 0($t0)
    nop
    addiu $t0, $t0, 1
    addiu $t1, $fp, 16
    sw $t0, 0($t1)
    j loop_cond5
    nop
loop_head6:
    addiu $t0, $fp, 16
    lw $t0, 0($t0)
    nop
    addiu $t0, $t0, 1
    addiu $t1, $fp, 16
    sw $t0, 0($t1)
loop_cond5:
    lw $t0, 0($fp)
    nop
    addiu $t1, $fp, 16
    lw $t1, 0($t1)
    nop
    sll $t1, $t1, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    nop
    addiu $t1, $fp, 12
    lw $t1, 0($t1)
    nop
    subu $t0, $t0, $t1
    sra $t0, $t0, 31
    bne $t0, $zero, loop_head6
    nop
loop_end7:
    addiu $t0, $fp, 20
    lw $t0, 0($t0)
    nop
    addiu $t0, $t0, -1
    addiu $t1, $fp, 20
    sw $t0, 0($t1)
    j loop_cond8
    nop
loop_head9:
    addiu $t0, $fp, 20
    lw $t0, 0($t0)
    nop
    addiu $t1, $fp, 4
    lw $t1, 0($t1)
    nop
    beq $t0, $t1, IF_T11
    nop
    j IF_END12
    nop
IF_T11:
    j loop_end10
    nop
IF_END12:
    addiu $t0, $fp, 20
    lw $t0, 0($t0)
    nop
    addiu $t0, $t0, -1
    addiu $t1, $fp, 20
    sw $t0, 0($t1)
loop_cond8:
    lw $t0, 0($fp)
    nop
    addiu $t1, $fp, 20
    lw $t1, 0($t1)
    nop
    sll $t1, $t1, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    nop
    addiu $t1, $fp, 12
    lw $t1, 0($t1)
    nop
    subu $t0, $t1, $t0
    sra $t0, $t0, 31
    bne $t0, $zero, loop_head9
    nop
loop_end10:
    addiu $t0, $fp, 16
    lw $t0, 0($t0)
    nop
    addiu $t1, $fp, 20
    lw $t1, 0($t1)
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
    nop
    addiu $t1, $fp, 16
    lw $t1, 0($t1)
    nop
    sll $t1, $t1, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    nop
    addiu $t1, $fp, 24
    sw $t0, 0($t1)
    lw $t0, 0($fp)
    nop
    addiu $t1, $fp, 20
    lw $t1, 0($t1)
    nop
    sll $t1, $t1, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    nop
    lw $t1, 0($fp)
    nop
    addiu $t2, $fp, 16
    lw $t2, 0($t2)
    nop
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addiu $t0, $fp, 24
    lw $t0, 0($t0)
    nop
    lw $t1, 0($fp)
    nop
    addiu $t2, $fp, 20
    lw $t2, 0($t2)
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
    nop
    addiu $t1, $fp, 16
    lw $t1, 0($t1)
    nop
    sll $t1, $t1, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    nop
    addiu $t1, $fp, 24
    sw $t0, 0($t1)
    lw $t0, 0($fp)
    nop
    addiu $t1, $fp, 8
    lw $t1, 0($t1)
    nop
    sll $t1, $t1, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    nop
    lw $t1, 0($fp)
    nop
    addiu $t2, $fp, 16
    lw $t2, 0($t2)
    nop
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addiu $t0, $fp, 24
    lw $t0, 0($t0)
    nop
    lw $t1, 0($fp)
    nop
    addiu $t2, $fp, 8
    lw $t2, 0($t2)
    nop
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addiu $t0, $fp, 0
    lw $t0, 0($t0)
    nop
    addu $a0, $t0, $zero
    addiu $t0, $fp, 4
    lw $t0, 0($t0)
    nop
    addu $a1, $t0, $zero
    addiu $t0, $fp, 16
    lw $t0, 0($t0)
    nop
    addiu $t0, $t0, -1
    addu $a2, $t0, $zero
    jal quicksort
    nop
    addiu $t0, $fp, 0
    lw $t0, 0($t0)
    nop
    addu $a0, $t0, $zero
    addiu $t0, $fp, 16
    lw $t0, 0($t0)
    nop
    addiu $t0, $t0, 1
    addu $a1, $t0, $zero
    addiu $t0, $fp, 8
    lw $t0, 0($t0)
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
    addiu $t1, $fp, 0
    addi $t2, $zero, 0
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 4
    addiu $t1, $fp, 0
    addi $t2, $zero, 1
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 2
    addiu $t1, $fp, 0
    addi $t2, $zero, 2
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 7
    addiu $t1, $fp, 0
    addi $t2, $zero, 3
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 3
    addiu $t1, $fp, 0
    addi $t2, $zero, 4
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 5
    addiu $t1, $fp, 0
    addi $t2, $zero, 5
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 9
    addiu $t1, $fp, 0
    addi $t2, $zero, 6
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 10
    addiu $t1, $fp, 0
    addi $t2, $zero, 7
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 1
    addiu $t1, $fp, 0
    addi $t2, $zero, 8
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 8
    addiu $t1, $fp, 0
    addi $t2, $zero, 9
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
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
