# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**QEditNA** is a minimalist text editor for Windows 10/11, a technology demonstrator emphasizing keyboard-centric operation, independence from complex frameworks, and data safety. Inspired by the classic QEdit, it uses only C++ with Windows API — no Qt, Electron, or other external frameworks.

**Key principle**: Small, fast, transparent, maximum keyboard control, zero external dependencies.

**Versioning**: MAJOR.MINOR.PATCH (semantic versioning)

## Tech Stack & Environment

- **Language**: C++ (C++17 or later)
- **Compiler**: MinGW (GCC toolchain for Windows)
- **API**: Windows API only — no external libraries
- **Platform**: Windows 10/11 (32-bit and 64-bit compatible if feasible)
- **Build system**: Makefile or CMake (to be determined)

## Build, Run & Development Commands

Commands will be populated once the build system is set up. Expected structure:

```bash
# Build
make release    # Optimized build
make debug      # Debug build with symbols
make clean      # Remove build artifacts

# Run
./build/QEditNA.exe              # Run editor
./build/QEditNA.exe <filename>   # Open file

# Testing (when test harness exists)
make test                        # Run all tests
make test TEST=command_goto      # Run single test
```

## Project Structure (Expected)

Once implemented, the codebase will follow this high-level organization:

```
QEditNA/
├── src/
│   ├── main.cpp              # Entry point, Windows window setup
│   ├── editor/
│   │   ├── editor_core.h/cpp      # Text buffer, cursor management, rendering
│   │   ├── command_engine.h/cpp   # Command dispatch & execution
│   │   └── macro_engine.h/cpp     # Macro recording, playback, storage
│   ├── commands/
│   │   ├── goto.h/cpp
│   │   ├── find.h/cpp
│   │   ├── delete.h/cpp
│   │   ├── pretty.h/cpp
│   │   └── [other commands]
│   ├── io/
│   │   ├── file_io.h/cpp         # File read/write with safety guards
│   │   └── backup.h/cpp          # Backup creation & management
│   ├── ui/
│   │   ├── window.h/cpp          # Main window creation & event loop
│   │   └── status_bar.h/cpp      # Status bar rendering
│   └── config/
│       ├── config.h              # Compile-time & runtime configuration
│       └── constants.h           # Constants (write-protect flag, etc.)
├── tests/
│   ├── test_command_engine.cpp
│   ├── test_macro_engine.cpp
│   ├── test_file_io.cpp
│   └── [other tests]
├── docs/
│   ├── ARCHITECTURE.md           # Detailed design docs
│   ├── COMMANDS.md               # List of all commands & behavior
│   ├── MACROS.md                 # Macro system design
│   ├── KEYBINDINGS.md            # Keyboard shortcuts & mappings
│   └── TEST_SCENARIOS.md         # Test cases & scenarios
├── Makefile (or CMakeLists.txt)
├── .gitignore
└── CLAUDE.md (this file)
```

## Core Architecture

### 1. Editor Core
- **Text Buffer**: Efficient line-based or gap-buffer storage for fast editing
- **Cursor State**: Position, selection, viewport
- **Rendering Pipeline**: Convert buffer state to screen output (handle line wrapping, status bar)

### 2. Command Engine
- Command registry: map command names to handler functions
- Execution context: current selection, file state, macro recording state
- Undo/redo stack (optional, scope TBD)
- Macro recording hooks: intercept commands during recording

### 3. Macro Engine
- **Recording**: Intercept command execution, store to macro buffer
- **Storage**: Macros live in memory during session (persistent storage TBD)
- **Playback**: Re-execute stored commands with same context
- **Keybinding**: F2-F12, Ctrl+*, Alt+* reserved for macros

### 4. File I/O Safety (Critical)
- **Write Protection Flag**: `WRITE_ENABLED` (compile-time constant)
  - In test builds (v0.x): must be `false`
  - Persists to release builds: set to `true`
- **File Operations**:
  - Read: always allowed
  - Write: check `WRITE_ENABLED` → if false, silently no-op or warn
- **Backup Strategy**: Before writing, create `.bak` file (TBD: how many backups to keep)

### 5. UI Layout
- **Main Window**: Spans entire client area with editor content
- **Status Bar** (1 line, top): Filename, file size, cursor position (row:col), mode info
- **No Menu Bar by Default**: Commands accessible via keyboard only (menu for discovery can be added later)

## Development Principles

1. **Spec-first**: Complete technical specification before implementation
2. **Keyboard-centric**: Every feature must be operable without mouse
3. **Small steps**: Each task/PR ≤ 1000 lines of changed code
4. **Data safety**: Paranoid about file operations; read-only by default in tests
5. **Minimal dependencies**: Resist adding external libraries; solve with Windows API
6. **Clear architecture**: Decoupled command/macro/editor cores for testability

## Key Decisions & Constraints

### Write Protection in Test Builds
- **Why**: Prevent accidental data loss during development
- **How**: Hardcoded `WRITE_ENABLED` constant in `config/config.h`
- **Impact**: File read works, write silently fails (or logs warning, TBD)

### Macro Persistence
- **Implemented**: in-memory during the session (`MacroEngine`, `src/editor/macro_engine.h/cpp`),
  with explicit save/load to `.mac` files via the F1 → Makra menu (see `docs/MACROS.md`).
  No auto-load/auto-save and no registry storage — the user opts in each time.

### Undo/Redo
- **Scope TBD**: Is undo required for v0.x? Defer if not essential

## Documentation Policy (Strict)

**Every code change must be accompanied by a corresponding documentation update**, unless the request explicitly states that documentation should NOT be changed. This applies to all changes — new features, bug fixes, refactors, API/behavior changes, command additions, keybinding changes, etc.

- Before finishing any coding task, check whether ARCHITECTURE.md, COMMANDS.md, MACROS.md, KEYBINDINGS.md, TEST_SCENARIOS.md, or this CLAUDE.md need updating to reflect the change.
- If a change has no user-visible or architectural impact (e.g. pure internal cleanup with no behavior change), documentation may be left unchanged, but this should be a deliberate judgment call, not an omission.
- When in doubt, update the documentation.

## Documentation

When implementing features, maintain:
- **ARCHITECTURE.md**: Detailed design, including call flows for complex features
- **COMMANDS.md**: Command list, parameters, keyboard shortcuts
- **MACROS.md**: Macro recording/playback, keybinding rules
- **TEST_SCENARIOS.md**: Test cases for each feature (acceptance criteria)

Each feature should have both design docs (before coding) and test scenarios (before release).

## Testing

- Unit tests for commands, file I/O, macro engine
- Integration tests for macro + command chains
- Manual keyboard testing (ensure all keyboard paths work)
- Data safety regression: verify write-protect flag prevents disk writes in test builds

## Common Gotchas & Reminders

1. **Windows API Calls**: Always check return codes; Windows API is error-prone
2. **Text Encoding**: Handle Unicode properly (UTF-8 or UTF-16 per Windows convention)
3. **File Locks**: Be aware of file locks when testing file I/O
4. **Macro Recording**: Do not record macro start/stop commands (avoid infinite loops)
5. **Performance**: Editor must remain responsive even with large files — test with 10MB+ file
6. **Backup Edge Case**: If backup creation fails, should primary write proceed? Define behavior upfront.

---

**Last Updated**: 2026-08-09 (project kickoff)
