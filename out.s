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
# FUNCTION START quicksort:
quicksort:
    addiu $sp, $sp, -36
    sw $ra, 32($sp)
    sw $fp, 28($sp)
    sw $s0, 0($sp)
    sw $s1, 4($sp)
    sw $s2, 8($sp)
    sw $s3, 12($sp)
    sw $s4, 16($sp)
    sw $s5, 20($sp)
    sw $s6, 24($sp)
    ori $fp, $sp, 0
    addu $s0, $a0, $zero
    addu $s1, $a1, $zero
    addu $s2, $a2, $zero
# PROLOGUE END
    subu $t0, $s1, $s2
    sra $t0, $t0, 31
    bne $t0, $zero, IF_T0
    nop
    j IF_END1
    nop
IF_T0:
    addu $t0, $s0, $zero
    sll $t1, $s2, 2
    addu $t0, $t0, $t1
    lw $s5, 0($t0)
    addiu $s3, $s1, -1
    addiu $s4, $s2, 0
    j loop_cond2
    nop
loop_head3:
    addiu $s3, $s3, 1
    j loop_cond5
    nop
loop_head6:
    addiu $s3, $s3, 1
loop_cond5:
    addu $t0, $s0, $zero
    sll $t1, $s3, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    nop
    subu $t0, $t0, $s5
    sra $t0, $t0, 31
    bne $t0, $zero, loop_head6
    nop
loop_end7:
    addiu $s4, $s4, -1
    j loop_cond8
    nop
loop_head9:
    beq $s4, $s1, IF_T11
    nop
    j IF_END12
    nop
IF_T11:
    j loop_end10
    nop
IF_END12:
    addiu $s4, $s4, -1
loop_cond8:
    addu $t0, $s0, $zero
    sll $t1, $s4, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    nop
    subu $t0, $s5, $t0
    sra $t0, $t0, 31
    bne $t0, $zero, loop_head9
    nop
loop_end10:
    subu $t0, $s3, $s4
    sra $t0, $t0, 31
    beq $t0, $zero, IF_T13
    nop
    j IF_END14
    nop
IF_T13:
    j loop_end4
    nop
IF_END14:
    addu $t0, $s0, $zero
    sll $t1, $s3, 2
    addu $t0, $t0, $t1
    lw $s6, 0($t0)
    addu $t0, $s0, $zero
    sll $t1, $s4, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    addu $t1, $s0, $zero
    sll $t2, $s3, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addu $t0, $s6, $zero
    addu $t1, $s0, $zero
    sll $t2, $s4, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
loop_cond2:
    addi $t0, $zero, 1
    addi $t1, $zero, 1
    j loop_head3
    nop
loop_end4:
    addu $t0, $s0, $zero
    sll $t1, $s3, 2
    addu $t0, $t0, $t1
    lw $s6, 0($t0)
    addu $t0, $s0, $zero
    sll $t1, $s2, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    addu $t1, $s0, $zero
    sll $t2, $s3, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addu $t0, $s6, $zero
    addu $t1, $s0, $zero
    sll $t2, $s2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addu $a0, $s0, $zero
    addu $a1, $s1, $zero
    addiu $a2, $s3, -1
    jal quicksort
    nop
    addu $a0, $s0, $zero
    addiu $a1, $s3, 1
    addu $a2, $s2, $zero
    jal quicksort
    nop
IF_END1:
# EPILOGUE START
    ori $sp, $fp, 0
    lw $s0, 0($sp)
    lw $s1, 4($sp)
    lw $s2, 8($sp)
    lw $s3, 12($sp)
    lw $s4, 16($sp)
    lw $s5, 20($sp)
    lw $s6, 24($sp)
    lw $fp, 28($sp)
    lw $ra, 32($sp)
    addiu $sp, $sp, 36
    jr $ra
    nop
# FUNCTION END
# FUNCTION START main:
main:
    addiu $sp, $sp, -48
    sw $ra, 44($sp)
    sw $fp, 40($sp)
    ori $fp, $sp, 0
# PROLOGUE END
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
    addiu $a0, $fp, 0
    addi $a1, $zero, 0
    addi $a2, $zero, 9
    jal quicksort
    nop
# EPILOGUE START
    ori $sp, $fp, 0
    lw $fp, 40($sp)
    lw $ra, 44($sp)
    addiu $sp, $sp, 48
    jr $ra
    nop
# FUNCTION END
    
.data
