#include <iostream>

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

	uintptr_t p_unpacked{ reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr, stub_config.original_data_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)) };
	if (!p_unpacked)
		return 2;

	uLongf dest_len{ stub_config.original_data_size };
	int return_code{ uncompress(reinterpret_cast<Bytef*>(p_unpacked), &dest_len, p_packed, stub_config.packed_data_size) };

	if (return_code != Z_OK)
	{
		VirtualFree(reinterpret_cast<void*>(p_unpacked), 0, MEM_RELEASE);
		return 3;
	}

	IMAGE_DOS_HEADER* p_dos_header{ reinterpret_cast<IMAGE_DOS_HEADER*>(p_unpacked) };
	IMAGE_NT_HEADERS64* p_nt_headers{ reinterpret_cast<IMAGE_NT_HEADERS64*>(p_unpacked + p_dos_header->e_lfanew) };

	uintptr_t p_final_unpacked{ reinterpret_cast<uintptr_t>(VirtualAlloc((void*)p_nt_headers->OptionalHeader.ImageBase, p_nt_headers->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)) }; // TEMP SETTING WITH EXECUTE TO CHANGE

	if (!p_final_unpacked)
		return 4;

	// Copy all headers
	size_t size_of_headers{ p_nt_headers->OptionalHeader.SizeOfHeaders };
	memcpy(reinterpret_cast<void*>(p_final_unpacked), reinterpret_cast<void*>(p_unpacked), size_of_headers);

	// Process sections
	IMAGE_SECTION_HEADER* p_section_headers{ IMAGE_FIRST_SECTION(p_nt_headers) };
	for (int i{}; i < p_nt_headers->FileHeader.NumberOfSections; i++)
	{
		IMAGE_SECTION_HEADER* current_section{ &p_section_headers[i] };
		uintptr_t dest_va{ p_final_unpacked + current_section->VirtualAddress };

		if (current_section->SizeOfRawData > 0)
		{
			uintptr_t src_raw = p_unpacked + current_section->PointerToRawData;
			memcpy(reinterpret_cast<void*>(dest_va), reinterpret_cast<void*>(src_raw), current_section->SizeOfRawData);

			if (current_section->Misc.VirtualSize > current_section->SizeOfRawData)
				memset(reinterpret_cast<void*>(dest_va + current_section->SizeOfRawData), 0, current_section->Misc.VirtualSize - current_section->SizeOfRawData);
		}
		else if (current_section->Misc.VirtualSize > 0)
			memset(reinterpret_cast<void*>(dest_va), 0, current_section->Misc.VirtualSize);
	}

	// Free old unpacked and assign p_unpacked to the final one
	VirtualFree(reinterpret_cast<void*>(p_unpacked), 0, MEM_RELEASE);
	p_unpacked = p_final_unpacked;

	p_dos_header = reinterpret_cast<IMAGE_DOS_HEADER*>(p_unpacked);
	p_nt_headers = reinterpret_cast<IMAGE_NT_HEADERS64*>(p_unpacked + p_dos_header->e_lfanew);

	uint32_t delta_address{ static_cast<uint32_t>(p_unpacked - p_nt_headers->OptionalHeader.ImageBase) };

	/* Perform relocation */
	if (delta_address)
	{
		if (!p_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
			return 5;

		IMAGE_BASE_RELOCATION* p_relocation{ reinterpret_cast<IMAGE_BASE_RELOCATION*>(p_unpacked + p_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress) };
		while (reinterpret_cast<uintptr_t>(p_relocation) < p_unpacked + p_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + p_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
		{
			uint16_t* p_relocation_entry{ reinterpret_cast<uint16_t*>(reinterpret_cast<uintptr_t>(p_relocation) + sizeof(IMAGE_BASE_RELOCATION)) };
			for (int i{}; i < (p_relocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(uint16_t); i++)
			{

				uint8_t type{ static_cast<uint8_t>((p_relocation_entry[i] & 0xF000) >> 12) };
				uintptr_t location{ p_unpacked + p_relocation->VirtualAddress + (p_relocation_entry[i] & 0x0FFF) };

				uint32_t full_value{};
				uint16_t next_entry{};

				// https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#base-relocation-types
				switch (type)
				{
				case IMAGE_REL_BASED_HIGH:
					*reinterpret_cast<uint16_t*>(location) += static_cast<uint16_t>(delta_address >> 16);
					break;
				case IMAGE_REL_BASED_LOW:
					*reinterpret_cast<uint16_t*>(location) += static_cast<uint16_t>(delta_address);
					break;
				case IMAGE_REL_BASED_HIGHLOW:
					*reinterpret_cast<uint32_t*>(location) += delta_address;
					break;
				case IMAGE_REL_BASED_HIGHADJ:
					full_value = (static_cast<uint32_t>(*reinterpret_cast<uint16_t*>(location)) << 16) + next_entry + delta_address;
					next_entry = p_relocation_entry[i + 1];
					*reinterpret_cast<uint16_t*>(location) = static_cast<uint16_t>(full_value >> 16);
					i++;
					break;
				case IMAGE_REL_BASED_DIR64:
					*reinterpret_cast<uint64_t*>(location) += delta_address;
					break;
				default:
					return 9;
				}
			}

			p_relocation = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<uintptr_t>(p_relocation) + p_relocation->SizeOfBlock);
		}
	}

	/* Fix imports */
	if (p_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)
	{
		IMAGE_IMPORT_DESCRIPTOR* p_import_descriptor{ reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(p_unpacked + p_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress) };
		while (p_import_descriptor->Name != 0)
		{
			const char* module_name{ reinterpret_cast<char*>(p_unpacked + p_import_descriptor->Name) };
			HMODULE h_module{ LoadLibraryA(module_name) };

			if (!h_module)
				return 6;

			IMAGE_THUNK_DATA* p_int{ reinterpret_cast<IMAGE_THUNK_DATA*>(p_unpacked + p_import_descriptor->OriginalFirstThunk) };
			IMAGE_THUNK_DATA* p_iat{ reinterpret_cast<IMAGE_THUNK_DATA*>(p_unpacked + p_import_descriptor->FirstThunk) };

			while (p_int->u1.AddressOfData != 0)
			{
				FARPROC function_address{};

				if (IMAGE_SNAP_BY_ORDINAL(p_int->u1.Ordinal))
				{
					WORD ordinal{ IMAGE_ORDINAL(p_int->u1.Ordinal) };
					function_address = GetProcAddress(h_module, MAKEINTRESOURCEA(ordinal));
				}
				else
				{
					IMAGE_IMPORT_BY_NAME* p_import_by_name = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(p_unpacked + p_int->u1.AddressOfData);
					function_address = GetProcAddress(h_module, p_import_by_name->Name);
				}

				if (function_address == nullptr)
					return 7;

				p_iat->u1.Function = reinterpret_cast<ULONG_PTR>(function_address);

				p_int++;
				p_iat++;
			}

			p_import_descriptor++;
		}
	}
	
	p_section_headers = IMAGE_FIRST_SECTION(p_nt_headers);
	for (int i{}; i < p_nt_headers->FileHeader.NumberOfSections; i++)
	{
		IMAGE_SECTION_HEADER* current_section{ &p_section_headers[i] };

		if (current_section->Misc.VirtualSize == 0)
			continue;

		uintptr_t dest_va{ p_unpacked + current_section->VirtualAddress };

		DWORD protection{ PAGE_NOACCESS };
		if (current_section->Characteristics & IMAGE_SCN_MEM_READ)
		{
			if (current_section->Characteristics & IMAGE_SCN_MEM_EXECUTE)
			{
				if (current_section->Characteristics & IMAGE_SCN_MEM_WRITE)
					protection = PAGE_EXECUTE_READWRITE;  // RWX
				else
					protection = PAGE_EXECUTE_READ;       // RX (.text)
			}
			else
			{
				if (current_section->Characteristics & IMAGE_SCN_MEM_WRITE)
					protection = PAGE_READWRITE;          // RW (.data, .bss)
				else
					protection = PAGE_READONLY;           // R (.rdata)
			}
		}

		DWORD old_protection;
		if (!VirtualProtect(reinterpret_cast<void*>(dest_va), current_section->Misc.VirtualSize, protection, &old_protection))
			return 8;
	}

	DWORD old_protection;
	VirtualProtect(reinterpret_cast<void*>(p_unpacked), p_nt_headers->OptionalHeader.SizeOfHeaders, PAGE_READONLY, &old_protection);

	// This assumes a console app entry point
	uintptr_t p_entry{ p_unpacked + p_nt_headers->OptionalHeader.AddressOfEntryPoint };
	int result{ reinterpret_cast<int(*)()>(p_unpacked + p_nt_headers->OptionalHeader.AddressOfEntryPoint)()};

	VirtualFree(reinterpret_cast<void*>(p_unpacked), 0, MEM_RELEASE);
	return result;
}