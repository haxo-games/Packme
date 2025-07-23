#pragma once

#include <cstdint>

extern "C"
{
    struct StubConfig
    {
        char signature[8];
        unsigned long packed_data_rva;
        unsigned long packed_data_size;
        unsigned long original_data_size;
    } volatile stub_config = { {'H', 'A', 'X', 'O', 'S', 'T', 'U', 'B'} };
}