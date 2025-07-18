#pragma once

#include <iostream>
#include <fstream>
#include <vector>

#include <windows.h>

namespace PE64
{
    //
    // [SECTION] Types
    //

    struct PE
    {
        IMAGE_DOS_HEADER dos_header;
        std::vector<uint8_t> dos_stub;
        uint32_t signature;
        IMAGE_FILE_HEADER file_header;
        IMAGE_OPTIONAL_HEADER64 optional_header;
        std::vector<IMAGE_SECTION_HEADER> sections;
        std::vector<std::vector<uint8_t>> section_data;
    };

    //
    // [SECTION] Functions
    //

    bool parsePeFromFile(const char* path, PE& output_pe)
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

        std::size_t dos_stub_size{ output_pe.dos_header.e_lfanew - sizeof(IMAGE_DOS_HEADER) };
        output_pe.dos_stub.resize(dos_stub_size);

        input_file.read(reinterpret_cast<char*>(output_pe.dos_stub.data()), dos_stub_size);
        input_file.read(reinterpret_cast<char*>(&output_pe.signature), sizeof(output_pe.signature));

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

    bool parsePeFromMemory(const std::uint8_t* p_memory, PE& output_pe)
    {
        std::uint32_t cursor{};

        memcpy(&output_pe.dos_header, p_memory + cursor, sizeof(output_pe.dos_header));
        cursor = sizeof(IMAGE_DOS_HEADER);

        std::size_t dos_stub_size{ output_pe.dos_header.e_lfanew - sizeof(IMAGE_DOS_HEADER) };
        output_pe.dos_stub.resize(dos_stub_size);
        memcpy(output_pe.dos_stub.data(), p_memory + cursor, dos_stub_size);
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

    bool parsePeFromResource(HRSRC h_resource, PE& output_pe)
    {
        HGLOBAL h_memory{ LoadResource(0, h_resource) };

        if (!h_memory)
        {
            std::cerr << "[x] Failed to get handle to resource memory" << std::endl;
            return true;
        }

        DWORD resource_size{ SizeofResource(0, h_resource) };
        LPVOID p_resource_data{ LockResource(h_memory) };

        if (!p_resource_data || resource_size == 0)
        {
            std::cerr << "[x] Failed to read resource data or size" << std::endl;
            return true;
        }

        if (PE64::parsePeFromMemory(static_cast<const std::uint8_t*>(p_resource_data), output_pe))
            return true;

        return false;
    }
}