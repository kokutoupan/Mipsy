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
    addiu $sp, $sp, -32
    sw $ra, 28($sp)
    sw $fp, 24($sp)
    ori $fp, $sp, 0
    addi $t0, $zero, 0
    addiu $t1, $fp, 20
    sw $t0, 0($t1)
    j loop_cond0
    nop
loop_head1:
    addiu $t0, $fp, 20
    lw $t0, 0($t0)
    nop
    addiu $t0, $t0, 3
    la $t1, foge
    nop
    addiu $t2, $fp, 20
    lw $t2, 0($t2)
    nop
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    sw $t0, 0($t1)
    addiu $t0, $fp, 20
    lw $t0, 0($t0)
    nop
    addiu $t0, $t0, 1
    addiu $t1, $fp, 20
    sw $t0, 0($t1)
loop_cond0:
    addiu $t0, $fp, 20
    lw $t0, 0($t0)
    nop
    addiu $t0, $t0, -2
    sra $t0, $t0, 31
    bne $t0, $zero, loop_head1
    nop
loop_end2:
    addi $t0, $zero, 0
    addiu $t1, $fp, 20
    sw $t0, 0($t1)
    addi $t0, $zero, 0
    la $t1, sum
    nop
    sw $t0, 0($t1)
    j loop_cond3
    nop
loop_head4:
    la $t0, sum
    nop
    lw $t0, 0($t0)
    nop
    la $t1, foge
    nop
    addiu $t2, $fp, 20
    lw $t2, 0($t2)
    nop
    sll $t2, $t2, 2
    addu $t1, $t1, $t2
    lw $t1, 0($t1)
    nop
    addu $t0, $t0, $t1
    la $t1, sum
    nop
    sw $t0, 0($t1)
    addiu $t0, $fp, 20
    lw $t0, 0($t0)
    nop
    addiu $t0, $t0, 1
    addiu $t1, $fp, 20
    sw $t0, 0($t1)
loop_cond3:
    addiu $t0, $fp, 20
    lw $t0, 0($t0)
    nop
    addiu $t0, $t0, -2
    sra $t0, $t0, 31
    bne $t0, $zero, loop_head4
    nop
loop_end5:
    ori $sp, $fp, 0
    lw $fp, 24($sp)
    lw $ra, 28($sp)
    addiu $sp, $sp, 32
    jr $ra
    nop
    
.data
sum:
    .word 0
foge:
    .space 16
