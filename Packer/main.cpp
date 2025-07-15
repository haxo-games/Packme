#include <iostream>
#include <fstream>

#include <windows.h>

#include "arguments.h"

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

    auto input_path{ Arguments::getValue<std::string>(arguments, "-i").value() };
    std::ifstream input_file(input_path, std::ios::binary);

    if (!input_file.is_open())
    {
        std::cerr << "[x] Failed to open file " << input_path << std::endl;
        return 2;
    }

    IMAGE_DOS_HEADER dos_header;
    input_file.read(reinterpret_cast<char*>(&dos_header), sizeof(dos_header));

    if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
    {
        std::cerr << "[x] Invalid DOS signature" << std::endl;
        return 3;
    }

    input_file.seekg(dos_header.e_lfanew + 4); // skip the signature (4 bytes)
    if (!input_file.good())
    {
        std::cerr << "[x] Failed to seek to PE header" << std::endl;
        return 4;
    }

    IMAGE_FILE_HEADER file_header;
    input_file.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));

    if (file_header.Machine != IMAGE_FILE_MACHINE_AMD64)
    {
        std::cout << "[x] File must be x86_64" << std::endl;
        return 5;
    }

    if (!(file_header.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE))
    {
        std::cout << "[x] Program cannot be run" << std::endl;
        return 6;
    }

    if (file_header.Characteristics & IMAGE_FILE_DLL)
    {
        std::cout << "[x] Program doesn't support DLLs yet" << std::endl;
        return 7;
    }

    IMAGE_OPTIONAL_HEADER64 optional_header;
    input_file.read(reinterpret_cast<char*>(&optional_header), sizeof(IMAGE_OPTIONAL_HEADER64));

    std::vector<IMAGE_SECTION_HEADER> sections(file_header.NumberOfSections);
    for (int i = 0; i < file_header.NumberOfSections; i++)
        input_file.read(reinterpret_cast<char*>(&sections[i]), sizeof(IMAGE_SECTION_HEADER));

    std::vector<std::vector<uint8_t>> section_data(file_header.NumberOfSections);
    for (int i = 0; i < file_header.NumberOfSections; i++)
    {
        if (sections[i].SizeOfRawData > 0)
        {
            section_data[i].resize(sections[i].SizeOfRawData);
            input_file.seekg(sections[i].PointerToRawData);
            input_file.read(reinterpret_cast<char*>(section_data[i].data()), sections[i].SizeOfRawData);
        }
    }

    input_file.close();

    return 0;
}