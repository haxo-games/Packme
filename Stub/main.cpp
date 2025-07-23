#include <windows.h>
#include <zlib.h>

#include "../shared/common.h"

#pragma comment(lib, "libucrt.lib")

void main()
{
	// DO NOT REMOVE! PREVENTS COMPILER FROM OPTIMIZING AWAY
	(void)stub_config;

	HMODULE h_module{ GetModuleHandle(0) };
	if (!h_module)
		ExitProcess(1);

	uint8_t* p_packed{ reinterpret_cast<uint8_t*>(h_module) + stub_config.packed_data_rva };

	void* p_unpacked{ VirtualAlloc(nullptr, stub_config.original_data_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) };
	if (!p_unpacked)
		ExitProcess(2);

	uLongf dest_len{ stub_config.original_data_size };
	int return_code{ uncompress(static_cast<Bytef*>(p_unpacked), &dest_len, p_packed, stub_config.packed_data_size)};

	if (return_code != Z_OK || dest_len != stub_config.original_data_size)
	{
		VirtualFree(p_unpacked, 0, MEM_RELEASE);
		ExitProcess(3);
	}

	IMAGE_DOS_HEADER* p_dos_header{ reinterpret_cast<IMAGE_DOS_HEADER*>(p_unpacked) };
	IMAGE_NT_HEADERS64* p_nt_headers{ reinterpret_cast<IMAGE_NT_HEADERS64*>(reinterpret_cast<uintptr_t>(p_unpacked) + p_dos_header->e_lfanew) };
	uintptr_t delta_address{ reinterpret_cast<uintptr_t>(p_unpacked) - p_nt_headers->OptionalHeader.ImageBase};

	/* Perform relocation */
	if (delta_address)
	{
		if (!p_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
			ExitProcess(4);
	}

	VirtualFree(p_unpacked, 0, MEM_RELEASE);

	ExitProcess(0);
}