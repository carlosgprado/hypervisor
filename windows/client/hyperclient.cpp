#include <stdio.h>
#include <windows.h>

#include "hyperclient.h"
#include "..\common.h"


int main(int argc, char *argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    char *pSysType = NULL;
    BOOL res = false;
    char outputBuffer[1024] = {0};
    char inputBuffer[1024] = {0};
    ULONG nrBytesReturned = 0;

    // --------------------------------------------------------------
    // Check support
    // --------------------------------------------------------------
    pSysType = getCPUId();

    printf("[+] The CPU manufacturer is: %s\n", pSysType);

    if (strncmp(pSysType, "GenuineIntel", 12) == 0) {
        printf("[+] This processor haz VT-x\n");
    } else {
        printf("[-] This program runs on Intel CPUs only!\n");
        return 1;
    }

    if (getVMXSupport()) {
        printf("[+] The CPU supports VMX operation!\n");
    } else {
        printf("[-] This CPU does NOT support VMX operation\n");
        return 1;
    }

    // --------------------------------------------------------------
    // Open device & issue IOCTL
    // --------------------------------------------------------------
    HANDLE hDev = CreateFileW(
        L"\\\\.\\Hypervisor",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL
    );

    res = DeviceIoControl(
        hDev,
        HYPER_IOCTL_METHOD_BUFFERED,
        &inputBuffer,
        (DWORD)strlen(inputBuffer) + 1,
        &outputBuffer,
        sizeof(outputBuffer),
        &nrBytesReturned,
        NULL
    );

    if (!res)
    {
        printf("[-] Error in DeviceIoControl: %d\n", GetLastError());
        return 1;
    }

    return 0;
}
