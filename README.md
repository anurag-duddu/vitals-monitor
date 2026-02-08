# Bedside Vitals Monitor

A bedside vitals monitor for Indian hospital general wards, targeting CDSCO Class B regulatory approval.

**Hardware:** STM32MP157F-DK2 (Cortex-A7 + M4, 512MB RAM, 4" touchscreen)
**Software:** Embedded Linux (Buildroot) + LVGL UI framework
**License:** Open source only, no GPL in application code (MIT/LGPL OK)

## Quick Start (Mac Simulator)

### Prerequisites

- macOS (Apple Silicon or Intel)
- Homebrew
- SDL2
- CMake

All prerequisites are already installed on this system.

### Building the Simulator

```bash
cd simulator/build
cmake ..
cmake --build . -j8
```

### Running the Simulator

```bash
./simulator
```

The simulator window will open at 800x480 resolution (matching the target hardware). Use your mouse to interact with the touchscreen interface. Press `Ctrl+C` in the terminal to exit.

## Project Status

**Phase 0: Development Environment Setup** ✅ COMPLETE

- [x] Mac development environment ready
- [x] LVGL v9.3 simulator running
- [x] Project repository initialized
- [x] Basic demo screen displaying

**Next:** Phase 1 - Basic UI Framework

## Project Structure

```
vitals-monitor/
├── README.md                 # This file
├── files/
│   ├── CLAUDE.md             # Development guide
│   ├── PRD.md                # Product requirements
│   └── ROADMAP.md            # Implementation roadmap
├── src/                      # Application source (future)
├── simulator/                # Mac SDL simulator
│   ├── build/                # Build output
│   ├── lvgl/                 # LVGL v9.3 library
│   ├── CMakeLists.txt        # Build configuration
│   ├── lv_conf.h             # LVGL configuration
│   ├── main.c                # Simulator main
│   ├── sdl_display.c/h       # SDL display driver
│   └── sdl_input.c/h         # SDL input driver
├── buildroot-external/       # Buildroot (future)
├── tests/                    # Tests (future)
└── scripts/                  # Scripts (future)
```

## Architecture

### Simulator Architecture

The Mac simulator uses a custom SDL2 integration for LVGL v9:

- **sdl_display.c/h**: Display driver that creates an SDL2 window and maps LVGL's framebuffer to an SDL texture
- **sdl_input.c/h**: Input driver that translates SDL mouse events to LVGL touch events
- **main.c**: Application entry point with event loop

This approach is simpler and more maintainable than using the legacy `lv_drivers` repository, which is designed for LVGL v8.

### Key Design Decisions

1. **LVGL v9.3**: Latest stable version with improved API
2. **Custom SDL wrapper**: Clean integration without legacy dependencies
3. **Direct render mode**: Framebuffer directly mapped to SDL texture for performance
4. **800x480 resolution**: Matches target hardware display

## Development Workflow

### Daily Development (Mac)

```bash
cd simulator/build
cmake --build .
./simulator
```

The simulator uses SDL2 to render LVGL in a window. Mouse = touch.

### Target Build (Future)

Docker-based Buildroot environment for cross-compilation to STM32MP157F-DK2.

## Documentation

- [files/CLAUDE.md](files/CLAUDE.md) - Complete development guide and patterns
- [files/ROADMAP.md](files/ROADMAP.md) - Phased implementation plan (Phase 0-13)
- [files/PRD.md](files/PRD.md) - Product requirements document

## Technology Stack

| Layer | Technology | Notes |
|-------|------------|-------|
| UI | LVGL 9.3 | MIT license, C API |
| Language | C (C99) | LVGL native, embedded standard |
| Build (Mac) | CMake + SDL2 | Simulator for development |
| Build (Target) | Buildroot | Docker container (future) |
| Database | SQLite 3 | Trends, config, audit logs (future) |
| IPC | nanomsg | Sensor data pub/sub (future) |

## License

MIT License - See LICENSE file for details

## Getting Help

See [files/CLAUDE.md](files/CLAUDE.md) for:
- Coding guidelines
- LVGL patterns
- Common tasks
- Testing procedures

## Phase 0 Completion Summary

✅ All Phase 0 tasks completed:
- SDL2, CMake, Homebrew installed and verified
- Project structure created
- LVGL v9.3 integrated
- Custom SDL2 drivers implemented
- CMake build system configured
- Simulator builds and runs successfully
- Demo screen displays "Hello, Vitals Monitor"

**Ready for Phase 1: Basic UI Framework**
