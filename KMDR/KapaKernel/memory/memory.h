#pragma once
#include "../dependencies/includes.h"

NTSTATUS ReadPhysicalMemory(PVOID TargetAddress, PVOID Buffer, SIZE_T Size, SIZE_T* BytesRead);
NTSTATUS WritePhysicalMemory(PVOID TargetAddress, PVOID Buffer, SIZE_T Size, SIZE_T* BytesWritten);
INT32 GetWindowsVersion();
void MyMemCopy(PVOID dest, PVOID src, SIZE_T size);
