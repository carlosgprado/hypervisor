PUBLIC enableVMXOperation
PUBLIC saveVMXOFFState
PUBLIC restoreVMXOFFState

;EXTERN g_stackPointerAtReturn:QWORD
;EXTERN g_basePointerAtReturn:QWORD

.data

g_stackPointerAtReturn dq 0
g_basePointerAtReturn dq 0

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

saveVMXOFFState PROC PUBLIC
    MOV g_stackPointerAtReturn, RSP
    MOV g_basePointerAtReturn, RBP
    RET
saveVMXOFFState ENDP

restoreVMXOFFState PROC PUBLIC
    VMXOFF  ; turn off before restoring

    MOV RSP, g_stackPointerAtReturn
    MOV RBP, g_basePointerAtReturn

    ; Skip the saved return address
    ; NOTE: architecture dependent
    ADD RSP, 8

    ; Return true
    XOR RAX, RAX
    INC RAX

    ; return section
    MOV RBX, [RSP+28h+8h]
    MOV RSI, [RSP+28h+10h]
    ADD RSP, 20h
    POP RDI

    RET
restoreVMXOFFState ENDP

;
; Gather a plethora of machine state for the VMCS
;
getGDTBase PROC PUBLIC
    LOCAL gdtr[10]:BYTE
    SGDT gdtr
    MOV RAX, QWORD PTR gdtr[2]
    RET
getGDTBase ENDP

getGDTLimit PROC PUBLIC
    LOCAL gdtr[10]:BYTE
    SGDT gdtr
    MOV AX, WORD PTR gdtr[0]
    RET
getGDTLimit ENDP

getCS PROC PUBLIC
    MOV RAX, CS
    RET
getCS ENDP

getDS PROC PUBLIC
    MOV RAX, DS
    RET
getDS ENDP

getES PROC PUBLIC
    MOV RAX, ES
    RET
getES ENDP

getSS PROC PUBLIC
    MOV RAX, SS
    RET
getSS ENDP

getFS PROC PUBLIC
    MOV RAX, FS
    RET
getFS ENDP

getGS PROC PUBLIC
    MOV RAX, GS
    RET
getGS ENDP

getLDTR PROC PUBLIC
    SLDT RAX
    RET
getLDTR ENDP

getTR PROC PUBLIC
    STR RAX
    RET
getTR ENDP

getIDTBase PROC PUBLIC
    LOCAL idtr[10]:BYTE
    SIDT idtr
    MOV RAX, QWORD PTR idtr[2]
    RET
getIDTBase ENDP

getIDTLimit PROC PUBLIC
    LOCAL idtr[10]:BYTE
    SIDT idtr
    MOV AX, WORD PTR idtr[0]
    RET
getIDTLimit ENDP

getRFLAGS PROC PUBLIC
    PUSHFQ
    POP RAX
    RET
getRFLAGS ENDP

END
