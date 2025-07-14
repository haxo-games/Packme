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

    input_file.seekg(dos_header.e_lfanew + 4); // skip the signature (4)
    if (!input_file.good()) 
    {
        std::cerr << "[x] Failed to seek to PE header" << std::endl;
        return 4;
    }

    IMAGE_FILE_HEADER file_header;
    input_file.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));

	return 0;
}