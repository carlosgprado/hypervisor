; Functions implemented in C, called by vmExitHandler
EXTERN mainVMExitHandler:PROC
EXTERN vmResumer:PROC

.code _text


vmExitHandler PROC PUBLIC
    ; Store the registers in the stack
    PUSH R15
    PUSH R14
    PUSH R13
    PUSH R12
    PUSH R11
    PUSH R10
    PUSH R9
    PUSH R8
    PUSH RSI
    PUSH RBP
    PUSH RBP  ; RSP (?)
    PUSH RBX
    PUSH RDX
    PUSH RCX
    PUSH RAX

    MOV RCX, RSP  ; Guest regs
    SUB RSP, 28h

    ; Exit handler implementation
    CALL mainVMExitHandler
    ADD RSP, 28h

    ; Restore the registers from the stack
    POP RAX
    POP RCX
    POP RDX
    POP RBX
    POP RBP  ; RSP (?)
    POP RBP
    POP RSI
    POP RDI
    POP R8
    POP R9
    POP R10
    POP R11
    POP R12
    POP R13
    POP R14
    POP R15

    SUB RSP, 100h  ; safe space to avoid collisions in future functions?
    JMP vmResumer
vmExitHandler ENDP

END
