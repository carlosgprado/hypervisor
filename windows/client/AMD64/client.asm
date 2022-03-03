.data
sysType BYTE 13 dup (?)  ; array of 13 chars

.code _text

getCPUId PROC PUBLIC
    XOR RAX, RAX
    CPUID

    MOV RAX, RBX
    MOV sysType[0], al
    MOV sysType[1], ah
    SHR RAX, 16
    MOV sysType[2], al
    MOV sysType[3], ah

    MOV RAX, RDX
    MOV sysType[4], al
    MOV sysType[5], ah
    SHR RAX, 16
    MOV sysType[6], al
    MOV sysType[7], ah

    MOV RAX, RCX
    MOV sysType[8], al
    MOV sysType[9], ah
    SHR RAX, 16
    MOV sysType[10], al
    MOV sysType[11], ah

    MOV sysType[12], 00h

    MOV RAX, OFFSET sysType
    RET
getCPUId ENDP

getVMXSupport PROC PUBLIC
    XOR RAX, RAX
    INC RAX
    CPUID
    BT RCX, 5
    JC _VMX_SUPPORTED
    XOR RAX, RAX
    RET
    _VMX_SUPPORTED:
    RET
getVMXSupport ENDP

END
