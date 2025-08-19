
# Bambu Lab IOTC Library

A clean-room implementation of the IOTC (Internet of Things Communication) library used by Bambu Lab 3D printers, along with a reconstructed Android application.

## Project Structure

```
├── app/                          # Android application
│   ├── src/main/
│   │   ├── java/bbl/intl/bambulab/com/    # Main app code
│   │   ├── cpp/                           # JNI bridge
│   │   ├── assets/native/                 # Native libraries
│   │   └── res/                          # Android resources
│   └── build.gradle
├── native/                       # Native IOTC library
│   ├── include/libIOTCAPIsT.h   # Header file
│   ├── libIOTCAPIsT.c           # Implementation
│   ├── lib/libIOTCAPIsT.so      # Compiled library
│   └── docs/                    # ThroughTek SDK documentation
├── research/                     # Research materials
│   ├── decompiled/              # Decompiled original libraries
│   └── analysis/                # Analysis notes and data
├── tests/                       # Test files
├── example.py                   # Python proof-of-concept
├── PROTOCOL.md                  # Protocol documentation
└── Makefile                     # Build configuration
```

## Features

- **Clean IOTC Implementation**: Drop-in replacement for ThroughTek's IOTCAPIs library
- **Android App**: Reconstructed Bambu Handy app for printer monitoring
- **Protocol Documentation**: Detailed breakdown of the communication protocol
- **Cross-platform**: Works on Linux, Android, and other platforms

## Building

### Native Library
```bash
make                    # Build the library
make test              # Run tests
make clean             # Clean build artifacts
```

### Android App
```bash
make android           # Build APK
```

## Getting Started

1. **Read the Protocol**: Check `PROTOCOL.md` for communication details
2. **Try the Example**: Run `python example.py` for a Python implementation
3. **Build Native Library**: Use `make` to compile the C library
4. **Build Android App**: Use `make android` for the mobile app

## Research

The `research/` directory contains:
- Decompiled original libraries (`AVAPIs.c`, `IOTC.c`)
- Analysis of server addresses and protocol details
- Original Windows DLLs and documentation

## License

Educational and research purposes only. Not affiliated with Bambu Lab.
