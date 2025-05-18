# GitNCurses

A terminal-based Git wrapper using ncurses that provides an interactive interface for Git commands.

## Prerequisites

- CMake (version 3.10 or higher)
- C++ compiler with C++17 support
- ncurses development library
- Git

## Building the Application

1. Create a build directory:
```bash
mkdir build
cd build
```

2. Generate build files:
```bash
cmake ..
```

3. Build the application:
```bash
make
```

## Running the Application

After building, you can run the application from the build directory:
```bash
./gitNCurses
```

## Usage

The application provides a simple interface for executing Git commands:

- Type Git commands directly after the `git>` prompt
- Use `help` to see available commands
- Use `exit` to quit the application

## Features

- Interactive terminal interface
- Command history
- Real-time command output
- Error handling
- Color support
- Scrollable output window

## License

This project is open source and available under the MIT License. 