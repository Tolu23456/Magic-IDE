# Magic IDE

A modern, fast, and powerful IDE built with C and web technologies. Magic IDE aims to provide VS Code-like functionality while being lighter and more efficient.

## Features

- **Monaco Editor Integration**: Full-featured code editor with syntax highlighting for 20+ languages
- **Multi-file Tabs**: Open and edit multiple files simultaneously
- **File Explorer**: Navigate and manage your project files
- **Integrated Terminal**: Built-in terminal with command execution
- **Find & Replace**: Powerful search and replace functionality
- **Settings Management**: Customizable IDE preferences
- **Language Server Protocol**: Foundation for IntelliSense and advanced language features
- **Git Integration**: Basic Git UI integration
- **Plugin Architecture**: Extensible through plugins

## Architecture

Magic IDE uses a hybrid architecture:

- **Backend (C)**: GTK+ 3.0 with WebKitGTK for native performance
- **Frontend (Web)**: HTML/CSS/JavaScript with Monaco Editor
- **IPC**: JSON-RPC communication between backend and frontend
- **Build System**: CMake for cross-platform builds

## Requirements

### Dependencies

- GTK+ 3.0
- WebKitGTK 4.1
- JSON-C
- PCRE (Perl Compatible Regular Expressions)
- CMake 3.10+
- GCC/Clang

### Installing Dependencies (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config
sudo apt install libgtk-3-dev libwebkit2gtk-4.1-dev libjson-c-dev libpcre3-dev
```

### Installing Dependencies (macOS with Homebrew)

```bash
brew install cmake pkg-config gtk+3 webkitgtk json-c pcre
```

## Building

### Quick Build

```bash
# Clone and build in one command
./build.sh
```

### Manual Build

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
make

# Run tests (optional)
make test

# Install (optional)
sudo make install
```

## Running

```bash
# From build directory
cd build
./magic-ide
```

## Project Structure

```
Magic-IDE/
├── CMakeLists.txt          # Build configuration
├── build.sh               # One-command build script
├── README.md              # This file
├── PLAN.md                # Development roadmap
├── .gitignore             # Git ignore rules
├── src/                   # Source code
│   ├── main.c            # Application entry point
│   ├── webview.c         # WebView management
│   ├── ipc.c             # Inter-process communication
│   ├── file_manager.c    # File I/O operations
│   ├── terminal.c        # Terminal emulation
│   └── lsp.c             # Language server protocol
├── include/               # Header files
│   ├── webview.h
│   ├── ipc.h
│   ├── file_manager.h
│   ├── terminal.h
│   └── lsp.h
├── web/                   # Frontend web assets
│   ├── index.html        # Main UI
│   ├── js/
│   │   └── app.js       # Frontend application logic
│   └── css/
│       └── main.css      # UI styling
├── tests/                 # Unit tests
│   └── test_file_manager.c
└── docs/                  # Documentation
```

## Development

### Code Style

- Use C11 standard
- Follow GTK+ coding conventions
- Add documentation for public APIs
- Use meaningful variable and function names
- Handle errors gracefully

### Adding New Features

1. Plan the feature in `PLAN.md`
2. Implement backend functionality in appropriate module
3. Add IPC handlers if frontend interaction is needed
4. Update frontend JavaScript and HTML as needed
5. Add tests for new functionality
6. Update documentation

### Testing

```bash
# Build and run tests
cd build
make test
```

### Code Formatting

```bash
# Format code (requires clang-format)
cd build
make format
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Roadmap

See `PLAN.md` for detailed development roadmap and upcoming features.

## Troubleshooting

### Build Issues

- Ensure all dependencies are installed
- Check CMake version (3.10+ required)
- Verify pkg-config can find all libraries

### Runtime Issues

- Make sure you're running on a system with GUI support
- Check that WebKitGTK is properly installed
- Verify file permissions for project directory

### Common Errors

- **WebView not loading**: Check WebKitGTK installation
- **IPC communication failing**: Verify JSON-C library
- **File operations failing**: Check file permissions

## Performance

Magic IDE is designed for performance:

- Native C backend for fast file operations
- Efficient IPC with minimal overhead
- Lazy loading of editor components
- Optimized memory usage

## Security

- Input validation on all IPC messages
- Safe file operations with path validation
- No arbitrary code execution from frontend
- Proper error handling to prevent crashes
