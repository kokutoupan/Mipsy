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
# FUNCTION START foo:
foo:
    addiu $sp, $sp, -20
    sw $ra, 16($sp)
    sw $fp, 12($sp)
    sw $s0, 8($sp)
    ori $fp, $sp, 0
    addu $s0, $a0, $zero
# PROLOGUE END
    addi $t0, $zero, 21
    sw $t0, 0($fp)
    lw $t0, 0($fp)
    lw $t1, 0($s0)
    nop
    multu $t0, $t1
    mflo $t0
    sw $t0, 4($fp)
    lw $v0, 4($fp)
# EPILOGUE START
$func_ep0:
    ori $sp, $fp, 0
    lw $s0, 8($sp)
    lw $ra, 16($sp)
    lw $fp, 12($sp)
    jr $ra
    addiu $sp, $sp, 20
# FUNCTION END
# FUNCTION START main:
main:
    addiu $sp, $sp, -24
    sw $ra, 20($sp)
    sw $fp, 16($sp)
    ori $fp, $sp, 0
# PROLOGUE END
    addi $t0, $zero, 10
    sw $t0, 0($fp)
    lw $t0, 0($fp)
    nop
    addiu $t0, $t0, 2
    sw $t0, 4($fp)
    jal foo
    addiu $a0, $fp, 4
    addiu $a0, $fp, 8
    addi $sp, $sp, -4
    jal foo
    sw $v0, 0($sp)
    addu $t1, $v0, $zero
    lw $t0, 0($sp)
    addi $sp, $sp, 4
    multu $t0, $v0
    mflo $t0
    addu $a0, $fp, $zero
    addi $sp, $sp, -4
    jal foo
    sw $t0, 0($sp)
    lw $t0, 0($sp)
    nop
    addu $t0, $v0, $t0
    sw $t0, 12($fp)
# EPILOGUE START
$func_ep1:
    ori $sp, $fp, 0
    lw $ra, 20($sp)
    lw $fp, 16($sp)
    jr $ra
    addiu $sp, $sp, 24
# FUNCTION END
    
.data
