#include "pch.h"


PVMState pVMStateArray;
ULONG cpuCount;


// --------------------------------------------------------
// Terminate VMX (VMXOFF)
// --------------------------------------------------------
void terminateVMX()
{
    KAFFINITY kAffinityMask;

    DbgPrint("[+] Terminating VMX...");

    for (ULONG i = 0; i < KeQueryActiveProcessorCount(0); i++)
    {
        kAffinityMask = ipow(2, i);
        KeSetSystemAffinityThread(kAffinityMask);
        DbgPrint("[+] Current thread is running on logical processor %d.", i);

        __vmx_off();
        MmFreeContiguousMemory(physToVirt(pVMStateArray[i].VMXON_REGION));
        MmFreeContiguousMemory(physToVirt(pVMStateArray[i].VMCS_REGION));
    }

    DbgPrint("[+] VMXOFF success.");
}


PVMState initiateVMX()
{
    KAFFINITY kAffinityMask;

    if (!isVMXSupported())
    {
        DbgPrint("[-] VMX is NOT supported in this machine!");
        return NULL;
    }

    cpuCount = KeQueryActiveProcessorCount(0);

    // Allocate memory for an array of VMState objects (one per CPU)
    pVMStateArray = (PVMState)ExAllocatePoolWithTag(NonPagedPool, sizeof(VMState) * cpuCount, POOLTAG);

    if (pVMStateArray == NULL)
    {
        DbgPrint("[-] Failed to allocate memory for array of VMState objects!");
        return NULL;
    }

    DbgPrint("\n=================================================================\n");

    for (ULONG i = 0; i < cpuCount; i++)
    {
        kAffinityMask = ipow(2, i);
        KeSetSystemAffinityThread(kAffinityMask);

        DbgPrint("[+] ++++ Current thread is executing in logical CPU %d ++++", i);

        enableVMXOperation();
        DbgPrint("[+] VMX Operation Enabled: OK");

        allocateVMXONRegion(&pVMStateArray[i]);
        allocateVMCSRegion(&pVMStateArray[i]);

        DbgPrint("[+] VMCS region allocatead at \t%p", pVMStateArray[i].VMCS_REGION);
        DbgPrint("[+] VMXON region allocatead at \t%p", pVMStateArray[i].VMXON_REGION);

        DbgPrint("\n=================================================================\n");
    }

    return pVMStateArray;
}


int ipow(int base, int exp)
{
    int result = 1;

    for (;;)
    {
        if (exp & 1)
            result *= base;

        exp >>= 1;

        if (!exp)
            break;

        base *= base;
    }

    return result;
}

bool isVMXSupported()
{
    CPUID data = { 0 };

    // Check the VMX bit
    __cpuid((int*)&data, 1);
    if ((data.ecx && (1 << 5)) == 0)
        return false;

    // Check this MSR for the lock bit
    IA32_FEATURE_CONTROL_MSR Control = { 0 };
    Control.All = __readmsr(MSR_IA32_FEATURE_CONTROL);

    // Bios lock check
    if (Control.Fields.Lock == false)
    {
        Control.Fields.Lock = true;
        Control.Fields.EnableVmxon = true;
        __writemsr(MSR_IA32_FEATURE_CONTROL, Control.All);
    }
    else if (Control.Fields.EnableVmxon == false)
    {
        DbgPrint("[-] VMX locked off in BIOS");
        return false;
    }

    return true;
}

// ------------------------------------------------------------------
// Intrinsic C-wrappers
// ------------------------------------------------------------------
bool VMPTRST()
{
    PHYSICAL_ADDRESS vmcspa;
    vmcspa.QuadPart = 0;

    __vmx_vmptrst((UINT64*)&vmcspa);

    DbgPrint("[+] VMPTRST at %p\n", vmcspa);

    return true;
}

bool VMCLEAR(PVMState pVMState)
{
    // Clears the corresponding VMCS status,
    // rendering it inactive
    int status = __vmx_vmclear(&pVMState->VMCS_REGION);

    DbgPrint("[+] VMCS VMCLEAR Status: %d", status);
    if (status)
    {
        DbgPrint("[-] VMCS clear failed with status: %d", status);
        return false;
    }

    return true;
}

bool VMPTRLD(PVMState pVMState)
{
    int status = __vmx_vmptrld(&pVMState->VMCS_REGION);

    if (status)
    {
        DbgPrint("[-] VMCS VMPTRLD failed with status: %d", status);
        return false;
    }

    return true;
}


bool launchVM(int processorID, PEPTP pEPTP)
{
    KAFFINITY kAffinityMask;

    // Pin to a specific core
    kAffinityMask = ipow(2, processorID);
    KeSetSystemAffinityThread(kAffinityMask);
    DbgPrint("[+] oooo Launching VM! oooo");
    DbgPrint("[+] ++++ Current thread executing on %d-th logical processor ++++", processorID);

    PAGED_CODE();

    // ------------------------------------------------------------
    // Allocate stack space for the VM exit handler.
    // ------------------------------------------------------------
    PVOID VMMStackVa = ExAllocatePoolWithTag(NonPagedPool, VMM_STACK_SIZE, POOLTAG);
    pVMStateArray[processorID].VMMStack = VMMStackVa;

    if (pVMStateArray[processorID].VMMStack == NULL)
    {
        DbgPrint("[-] Error allocating VMM stack for CPU %d", processorID);
        return false;
    }
    else {
        DbgPrint("[+] VMM stack for CPU %d at 0x%X", processorID, pVMStateArray[processorID].VMMStack);
    }

    RtlZeroMemory(pVMStateArray[processorID].VMMStack, VMM_STACK_SIZE);

    // ------------------------------------------------------------
    // Allocate space for MSR bitmap.
    // ------------------------------------------------------------
    pVMStateArray[processorID].MSRBitmap = MmAllocateNonCachedMemory(PAGE_SIZE);

    if (pVMStateArray[processorID].MSRBitmap == NULL)
    {
        DbgPrint("[-] Error allocating MSR bitmap for CPU %d", processorID);
        return false;
    }
    else {
        DbgPrint("[+] MSR bitmap for CPU %d at 0x%X", processorID, pVMStateArray[processorID].MSRBitmap);
    }

    RtlZeroMemory(pVMStateArray[processorID].MSRBitmap, PAGE_SIZE);

    pVMStateArray[processorID].MSRBitmapPhys = (PVOID)virtToPhys(pVMStateArray[processorID].MSRBitmap);

    // Clear the VMCS state
    if (!VMCLEAR(&pVMStateArray[processorID]))
        goto ErrorReturn;

    // Load VMCS (set the current VMCS region)
    if (!VMPTRLD(&pVMStateArray[processorID]))
        goto ErrorReturn;

    DbgPrint("[+] Setting up VMCS");
    setupVMCS(&pVMStateArray[processorID], pEPTP);

    // Save the stack's state
    saveVMXOFFState();

    // Let's launch the VM! :)
    DbgPrint("[+] Executing VMLAUNCH.");
    __vmx_vmlaunch();

    // This should not be reachable
    ULONG64 errorCode = 0;
    __vmx_vmread(VM_INSTRUCTION_ERROR, &errorCode);
    __vmx_off();
    DbgPrint("[-] VMLAUNCH error: %x", errorCode);
    DbgBreakPoint();

ErrorReturn:
    DbgPrint("[-] Failed to setup VMCS!");
    return false;
}


bool setupVMCS(PVMState pVMState, PEPTP pEPTP)
{
    // TODO: for now only ;)
    UNREFERENCED_PARAMETER(pEPTP);

    // Fill the VMCS host segment registers.
    // These (HOST_*) are set by the hypervisor when a VM-exit occurs.
    // NOTE: The 0xF8 mask clears the 3 LSBs (Intel requirement)
    __vmx_vmwrite(HOST_ES_SELECTOR, getES() & 0xF8);
    __vmx_vmwrite(HOST_CS_SELECTOR, getCS() & 0xF8);
    __vmx_vmwrite(HOST_SS_SELECTOR, getSS() & 0xF8);
    __vmx_vmwrite(HOST_DS_SELECTOR, getDS() & 0xF8);
    __vmx_vmwrite(HOST_FS_SELECTOR, getFS() & 0xF8);
    __vmx_vmwrite(HOST_GS_SELECTOR, getGS() & 0xF8);
    __vmx_vmwrite(HOST_TR_SELECTOR, getTR() & 0xF8);

    // Sets the link pointer to the required value for 4KB VMCS
    __vmx_vmwrite(VMCS_LINK_POINTER, ~0ULL);

    // Sets the IA32_DEBUGCTL, which controls a gazillion
    // of debugging options, on the -guest-
    __vmx_vmwrite(GUEST_IA32_DEBUGCTL, __readmsr(MSR_IA32_DEBUGCTL) & 0xFFFFFFFF);
    __vmx_vmwrite(GUEST_IA32_DEBUGCTL_HIGH, __readmsr(MSR_IA32_DEBUGCTL) >> 32);

    // Setup the time-stamp counter offset (TSC)
    __vmx_vmwrite(TSC_OFFSET, 0);
    __vmx_vmwrite(TSC_OFFSET_HIGH, 0);
    __vmx_vmwrite(PAGE_FAULT_ERROR_CODE_MASK, 0);
    __vmx_vmwrite(PAGE_FAULT_ERROR_CODE_MATCH, 0);
    __vmx_vmwrite(VM_EXIT_MSR_STORE_COUNT, 0);
    __vmx_vmwrite(VM_EXIT_MSR_LOAD_COUNT, 0);
    __vmx_vmwrite(VM_ENTRY_MSR_LOAD_COUNT, 0);
    __vmx_vmwrite(VM_ENTRY_INTR_INFO_FIELD, 0);

    // Configure segment registers and other GDT related information
    // for the Host, that is, when a VM-exit occurs
    PVOID gdtBase = (PVOID)getGDTBase();

    fillGuestSelectorData(gdtBase, ES, getES());
    fillGuestSelectorData(gdtBase, CS, getCS());
    fillGuestSelectorData(gdtBase, SS, getSS());
    fillGuestSelectorData(gdtBase, DS, getDS());
    fillGuestSelectorData(gdtBase, FS, getFS());
    fillGuestSelectorData(gdtBase, GS, getGS());
    fillGuestSelectorData(gdtBase, LDTR, getLDTR());
    fillGuestSelectorData(gdtBase, TR, getTR());

    __vmx_vmwrite(GUEST_INTERRUPTIBILITY_INFO, 0);
    __vmx_vmwrite(GUEST_ACTIVITY_STATE, 0);

    // -----------------------------------------------------------------
    // VM-execution controls information
    // -----------------------------------------------------------------
    __vmx_vmwrite(CPU_BASED_VM_EXEC_CONTROL,
        adjustControls(
            CPU_BASED_HLT_EXITING | CPU_BASED_ACTIVATE_SECONDARY_CONTROLS,
            MSR_IA32_VMX_PROCBASED_CTLS));

    __vmx_vmwrite(SECONDARY_VM_EXEC_CONTROL,
        adjustControls(
            CPU_BASED_CTL2_RDTSCP, /* | CPU_BASED_CTL2_ENABLE_EPT */
            MSR_IA32_VMX_PROCBASED_CTLS2));

    __vmx_vmwrite(PIN_BASED_VM_EXEC_CONTROL,
        adjustControls(0, MSR_IA32_VMX_PINBASED_CTLS));

    __vmx_vmwrite(VM_EXIT_CONTROLS,
        adjustControls(
            VM_EXIT_IA32E_MODE | VM_EXIT_ACK_INTR_ON_EXIT,
            MSR_IA32_VMX_EXIT_CTLS));

    __vmx_vmwrite(VM_ENTRY_CONTROLS,
        adjustControls(
            VM_ENTRY_IA32E_MODE,
            MSR_IA32_VMX_ENTRY_CTLS));

    // -----------------------------------------------------------------
    // Control Register
    // Guest and host have the same initial values
    // -----------------------------------------------------------------
    __vmx_vmwrite(GUEST_CR0, __readcr0());
    __vmx_vmwrite(GUEST_CR3, __readcr3());
    __vmx_vmwrite(GUEST_CR4, __readcr4());

    __vmx_vmwrite(GUEST_DR7, 0x400);

    __vmx_vmwrite(HOST_CR0, __readcr0());
    __vmx_vmwrite(HOST_CR3, __readcr3());
    __vmx_vmwrite(HOST_CR4, __readcr4());

    // -----------------------------------------------------------------
    // IDT & GDT (guest only)
    // -----------------------------------------------------------------
    __vmx_vmwrite(GUEST_GDTR_BASE, getGDTBase());
    __vmx_vmwrite(GUEST_IDTR_BASE, getIDTBase());
    __vmx_vmwrite(GUEST_GDTR_LIMIT, getGDTLimit());
    __vmx_vmwrite(GUEST_IDTR_LIMIT, getIDTLimit());

    // RFLAGS
    __vmx_vmwrite(GUEST_RFLAGS, getRFLAGS());

    // -----------------------------------------------------------------
    // This applies only to x86 guests (SYSENTER)
    // We'll define it for compatibility
    // -----------------------------------------------------------------
    __vmx_vmwrite(GUEST_SYSENTER_CS, __readmsr(MSR_IA32_SYSENTER_CS));
    __vmx_vmwrite(GUEST_SYSENTER_EIP, __readmsr(MSR_IA32_SYSENTER_EIP));
    __vmx_vmwrite(GUEST_SYSENTER_ESP, __readmsr(MSR_IA32_SYSENTER_ESP));
    __vmx_vmwrite(HOST_IA32_SYSENTER_CS, __readmsr(MSR_IA32_SYSENTER_CS));
    __vmx_vmwrite(HOST_IA32_SYSENTER_EIP, __readmsr(MSR_IA32_SYSENTER_EIP));
    __vmx_vmwrite(HOST_IA32_SYSENTER_ESP, __readmsr(MSR_IA32_SYSENTER_ESP));

    // -----------------------------------------------------------------
    // Set a bunch of host-related registers to the VMCS
    // -----------------------------------------------------------------
    SEGMENT_SELECTOR segmentSelector = { 0 };
    getSegmentDescriptor(&segmentSelector, (USHORT)getTR(), (PVOID)getGDTBase());

    __vmx_vmwrite(HOST_TR_BASE, segmentSelector.BASE);
    __vmx_vmwrite(HOST_FS_BASE, __readmsr(MSR_FS_BASE));
    __vmx_vmwrite(HOST_GS_BASE, __readmsr(MSR_GS_BASE));
    __vmx_vmwrite(HOST_GDTR_BASE, getGDTBase());
    __vmx_vmwrite(HOST_IDTR_BASE, getIDTBase());

    // -----------------------------------------------------------------
    // Guest's RIP & RSP
    // VMLAUNCH uses this RIP
    // VM-exit uses RIP & RSP from the host; RIP should point to a 
    // function resposible for managing VMX events (a la hooks)
    // -----------------------------------------------------------------
    __vmx_vmwrite(GUEST_RSP, (ULONG_PTR)virtualGuestMemoryAddress);  // Guest SP
    __vmx_vmwrite(GUEST_RIP, (ULONG_PTR)virtualGuestMemoryAddress);  // Guest IP

    // Set the host RSP to the bottom of the stack
    __vmx_vmwrite(HOST_RSP, (ULONG_PTR)pVMState->VMMStack + VMM_STACK_SIZE - 1);

    // This points to a function defined on the host
    // Using a host memory address is not an issue,
    // since guest & host have the same value of the CR3 register
    // so they use the same set of translation tables virt <-> phys
    __vmx_vmwrite(HOST_RIP, (ULONG_PTR)vmExitHandler);

    return true;
}

void fillGuestSelectorData(
    __in PVOID gdtBase,
    __in ULONG segReg,
    __in USHORT selector
)
{
    SEGMENT_SELECTOR segmentSelector = { 0 };
    ULONG ulAccessRights;

    DbgPrint("[+] fillGuestSelectorData - segReg: %d", segReg);

    getSegmentDescriptor(&segmentSelector, selector, gdtBase);
    ulAccessRights = *(PULONG)((PUCHAR)&segmentSelector.ATTRIBUTES)[0] + (((PUCHAR)&segmentSelector.ATTRIBUTES)[1] << 12);

    if (!selector)
        ulAccessRights |= 0x10000;

    DbgPrint("[+] fillGuestSelectorData - vmx_vmwrite...");
    __vmx_vmwrite(GUEST_ES_SELECTOR + segReg * 2, selector);
    __vmx_vmwrite(GUEST_ES_LIMIT + segReg * 2, segmentSelector.LIMIT);
    __vmx_vmwrite(GUEST_ES_AR_BYTES + segReg * 2, ulAccessRights);
    __vmx_vmwrite(GUEST_ES_BASE + segReg * 2, segmentSelector.BASE);
}

bool getSegmentDescriptor(
    PSEGMENT_SELECTOR pSegmentSelector,
    USHORT selector,
    PVOID gdtBase
)
{
    PSEGMENT_DESCRIPTOR pSegmentDescriptor;

    if (!pSegmentSelector)
        return false;

    if (selector & 0x4)
        return false;

    pSegmentDescriptor = (PSEGMENT_DESCRIPTOR)((PUCHAR)gdtBase + (selector & ~0x7));

    pSegmentSelector->SEL = selector;
    pSegmentSelector->BASE = pSegmentDescriptor->BASE0 | pSegmentDescriptor->BASE1 << 16 | pSegmentDescriptor->BASE2 << 24;
    pSegmentSelector->LIMIT = pSegmentDescriptor->LIMIT0 | (pSegmentDescriptor->LIMIT1ATTR1 & 0xF) << 16;
    pSegmentSelector->ATTRIBUTES.UCHARs = pSegmentDescriptor->ATTR0 | (pSegmentDescriptor->LIMIT1ATTR1 & 0xF0) << 4;

    if (!(pSegmentDescriptor->ATTR0 & 0x10)) {  // LA_ACCESSED
        ULONG64 tmp;
        // This is a TSS or callgate. Save the base's high part
        tmp = *(PULONG64)((PUCHAR)pSegmentDescriptor + 8);
        pSegmentSelector->BASE = (pSegmentSelector->BASE & 0xFFFFFFFF) | (tmp << 32);
    }

    if (pSegmentSelector->ATTRIBUTES.Fields.G) {
        // 4096-bit granularity is enabled for this segment, scale the limit
        pSegmentSelector->LIMIT = (pSegmentSelector->LIMIT << 12) + 0xFFF;
    }

    return true;
}

ULONG adjustControls(ULONG ctl, ULONG msr)
{
    MSR msrValue = { 0 };

    msrValue.Content = __readmsr(msr);
    ctl &= msrValue.Parts.High;
    ctl |= msrValue.Parts.Low;

    return ctl;
}

void mainVMExitHandler(PGUEST_REGS pGuestRegs)
{
    // For now only ;)
    UNREFERENCED_PARAMETER(pGuestRegs);

    ULONG_PTR exitReason = 0;
    __vmx_vmread(VM_EXIT_REASON, &exitReason);

    ULONG_PTR exitQualification = 0;
    __vmx_vmread(EXIT_QUALIFICATION, &exitQualification);

    DbgPrint("\nVM_EXIT_REASON: 0x%X", exitReason & 0xFFFF);
    DbgPrint("EXIT_QUALIFICATION: 0x%X", exitQualification);

    switch (exitReason)
    {
    case EXIT_REASON_VMCLEAR:
    case EXIT_REASON_VMPTRLD:
    case EXIT_REASON_VMPTRST:
    case EXIT_REASON_VMREAD:
    case EXIT_REASON_VMRESUME:
    case EXIT_REASON_VMWRITE:
    case EXIT_REASON_VMXOFF:
    case EXIT_REASON_VMXON:
    case EXIT_REASON_VMLAUNCH:
    {
        // Ignore this
        break;
    }

    case EXIT_REASON_HLT:
    {
        DbgPrint("[+] HLT detected!");
        restoreVMXOFFState();
        break;
    }

    case EXIT_REASON_EXCEPTION_NMI:
    case EXIT_REASON_CPUID:
    case EXIT_REASON_INVD:
    case EXIT_REASON_VMCALL:
    case EXIT_REASON_CR_ACCESS:
    case EXIT_REASON_MSR_READ:
    case EXIT_REASON_MSR_WRITE:
    case EXIT_REASON_EPT_VIOLATION:
    {
        break;
    }

    default:
    {
        break;
    }
    }  // end switch
}

void resumeToNextIns()
{
    ULONG_PTR nextGuestRIP = 0;
    ULONG_PTR currentGuestRIP = 0;
    ULONG64 exitInsLen = 0;

    __vmx_vmread(GUEST_RIP, &currentGuestRIP);
    __vmx_vmread(VM_EXIT_INSTRUCTION_LEN, &exitInsLen);

    nextGuestRIP = (ULONG_PTR)((PUCHAR)currentGuestRIP + exitInsLen);

    __vmx_vmwrite(GUEST_RIP, nextGuestRIP);
}

void vmResumer()
{
    __vmx_vmresume();

    // If OK, the following is not reachable :)
    ULONG64 errorCode = 0;
    __vmx_vmread(VM_INSTRUCTION_ERROR, &errorCode);
    __vmx_off();
    DbgPrint("[-] VMRESUME error: 0x%X\n", errorCode);

    // This is a FATAL error.
    // The best course of action is to break.
    DbgBreakPoint();
}
