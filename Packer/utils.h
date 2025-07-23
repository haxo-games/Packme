#pragma once

#include <iostream>

#include <cstdint>

namespace Utils
{
    size_t stupidPatternScanData(const uint8_t* scan_for, unsigned int size_of_scan_for, const uint8_t* p_target, size_t target_size)
    {
        unsigned int cursor{};

        for (size_t i{}; i < target_size; i++)
        {
            if (p_target[i] == scan_for[cursor])
            {
                cursor++;
                if (cursor == size_of_scan_for)
                    return reinterpret_cast<size_t>(&p_target[i - size_of_scan_for + 1]);
            }
            else
            {
                if (cursor > 0 && p_target[i] == scan_for[0])
                    cursor = 1;
                else
                    cursor = 0;
            }
        }

        std::cerr << "[x] Failed to find pattern match" << std::endl;
        return 0;
    }

    template<typename T> 
    T align(T value, T alignment)
    {
        // We subtract 1 to handle the already-aligned case correctly.
        // Without -1: align(4096, 4096) = (4096+4096)/4096 * 4096 = 8192 <-- BAD
        // With -1: align(4096, 4096) = (4096+4095)/4096 * 4096 = 4096    <-- GOOD
        return ((value + alignment - 1) / alignment) * alignment;
    }
}