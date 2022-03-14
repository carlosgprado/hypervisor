#ifndef _H_HYPERVISOR_CLIENT
#define _H_HYPERVISOR_CLIENT

// Function declarations
extern "C" {
    extern char* getCPUId(void);
    extern bool getVMXSupport(void);
}

#endif
