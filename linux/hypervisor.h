#ifndef __HYPERVISOR_H__
#define __HYPERVISOR_H__

/* Constant definitions */
#define X86_CR4_VMXE_BIT    13
#define X86_CR4_VMXE        _BITUL(X86_CR4_VMXE_BIT)
#define FEATURE_CONTROL_VMXON_ENABLED_OUTSIDE_SMX   (1 << 2)
#define FEATURE_CONTROL_LOCKED  (1 << 0)
#define MYPAGE_SIZE 4096

#define EAX_EDX_VAL(val, low, high) ((low) | (high) << 32)
#define EAX_EDX_RET(val, low, high) "=a" (low), "=d" (high)

/* Function declarations */
bool vmxSupport(void);
bool getVMXOperation(void);
static inline uint32_t vmcs_revision_id(void);
static inline int _vmxon(uint64_t);
static inline int _vmxoff(void);

static inline unsigned long long __rdmsr1(unsigned int);

#endif

