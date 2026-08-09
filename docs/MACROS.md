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

- **F2–F12** (no modifier), excluding **F5** and **F6** (hardcoded to `macro.new` /
  `macro.stop-recording` — see [KEYBINDINGS.md](KEYBINDINGS.md))
- **Ctrl+&lt;letter or digit&gt;**, excluding Ctrl+S and Ctrl+Y (already bound to
  `file.save` / `edit.delete-line`)
- **Alt+&lt;letter or digit&gt;**

`ResolveMacroHotkey()` in `macro_engine.h/cpp` is the single place that validates a raw
keystroke against this shape and produces the canonical hotkey id used as the storage key
(e.g. `"F2"`, `"Ctrl+G"`, `"Alt+3"`); it defers the exclusion list itself to
`IsReservedHotkeyId()` (F1, F5, F6, Ctrl+S, Ctrl+Y — F1 is excluded implicitly by the
F2–F12 range, the rest explicitly), which is also what `LoadFromFile` checks so a
hand-edited `.mac` file can't sneak a macro onto one of these ids by naming the section
directly (e.g. `[F5]`), and what `macro.new`'s `CommandEngine` handler checks so a literal
(non-`<SHORTKEY>`) argument can't either.

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

## Placeholder arguments (`<TEXT>`, `<SHORTKEY>`)

A hand-written (or edited) `.mac` step can use `<TEXT>` or `<SHORTKEY>` as its argument
instead of a literal value. Playback recognizes these two exact strings and pauses to ask
the user for the real argument, used for that one step only — the macro itself still
stores the placeholder, so it asks again every time it plays:

- **`<TEXT>`** opens a text input box ("Doplňte text pro *command*: "). Enter confirms and
  resumes playback with the typed text as the argument; Esc aborts the rest of the macro
  (including any outer macro that called it via `macro.play`).
- **`<SHORTKEY>`** shows a prompt and waits for one hotkey-shaped keystroke (F2–F12,
  Ctrl+letter/digit, Alt+letter/digit — the same shape `ResolveMacroHotkey` accepts
  elsewhere), then resumes playback with the resolved hotkey id (e.g. `"F5"`, `"Ctrl+G"`)
  as the argument. Esc aborts the rest of the macro.

This is what makes the four macro-management commands usable *from inside* a macro despite
needing an argument they can't have at record time — e.g. a step `macro.new <SHORTKEY>`
prompts for the hotkey to start recording onto, and `macro.save <TEXT>` prompts for a
filename. See [COMMANDS.md](COMMANDS.md#macros) for what those four commands do once they
have the argument.

Implementation: `ContinuePlayback()` in `src/ui/window.cpp` drives playback as an explicit
stack of `PlaybackFrame`s (macro + next-step index) rather than the recursive loop it used
before placeholders existed — a stack is necessary because a placeholder can suspend
playback for an arbitrary number of window messages (waiting on `WM_CHAR`/`WM_KEYDOWN`)
and must resume later at exactly the right step, including inside a nested `macro.play`.
`PlaybackWait` (`kNone`/`kText`/`kShortkey`) tracks what, if anything, playback is currently
waiting on; `OnChar`/`OnKeyDown` check it before any other key handling.

## Loading macros at startup (`-m`)

`QEditNA.exe -m <filename>` loads a `.mac` file at startup, equivalent to running the
"Načíst makra" menu command with that filename right after the window is created (see
`RunEditor()` in `src/ui/window.cpp`). The `.mac` extension is appended automatically if
missing, same as the interactive flow. Command-line parsing (`main.cpp`) recognizes `-m`
anywhere in the argument list; the remaining argument (if any) is still treated as the file
to open in the editor. On success the status bar shows the same "Makra načtena ze souboru
…" message as the interactive load; on failure a message box reports the error and the
editor still starts normally.

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

  `SaveToFile` always writes the tab form above, but `LoadFromFile`/`ParseMacros` (the
  parser also used at startup via `-m`) accept a run of tabs and/or spaces as the
  separator, since a hand-edited `.mac` file can't visually tell them apart — so
  `find.search <TEXT>` (space) and `find.search␉<TEXT>` (tab) parse the same. Only the
  *first* run of whitespace is treated as the separator; anything after it, including
  further spaces, belongs to the argument.

## Constraints carried over from the original design notes

- Macro start/stop commands are never recorded (avoids infinite loops on playback): all
  five macro-management commands are registered in `CommandEngine` now, so they're
  recordable in principle, but in practice are only ever *played back*, not recorded,
  since the F1 menu path that drives them interactively still bypasses
  `CommandEngine`/`RecordIfNeeded` entirely — see [COMMANDS.md](COMMANDS.md#macros).
- Recording captures resolved commands, not raw keystrokes, via the existing
  `CommandEngine::Execute` choke point.
