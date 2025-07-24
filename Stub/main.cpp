#include <cstdio>

#include <windows.h>
#include <zlib.h>

#include "../shared/common.h"

int main()
{
	// DO NOT REMOVE! PREVENTS COMPILER FROM OPTIMIZING AWAY
	(void)stub_config;

	HMODULE h_module{ GetModuleHandle(0) };
	if (!h_module)
		return 1;

	uint8_t* p_packed{ reinterpret_cast<uint8_t*>(h_module) + stub_config.packed_data_rva };

	void* p_unpacked{ VirtualAlloc(nullptr, stub_config.original_data_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE) };
	if (!p_unpacked)
		return 2;

	uLongf dest_len{ stub_config.original_data_size };
	int return_code{ uncompress(static_cast<Bytef*>(p_unpacked), &dest_len, p_packed, stub_config.packed_data_size) };

	if (return_code != Z_OK)
	{
		VirtualFree(p_unpacked, 0, MEM_RELEASE);
		return 3;
	}

	IMAGE_DOS_HEADER* p_dos_header{ reinterpret_cast<IMAGE_DOS_HEADER*>(p_unpacked) };
	IMAGE_NT_HEADERS64* p_nt_headers{ reinterpret_cast<IMAGE_NT_HEADERS64*>(reinterpret_cast<uintptr_t>(p_unpacked) + p_dos_header->e_lfanew) };
	uintptr_t delta_address{ reinterpret_cast<uintptr_t>(p_unpacked) - p_nt_headers->OptionalHeader.ImageBase };

	/* Perform relocation */
	if (delta_address)
	{
		if (!p_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
			return 4;

		IMAGE_BASE_RELOCATION* p_relocation{ reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<uintptr_t>(p_unpacked) + p_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress) };
		while (reinterpret_cast<uintptr_t>(p_relocation) < reinterpret_cast<uintptr_t>(p_unpacked) + p_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + p_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
		{
			uint16_t* p_relocation_entry{ reinterpret_cast<uint16_t*>(reinterpret_cast<uintptr_t>(p_relocation) + sizeof(IMAGE_BASE_RELOCATION)) };
			for (int i{}; i < (p_relocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(uint16_t); i++)
			{
				uint8_t type{ (p_relocation_entry[i] & 0xF000) >> 12 };
				uintptr_t location{ reinterpret_cast<uintptr_t>(p_unpacked) + p_relocation->VirtualAddress + (p_relocation_entry[i] & 0x0FFF) };
				
				// https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#base-relocation-types
				switch (type)
				{
				case IMAGE_REL_BASED_ABSOLUTE:
					break;
				case IMAGE_REL_BASED_HIGH:
					break;
				case IMAGE_REL_BASED_LOW:
					break;
				case IMAGE_REL_BASED_HIGHLOW:
					break;
				case IMAGE_REL_BASED_HIGHADJ:
					break;
				case IMAGE_REL_BASED_DIR64:
					break;
				}
			}

			p_relocation = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<uintptr_t>(p_relocation) + p_relocation->SizeOfBlock);
		}
	}

	VirtualFree(p_unpacked, 0, MEM_RELEASE);

	return 0;
}