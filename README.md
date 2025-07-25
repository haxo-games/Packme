# Packme

## What is this?

A simple command-line packer for Windows x86_64 executables. Like many of our public projects this was made because we needed it to better explain 
some of [our courses](https://haxo.games/courses).

## How does it work?

The Visual Studio project as of writing this consists of two solutions: "Packer" and "Stub". The project is configured to compile "Stub" first and then "Packer". The reason
for that has to do with how the packer handles the stub. It is directly added to its resources which the packer will just read and add a section containing the compressed PE.
It then locates a specific structures within the stub's `.data` section and sets its fields to specify where the packed section is, how big it is and how much space it requires once
it is unpacked.

## Build

All this requires is Visual Studio 2022 (minimum) and the "Desktop Development with C++" package installed. The reason this uses Visual Studio in the first place and not a
cleaner build system like [CMake](https://cmake.org/) is because I want to keep the build environment as simplistic and similar to what is used throughout the course as possible for beginners.

## Usage

In a terminal run the built executable with the following arguments provided according to your needs:
| Option | Type   | Required | Description                    |
|--------|--------|----------|--------------------------------|
| `-i`   | string | Yes      | Input file to pack             |

## Contributing

This project is free and open source and will remain so forever. You are welcome to contribute. Simply make a pull request for whatever it is you would like to add, but
there are a few things you need to keep in mind:
1. C++17 only for now.
2. snake_case for variable names, PascalCase for namespaces and structures and camelCase for methods and function names (there might be more specefics so please just
base your style on the already existing code).
3. Make an issue describing your feature or bugfix and specify your intent to address said issue. Once you make the pull request you can auto-close the issue by doing `close #<issue_number_here>`.
4. When making a branch give it a meaningful name like `feature/name_of_feature` or `fix/name_of_fix`.

## License

This project is licensed under the MIT License with some specifications - see the [LICENSE](LICENSE) file for details.
