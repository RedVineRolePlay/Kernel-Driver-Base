#pragma once
#include <windef.h>

#define KapaRead CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1457, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define KapaWrite CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1458, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define KapaBase CTL_CODE(FILE_DEVICE_UNKNOWN, 0x158B, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define KapaCr3  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1590, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define KapaSecure 0x9A3F7D2

#pragma pack(push, 1)
typedef struct _ReadWriteRequest {
    INT32 Security;
    INT32 ProcessId;
    ULONGLONG Address;
    ULONGLONG Buffer;
    ULONGLONG Size;
    BOOLEAN Write;
} ReadWriteRequest, * PReadWriteRequest;

typedef struct _BaseAddressRequest {
    INT32 Security;
    INT32 ProcessId;
    ULONGLONG Address; // Value not pointer
} BaseAddressRequest, * PBaseAddressRequest;

typedef struct _Cr3Request {
    INT32 Security;
    INT32 ProcessId;
    ULONGLONG Cr3; // Value not pointer
} Cr3Request, * PCr3Request;
#pragma pack(pop)