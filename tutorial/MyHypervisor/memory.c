#include "pch.h"

PVOID virtualGuestMemoryAddress;


ULONG_PTR virtToPhys(PVOID va)
{
    PHYSICAL_ADDRESS phys;

    phys = MmGetPhysicalAddress(va);
    return phys.QuadPart;
}

PVOID physToVirt(ULONG_PTR pa)
{
    PHYSICAL_ADDRESS phys;

    phys.QuadPart = pa;
    return MmGetVirtualForPhysical(phys);
}

bool allocateVMXONRegion(PVMState pVMState)
{
    if (KeGetCurrentIrql() > DISPATCH_LEVEL)
    {
        // IRQL too high, memory allocation not possible
        KeRaiseIrqlToDpcLevel();
    }

    PHYSICAL_ADDRESS physMax = { 0 };
    physMax.QuadPart = MAXULONG_PTR;

    int VMXONSize = 2 * VMXON_SIZE;
    PVOID buffer = MmAllocateContiguousMemory(VMXONSize + ALIGNMENT_PAGE_SIZE, physMax);

    if (buffer == NULL)
    {
        DbgPrint("[-] Error: Could not allocate buffer for VMXON region.");
        return false;
    }

    RtlSecureZeroMemory(buffer, VMXONSize + ALIGNMENT_PAGE_SIZE);

    ULONG_PTR physBuffer = virtToPhys(buffer);
    ULONG_PTR alignedPhysBuffer = (ULONG_PTR)(physBuffer + ALIGNMENT_PAGE_SIZE - 1) & ~(ALIGNMENT_PAGE_SIZE - 1);
    ULONG_PTR alignedVirtBuffer = (ULONG_PTR)((BYTE*)buffer + ALIGNMENT_PAGE_SIZE - 1) & ~(ALIGNMENT_PAGE_SIZE - 1);

    DbgPrint("[+] Allocated aligned virtual buffer for VMXON @ %p", alignedVirtBuffer);
    DbgPrint("[+] Allocated aligned physical buffer for VMXON @ %p", alignedPhysBuffer);

    // Get IA32_VMX_BASIC_MSR's RevisionId
    IA32_VMX_BASIC_MSR basic = { 0 };
    basic.All = __readmsr(MSR_IA32_VMX_BASIC);
    DbgPrint("[+] MSR_IA32_VMX_BASIC (0x%X) Revision ID: %llx",
        MSR_IA32_VMX_BASIC,
        basic.Fields.RevisionIdentifier
    );

    // Writing the Revision Identifier to the first QWORD
    *(UINT64*)alignedVirtBuffer = basic.Fields.RevisionIdentifier;

    // Finally issue the VMXON instruction
    int status = __vmx_on(&alignedPhysBuffer);
    if (status)
    {
        DbgPrint("[-] VMXON failed with status: %d", status);
        return false;
    }

    // Keep track of this
    pVMState->VMXON_REGION = alignedPhysBuffer;

    return true;
}

bool allocateVMCSRegion(PVMState pVMState)
{
    // This function is pretty similar to
    // allocateVMXONRegion (see above)
    if (KeGetCurrentIrql() > DISPATCH_LEVEL)
        KeRaiseIrqlToDpcLevel();

    PHYSICAL_ADDRESS physMax = { 0 };
    physMax.QuadPart = MAXULONG_PTR;

    UINT VMCSSize = 2 * VMCS_SIZE;
    PVOID buffer = MmAllocateContiguousMemory(VMCSSize + ALIGNMENT_PAGE_SIZE, physMax);
    if (buffer == NULL)
    {
        DbgPrint("[-] Error: Could not allocate buffer for VMCS region");
        return false;
    }

    RtlSecureZeroMemory(buffer, VMCSSize + ALIGNMENT_PAGE_SIZE);

    ULONG_PTR physBuffer = virtToPhys(buffer);
    ULONG_PTR alignedPhysBuffer = (ULONG_PTR)(physBuffer + ALIGNMENT_PAGE_SIZE - 1) & ~(ALIGNMENT_PAGE_SIZE - 1);
    ULONG_PTR alignedVirtBuffer = (ULONG_PTR)((BYTE*)buffer + ALIGNMENT_PAGE_SIZE - 1) & ~(ALIGNMENT_PAGE_SIZE - 1);

    DbgPrint("[+] Allocated aligned virtual buffer for VMCS @ %p", alignedVirtBuffer);
    DbgPrint("[+] Allocated aligned physical buffer for VMCS @ %p", alignedPhysBuffer);

    // Get IA32_VMX_BASIC_MSR's RevisionId
    IA32_VMX_BASIC_MSR basic = { 0 };
    basic.All = __readmsr(MSR_IA32_VMX_BASIC);
    DbgPrint("[+] MSR_IA32_VMX_BASIC (0x%X) Revision ID: %llx",
        MSR_IA32_VMX_BASIC,
        basic.Fields.RevisionIdentifier
    );

    // Writing the Revision Identifier to the first QWORD
    *(UINT64*)alignedVirtBuffer = basic.Fields.RevisionIdentifier;

    int status = __vmx_vmptrld(&alignedPhysBuffer);
    if (status)
    {
        DbgPrint("[-] VMPTRLD failed with status: %d", status);
        return false;
    }

    pVMState->VMCS_REGION = alignedPhysBuffer;

    return true;
}

PEPTP initializeEPTP()
{
    // Allocates and initializes all the EPT-related structures

    PAGED_CODE();  // Run at low IRQL (allow paging)

    // Allocate Extended Page Table Pointer (EPTP)
    PEPTP EPTPointer = (PEPTP)ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOLTAG);

    if (!EPTPointer)
    {
        DbgPrint("[-] Failed to allocate EPTP!");
        return NULL;
    }

    RtlZeroMemory(EPTPointer, PAGE_SIZE);

    // Allocate EPT PML4
    PEPT_PML4E EPT_PML4 = (PEPT_PML4E)ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOLTAG);

    if (!EPT_PML4)
    {
        ExFreePoolWithTag(EPTPointer, POOLTAG);
        DbgPrint("[-] Failed to allocate EPT_PML4!");
        return NULL;
    }

    RtlZeroMemory(EPT_PML4, PAGE_SIZE);

    //Allocate EPT Page Directory Pointer Table
    PEPT_PDPTE EPT_PDPT = (PEPT_PDPTE)ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOLTAG);
    if (!EPT_PDPT) {
        ExFreePoolWithTag(EPT_PML4, POOLTAG);
        ExFreePoolWithTag(EPTPointer, POOLTAG);
        DbgPrint("[-] Failed to allocate EPT_PDPT!");
        return NULL;
    }

    RtlZeroMemory(EPT_PDPT, PAGE_SIZE);

    //	Allocate EPT Page Directory
    PEPT_PDE EPT_PD = (PEPT_PDE)ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOLTAG);

    if (!EPT_PD) {
        ExFreePoolWithTag(EPT_PDPT, POOLTAG);
        ExFreePoolWithTag(EPT_PML4, POOLTAG);
        ExFreePoolWithTag(EPTPointer, POOLTAG);
        DbgPrint("[-] Failed to allocate EPT_PD!");
        return NULL;
    }

    RtlZeroMemory(EPT_PD, PAGE_SIZE);

    //	Allocate EPT Page Table Entry
    PEPT_PTE EPT_PT = (PEPT_PTE)ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, POOLTAG);

    if (!EPT_PT) {
        ExFreePoolWithTag(EPT_PD, POOLTAG);
        ExFreePoolWithTag(EPT_PDPT, POOLTAG);
        ExFreePoolWithTag(EPT_PML4, POOLTAG);
        ExFreePoolWithTag(EPTPointer, POOLTAG);
        DbgPrint("[-] Failed to allocate EPT_PT!");
        return NULL;
    }

    RtlZeroMemory(EPT_PT, PAGE_SIZE);

    // Setup EPT by allocating two contiguous pages (RIP & RSP)
    // We allocate additional ones for the corresponding page table plumbing
    const int nrEPTPages = 10;
    PVOID guestMemory = ExAllocatePoolWithTag(NonPagedPool, nrEPTPages * PAGE_SIZE, POOLTAG);

    virtualGuestMemoryAddress = guestMemory;

    RtlZeroMemory(guestMemory, nrEPTPages * PAGE_SIZE);

    for (int i = 0; i < nrEPTPages; i++)
    {
        EPT_PT[i].Fields.AccessedFlag = 0;
        EPT_PT[i].Fields.DirtyFlag = 0;
        EPT_PT[i].Fields.EPTMemoryType = 6;  // write-back (we want our memory cacheable)
        EPT_PT[i].Fields.Execute = 1;
        EPT_PT[i].Fields.ExecuteForUserMode = 0;
        EPT_PT[i].Fields.IgnorePAT = 0;
        EPT_PT[i].Fields.PhysicalAddress = virtToPhys((PVOID)((UINT64)guestMemory + i * PAGE_SIZE)) / PAGE_SIZE;
        EPT_PT[i].Fields.Read = 1;
        EPT_PT[i].Fields.SuppressVE = 0;
        EPT_PT[i].Fields.Write = 1;
    }

    // Now we need to construct the translation table backwards,
    // from the PTE to the root (PML4E, EPTP)

    // Start with the PDE
    EPT_PD->Fields.Accessed = 0;
    EPT_PD->Fields.Execute = 1;
    EPT_PD->Fields.ExecuteForUserMode = 0;
    EPT_PD->Fields.Ignored1 = 0;
    EPT_PD->Fields.Ignored2 = 0;
    EPT_PD->Fields.Ignored3 = 0;
    EPT_PD->Fields.PhysicalAddress = virtToPhys(EPT_PT) / PAGE_SIZE;
    EPT_PD->Fields.Read = 1;
    EPT_PD->Fields.Reserved1 = 0;
    EPT_PD->Fields.Reserved2 = 0;
    EPT_PD->Fields.Write = 1;

    // Now the PDPT, linking to the PDE
    EPT_PDPT->Fields.Accessed = 0;
    EPT_PDPT->Fields.Execute = 1;
    EPT_PDPT->Fields.ExecuteForUserMode = 0;
    EPT_PDPT->Fields.Ignored1 = 0;
    EPT_PDPT->Fields.Ignored2 = 0;
    EPT_PDPT->Fields.Ignored3 = 0;
    EPT_PDPT->Fields.PhysicalAddress = virtToPhys(EPT_PD) / PAGE_SIZE;
    EPT_PDPT->Fields.Read = 1;
    EPT_PDPT->Fields.Reserved1 = 0;
    EPT_PDPT->Fields.Reserved2 = 0;
    EPT_PDPT->Fields.Write = 1;

    // Set up PML4E, linking to the PDPT
    EPT_PML4->Fields.Accessed = 0;
    EPT_PML4->Fields.Execute = 1;
    EPT_PML4->Fields.ExecuteForUserMode = 0;
    EPT_PML4->Fields.Ignored1 = 0;
    EPT_PML4->Fields.Ignored2 = 0;
    EPT_PML4->Fields.Ignored3 = 0;
    EPT_PML4->Fields.PhysicalAddress = virtToPhys(EPT_PDPT) / PAGE_SIZE;
    EPT_PML4->Fields.Read = 1;
    EPT_PML4->Fields.Reserved1 = 0;
    EPT_PML4->Fields.Reserved2 = 0;
    EPT_PML4->Fields.Write = 1;

    // Finally, set up EPTP
    // NOTE: Setting DirtyAndAccessEnabled causes processor
    // accesses to guest paging structure entries to be treated
    // as writes. 
    EPTPointer->Fields.DirtyAndAccessEnabled = 1;
    EPTPointer->Fields.MemoryType = 6;  // 6 = Write-back
    EPTPointer->Fields.PageWalkLength = 3;  // 4 (tables walked) - 1 == 3
    EPTPointer->Fields.PML4Address = virtToPhys(EPT_PML4) / PAGE_SIZE;
    EPTPointer->Fields.Reserved1 = 0;
    EPTPointer->Fields.Reserved2 = 0;

    DbgPrint("[+] Extended Page Table Pointer allocated at %p", EPTPointer);

    return EPTPointer;
}
