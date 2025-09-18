#include <ntifs.h>
#include "process.h"
#include "../paging/paging.h"
#include "../memory/memory.h"


extern "C" PVOID NTAPI PsGetProcessSectionBaseAddress(PEPROCESS Process);

ULONGLONG GetProcessBaseAddress(PEPROCESS Process) {
    if (!Process) return 0;
    

    PVOID baseAddress = PsGetProcessSectionBaseAddress(Process);
    
    if (!baseAddress) {
        
        __try {
            PPEB pPeb = PsGetProcessPeb(Process);
            if (pPeb && MmIsAddressValid(pPeb)) {
               
                if (MmIsAddressValid(&pPeb->ImageBaseAddress)) {
                    baseAddress = pPeb->ImageBaseAddress;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            baseAddress = nullptr;
        }
    }
    
    return (ULONGLONG)baseAddress;
}

NTSTATUS HandleReadRequest(PReadWriteRequest Request) {
    if (Request->Security != KapaSecure || !Request->ProcessId)
        return STATUS_UNSUCCESSFUL;

    PEPROCESS Process = nullptr;
    NTSTATUS status = PsLookupProcessByProcessId((HANDLE)Request->ProcessId, &Process);
    if (!NT_SUCCESS(status) || !Process)
        return STATUS_UNSUCCESSFUL;

    ULONGLONG cr3 = GetProcessCr3(Process);
    ObDereferenceObject(Process);
    
    if (!cr3) return STATUS_UNSUCCESSFUL;

    SIZE_T offset = 0;
    SIZE_T totalSize = Request->Size;

    INT64 physAddr = TranslateLinearAddress(cr3, Request->Address + offset);
    if (!physAddr) return STATUS_UNSUCCESSFUL;

    ULONG64 finalSize = FindMin(PAGE_SIZE - (physAddr & 0xFFF), totalSize);
    SIZE_T bytesRead = 0;

    // For read operations, we need to copy data to the user buffer
    if (!Request->Write) {
        // Allocate a temporary buffer in kernel space
        PVOID tempBuffer = ExAllocatePoolWithTag(NonPagedPool, finalSize, 'Kapa');
        if (!tempBuffer) return STATUS_INSUFFICIENT_RESOURCES;

        NTSTATUS readStatus = ReadPhysicalMemory(PVOID(physAddr), tempBuffer, finalSize, &bytesRead);
        
        if (NT_SUCCESS(readStatus) && bytesRead > 0) {
            // Copy from kernel buffer to user buffer
            __try {
                MyMemCopy((PVOID)Request->Buffer, tempBuffer, bytesRead);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                ExFreePoolWithTag(tempBuffer, 'Kapa');
                return STATUS_UNSUCCESSFUL;
            }
        }
        
        ExFreePoolWithTag(tempBuffer, 'Kapa');
        return STATUS_SUCCESS;
    }
    else {
        // For write operations, copy from user buffer to kernel buffer first
        PVOID tempBuffer = ExAllocatePoolWithTag(NonPagedPool, finalSize, 'Kapa');
        if (!tempBuffer) return STATUS_INSUFFICIENT_RESOURCES;

        // Copy from user buffer to kernel buffer
        __try {
            MyMemCopy(tempBuffer, (PVOID)Request->Buffer, finalSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            ExFreePoolWithTag(tempBuffer, 'Kapa');
            return STATUS_UNSUCCESSFUL;
        }

        SIZE_T bytesWritten = 0;
        NTSTATUS writeStatus = WritePhysicalMemory(PVOID(physAddr), tempBuffer, finalSize, &bytesWritten);
        
        ExFreePoolWithTag(tempBuffer, 'Kapa');
        return STATUS_SUCCESS;
    }
}

NTSTATUS HandleWriteRequest(PReadWriteRequest Request) {
    if (Request->Security != KapaSecure || !Request->ProcessId)
        return STATUS_UNSUCCESSFUL;

    PEPROCESS Process = nullptr;
    NTSTATUS status = PsLookupProcessByProcessId((HANDLE)Request->ProcessId, &Process);
    if (!NT_SUCCESS(status) || !Process)
        return STATUS_UNSUCCESSFUL;

    ULONGLONG cr3 = GetProcessCr3(Process);
    ObDereferenceObject(Process);
    
    if (!cr3) {
        DbgPrint("[KapaKernel] Write failed: Could not get CR3 for process %d\n", Request->ProcessId);
        return STATUS_UNSUCCESSFUL;
    }

    SIZE_T offset = 0;
    SIZE_T totalSize = Request->Size;

    // First check if the virtual address is valid
    if (!MmIsAddressValid((PVOID)Request->Address)) {
        DbgPrint("[KapaKernel] Write failed: Virtual address 0x%llx is not valid\n", Request->Address);
        return STATUS_UNSUCCESSFUL;
    }
    
    INT64 physAddr = TranslateLinearAddress(cr3, Request->Address + offset);
    if (!physAddr) {
        DbgPrint("[KapaKernel] Write failed: Could not translate virtual address 0x%llx\n", Request->Address);
        return STATUS_UNSUCCESSFUL;
    }

    ULONG64 finalSize = FindMin(PAGE_SIZE - (physAddr & 0xFFF), totalSize);
    
    DbgPrint("[KapaKernel] Write: Virtual=0x%llx, Physical=0x%llx, Size=%llu\n", 
             Request->Address, physAddr, finalSize);
    
    // For write operations, copy from user buffer to kernel buffer first
    PVOID tempBuffer = ExAllocatePoolWithTag(NonPagedPool, finalSize, 'Kapa');
    if (!tempBuffer) {
        DbgPrint("[KapaKernel] Write failed: Could not allocate buffer\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Copy from user buffer to kernel buffer
    __try {
        MyMemCopy(tempBuffer, (PVOID)Request->Buffer, finalSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        DbgPrint("[KapaKernel] Write failed: Exception copying user buffer\n");
        ExFreePoolWithTag(tempBuffer, 'Kapa');
        return STATUS_UNSUCCESSFUL;
    }

    SIZE_T bytesWritten = 0;
    NTSTATUS writeStatus = WritePhysicalMemory(PVOID(physAddr), tempBuffer, finalSize, &bytesWritten);
    
    DbgPrint("[KapaKernel] Write result: Status=0x%x, BytesWritten=%zu\n", writeStatus, bytesWritten);
    
    ExFreePoolWithTag(tempBuffer, 'Kapa');
    
    // Return the actual write status instead of always SUCCESS
    return writeStatus;
}

NTSTATUS HandleBaseAddressRequest(PBaseAddressRequest Request) {
    if (Request->Security != KapaSecure || !Request->ProcessId)
        return STATUS_UNSUCCESSFUL;

    PEPROCESS Process = nullptr;
    NTSTATUS status = PsLookupProcessByProcessId((HANDLE)Request->ProcessId, &Process);
    if (!NT_SUCCESS(status) || !Process)
        return STATUS_UNSUCCESSFUL;

    ULONGLONG base = GetProcessBaseAddress(Process);
    ObDereferenceObject(Process);
    
    if (!base) return STATUS_UNSUCCESSFUL;

    
    Request->Address = base; 
    return STATUS_SUCCESS;
}

NTSTATUS HandleCr3Request(PCr3Request Request) {
    if (Request->Security != KapaSecure || !Request->ProcessId)
        return STATUS_UNSUCCESSFUL;

    PEPROCESS Process = nullptr;
    NTSTATUS status = PsLookupProcessByProcessId((HANDLE)Request->ProcessId, &Process);
    if (!NT_SUCCESS(status) || !Process)
        return STATUS_UNSUCCESSFUL;

    ULONGLONG cr3 = GetProcessCr3(Process);
    ObDereferenceObject(Process);
    
    if (!cr3) return STATUS_UNSUCCESSFUL;

    Request->Cr3 = cr3; 
    return STATUS_SUCCESS;
}

NTSTATUS IoControlHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG bytesReturned = 0;
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

    ULONG IoControlCode = stack->Parameters.DeviceIoControl.IoControlCode;
    ULONG InputBufferLength = stack->Parameters.DeviceIoControl.InputBufferLength;

    if (IoControlCode == KapaRead && InputBufferLength == sizeof(ReadWriteRequest)) {
        status = HandleReadRequest((PReadWriteRequest)Irp->AssociatedIrp.SystemBuffer);
        bytesReturned = sizeof(ReadWriteRequest);
    }
    else if (IoControlCode == KapaWrite && InputBufferLength == sizeof(ReadWriteRequest)) {
        status = HandleWriteRequest((PReadWriteRequest)Irp->AssociatedIrp.SystemBuffer);
        bytesReturned = sizeof(ReadWriteRequest);
    }
    else if (IoControlCode == KapaBase && InputBufferLength == sizeof(BaseAddressRequest)) {
        status = HandleBaseAddressRequest((PBaseAddressRequest)Irp->AssociatedIrp.SystemBuffer);
        bytesReturned = sizeof(BaseAddressRequest);
    }
    else if (IoControlCode == KapaCr3 && InputBufferLength == sizeof(Cr3Request)) {
        status = HandleCr3Request((PCr3Request)Irp->AssociatedIrp.SystemBuffer);
        bytesReturned = sizeof(Cr3Request);
    }
    else {
        status = STATUS_INFO_LENGTH_MISMATCH;
        bytesReturned = 0;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = bytesReturned;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS DispatchHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);

    switch (Stack->MajorFunction) {
    case IRP_MJ_CREATE:
    case IRP_MJ_CLOSE:
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;
    default:
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;
    }
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Irp->IoStatus.Status;
}

NTSTATUS UnsupportedDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Irp->IoStatus.Status;
}

UINT64 GetProcessCr3(PEPROCESS Process) {
    if (!Process) return 0;
    
 
    if (!MmIsAddressValid(Process)) return 0;
    
    uintptr_t dirbase = 0;
    
    __try {
        
        dirbase = *(uintptr_t*)((UINT8*)Process + 0x28);
        
        
        if (!dirbase) {
            ULONG versionOffset = GetWindowsVersion();
            if (versionOffset && MmIsAddressValid((UINT8*)Process + versionOffset)) {
                dirbase = *(uintptr_t*)((UINT8*)Process + versionOffset);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        dirbase = 0;
    }

    
    if ((dirbase >> 0x38) == 0x40) {
        uintptr_t savedDirBase = 0;
        
        __try {
            KAPC_STATE apc{};
            KeStackAttachProcess((PRKPROCESS)Process, &apc);
            savedDirBase = __readcr3();
            KeUnstackDetachProcess(&apc);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            savedDirBase = 0;
        }
        
        if (savedDirBase) return savedDirBase;
    }

    return dirbase;
}