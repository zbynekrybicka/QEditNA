# Macros

**Status: implemented (v0.x scope below).** `src/editor/macro_engine.h/cpp` holds the
`MacroEngine` class; `src/ui/window.cpp` wires it into the F1 menu and key routing.

## Concept

A macro is a flat, unconditional sequence of `(command, argument)` pairs — exactly what
`CommandEngine::Execute` already takes. Recording taps the single choke point every
key/menu action already runs through: `Run()` / `RunFind()` in `src/ui/window.cpp`. There
is no branching, no recorded macro start/stop, and commands run identically whether they
came from a key, the F1 menu, or macro playback (see
[ARCHITECTURE.md](ARCHITECTURE.md#macro-engine)).

## Hotkeys

Macros are bound to one hotkey each, drawn from the reserved ranges in
[KEYBINDINGS.md](KEYBINDINGS.md#macros):

- **F2–F12** (no modifier)
- **Ctrl+&lt;letter or digit&gt;**, excluding Ctrl+S and Ctrl+Y (already bound to
  `file.save` / `edit.delete-line`)
- **Alt+&lt;letter or digit&gt;**

`ResolveMacroHotkey()` in `macro_engine.h/cpp` is the single place that validates a raw
keystroke against this shape and produces the canonical hotkey id used as the storage key
(e.g. `"F2"`, `"Ctrl+G"`, `"Alt+3"`).

## Menu commands (F1 → Makra)

| Label                | Effect |
|-----------------------|--------|
| Nové makro             | Prompts for a hotkey (Esc cancels). If that hotkey already has a macro, asks Enter=overwrite / Esc=cancel. Once accepted, recording starts immediately — every subsequent command (key or menu) is appended to the macro until "Ukončit nahrávání" is chosen. |
| Ukončit nahrávání      | Stops recording and commits the macro. Disabled (dimmed, no-op) whenever nothing is being recorded. The stop command itself is never recorded. |
| Smazat makro           | Prompts for a hotkey (Esc cancels) and removes its macro, if any. |
| Uložit makra           | Prompts for a filename, appends `.mac` if the typed name doesn't already end with it (case-insensitively), and writes every in-memory macro to that file. Subject to the same write-protect gate as document saves (`config::kWriteEnabled`) — refused in test builds. |
| Načíst makra           | Prompts for a `.mac` filename. Reads it, validates the file header and that it contains at least one macro, and merges the loaded macros into memory (a loaded hotkey overwrites any in-memory macro on the same hotkey). Reading is always allowed, regardless of the write-protect flag. |

Playing a macro (pressing its bound hotkey) is itself recordable: if triggered while
another macro is being recorded, it's captured as a single `macro.play` step referencing
the played hotkey, not inlined — so nested/composed macros are supported.

## Storage

- **In memory**: `std::map<std::wstring hotkeyId, Macro>` inside `MacroEngine`, keyed by
  the canonical hotkey id. Lost on exit unless saved.
- **On disk** (`.mac` files, UTF-8 text):
  ```
  QEDITNA-MACRO v1
  [F2]
  cursor.line-start	
  find.search	TODO
  [Ctrl+G]
  macro.play	F2
  ```
  A header line identifies the format; `[hotkeyId]` starts a section; each following line
  is `command<TAB>argument` until the next section or end of file. `macro.play` is the one
  command name that never reaches `CommandEngine` — `RunMacro()` in `window.cpp`
  special-cases it as a nested playback call.

## Constraints carried over from the original design notes

- Macro start/stop commands are never recorded (avoids infinite loops on playback) — this
  now also covers the other four macro-management commands (`macro.new`, `macro.delete`,
  `macro.save`, `macro.load`), none of which are ever recorded since they're handled
  entirely in `window.cpp` and never routed through `Run()`/`RunFind()`.
- Recording captures resolved commands, not raw keystrokes, via the existing
  `CommandEngine::Execute` choke point.
