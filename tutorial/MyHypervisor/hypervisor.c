#include "pch.h"

#include "hypervisor.h"
#include "common.h"


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath) {
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UINT64 uiIndex = 0;
    PDEVICE_OBJECT pDeviceObject = NULL;
    UNICODE_STRING usDriverName;
    UNICODE_STRING usDosDeviceName;

    UNREFERENCED_PARAMETER(pRegistryPath);

    DbgPrint("[+] DriverEntry called");

    // Create device and alias
    RtlInitUnicodeString(&usDriverName, L"\\Device\\MyHypervisor");
    RtlInitUnicodeString(&usDosDeviceName, L"\\DosDevices\\MyHypervisor");

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

    if (ntStatus == STATUS_SUCCESS) {
        // Major function definitions
        for (uiIndex = 0; uiIndex < IRP_MJ_MAXIMUM_FUNCTION; uiIndex++)
        {
            pDriverObject->MajorFunction[uiIndex] = DrvUnsupported;
        }

        pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DrvClose;
        pDriverObject->MajorFunction[IRP_MJ_CREATE] = DrvCreate;
        pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DrvIoctlDispatcher;
        pDriverObject->MajorFunction[IRP_MJ_READ] = DrvRead;
        pDriverObject->MajorFunction[IRP_MJ_WRITE] = DrvWrite;
    }
    else {
        DbgPrint("[-] Error creating device. Aborting...");
    }

    // ---------------------------------------------------------------------
    // Let's do some VMX shit already ;)
    // ---------------------------------------------------------------------
    PEPTP pEPTP = initializeEPTP();
    DbgPrint("[+] EPTP @ 0x%X\n", pEPTP);

    if (initiateVMX())
    {
        DbgPrint("[+] VMX initialized: OK");
        ntStatus = STATUS_SUCCESS;
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
    NTSTATUS ntStatus = STATUS_FAIL_CHECK;

    DbgPrint("[+] DrvCreate called!");

    pIrp->IoStatus.Status = ntStatus;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return ntStatus;
}

NTSTATUS DrvClose(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    DbgPrint("[+] DrvClose called!");

    terminateVMX();

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
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG inBufferLen, outBufferLen;
    //PCHAR inBuffer, outBuffer;

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

    return ntStatus;
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

    RtlInitUnicodeString(&usDosDeviceName, L"\\DosDevices\\MyHypervisor");
    IoDeleteSymbolicLink(&usDosDeviceName);
    IoDeleteDevice(pDriverObject->DeviceObject);
}
