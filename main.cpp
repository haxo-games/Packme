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
    std::ifstream input_file(input_path.c_str(), std::ios::binary);

    if (!input_file.is_open())
    {
        std::cerr << "[x] Failed to open file " << input_path << std::endl;
        return 2;
    }

    IMAGE_DOS_HEADER dos_header{};
    input_file.read(reinterpret_cast<char*>(&dos_header), sizeof(dos_header));

	return 0;
}