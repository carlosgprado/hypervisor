#include <stdio.h>
#include <unistd.h>
#include <string>

#include "hyperclient.h"

using std;


int main(int argc, char *argv[])
{
    string cpuId;
    cpuId = getCPUId();

    cout << "[+] The CPU manufacturer is: " << cpuId << endl;

    if (cpuId == "GenuineIntel") {
        cout << "[+] This processor haz VT-x" << endl;
    } else {
        cout << "[-] This program runs on Intel CPUs only!" << endl;
        return 1;
    }

    if (getVMXSupport()) {
        cout << "[+] The CPU supports VMX operation!" << endl;
    } else {
        cout << "[-] This CPU is crap" << endl;
        return 1;
    }

    return 0;
}


string getCPUId()
{
    char sysType[13] = {0};
    string cpuId;

    __asm
    {
        // Executing CPUID with EAX == 0 will return
        // the vendor string in other registers
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
    }

    cpuId.assign(sysType, 12);

    return cpuId;
}


bool getVMXSupport(void)
{
    bool VMX = false;

    __asm {
        XOR EAX, EAX
        INC EAX
        CPUID
        BT ECX. 0x05
        JC _VMX_SUPPORTED
        _VMX_NOT_SUPPORTED:
        JMP _NOP_INS
        _VMX_SUPPORTED:
        MOV VMX, 0x01
        _NOP_INS:
        NOP
    }

    return VMX;
}
