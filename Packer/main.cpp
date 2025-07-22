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

    PE64::PE input_pe;
    auto input_path{ Arguments::getValue<std::string>(arguments, "-i").value() };
    if (PE64::parsePeFromFile(input_path.c_str(), input_pe))
        return 1;

    HRSRC h_stub_resource{ FindResourceW(0, MAKEINTRESOURCE(STUB_BINARY), RT_RCDATA) };

    if (!h_stub_resource)
    {
        std::cerr << "[x] Failed to get handle to stub resource" << std::endl;
        return 2;
    }

    PE64::PE stub_pe;
    if (PE64::parsePeFromResource(h_stub_resource, stub_pe))
        return 3;

    auto compressed_input_pe{ input_pe.zlibCompress() };

    IMAGE_SECTION_HEADER new_section{};
    memcpy(new_section.Name, ".packed", 8);
    new_section.VirtualAddress = stub_pe.optional_header.SizeOfImage;
    new_section.Misc.VirtualSize = compressed_input_pe.size();
    new_section.PointerToRawData = stub_pe.sections.back().PointerToRawData + stub_pe.sections.back().SizeOfRawData;
    new_section.SizeOfRawData = compressed_input_pe.size();
    new_section.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;

    stub_pe.sections.push_back(new_section);
    stub_pe.section_data.push_back(compressed_input_pe);
    stub_pe.file_header.NumberOfSections++;
    stub_pe.optional_header.SizeOfImage += compressed_input_pe.size();

    // Stub project is configured to have 3 secttions: .text, .rdata and .data (data comes last).
    StubConfig* p_stub_stub_config{ reinterpret_cast<StubConfig*>(Utils::stupidPatternScanData((uint8_t*)(stub_config.signature), sizeof(stub_config.signature), stub_pe.section_data[2].data(), stub_pe.sections[2].SizeOfRawData)) };
    if (p_stub_stub_config == 0)
    {
        std::cerr << "[x] Failed to find match for stub config pattern in stub" << std::endl;
        return 4;
    }

    // Setup config and write to stub
    stub_config.packed_data_rva = new_section.VirtualAddress;
    stub_config.packed_data_size = compressed_input_pe.size();
    stub_config.original_data_size = input_pe.getSize();
    memcpy(p_stub_stub_config, reinterpret_cast<void*>(const_cast<StubConfig*>(&stub_config)), sizeof(stub_config));

    std::ofstream output("packed.exe", std::ios::binary);
    auto raw_stub{ stub_pe.getRaw() };
    output.write(reinterpret_cast<const char*>(raw_stub.data()), raw_stub.size());
    output.close();

    return 0;
}