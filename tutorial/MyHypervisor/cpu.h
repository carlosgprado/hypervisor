#pragma once

#ifndef _H_HYPER_CPU
#define _H_HYPER_CPU

// Bunch of MSR definitions, like there's not tomorrow
#define MSR_APIC_BASE                       0x01B
#define MSR_IA32_FEATURE_CONTROL            0x03A
#define MSR_IA32_VMX_BASIC                  0x480
#define MSR_IA32_VMX_PINBASED_CTLS          0x481
#define MSR_IA32_VMX_PROCBASED_CTLS         0x482
#define MSR_IA32_VMX_EXIT_CTLS              0x483
#define MSR_IA32_VMX_ENTRY_CTLS             0x484
#define MSR_IA32_VMX_MISC                   0x485
#define MSR_IA32_VMX_CR0_FIXED0             0x486
#define MSR_IA32_VMX_CR0_FIXED1             0x487
#define MSR_IA32_VMX_CR4_FIXED0             0x488
#define MSR_IA32_VMX_CR4_FIXED1             0x489
#define MSR_IA32_VMX_VMCS_ENUM              0x48A
#define MSR_IA32_VMX_PROCBASED_CTLS2        0x48B
#define MSR_IA32_VMX_EPT_VPID_CAP           0x48C
#define MSR_IA32_VMX_TRUE_PINBASED_CTLS     0x48D
#define MSR_IA32_VMX_TRUE_PROCBASED_CTLS    0x48E
#define MSR_IA32_VMX_TRUE_EXIT_CTLS         0x48F
#define MSR_IA32_VMX_TRUE_ENTRY_CTLS        0x490
#define MSR_IA32_VMX_VMFUNC                 0x491
#define MSR_IA32_SYSENTER_CS                0x174
#define MSR_IA32_SYSENTER_ESP               0x175
#define MSR_IA32_SYSENTER_EIP               0x176
#define MSR_IA32_DEBUGCTL                   0x1D9
#define MSR_LSTAR                           0xC0000082
#define MSR_FS_BASE                         0xC0000100
#define MSR_GS_BASE                         0xC0000101
#define MSR_SHADOW_GS_BASE                  0xC0000102


#define POOLTAG     0x4D594856  // "MYHV"


// Functions defined in driver.asm
// NOTE: extern "C" avoids name mangling for these functions
extern "C" {
    /// <summary>
    /// Sets the 14th bitof CR4. Implemented in ASM.
    /// </summary>
    extern void inline enableVMXOperation(void);
    extern void inline saveVMXOFFState(void);
    extern void inline restoreVMXOFFState(void);
    extern void inline vmExitHandler(void);
    extern ULONG_PTR getGDTBase(void);
    extern USHORT getGDTLimit(void);
    extern USHORT getCS(void);
    extern USHORT getDS(void);
    extern USHORT getES(void);
    extern USHORT getSS(void);
    extern USHORT getFS(void);
    extern USHORT getGS(void);
    extern USHORT getLDTR(void);
    extern USHORT getTR(void);
    extern ULONG_PTR getIDTBase(void);
    extern USHORT getIDTLimit(void);
    extern ULONG_PTR getRFLAGS(void);
}


typedef struct _CPUID
{
    int eax;
    int ebx;
    int ecx;
    int edx;
} CPUID, * PCPUID;

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
} IA32_FEATURE_CONTROL_MSR, * PIA32_FEATURE_CONTROL_MSR;

typedef union _IA32_VMX_BASIC_MSR
{
    ULONG64 All;
    struct
    {
        ULONG32 RevisionIdentifier : 31;   // [0-30]
        ULONG32 Reserved1 : 1;             // [31]
        ULONG32 RegionSize : 12;           // [32-43]
        ULONG32 RegionClear : 1;           // [44]
        ULONG32 Reserved2 : 3;             // [45-47]
        ULONG32 SupportedIA64 : 1;         // [48]
        ULONG32 SupportedDualMoniter : 1;  // [49]
        ULONG32 MemoryType : 4;            // [50-53]
        ULONG32 VmExitReport : 1;          // [54]
        ULONG32 VmxCapabilityHint : 1;     // [55]
        ULONG32 Reserved3 : 8;             // [56-63]
    } Fields;
} IA32_VMX_BASIC_MSR, * PIA32_VMX_BASIC_MSR;


typedef union _SEGMENT_ATTRIBUTES
{
    USHORT UCHARs;

    struct
    {
        USHORT TYPE : 4;
        USHORT S : 1;
        USHORT DPL : 2;
        USHORT P : 1;

        USHORT AVL : 1;
        USHORT L : 1;
        USHORT DB : 1;
        USHORT G : 1;
        USHORT GAP : 4;
    } Fields;
} SEGMENT_ATTRIBUTES;


typedef struct _SEGMENT_SELECTOR
{
    USHORT SEL;
    SEGMENT_ATTRIBUTES ATTRIBUTES;
    ULONG32 LIMIT;
    ULONG64 BASE;
} SEGMENT_SELECTOR, * PSEGMENT_SELECTOR;


typedef struct _SEGMENT_DESCRIPTOR
{
    USHORT LIMIT0;
    USHORT BASE0;
    UCHAR BASE1;
    UCHAR ATTR0;
    UCHAR LIMIT1ATTR1;
    UCHAR BASE2;
} SEGMENT_DESCRIPTOR, * PSEGMENT_DESCRIPTOR;

enum SEGREGS
{
    ES = 0,
    CS,
    SS,
    DS,
    FS,
    GS,
    LDTR,
    TR
};

typedef struct _GUEST_REGS
{
    ULONG64 RAX;                  // 0x00         // NOT VALID FOR SVM
    ULONG64 RCX;
    ULONG64 RDX;                  // 0x10
    ULONG64 RBX;
    ULONG64 RSP;                  // 0x20         // rsp is not stored here on SVM
    ULONG64 RBP;
    ULONG64 RSI;                  // 0x30
    ULONG64 RDI;
    ULONG64 R8;                   // 0x40
    ULONG64 R9;
    ULONG64 R10;                  // 0x50
    ULONG64 R11;
    ULONG64 R12;                  // 0x60
    ULONG64 R13;
    ULONG64 R14;                  // 0x70
    ULONG64 R15;
} GUEST_REGS, * PGUEST_REGS;

typedef union _MSR
{
    struct
    {
        ULONG Low;
        ULONG High;
    } Parts;

    ULONG64 Content;
} MSR, * PMSR;

/// A wrapper for VMXOFF intrinsic
/// It calls __vmx_off in each core (affinity)
void terminateVMX(void);

/// Initializes VMX operations.
/// Allocates VMStates objects for each core and
/// initialize them by pinning to each core (affinity)
PVMState initiateVMX(void);

/// <summary>
/// Auxiliary function to calculate kAffinityMask
/// </summary>
/// <param name="base"></param>
/// <param name="exp"></param>
/// <returns></returns>
int ipow(int base, int exp);

/// <summary>
/// Checks whether this system supports VMX
/// by querying CPUID and reading MSR_IA32_FEATURE_CONTROL
/// </summary>
/// <returns>boolean</returns>
bool isVMXSupported(void);

/// <summary>
/// Wrapper for VMPTRST
/// </summary>
/// <returns>boolean</returns>
bool VMPTRST(void);

/// <summary>
/// Wrapper for VMCLEAR
/// </summary>
/// <param name="pVMState">Pointer to core's VMState</param>
/// <returns>boolean</returns>
bool VMCLEAR(PVMState);

/// <summary>
/// Wrapper for VMPTRLD
/// </summary>
/// <param name="pVMState">Pointer to core's VMState</param>
/// <returns>boolean</returns>
bool VMPTRLD(PVMState);

/// <summary>
/// Runs the VM on a specific core
/// </summary>
/// <param name="processorID"></param>
/// <param name="pEPTP"></param>
bool launchVM(int processorID, PEPTP pEPTP);

/// <summary>
/// Sets up all the VMCS options, as well as
/// the guest and host state.
/// </summary>
/// <param name="pVMState">Pointer to core's VMState</param>
/// <param name="pEPTP"></param>
/// <returns></returns>
bool setupVMCS(PVMState pVMState, PEPTP pEPTP);

/// <summary>
/// 
/// </summary>
/// <param name="gdtBase"></param>
/// <param name="segReg"></param>
/// <param name="selector"></param>
void fillGuestSelectorData(
    PVOID gdtBase,
    ULONG segReg,
    USHORT selector
);

/// <summary>
/// 
/// </summary>
/// <param name="pSegmentSelector"></param>
/// <param name="selector"></param>
/// <param name="gdtBase"></param>
/// <returns></returns>
bool getSegmentDescriptor(
    PSEGMENT_SELECTOR pSegmentSelector,
    USHORT selector,
    PVOID gdtBase
);

/// <summary>
/// 
/// </summary>
/// <param name="ctl"></param>
/// <param name="msr"></param>
/// <returns></returns>
ULONG adjustControls(ULONG ctl, ULONG msr);

/// <summary>
/// Large switch case operating on base of 
/// the VM_EXIT_REASON value
/// </summary>
/// <param name="pGuestRegs">Pointer to structure containing guest register state</param>
extern "C" void mainVMExitHandler(PGUEST_REGS pGuestRegs);

/// <summary>
/// After a VM-exit occurs, the guest RIP remains unchanged.
/// The VMM is tasked with changing it, or not
/// </summary>
void resumeToNextIns(void);

/// <summary>
/// A wrapper around the VMRESUME instruction
/// with some debugging / error handling
/// </summary>
extern "C" void vmResumer(void);

#endif  // ! _H_HYPER_CPU
