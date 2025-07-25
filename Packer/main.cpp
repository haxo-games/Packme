#include <iostream>

#include <windows.h>

#include "arguments.h"
#include "resource.h"
#include "pe64.h"
#include "utils.h"
#include "../shared/common.h"

int main(int argc, char** argv)
{
    // Yes an entire little arguments parsing system seems overkill, but it's here for future use
    Arguments::Map arguments =
    {
        {"-i", Arguments::Argument(Arguments::Type::STRING, true)},
    };

    if (int return_code{ Arguments::parse(argc, argv, arguments) }; return_code != 0)
    {
        std::cerr << "[x] Failed to parse arguments with error code " << return_code << std::endl;
        return 1;
    }

    std::cout << "[+] Reading input...\n";
    PE64::PE input_pe;
    auto input_path{ Arguments::getValue<std::string>(arguments, "-i").value() };
    if (PE64::parsePeFromFile(input_path.c_str(), input_pe))
        return 1;

    HRSRC h_stub_resource{ FindResourceW(0, MAKEINTRESOURCE(IDR_EXECUTABLE_STUB), RT_RCDATA) };

    if (!h_stub_resource)
    {
        std::cerr << "[x] Failed to get handle to stub resource" << std::endl;
        return 2;
    }

    PE64::PE stub_pe;
    if (PE64::parsePeFromResource(h_stub_resource, stub_pe))
        return 3;

    auto compressed_input_pe{ input_pe.zlibCompress() };
    size_t original_compressed_pe_size{ compressed_input_pe.size() };
    size_t new_compressed_pe_size{ Utils::align<std::size_t>(compressed_input_pe.size(), stub_pe.optional_header.FileAlignment) };
    IMAGE_SECTION_HEADER* p_last_physical_section{ stub_pe.findLastPhysicalSection() };
    IMAGE_SECTION_HEADER* p_last_virtual_section{ stub_pe.findLastVirtualSection() };

    std::cout << "[+] Compressing input...\n";
    compressed_input_pe.resize(new_compressed_pe_size);
    std::cout << "[+] Input compressed!\n";

    // Setup the new section which will contain the packed binary
    IMAGE_SECTION_HEADER new_section{};
    memcpy(new_section.Name, ".packed", 8);
    new_section.VirtualAddress = Utils::align<DWORD>(p_last_virtual_section->VirtualAddress + p_last_virtual_section->Misc.VirtualSize, stub_pe.optional_header.SectionAlignment);
    new_section.Misc.VirtualSize = original_compressed_pe_size;
    new_section.PointerToRawData = Utils::align<DWORD>(p_last_physical_section->PointerToRawData + p_last_physical_section->SizeOfRawData, stub_pe.optional_header.FileAlignment);
    new_section.SizeOfRawData = new_compressed_pe_size;
    new_section.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;

    stub_pe.optional_header.Subsystem = input_pe.optional_header.Subsystem;
    stub_pe.insertSection(new_section, compressed_input_pe);

    auto p_init_data_section{ stub_pe.findSectionByName(".data") };
    if (!p_init_data_section)
    {
        std::cerr << "[x] Failed to find \".data\" section" << std::endl;
        return 4;
    }

    auto p_init_data{ stub_pe.getSectionData(*p_init_data_section) };
    if (!p_init_data)
    {
        std::cerr << "[x] Failed to find \".data\" section's data" << std::endl;
        return 5;
    }

    StubConfig* p_stub_stub_config{ reinterpret_cast<StubConfig*>(Utils::stupidPatternScanData((uint8_t*)(stub_config.signature), sizeof(stub_config.signature), p_init_data, p_init_data_section->SizeOfRawData)) };
    if (p_stub_stub_config == 0)
    {
        std::cerr << "[x] Failed to find match for stub config pattern in stub" << std::endl;
        return 6;
    }

    // Setup config and write to stub
    stub_config.packed_data_rva = new_section.VirtualAddress;
    stub_config.packed_data_size = original_compressed_pe_size;
    stub_config.original_data_size = input_pe.getSize();
    memcpy(p_stub_stub_config, reinterpret_cast<void*>(const_cast<StubConfig*>(&stub_config)), sizeof(stub_config));

    std::ofstream output("packed.exe", std::ios::binary);
    auto raw_stub{ stub_pe.getRaw() };
    output.write(reinterpret_cast<const char*>(raw_stub.data()), raw_stub.size());
    output.close();

    std::cout << "[+] PE packed! Check \"packed.exe\" for the result." << std::endl;

    return 0;
}