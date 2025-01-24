# Screen Off Utility

A lightweight Windows utility to quickly turn off your screen. Built with native Win32 API for optimal performance and modern Windows 11 visual style.

## Features

- One-click screen turn off
- Auto turn off on startup option (saved in registry)
- Modern Windows 11 UI design with rounded corners
- Minimal memory footprint
- No installation required (portable)

## Building from source

### Prerequisites
- MinGW-w64
- Windows SDK
- Resource compiler (windres)

### Build commands
```bash
# Compile resources
windres resource.rc -O coff -o resource.res

# Build executable
g++ -o screen_off.exe main.cpp resource.res -mwindows -static-libgcc -static-libstdc++ -lcomctl32 -luxtheme -ldwmapi
```

## Usage

Simply run `screen_off.exe`. The application provides:
- A button to instantly turn off your screen
- A checkbox to enable automatic screen turn off when starting the application
- Your preferences are automatically saved in the Windows registry

## License

This project is licensed under the MIT License - see the LICENSE file for details
