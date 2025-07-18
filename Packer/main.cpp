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
        return 5;

    return 0;
}