#include "paging.h"
#include "../memory/memory.h"

UINT64 TranslateLinearAddress(UINT64 DirectoryTableBase, UINT64 VirtualAddress) {
    DirectoryTableBase &= ~0xf;

    UINT64 offset = VirtualAddress & ~(~0ull << PageOffsetSize);
    UINT64 pte = (VirtualAddress >> 12) & 0x1ff;
    UINT64 pt = (VirtualAddress >> 21) & 0x1ff;
    UINT64 pd = (VirtualAddress >> 30) & 0x1ff;
    UINT64 pdp = (VirtualAddress >> 39) & 0x1ff;

    SIZE_T read = 0;
    UINT64 entry = 0;

    
    ReadPhysicalMemory(PVOID(DirectoryTableBase + 8 * pdp), &entry, sizeof(entry), &read);
    if (~entry & 1) return 0;

    
    ReadPhysicalMemory(PVOID((entry & PageMask) + 8 * pd), &entry, sizeof(entry), &read);
    if (~entry & 1) return 0;
    if (entry & 0x80)
        return (entry & (~0ull << 42 >> 12)) + (VirtualAddress & ~(~0ull << 30));

    
    ReadPhysicalMemory(PVOID((entry & PageMask) + 8 * pt), &entry, sizeof(entry), &read);
    if (~entry & 1) return 0;
    if (entry & 0x80)
        return (entry & PageMask) + (VirtualAddress & ~(~0ull << 21));

    
    UINT64 final = 0;
    ReadPhysicalMemory(PVOID((entry & PageMask) + 8 * pte), &final, sizeof(final), &read);
    final &= PageMask;
    if (!final) return 0;

    return final + offset;
}

ULONG64 FindMin(INT32 A, SIZE_T B) {
    INT32 BInt = (INT32)B;
    return (A < BInt) ? A : BInt;
}
