# BZip2 Installation Verification

## Required Files

### Headers (Mandatory)
- `include/bzlib.h` - Main bzip2 header

### Libraries (at least one is mandatory)
For 64-bit Windows:
- `lib/x64/Release/libbz2.lib` OR
- `lib/x64/Release/bzip2.lib` OR
- `lib/x64/Release/bz2.lib`

For 32-bit Windows:
- `lib/x86/Release/libbz2.lib` OR
- `lib/x86/Release/bzip2.lib` OR
- `lib/x86/Release/bz2.lib`

For Linux/macOS:
- `lib/libbz2.a` OR
- `lib/libbzip2.a`

### Optional
- Debug versions (libbz2d.lib, bzip2d.lib)
- DLLs if using dynamic linking (libbz2.dll, bzip2.dll)

## Verification

To verify files are in place:

### Windows (PowerShell)
```powershell
# From X-TO-FBX directory
Get-ChildItem -Recurse third_party/bzip2 -Include *.h,*.lib,*.dll
```

### Windows (CMD)
```cmd
# From X-TO-FBX directory
dir third_party\bzip2\include\*.h
dir third_party\bzip2\lib\x64\Release\*.lib
```

### Linux/macOS
```bash
# From X-TO-FBX directory
find third_party/bzip2 -name "*.h" -o -name "*.lib" -o -name "*.a"
```

## Compilation Test

Once you've placed the files, test the configuration:

```bash
cd X-TO-FBX
mkdir build_test
cd build_test
cmake ..
```

If you see messages like:
```
-- Found bundled bzip2 SDK at: /path/to/X-TO-FBX/third_party/bzip2
-- bzip2 Include Dir: /path/to/X-TO-FBX/third_party/bzip2/include
-- bzip2 Library: /path/to/X-TO-FBX/third_party/bzip2/lib/x64/Release/libbz2.lib
```

Then the configuration is successful!

## Download bzip2

You can download bzip2 from:
- **Windows**: https://sourceforge.net/projects/gnuwin32/files/bzip2/
- **Linux**: Use package manager: `sudo apt-get install libbz2-dev` (Ubuntu/Debian)
- **macOS**: Use Homebrew: `brew install bzip2`

## Troubleshooting

### Error: "bzip2 headers not found"
- Verify that `bzlib.h` is in `third_party/bzip2/include/`
- Download bzip2 development headers

### Error: "bzip2 library not found"
- Verify you have at least one .lib/.a file in the appropriate directory
- For 64-bit: `third_party/bzip2/lib/x64/Release/`
- For 32-bit: `third_party/bzip2/lib/x86/Release/`
- For Linux/macOS: `third_party/bzip2/lib/`

### Warning: "bzip2 decompression will be disabled"
- This is not fatal - the converter will still work with zlib and uncompressed files
- To enable bzip2 support, install the bzip2 library as described above

### Linking errors
- If using DLLs, ensure they're in PATH or output directory
- Consider using static versions (libbz2.lib) for simplicity
