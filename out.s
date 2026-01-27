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
    addiu $sp, $sp, -16
    sw $ra, 12($sp)
    sw $fp, 8($sp)
    ori $fp, $sp, 0
    addi $t0, $zero, 1
    addiu $t1, $fp, 4
    sw $t0, 0($t1)
    addiu $t0, $fp, 4
    lw $t0, 0($t0)
    nop
    addi $t1, $zero, 2
    sllv $t0, $t0, $t1
    addiu $t1, $fp, 4
    sw $t0, 0($t1)
    addi $t0, $zero, 53
    addiu $t1, $fp, 0
    sw $t0, 0($t1)
    addiu $t0, $fp, 0
    lw $t0, 0($t0)
    nop
    addiu $t1, $fp, 4
    lw $t1, 0($t1)
    nop
    divu $t0, $t1
    mfhi $t0
    addiu $t1, $fp, 0
    sw $t0, 0($t1)
    ori $sp, $fp, 0
    lw $fp, 8($sp)
    lw $ra, 12($sp)
    addiu $sp, $sp, 16
    jr $ra
    nop
    
.data
