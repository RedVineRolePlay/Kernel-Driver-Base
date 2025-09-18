#include "memory.h"
extern "C"
NTSTATUS
MmCopyVirtualMemory(
    PEPROCESS SourceProcess,
    PVOID SourceAddress,
    PEPROCESS TargetProcess,
    PVOID TargetAddress,
    SIZE_T BufferSize,
    KPROCESSOR_MODE PreviousMode,
    PSIZE_T ReturnSize
);


NTSTATUS ReadPhysicalMemory(PVOID TargetAddress, PVOID Buffer, SIZE_T Size, SIZE_T* BytesRead) {
    if (!TargetAddress || !Buffer || !Size || !BytesRead) {
        return STATUS_INVALID_PARAMETER;
    }
    
    *BytesRead = 0;
    
    __try {
        MM_COPY_ADDRESS copy = {};
        copy.PhysicalAddress.QuadPart = (LONGLONG)TargetAddress;
        NTSTATUS status = MmCopyMemory(Buffer, copy, Size, MM_COPY_MEMORY_PHYSICAL, BytesRead);
        return status;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return STATUS_UNSUCCESSFUL;
    }
}
NTSTATUS WriteVirtualMemory(
    PEPROCESS TargetProcess,
    PVOID TargetAddress,
    PVOID Buffer,
    SIZE_T Size,
    SIZE_T* BytesWritten
) {
    if (!TargetAddress || !Buffer || !Size || !BytesWritten || !TargetProcess) {
        DbgPrint("[KapaKernel] WriteVirtualMemory: Invalid parameters\n");
        return STATUS_INVALID_PARAMETER;
    }

    *BytesWritten = 0;

    return MmCopyVirtualMemory(
        PsGetCurrentProcess(),    // Source: current (driver)
        Buffer,                   // Buffer with data to write
        TargetProcess,            // Destination process
        TargetAddress,            // Destination address in target
        Size,                     // Number of bytes
        KernelMode,               // Previous mode
        BytesWritten
    );
}


INT32 GetWindowsVersion() {
    RTL_OSVERSIONINFOW info = {};
    info.dwOSVersionInfoSize = sizeof(info);
    
    NTSTATUS status = RtlGetVersion(&info);
    if (!NT_SUCCESS(status))
        return 0x0388; 

    switch (info.dwBuildNumber) {
    case 17134: return 0x0278; // Windows 10 1803
    case 17763: return 0x0278; // Windows 10 1809
    case 18362: return 0x0280; // Windows 10 1903
    case 18363: return 0x0280; // Windows 10 1909
    case 19041: return 0x0388; // Windows 10 2004
    case 19569: return 0x0388; // Windows 10 20H2
    case 20180: return 0x0388; // Windows 10 21H1
    case 22000: return 0x0388; // Windows 11 21H2
    case 22621: return 0x0388; // Windows 11 22H2
    case 22631: return 0x0388; 
    case 26100: return 0x0388; 
    default:    return 0x0388; 
    }
}

void MyMemCopy(PVOID dest, PVOID src, SIZE_T size) {
    if (!dest || !src || !size) return;
    
    __try {
        auto* d = reinterpret_cast<volatile unsigned char*>(dest);
        auto* s = reinterpret_cast<volatile unsigned char*>(src);

        for (SIZE_T i = 0; i < size; ++i)
            d[i] = s[i];
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        
    }
}