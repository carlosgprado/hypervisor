VOID DrvUnload(PDRIVER_OBJECT pDriverObject);
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath);

// Major function declarations
NTSTATUS DrvUnsupported(PDEVICE_OBJECT pDeviceObject, PIRP pIRP);
NTSTATUS DrvCreate(PDEVICE_OBJECT pDeviceObject, PIRP pIRP);
NTSTATUS DrvIoctlDispatcher(PDEVICE_OBJECT pDeviceObject, PIRP pIRP);
NTSTATUS DrvRead(PDEVICE_OBJECT pDeviceObject, PIRP pIRP);
NTSTATUS DrvWrite(PDEVICE_OBJECT pDeviceObject, PIRP pIRP);

