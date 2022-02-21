#include <ntddk.h>
//#include <wdf.h>
#include <wdm.h>

#include "hypervisor.h"

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DrvUnload)


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath) {
    NTSTATUS ntStatus = STATUS_SUCCESS;
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
        IoCreateSymbolicLink(&usDosDeviceName, &usDriverName);
    }

    return ntStatus;
}


VOID DrvUnload(PDRIVER_OBJECT pDriverObject)
{
    UNICODE_STRING usDosDeviceName;

    DbgPrint("[+] DrvUnload called");

    RtlInitUnicodeString(&usDosDeviceName, L"\\DosDevices\\Hypervisor");
    IoDeleteSymbolicLink(&usDosDeviceName);
    IoDeleteDevice(pDriverObject->DeviceObject);
}

