# X2FBX Converter

A professional C++ tool for converting DirectX .x files to FBX format with proper animation timing correction and bone hierarchy preservation.

## 🚀 Features

- **Complete .x File Parsing**: Supports DirectX .x files (text format)
- **Critical Animation Timing Fix**: Automatically detects and corrects animation timing issues
- **Multiple Animation Export**: Exports each animation as a separate FBX file
- **Bone Hierarchy Preservation**: Maintains skeleton structure and transformations
- **Detailed Logging**: Comprehensive logging with timing analysis
- **Validation System**: Ensures data integrity throughout conversion
- **Command Line Interface**: Easy to use console application

## 📋 Requirements

### System Requirements
- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.20 or higher
- FBX SDK 2020.3+ (optional, for full FBX export functionality)

### Supported Platforms
- Linux (Ubuntu 20.04+, CentOS 8+)
- Windows (Windows 10+)
- macOS (10.15+)

## 🛠 Building

### 1. Clone the Repository
```bash
git clone <repository-url>
cd x2fbx-converter
```

### 2. Install Dependencies

#### Option A: With FBX SDK (Recommended)
1. Download FBX SDK from [Autodesk Developer Portal](https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-3-4)
2. Extract to `third_party/fbx_sdk/` or set `FBX_SDK_ROOT` environment variable

#### Option B: Without FBX SDK (Basic functionality)
The converter will work without FBX SDK but will create placeholder FBX files instead of actual FBX exports.

### 3. Build with CMake

#### Linux/macOS:
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

#### Windows (Visual Studio):
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

### 4. Run Tests (Optional)
```bash
cd build
ctest --verbose
```

## 🎯 Usage

### Basic Usage
```bash
./x2fbx-converter input_file.x
```

### Advanced Usage
```bash
./x2fbx-converter [OPTIONS] input_file.x

Options:
  -h, --help                    Show help message
  -v, --version                 Show version information
  -o, --output <directory>      Output directory (default: ./output)
  --verbose                     Enable verbose logging
  --strict                      Enable strict parsing mode
  --no-timing-validation        Disable animation timing validation
  --no-report                   Don't generate conversion report
  --log-level <level>           Set log level (debug, info, warning, error)
```

### Examples

Convert a character model with animations:
```bash
./x2fbx-converter character.x
```

Convert with verbose output to custom directory:
```bash
./x2fbx-converter --verbose --output ./fbx_files character.x
```

Convert with debug logging and strict mode:
```bash
./x2fbx-converter --strict --log-level debug model.x
```

## 📂 Output Files

The converter creates separate FBX files for each animation found in the .x file:

- `mesh_walk.fbx` - Walking animation
- `mesh_run.fbx` - Running animation
- `mesh_idle.fbx` - Idle animation
- etc.

If no animations are found, a single `mesh.fbx` file is created with the static mesh.

## 🔧 Critical Animation Timing Fix

This converter specifically addresses the common issue where DirectX .x animations have incorrect timing when converted to FBX. The problem occurs because:

1. **DirectX .x files** use `TicksPerSecond` (typically 4800)
2. **FBX files** use different timing units
3. **Incorrect conversion** results in animations lasting thousands of frames

### How the Fix Works

1. **Automatic Detection**: Analyzes keyframe patterns to detect correct timing
2. **Timing Validation**: Ensures animation durations are reasonable (0.05s to 10 minutes)
3. **Correction Algorithm**: Adjusts timing to preserve original animation speed
4. **Validation**: Verifies corrections maintain animation integrity

### Timing Detection Methods

- Parse `AnimTicksPerSecond` from .x file headers
- Analyze keyframe intervals for frame-based patterns
- Test common DirectX tick rates (160, 1000, 2400, 4800, 9600)
- Use heuristics to score timing candidates

## 📊 Validation and Quality Assurance

The converter includes comprehensive validation:

### Mesh Validation
- Vertex count and face indices validation
- Material reference validation
- UV coordinate verification

### Skeleton Validation
- Bone hierarchy integrity
- Bind pose matrix validation
- Skin weight normalization

### Animation Validation
- Keyframe timing consistency
- Bone reference validation
- Duration reasonableness checks

## 🐛 Troubleshooting

### Common Issues

**"Invalid or non-existent .x file"**
- Ensure the file exists and has read permissions
- Verify the file has a valid .x file signature

**"Animation timing correction failed"**
- The .x file may have corrupt animation data
- Try running with `--verbose` to see detailed error messages
- Check the log file for specific validation errors

**"Failed to create output directory"**
- Ensure you have write permissions to the output directory
- Check available disk space

### Debug Mode

For detailed troubleshooting, run with debug logging:
```bash
./x2fbx-converter --log-level debug --verbose input.x
```

This will create a detailed log file `x2fbx_converter.log` with timing analysis and validation results.

## 📁 Project Structure

```
x2fbx-converter/
├── src/
│   ├── core/               # Core conversion logic
│   ├── parsers/            # .x file parsing
│   ├── exporters/          # FBX export (when SDK available)
│   ├── utils/              # Utilities and helpers
│   ├── data/               # Data structures
│   └── main.cpp            # Application entry point
├── include/                # Header files
├── tests/                  # Test suite
├── examples/               # Example .x files
├── third_party/            # External dependencies
└── CMakeLists.txt          # Build configuration
```

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes with appropriate tests
4. Ensure all tests pass
5. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## ⚠️ Known Limitations

- Binary .x files are not yet supported (text format only)
- Compressed .x files are not supported
- Some advanced .x features may not be fully supported
- Without FBX SDK, only placeholder files are generated

## 🔄 Future Enhancements

- Binary .x file support
- Full FBX SDK integration
- Advanced material conversion
- Texture path resolution
- Animation blending support
- GUI version

## 📞 Support

For issues and questions:
1. Check the troubleshooting section
2. Review the log files for detailed error information
3. Create an issue with detailed reproduction steps

---

**Note**: This converter was specifically designed to solve critical animation timing issues when converting from DirectX .x to FBX format. The timing correction algorithm is the core feature that ensures animations maintain their original speed and duration.
