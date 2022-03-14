#include <ntddk.h>
#include <wdm.h>

#include "common.h"
#include "hypervisor.h"

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DrvUnload)


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath) {
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UINT64 uiIndex = 0;
    PDEVICE_OBJECT pDeviceObject = NULL;
    UNICODE_STRING usDriverName;
    UNICODE_STRING usDosDeviceName;

    UNREFERENCED_PARAMETER(pRegistryPath);

    DbgPrint("[+] DriverEntry called");

    // Create device and alias
    RtlInitUnicodeString(&usDriverName, L"\\Device\\Hypervisor");
    RtlInitUnicodeString(&usDosDeviceName, L"\\DosDevices\\Hypervisor");

    ntStatus = IoCreateDevice(
            pDriverObject,
            0,
            &usDriverName,
            FILE_DEVICE_UNKNOWN,
            FILE_DEVICE_SECURE_OPEN,
            FALSE,
            &pDeviceObject
            );

    if (ntStatus == STATUS_SUCCESS)
    {
        pDriverObject->DriverUnload = DrvUnload;
        pDeviceObject->Flags |= IO_TYPE_DEVICE;
        pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        ntStatus = IoCreateSymbolicLink(&usDosDeviceName, &usDriverName);
    }

    if(ntStatus == STATUS_SUCCESS) {
        // Major function definitions
        for(uiIndex = 0; uiIndex < IRP_MJ_MAXIMUM_FUNCTION; uiIndex++)
        {
            pDriverObject->MajorFunction[uiIndex] = DrvUnsupported;
        }

        pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DrvClose;
        pDriverObject->MajorFunction[IRP_MJ_CREATE] = DrvCreate;
        pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DrvIoctlDispatcher;
        pDriverObject->MajorFunction[IRP_MJ_READ] = DrvRead;
        pDriverObject->MajorFunction[IRP_MJ_WRITE] = DrvWrite;
    } else {
        DbgPrint("[-] Error creating device. Aborting...");
    }

    return ntStatus;
}


NTSTATUS DrvUnsupported(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    DbgPrint("[-] Major function not supported!");

    // Simply complete the IRP request
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DrvCreate(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);
    UNREFERENCED_PARAMETER(pIrp);

    enableVMXOperation();
    DbgPrint("[+] VMX Operation Enabled: OK");

    return STATUS_SUCCESS;
}

NTSTATUS DrvClose(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    // Simply complete the IRP request
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DrvIoctlDispatcher(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);
    PIO_STACK_LOCATION irpSp;
    NTSTATUS ntStatus;
    ULONG inBufferLen, outBufferLen;
    PCHAR inBuffer, outBuffer;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(pIrp);
    inBufferLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outBufferLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

    if (!inBufferLen || !outBufferLen)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto End;
    }

End:
    pIrp->IoStatus.Status = ntStatus;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DrvRead(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    // Simply complete the IRP request
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DrvWrite(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    // Simply complete the IRP request
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

VOID DrvUnload(PDRIVER_OBJECT pDriverObject)
{
    UNICODE_STRING usDosDeviceName;

    DbgPrint("[+] DrvUnload called");

    RtlInitUnicodeString(&usDosDeviceName, L"\\DosDevices\\Hypervisor");
    IoDeleteSymbolicLink(&usDosDeviceName);
    IoDeleteDevice(pDriverObject->DeviceObject);
}

// --------------------------------------------------------
// Set CPU affinity
// --------------------------------------------------------
void setCPUAffinity()
{
    KAFFINITY kAffinityMask;

    for (size_t i = 0; i < KeQueryActiveProcessorCount(0); i++)
    {
        kAffinityMask = ipow(2, i);
        KeSetSystemAffinityThread(kAffinityMask);

        DbgPrint("----------------------------------------------");
        DbgPrint("Current thread is running on logical processor %d.", i);
    }
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

        base *= base
    }

    return result;
}

/* --------------------- [ Physical / Virtual Conversions ] --------------------- */
UINT64 virtToPhys(void *va)
{
    PHYSICAL_ADDRESS phys;

    phys = MmGetPhysicalAddress(va);
    return phys.QuadPart;
}

PVOID physToVirt(UINT64 pa)
{
    PHYSICAL_ADDRESS phys;

    phys.QuadPart = pa;
    return MmGetVirtualForPhysical(phys);
}

bool isVMXSupported()
{
    CPUID data = { 0 };

    // Check the VMX bit
    __cpuid((int*)data, 1);
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
    } else if (Control.Fields.EnableVmxon == false)
    {
        DbgPrint("[-] VMX locked off in BIOS");
        return false;
    }

    return true;
}
