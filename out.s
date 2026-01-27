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
    addiu $sp, $sp, -12
    sw $ra, 8($sp)
    sw $fp, 4($sp)
    ori $fp, $sp, 0
    addi $t0, $zero, 3
    addi $t1, $zero, 7
    multu $t0, $t1
    mflo $t0
    addi $t1, $zero, 1
    multu $t0, $t1
    mflo $t0
    addiu $t0, $t0, 21
    addiu $t1, $fp, 0
    sw $t0, 0($t1)
    ori $sp, $fp, 0
    lw $fp, 4($sp)
    lw $ra, 8($sp)
    addiu $sp, $sp, 12
    jr $ra
    nop
    
.data
