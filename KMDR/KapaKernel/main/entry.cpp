#include "../dependencies/includes.h"
#include "../main/comms.h"
#include "../process/process.h"

UNICODE_STRING DriverName, SymbolicLinkName;
extern "C" NTSTATUS NTAPI IoCreateDriver(PUNICODE_STRING DriverName, PDRIVER_INITIALIZE InitializationFunction);
void UnloadDriver(PDRIVER_OBJECT DriverObject) {
    IoDeleteSymbolicLink(&SymbolicLinkName);
    IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS InitializeDriver(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    RtlInitUnicodeString(&DriverName, L"\\Device\\{6b61706173657276696365736f6e746f70}");
    RtlInitUnicodeString(&SymbolicLinkName, L"\\DosDevices\\{6b61706173657276696365736f6e746f70}");

    PDEVICE_OBJECT DeviceObject = nullptr;
    NTSTATUS status = IoCreateDevice(DriverObject, 0, &DriverName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);
    if (!NT_SUCCESS(status)) {
        DbgPrint("[KapaKernel] Failed to create device: 0x%08X\n", status);
        return status;
    }

    status = IoCreateSymbolicLink(&SymbolicLinkName, &DriverName);
    if (!NT_SUCCESS(status)) {
        DbgPrint("[KapaKernel] Failed to create symbolic link: 0x%08X\n", status);
        IoDeleteDevice(DeviceObject);
        return status;
    }

    for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        DriverObject->MajorFunction[i] = &UnsupportedDispatch;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = &DispatchHandler;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = &DispatchHandler;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = &IoControlHandler;
    DriverObject->DriverUnload = &UnloadDriver;

    DeviceObject->Flags |= DO_BUFFERED_IO;
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    DbgPrint("[KapaKernel] Driver initialized successfully\n");
    return STATUS_SUCCESS;
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
        UNREFERENCED_PARAMETER(DriverObject);
        UNREFERENCED_PARAMETER(RegistryPath);


        return IoCreateDriver(NULL, &InitializeDriver);
    }