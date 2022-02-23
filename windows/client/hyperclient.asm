PUBLIC MainAsm
.code _text

getCPUId PROC PUBLIC
    XOR EAX, EAX
    CPUID

    MOV EAX, EBX
    MOV sysType[0], al
    MOV sysType[1], ah
    SHR EAX, 16
    MOV sysType[2], al
    MOV sysType[3], ah

    MOV EAX, EDX
    MOV sysType[4], al
    MOV sysType[5], ah
    SHR EAX, 16
    MOV sysType[6], al
    MOV sysType[7], ah

    MOV EAX, ECX
    MOV sysType[8], al
    MOV sysType[9], ah
    SHR EAX, 16
    MOV sysType[10], al
    MOV sysType[11], ah

    MOV sysType[12], 0x00
    RET
getCPUId ENDP

getVMXSupport PROC PUBLIC
    XOR EAX, EAX
    INC EAX
    CPUID
    BT ECX. 0x05
    JC _VMX_SUPPORTED
    XOR EAX, EAX
    RET
    _VMX_SUPPORTED:
    MOV EAX 0x01
    RET
getVMXSupport ENDP

END

