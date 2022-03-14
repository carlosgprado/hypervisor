#ifndef _H_HYPERVISOR
#define _H_HYPERVISOR

// Fake boolean :)
typedef enum { false, true} bool;


// Functions defined in .asm
extern void enableVMXOperation(void);

// Driver plumbing
VOID DrvUnload(PDRIVER_OBJECT pDriverObject);
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);

// Major function declarations
NTSTATUS DrvUnsupported(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DrvCreate(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DrvClose(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DrvIoctlDispatcher(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DrvRead(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DrvWrite(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

// Auxiliary functions
void setCPUAffinity(void);
int ipow(int base, int exp);
UINT64 virtToPhys(void *va);
PVOID physToVirt(UINT64 pa);
bool isVMXSupported(void);

// ------------------------------------------------------------
// Structures
// ------------------------------------------------------------
typedef union _IA32_FEATURE_CONTROL_MSR
{
    ULONG64 All;
    struct
    {
        ULONG64 Lock : 1;                // [0]
        ULONG64 EnableSMX : 1;           // [1]
        ULONG64 EnableVmxon : 1;         // [2]
        ULONG64 Reserved2 : 5;           // [3-7]
        ULONG64 EnableLocalSENTER : 7;   // [8-14]
        ULONG64 EnableGlobalSENTER : 1;  // [15]
        ULONG64 Reserved3a : 16;         //
        ULONG64 Reserved3b : 32;         // [16-63]
    } Fields;
} IA32_FEATURE_CONTROL_MSR, *PIA32_FEATURE_CONTROL_MSR;

typedef struct _CPUID
{
    int eax;
    int ebx;
    int ecx;
    int edx;
} CPUID, *PCPUID;

typedef struct _VMState
{
    UINT64 VMXON_REGION;
    UINT64 VMCS_REGION;
} VMState, *PVMState;

#endif
