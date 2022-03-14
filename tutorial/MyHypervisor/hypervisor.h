#pragma once

#ifndef _H_HYPERVISOR
#define _H_HYPERVISOR

// Executed when unloading the driver, doh
VOID DrvUnload(PDRIVER_OBJECT pDriverObject);

// This is the entry-point for the driver
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);

// Major function declarations
NTSTATUS DrvUnsupported(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DrvCreate(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DrvClose(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DrvIoctlDispatcher(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DrvRead(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DrvWrite(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

#endif
