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
main:
    addiu $sp, $sp, -68
    sw $ra, 64($sp)
    sw $fp, 60($sp)
    ori $fp, $sp, 0
    addi $t0, $zero, 1
    addiu $t1, $fp, 0
    addi $t2, $zero, 0
    ori $t3, $zero, 8
    mult $t2, $t3
    mflo $t2
    addu $t1, $t1, $t2
    addi $t2, $zero, 0
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 2
    addiu $t1, $fp, 0
    addi $t2, $zero, 0
    ori $t3, $zero, 8
    mult $t2, $t3
    mflo $t2
    addu $t1, $t1, $t2
    addi $t2, $zero, 1
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 3
    addiu $t1, $fp, 0
    addi $t2, $zero, 1
    ori $t3, $zero, 8
    mult $t2, $t3
    mflo $t2
    addu $t1, $t1, $t2
    addi $t2, $zero, 0
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 4
    addiu $t1, $fp, 0
    addi $t2, $zero, 1
    ori $t3, $zero, 8
    mult $t2, $t3
    mflo $t2
    addu $t1, $t1, $t2
    addi $t2, $zero, 1
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 5
    addiu $t1, $fp, 16
    addi $t2, $zero, 0
    ori $t3, $zero, 8
    mult $t2, $t3
    mflo $t2
    addu $t1, $t1, $t2
    addi $t2, $zero, 0
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 6
    addiu $t1, $fp, 16
    addi $t2, $zero, 0
    ori $t3, $zero, 8
    mult $t2, $t3
    mflo $t2
    addu $t1, $t1, $t2
    addi $t2, $zero, 1
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 7
    addiu $t1, $fp, 16
    addi $t2, $zero, 1
    ori $t3, $zero, 8
    mult $t2, $t3
    mflo $t2
    addu $t1, $t1, $t2
    addi $t2, $zero, 0
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 8
    addiu $t1, $fp, 16
    addi $t2, $zero, 1
    ori $t3, $zero, 8
    mult $t2, $t3
    mflo $t2
    addu $t1, $t1, $t2
    addi $t2, $zero, 1
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addi $t0, $zero, 0
    addiu $t1, $fp, 48
    sw $t0, 0($t1)
    j loop_cond0
    nop
loop_head1:
    addi $t0, $zero, 0
    addiu $t1, $fp, 52
    sw $t0, 0($t1)
    j loop_cond3
    nop
loop_head4:
    addi $t0, $zero, 0
    addiu $t1, $fp, 32
    lw $t2, 48($fp)
    ori $t3, $zero, 8
    mult $t2, $t3
    mflo $t2
    addu $t1, $t1, $t2
    lw $t2, 52($fp)
    nop
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    lw $t0, 52($fp)
    nop
    addiu $t0, $t0, 1
    addiu $t1, $fp, 52
    sw $t0, 0($t1)
loop_cond3:
    lw $t0, 52($fp)
    nop
    addiu $t0, $t0, -2
    sra $t0, $t0, 31
    bne $t0, $zero, loop_head4
    nop
loop_end5:
    lw $t0, 48($fp)
    nop
    addiu $t0, $t0, 1
    addiu $t1, $fp, 48
    sw $t0, 0($t1)
loop_cond0:
    lw $t0, 48($fp)
    nop
    addiu $t0, $t0, -2
    sra $t0, $t0, 31
    bne $t0, $zero, loop_head1
    nop
loop_end2:
    addi $t0, $zero, 0
    addiu $t1, $fp, 48
    sw $t0, 0($t1)
    j loop_cond6
    nop
loop_head7:
    addi $t0, $zero, 0
    addiu $t1, $fp, 52
    sw $t0, 0($t1)
    j loop_cond9
    nop
loop_head10:
    addi $t0, $zero, 0
    addiu $t1, $fp, 56
    sw $t0, 0($t1)
    j loop_cond12
    nop
loop_head13:
    addiu $t0, $fp, 32
    lw $t1, 48($fp)
    ori $t2, $zero, 8
    mult $t1, $t2
    mflo $t1
    addu $t0, $t0, $t1
    lw $t1, 52($fp)
    nop
    sll $t1, $t1, 2
    addu $t0, $t0, $t1
    lw $t0, 0($t0)
    addiu $t1, $fp, 0
    lw $t2, 48($fp)
    ori $t3, $zero, 8
    mult $t2, $t3
    mflo $t2
    addu $t1, $t1, $t2
    lw $t2, 56($fp)
    nop
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    lw $t1, 0($t1)
    addiu $t2, $fp, 16
    lw $t3, 56($fp)
    ori $t4, $zero, 8
    mult $t3, $t4
    mflo $t3
    addu $t2, $t2, $t3
    lw $t3, 52($fp)
    nop
    sll $t3, $t3, 2
    addu $t2, $t2, $t3
    lw $t2, 0($t2)
    nop
    multu $t1, $t2
    mflo $t1
    addu $t0, $t0, $t1
    addiu $t1, $fp, 32
    lw $t2, 48($fp)
    ori $t3, $zero, 8
    mult $t2, $t3
    mflo $t2
    addu $t1, $t1, $t2
    lw $t2, 52($fp)
    nop
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    lw $t0, 56($fp)
    nop
    addiu $t0, $t0, 1
    addiu $t1, $fp, 56
    sw $t0, 0($t1)
loop_cond12:
    lw $t0, 56($fp)
    nop
    addiu $t0, $t0, -2
    sra $t0, $t0, 31
    bne $t0, $zero, loop_head13
    nop
loop_end14:
    lw $t0, 52($fp)
    nop
    addiu $t0, $t0, 1
    addiu $t1, $fp, 52
    sw $t0, 0($t1)
loop_cond9:
    lw $t0, 52($fp)
    nop
    addiu $t0, $t0, -2
    sra $t0, $t0, 31
    bne $t0, $zero, loop_head10
    nop
loop_end11:
    lw $t0, 48($fp)
    nop
    addiu $t0, $t0, 1
    addiu $t1, $fp, 48
    sw $t0, 0($t1)
loop_cond6:
    lw $t0, 48($fp)
    nop
    addiu $t0, $t0, -2
    sra $t0, $t0, 31
    bne $t0, $zero, loop_head7
    nop
loop_end8:
    ori $sp, $fp, 0
    lw $fp, 60($sp)
    lw $ra, 64($sp)
    addiu $sp, $sp, 68
    jr $ra
    nop
    
.data
