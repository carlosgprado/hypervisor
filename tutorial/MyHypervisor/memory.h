#pragma once

#ifndef _H_HYPER_MEMORY
#define _H_HYPER_MEMORY

#define VMXON_SIZE   0x1000
#define VMCS_SIZE    0x1000
#define VMM_STACK_SIZE  0x8000
#define ALIGNMENT_PAGE_SIZE 0x1000

extern PVOID virtualGuestMemoryAddress;


/// <summary>
/// Keeps track of VM state (one for each logical CPU, i.e. core)
/// </summary>
typedef struct _VMState
{
    ULONG_PTR VMXON_REGION;
    ULONG_PTR VMCS_REGION;
    ULONG_PTR EPTP;
    PVOID VMMStack;
    PVOID MSRBitmap;
    PVOID MSRBitmapPhys;
} VMState, * PVMState;

//  Keeps track of VM state, for all cores
extern "C" PVMState pVMStateArray;

// ------------------------------------------------------------------
// Structures related to ETP
// ------------------------------------------------------------------

/// <summary>
/// Extended Page Table (EPT) Pointer.
/// Essentially this points to the PML4 structure
/// like the CR3 register does :) plus some control fields
/// </summary>
typedef union _EPTP {
    ULONG64 All;

    struct {
        UINT64 MemoryType : 3;  // bit 2:0 (0 = Uncacheable (UC), 6 = Writeback (WB))
        UINT64 PageWalkLength : 3; // bit 5:3 (EPT page-walk length minus one)
        UINT64 DirtyAndAccessEnabled : 1;  // bit 6
        UINT64 Reserved1 : 5;  // bit 11:7
        UINT64 PML4Address : 36;
        UINT64 Reserved2 : 16;
    } Fields;
} EPTP, * PEPTP;

/// <summary>
/// Extended Page Table (EPT) PML4 Entry.
/// </summary>
typedef union _EPT_PML4E {
    ULONG64 All;

    struct
    {
        UINT64 Read : 1; // bit 0
        UINT64 Write : 1; // bit 1
        UINT64 Execute : 1; // bit 2
        UINT64 Reserved1 : 5; // bits 7:3 (must be zero)
        UINT64 Accessed : 1;  // bit 8
        UINT64 Ignored1 : 1;  // bit 9
        UINT64 ExecuteForUserMode : 1; // bit 10
        UINT64 Ignored2 : 1;  // bit 11
        UINT64 PhysicalAddress : 36; // bit (N-1):12 ofr Page Frame Number (PFN)
        UINT64 Reserved2 : 4;  // bits 51:N
        UINT64 Ignored3 : 12;  // bit 63:5s
    } Fields;
} EPT_PML4E, * PEPT_PML4E;

/// <summary>
/// Extended Page Table (EPT) Page Directory Pointer Table Entry (PDPTE).
/// </summary>
typedef union _EPT_PDPTE {
    ULONG64 All;

    struct
    {
        UINT64 Read : 1; // bit 0
        UINT64 Write : 1; // bit 1
        UINT64 Execute : 1;  // bit 2
        UINT64 Reserved1 : 5; // bit 7:3 (must be zero)
        UINT64 Accessed : 1;  // bit 8
        UINT64 Ignored1 : 1;  // bit 9
        UINT64 ExecuteForUserMode : 1; // bit 10 (are instruction fetches allowed from user mode?)
        UINT64 Ignored2 : 1;  // bit 11
        UINT64 PhysicalAddress : 36;  // bit (N-1):12 or Page Frame Number (PFN)
        UINT64 Reserved2 : 4;  // bit 51:N
        UINT64 Ignored3 : 12;  // bit 63:52
    } Fields;
} EPT_PDPTE, * PEPT_PDPTE;

/// <summary>
/// Extended Page Table (EPT) Page Directory Entry (PDE).
/// Note that this is the same as _EPT_PDPTE
/// </summary>
typedef union _EPT_PDE {
    ULONG64 All;

    struct
    {
        UINT64 Read : 1;  // bit 0
        UINT64 Write : 1;  // bit 1
        UINT64 Execute : 1;  // bit 2
        UINT64 Reserved1 : 5;  // bit 7:3 (must be zero)
        UINT64 Accessed : 1;  // bit 8
        UINT64 Ignored1 : 1;  // bit 9
        UINT64 ExecuteForUserMode : 1; // bit 10
        UINT64 Ignored2 : 1;  // bit 11
        UINT64 PhysicalAddress : 36;  // bit (N-1):12 or Page Frame Number (PFN)
        UINT64 Reserved2 : 4;  // bit 51:N
        UINT64 Ignored3 : 12;  // bit 63:52
    } Fields;
} EPT_PDE, * PEPT_PDE;

/// <summary>
/// Extended Page Table (EPT) Page Table Entry (PTE).
/// </summary>
typedef union _EPT_PTE {
    ULONG64 All;

    struct
    {
        UINT64 Read : 1;  // bit 0
        UINT64 Write : 1;  // bit 1
        UINT64 Execute : 1;  // bit 2
        UINT64 EPTMemoryType : 3;  // bits 5:3 (EPT memory type)
        UINT64 IgnorePAT : 1; // bit 6
        UINT64 Ignored1 : 1;  // bit 7
        UINT64 AccessedFlag : 1;  // bit 8
        UINT64 DirtyFlag : 1;  // bit 9
        UINT64 ExecuteForUserMode : 1;  // bit 10
        UINT64 Ignored2 : 1;  // bit 11
        UINT64 PhysicalAddress : 36;  // bit (N-1):12 or Page Frame Number (PFN)
        UINT64 Reserved : 4;  // bit 51:N
        UINT64 Ignored3 : 11;  // bit 62:52
        UINT64 SuppressVE : 1;  // bit 63
    } Fields;
} EPT_PTE, * PEPT_PTE;

/// <summary>
/// Virtual address to corresponding physical one.
/// Uses Windows API
/// </summary>
/// <param name="va">Virtual address</param>
/// <returns>Physical address</returns>
ULONG_PTR virtToPhys(PVOID va);

/// <summary>
/// Physical address conversion to virtual one.
/// Uses Windows API
/// </summary>
/// <param name="pa">Physical address</param>
/// <returns>Virtual address</returns>
PVOID physToVirt(ULONG_PTR pa);

/// <summary>
/// Allocates and initializes the VMXON region for a 
/// specific core, and issues the VMXON instruction on it
/// </summary>
/// <param name="pVMState">Pointer to VMState for this core</param>
/// <returns>boolean</returns>
bool allocateVMXONRegion(PVMState);

/// <summary>
/// Allocates and initializes the VMCS region for a
/// specific core and issues the VMPTRLD instruction on it
/// </summary>
/// <param name="pVMState">Pointer to VMState for this core</param>
/// <returns>boolean</returns>
bool allocateVMCSRegion(PVMState);

/// <summary>
/// Allocates and initializes all the EPT-related structures
/// </summary>
/// <returns>PETP</returns>
PEPTP initializeEPTP(void);

#endif // !_H_HYPER_MEMORY
