#include <iostream>
#include <fstream>
#include <vector>

#include <windows.h>

#include "arguments.h"
#include "resource.h"

struct PE64
{
    IMAGE_DOS_HEADER dos_header;
    // DOS stub usually here
    uint32_t signature;
    IMAGE_FILE_HEADER file_header;
    IMAGE_OPTIONAL_HEADER64 optional_header;
    std::vector<IMAGE_SECTION_HEADER> sections;
    std::vector<std::vector<uint8_t>> section_data;
};

bool parsePeFromFile(const char* path, PE64& output_pe)
{
    std::ifstream input_file(path, std::ios::binary);

    if (!input_file.is_open())
    {
        std::cerr << "[x] Failed to open file " << path << std::endl;
        return true;
    }

    input_file.read(reinterpret_cast<char*>(&output_pe.dos_header), sizeof(output_pe.dos_header));

    if (output_pe.dos_header.e_magic != IMAGE_DOS_SIGNATURE)
    {
        std::cerr << "[x] Invalid DOS signature" << std::endl;
        return true;
    }

    input_file.seekg(output_pe.dos_header.e_lfanew + 4); // skip the signature (4 bytes)
    if (!input_file.good())
    {
        std::cerr << "[x] Failed to seek to PE header" << std::endl;
        return true;
    }

    input_file.read(reinterpret_cast<char*>(&output_pe.file_header), sizeof(output_pe.file_header));

    if (output_pe.file_header.Machine != IMAGE_FILE_MACHINE_AMD64)
    {
        std::cerr << "[x] File must be x86_64" << std::endl;
        return true;
    }

    if (!(output_pe.file_header.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE))
    {
        std::cerr << "[x] Program cannot be run" << std::endl;
        return true;
    }

    if (output_pe.file_header.Characteristics & IMAGE_FILE_DLL)
    {
        std::cerr << "[x] Program doesn't support DLLs yet" << std::endl;
        return true;
    }

    input_file.read(reinterpret_cast<char*>(&output_pe.optional_header), sizeof(IMAGE_OPTIONAL_HEADER64));

    output_pe.sections.resize(output_pe.file_header.NumberOfSections);
    for (int i{}; i < output_pe.file_header.NumberOfSections; i++)
        input_file.read(reinterpret_cast<char*>(&output_pe.sections[i]), sizeof(IMAGE_SECTION_HEADER));

    output_pe.section_data.resize(output_pe.file_header.NumberOfSections);
    for (int i{}; i < output_pe.file_header.NumberOfSections; i++)
    {
        if (output_pe.sections[i].SizeOfRawData > 0)
        {
            output_pe.section_data[i].resize(output_pe.sections[i].SizeOfRawData);
            input_file.seekg(output_pe.sections[i].PointerToRawData);
            input_file.read(reinterpret_cast<char*>(output_pe.section_data[i].data()), output_pe.sections[i].SizeOfRawData);
        }
    }

    input_file.close();

    return false;
}

bool parsePeFromMemory(const std::uint8_t* p_memory, PE64& output_pe)
{
    std::uint32_t cursor{};

    memcpy(&output_pe.dos_header, p_memory + cursor, sizeof(output_pe.dos_header));
    cursor = sizeof(IMAGE_DOS_HEADER);

    std::size_t dos_stub_size = output_pe.dos_header.e_lfanew - sizeof(IMAGE_DOS_HEADER);
    cursor = output_pe.dos_header.e_lfanew;

    memcpy(&output_pe.signature, p_memory + cursor, sizeof(output_pe.signature));
    cursor += sizeof(output_pe.signature);

    memcpy(&output_pe.file_header, p_memory + cursor, sizeof(output_pe.file_header));
    cursor += sizeof(output_pe.file_header);

    memcpy(&output_pe.optional_header, p_memory + cursor, sizeof(output_pe.optional_header));
    cursor += sizeof(output_pe.optional_header);

    output_pe.sections.resize(output_pe.file_header.NumberOfSections);
    memcpy(output_pe.sections.data(), p_memory + cursor, output_pe.file_header.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));

    return true;
}

int main(int argc, char** argv)
{
    Arguments::Map arguments =
    {
        {"-i", Arguments::Argument(Arguments::Type::STRING, true)},
    };

    int return_code{ Arguments::parse(argc, argv, arguments) };

    if (return_code != 0)
    {
        std::cerr << "[x] Failed to parse arguments with error code " << return_code << std::endl;
        return 1;
    }

    PE64 input_pe;
    auto input_path{ Arguments::getValue<std::string>(arguments, "-i").value() };
    if (parsePeFromFile(input_path.c_str(), input_pe))
        return 1;

    HRSRC h_resource{ FindResourceW(0, MAKEINTRESOURCE(STUB_BINARY), RT_RCDATA) };

    if (!h_resource)
    {
        std::cerr << "[x] Failed to get handle to stub resource" << std::endl;
        return 2;
    }

    HGLOBAL h_memory{ LoadResource(0, h_resource) };

    if (!h_memory)
    {
        std::cerr << "[x] Failed to get handle to resource memory" << std::endl;
        return 3;
    }

    DWORD resource_size{ SizeofResource(0, h_resource) };
    LPVOID p_resource_data{ LockResource(h_memory) };

    if (!p_resource_data || resource_size == 0)
    {
        std::cerr << "[x] Failed to read resource data or size" << std::endl;
        return 4;
    }

    std::vector<std::uint8_t> stub_data(resource_size);
    memcpy(stub_data.data(), p_resource_data, resource_size);

    PE64 stub_pe;
    if (parsePeFromMemory(stub_data.data(), stub_pe))
        return 5;

    stub_data.clear();

    return 0;
}