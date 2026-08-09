# Macros

**Status: not implemented.** There is no `macro_engine.h/cpp`, no recording state, and no
keys bound to macro start/stop/playback anywhere in the codebase.

This document exists as a placeholder per the structure in [CLAUDE.md](../CLAUDE.md) and
records the constraints already decided, so a future implementation stays consistent with
choices made in the rest of the app:

## Known constraints (from CLAUDE.md)

- Macro keybindings are reserved: F2–F12, Ctrl+\*, Alt+\* (see
  [KEYBINDINGS.md](KEYBINDINGS.md)).
- Macro storage scope is **in-memory only for v0.x** — no persistence across sessions is
  planned yet (`.qeditrc` or registry storage is a later decision, not started).
- Recording must intercept command execution rather than raw key events, so that it
  captures resolved commands, not physical keys. The natural hook point is
  `CommandEngine::Execute` (see `src/editor/command_engine.h`) or the `Run()` wrapper in
  `src/ui/window.cpp`, both of which already funnel every action through one place.
- Macro start/stop commands themselves must not be recorded into the macro being
  recorded (avoid infinite loops on playback).

## Open questions (unresolved)

- Where does recording toggle live — a flag on `CommandEngine`, or a separate
  `MacroEngine` that wraps it?
- Named macros vs. numbered slots (F2–F12 implies up to 11 slots)?
- Undo/redo does not exist yet either (see [ARCHITECTURE.md](ARCHITECTURE.md)) — macro
  playback semantics under undo are undecided.
