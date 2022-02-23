VOID DrvUnload(PDRIVER_OBJECT pDriverObject);
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);

// Major function declarations
NTSTATUS DrvUnsupported(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DrvCreate(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DrvClose(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DrvIoctlDispatcher(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DrvRead(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS DrvWrite(PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

