#include <iostream>

#include <windows.h>

#include "arguments.h"
#include "resource.h"
#include "pe64.h"

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

    PE64::PE input_pe;
    auto input_path{ Arguments::getValue<std::string>(arguments, "-i").value() };
    if (PE64::parsePeFromFile(input_path.c_str(), input_pe))
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

    PE64::PE stub_pe;
    if (PE64::parsePeFromMemory(stub_data.data(), stub_pe))
        return 5;

    stub_data.clear();



    return 0;
}