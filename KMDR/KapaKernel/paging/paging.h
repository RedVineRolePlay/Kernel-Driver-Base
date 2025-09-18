#pragma once
#include "../dependencies/includes.h"

UINT64 TranslateLinearAddress(UINT64 DirectoryTableBase, UINT64 VirtualAddress);
ULONG64 FindMin(INT32 A, SIZE_T B);

#define PageOffsetSize 12
static const UINT64 PageMask = (~0xfull << 8) & 0xfffffffffull;
