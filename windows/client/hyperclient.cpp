#include <windows.h>
#include <stdio.h>

#include "hyperclient.h"


char sysType[13] = {0};


int main(int argc, char *argv[])
{
    getCPUId();

    printf("[+] The CPU manufacturer is: %s\n", sysType);

    if (strncmp(sysType, "GenuineIntel", 12) == 0) {
        printf("[+] This processor haz VT-x\n");
    } else {
        printf("[-] This program runs on Intel CPUs only!\n");
        return 1;
    }

    if (getVMXSupport()) {
        printf("[+] The CPU supports VMX operation!\n");
    } else {
        printf("[-] This CPU is crap\n");
        return 1;
    }

    return 0;
}
