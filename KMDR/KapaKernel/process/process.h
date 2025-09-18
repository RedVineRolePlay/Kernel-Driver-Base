#pragma once

#include "../dependencies/includes.h"
#include "../main/comms.h"
#include "ntifs.h"


extern "C" PVOID NTAPI PsGetProcessSectionBaseAddress(PEPROCESS Process);


NTSTATUS IoControlHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DispatchHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS UnsupportedDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp);


NTSTATUS HandleReadRequest(PReadWriteRequest Request);
NTSTATUS HandleWriteRequest(PReadWriteRequest Request);
NTSTATUS HandleBaseAddressRequest(PBaseAddressRequest Request);
NTSTATUS HandleCr3Request(PCr3Request Request);


UINT64 GetProcessCr3(PEPROCESS Process);
ULONGLONG GetProcessBaseAddress(PEPROCESS Process);


typedef struct _PEB {
    BYTE Reserved1[2];
    BYTE BeingDebugged;
    BYTE Reserved2[1];
    PVOID Reserved3[2];
    PVOID ImageBaseAddress;
} PEB, * PPEB;


extern "C" PPEB PsGetProcessPeb(PEPROCESS Process);