ZLIB SDK PLACEMENT INSTRUCTIONS
================================

Please extract your zlib SDK files into the following structure:

X-TO-FBX/third_party/zlib/
├── include/           <- Place header files here (.h files)
│   ├── zlib.h
│   ├── zconf.h
│   └── ...
├── lib/
│   ├── x64/
│   │   ├── Release/   <- Place release library files here
│   │   │   ├── zlib.lib
│   │   │   ├── zlibstatic.lib
│   │   │   └── ...
│   │   └── Debug/     <- Place debug library files here (optional)
│   │       ├── zlibd.lib
│   │       ├── zlibstaticd.lib
│   │       └── ...
│   └── x86/           <- 32-bit libraries (optional)
└── bin/               <- Place DLL files here (if using dynamic linking)
    ├── x64/
    │   ├── Release/
    │   │   └── zlib.dll
    │   └── Debug/
    │       └── zlibd.dll
    └── x86/

REQUIRED FILES:
- include/zlib.h      (main zlib header)
- include/zconf.h     (configuration header)
- lib/x64/Release/zlib.lib or zlibstatic.lib

OPTIONAL FILES:
- Debug versions of libraries
- DLL files (if using dynamic linking)
- 32-bit versions

After placing the files, delete this README_PLACEMENT.txt file.
