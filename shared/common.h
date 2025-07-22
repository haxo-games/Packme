#pragma once

#include <cstdint>

extern "C"
{
    struct StubConfig
    {
        char signature[8];
        unsigned int packed_data_rva;
        unsigned int packed_data_size;
        unsigned int original_data_size;
    } volatile stub_config = { {'H', 'A', 'X', 'O', 'S', 'T', 'U', 'B'} };
}