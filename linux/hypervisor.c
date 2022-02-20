#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <stdbool.h>
#include <asm/msr.h>

#include "hypervisor.h"


bool vmxSupport(void)
{
    // Look for CPUID.1:ECX.VMX[bit 5] = 1
    // This is how we know whether there is VMX support
    int getVMXSupport, VMXBit;

    __asm__("mov $1, %rax");
    __asm__("cpuid");
    __asm__("mov %%ecx, %0\n\t":"=r" (getVMXSupport));

    VMXBit = (getVMXSupport >> 5) & 1;

    if (VMXBit == 1)
        return true;
    else
        return false;
}

static inline unsigned long long __rdmsr1(unsigned int msr)
{
    DECLARE_ARGS(val, low, high);

    asm  volatile("1: rdmsr\n"
        "2:\n"
        _ASM_EXTABLE_HANDLE(1b, 2b, ex_handler_rdmsr_unsafe)
        : EAX_EDX_RET(val, low, high) : "c" (msr)
        );

    return EAX_EDX_VAL(val, low, high);
}

static inline uint32_t vmcs_revision_id(void) {
    return __rdmsr1(MSR_IA32_VMX_BASIC);
}

static inline int _vmxon(uint64_t phys) {
    uint8_t ret;

    __asm__ __volatile__ ("vmxon %[pa]; setna %[ret]"
        : [ret]"=rm"(ret)
        : [pa]"m"(phys)
        : "cc", "memory");

    return ret;
}

static inline int _vmxoff() {
    __asm__ __volatile__("vmxoff\n" : : : "cc");

    return 0;
}

bool getVMXOperation()
{
    unsigned long cr0 = 0;
    unsigned long cr4 = 0;
    unsigned int required = 0;
    unsigned int featureControl = 0;
    uint64_t *VMXONRegion = NULL;
    uint64_t *VMXONPhyRegion = NULL;
    u32 low1 = 0;

    // Setting CR4.VMXE[bit 13] = 1
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4) : : "memory");
    cr4 |= X86_CR4_VMXE;
    __asm__ __volatile__("mov %0, %%cr4" : : "r"(cr4) : "memory");

    /** Setting Feature Control MSR to allow VMXON
        Bit 0: Lock bit. If clear, VMXON causes a #GP
        Bit 2: Enables VMXON outside of SMX operation. If clear,
        VMXON outside of SMX causes a #GP
    */
    required = FEATURE_CONTROL_VMXON_ENABLED_OUTSIDE_SMX;
    required |= FEATURE_CONTROL_LOCKED;
    featureControl = __rdmsr1(MSR_IA32_FEATURE_CONTROL);

    printk(KERN_INFO, "RDMSR output is %ld", (long)featureControl);

    if ((featureControl & required) != required)
    {
        wrmsr(MSR_IA32_FEATURE_CONTROL, featureControl | required, low1);
    }

    /** Make sure bits are correct in CR0, CR4 to 
        allow VMXON without restrictions
    */
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0) : : "memory");
    cr0 &= __rdmsr1(MSR_IA32_VMX_CR0_FIXED1);
    cr0 |= __rdmsr1(MSR_IA32_VMX_CR0_FIXED0);
    __asm__ __volatile__("mov %0, %%cr0" : "=r"(cr0) : : "memory");

    __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4) : : "memory");
    cr4 &= __rdmsr1(MSR_IA32_VMX_CR4_FIXED1);
    cr4 |= __rdmsr1(MSR_IA32_VMX_CR4_FIXED0);
    __asm__ __volatile__("mov %0, %%cr4" : "=r"(cr4) : : "memory");

    /** Allocating 4KB of memory for the 
        VMXON region.
        This has to be zeroized.
    */
    VMXONRegion = kzalloc(MYPAGE_SIZE, GFP_KERNEL);
    if (VMXONRegion == NULL) {
        printk(KERN_INFO, "[+] Failed to allocate VMXON region.");
        return false;
    }

    VMXONPhyRegion = __pa(VMXONRegion);

    // Write the VMCS revision ID to the first DWORD
    *(uint32_t *)VMXONRegion = vmcs_revision_id();

    // Finally, enter the VMXON mode!
    if (_vmxon(VMXONPhyRegion))
        return false;

    return true;
}


// ===============================================================
// Linux Kernel Module Stuff
// ===============================================================
static int my_init(void)
{
    printk(KERN_INFO, "[+] Installing HyperVisor!");

    /**
    if (!vmxSupport()) {
        printk(KERN_INFO, "[-] VMX support not present! Exiting...");
        return 0;
    } else {
        printk(KERN_INFO, "[+] VMX support present. OK.");
    }

    if (!getVMXOperation()) {
        printk(KERN_INFO, "[-] VMX Operation failed! Exiting...");
        return 0;
    } else {
        printk(KERN_INFO, "[+] VMX Operation success. OK.");
    }

    // This was only a test. Stop the VMX operation.
    _vmxoff();
    */

    return 0;
}


static void my_exit(void)
{
    printk(KERN_WARNING, "[+] Removing HyperVisor!");
}


module_init(my_init);
module_exit(my_exit);

