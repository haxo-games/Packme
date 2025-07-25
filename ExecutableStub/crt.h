#pragma once

#include <windows.h>

#pragma function(memcpy)
#pragma optimize("", off)
#pragma function(memset)
#pragma optimize("", off)

extern "C"
{
    void* memcpy(void* dest, const void* src, size_t count) 
    {
        char* d = (char*)dest;
        const char* s = (const char*)src;
        for (size_t i = 0; i < count; i++)
            d[i] = s[i];

        return dest;
    }

    __declspec(restrict) __declspec(noalias) void* malloc(size_t size)
    {
        return HeapAlloc(GetProcessHeap(), 0, size);
    }

    void free(void* ptr) 
    {
        if (ptr)
            HeapFree(GetProcessHeap(), 0, ptr);
    }

    void* __cdecl memset(void* dest, int value, size_t count) 
    {
        if (!dest) return dest;
        char* d = (char*)dest;
        char val = (char)value;
        while (count--) *d++ = val;
        return dest;
    }
}