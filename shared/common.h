#pragma once

#include <cstdint>

struct StubConfig
{
	char signature[8]{ 'H', 'A', 'X', 'O', 'S', 'T', 'U', 'B' };
	uint32_t packed_data_rva;
	uint32_t packed_data_size;
};