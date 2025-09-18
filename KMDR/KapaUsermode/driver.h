#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>

#define KapaRead CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1457, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define KapaWrite CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1458, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define KapaBase CTL_CODE(FILE_DEVICE_UNKNOWN, 0x158B, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define KapaCr3  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1590, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define KapaSecure 0x9A3F7D2

#pragma pack(push, 1)
typedef struct _ReadWriteRequest {
    int32_t Security;
    int32_t ProcessId;
    uint64_t Address;
    uint64_t Buffer;
    uint64_t Size;
    bool Write;
} ReadWriteRequest, * PReadWriteRequest;

typedef struct _BaseAddressRequest {
    int32_t Security;
    int32_t ProcessId;
    uint64_t Address; 
} BaseAddressRequest, * PBaseAddressRequest;

typedef struct _Cr3Request {
    int32_t Security;
    int32_t ProcessId;
    uint64_t Cr3; 
} Cr3Request, * PCr3Request;
#pragma pack(pop)

namespace DriverHandle {
    HANDLE DriverHandle;
    INT32 ProcessIdentifier;
    bool init() {
        DriverHandle = CreateFileW(L"\\\\.\\{6b61706173657276696365736f6e746f70}", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        return (DriverHandle != INVALID_HANDLE_VALUE);
    }

    bool ReadPhysicalMemory(uint64_t address, void* buffer, size_t size) {
        if (DriverHandle == INVALID_HANDLE_VALUE) return false;
        ReadWriteRequest request = { 0 };
        request.Security = KapaSecure;
        request.ProcessId = ProcessIdentifier;
        request.Address = address;
        request.Buffer = (uint64_t)buffer;
        request.Size = size;
        request.Write = false;
        DWORD bytesReturned;
        return DeviceIoControl(DriverHandle, KapaRead, &request, sizeof(request), &request, sizeof(request), &bytesReturned, nullptr) != 0;
    }

    bool WritePhysicalMemory(uint64_t address, void* buffer, size_t size) {
        if (DriverHandle == INVALID_HANDLE_VALUE) return false;
        ReadWriteRequest request = { 0 };
        request.Security = KapaSecure;
        request.ProcessId = ProcessIdentifier;
        request.Address = address;
        request.Buffer = (uint64_t)buffer;
        request.Size = size;
        request.Write = true;
        DWORD bytesReturned;
        return DeviceIoControl(DriverHandle, KapaWrite, &request, sizeof(request), &request, sizeof(request), &bytesReturned, nullptr) != 0;
    }

    bool GetBaseAddress(uint64_t* address) {
        if (DriverHandle == INVALID_HANDLE_VALUE) return false;
        BaseAddressRequest request = { 0 };
        request.Security = KapaSecure;
        request.ProcessId = ProcessIdentifier;
        DWORD bytesReturned;
        bool success = DeviceIoControl(DriverHandle, KapaBase, &request, sizeof(request), &request, sizeof(request), &bytesReturned, nullptr) != 0;
        if (success) {
            *address = request.Address;
        }
        return success;
    }

    bool GetCr3(uint64_t* cr3) {
        if (DriverHandle == INVALID_HANDLE_VALUE) return false;
        Cr3Request request = { 0 };
        request.Security = KapaSecure;
        request.ProcessId = ProcessIdentifier;
        DWORD bytesReturned;
        bool success = DeviceIoControl(DriverHandle, KapaCr3, &request, sizeof(request), &request, sizeof(request), &bytesReturned, nullptr) != 0;
        if (success) {
            *cr3 = request.Cr3;
        }
        return success;
    }

    INT32 GetProcessId(const wchar_t* processName) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return 0;
        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32W);
        if (Process32FirstW(snapshot, &processEntry)) {
            do {
                if (_wcsicmp(processEntry.szExeFile, processName) == 0) {
                    CloseHandle(snapshot);
                    return processEntry.th32ProcessID;
                }
            } while (Process32NextW(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
        return 0;
    }
}