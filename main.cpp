#include <iostream>

#include "arguments.h"

int main(int argc, char** argv)
{
    Arguments::Map arguments = 
    {
        {"-i", {Arguments::Type::BOOLEAN, true, std::nullopt}},
    };

    int return_code{ Arguments::parse(argc, argv, arguments) };

    if (return_code != 0)
    {
        std::cerr << "[x] Failed to parse arguments with error code " << return_code << std::endl;
        return 1;
    }

	return 0;
}