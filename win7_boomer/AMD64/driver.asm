.code _text

enableVMXOperation PROC PUBLIC
    PUSH RAX
    XOR RAX, RAX
    MOV RAX, CR4
    OR RAX, 2000h   ; Set the 14th bit
    MOV CR4, RAX

    POP RAX
    RET
enableVMXOperation ENDP

END
