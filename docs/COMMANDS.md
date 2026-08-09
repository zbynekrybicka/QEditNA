# Commands

All commands are registered in `RegisterBuiltinCommands` (`src/editor/command_engine.cpp`)
and executed through `CommandEngine::Execute(name, CommandContext&)`. This is the current,
implemented v0.1 command set — not a target list.

Every command receives a `CommandContext` with a pointer to the `EditorCore`, the current
viewport size (`visibleLines`/`visibleCols`), an optional `argument`, and an output
`message` shown in the status bar.

## Cursor movement

| Command              | Effect                          | Key                  |
|-----------------------|----------------------------------|-----------------------|
| `cursor.left`          | Move cursor one character left  | Left                  |
| `cursor.right`         | Move cursor one character right | Right                 |
| `cursor.up`            | Move cursor one line up         | Up                    |
| `cursor.down`          | Move cursor one line down       | Down                  |
| `cursor.line-start`    | Move to start of line           | Home                  |
| `cursor.line-end`      | Move to end of line             | End                   |
| `cursor.file-start`    | Move to start of file           | Ctrl+Left, Ctrl+Home  |
| `cursor.file-end`      | Move to end of file             | Ctrl+Right, Ctrl+End  |
| `cursor.page-up`       | Move one page up                | Page Up               |
| `cursor.page-down`     | Move one page down              | Page Down             |

`GotoLine(oneBasedLine)` exists on `EditorCore` but is not currently wired to a command
or key (no `goto` command yet, despite `docs/` in CLAUDE.md's expected layout naming one).

## Editing

| Command              | Effect                              | Key            |
|------------------------|--------------------------------------|-----------------|
| `edit.newline`          | Insert a line break                 | Enter           |
| `edit.delete-back`      | Delete character before cursor      | Backspace       |
| `edit.delete-forward`   | Delete character under cursor       | Delete          |
| `edit.delete-line`      | Delete the current line             | Ctrl+Y          |
| `edit.insert-char`      | Insert one character (`context.argument[0]`) at the cursor | any printable character |

Printable characters go through `edit.insert-char` from `WM_CHAR` (`OnChar` in
`window.cpp`) rather than being handled as a hardcoded default like Enter/Backspace,
specifically so macro recording — which taps `CommandEngine::Execute` — sees typed text
and can play it back. See [MACROS.md](MACROS.md).

## File

| Command       | Effect                                      | Key     |
|---------------|----------------------------------------------|---------|
| `file.save`    | Write the buffer to disk (subject to the write-protect flag — see [ARCHITECTURE.md](ARCHITECTURE.md)) | Ctrl+S |

## Block selection

Reachable only through the F1 command menu's **Blok** submenu — no dedicated key
bindings (see [KEYBINDINGS.md](KEYBINDINGS.md#block-selection)). Implemented on
`EditorCore` (`BlockMarkStart`/`BlockMarkEnd`/`BlockCancel`/`BlockCopy`/`BlockMove`/
`BlockDelete`, `src/editor/editor_core.h/cpp`) around a `Selection` (`BlockMode`
`Line`/`Column`, an `anchor` and an `end` position).

A **line block** marks whole lines between the start and end positions. A
**column block** marks the rectangle between them — same rows, but only the
columns between the two cursor positions on each row.

| Command                      | Effect |
|-------------------------------|--------|
| `block.mark-line-start`        | Cancels any existing selection and starts marking a line block anchored at the cursor. Moving the cursor afterwards grows/shrinks the selection to whole lines between anchor and cursor. |
| `block.mark-line-end`          | Locks the line block currently being marked — further cursor movement no longer resizes it. Fails if nothing is being marked. |
| `block.mark-column-start`      | Same as `block.mark-line-start` but for a column (rectangular) block. |
| `block.mark-column-end`        | Locks the column block currently being marked. Fails if nothing is being marked. |
| `block.copy`                   | Inserts a copy of the marked block's content at the cursor. The selection is left in place, so the same block can be pasted again. Fails if nothing is selected. |
| `block.move`                   | Moves the marked block's content to the cursor (copy, then erase the original). Cancels the selection afterwards. Fails if nothing is selected. |
| `block.delete`                 | Erases the marked block's content in place. Cancels the selection afterwards. Fails if nothing is selected. |
| `block.cancel`                 | Clears the current selection without touching the buffer. Fails if nothing is selected. |

The selection is **not** cleared by ordinary typing or editing once locked — it
persists until `block.copy`'s implicit keep, `block.move`/`block.delete`'s implicit
cancel, or an explicit `block.cancel`. There is no system clipboard integration;
block content only ever moves within the buffer via the cursor position.

## Find

Case-insensitive substring search (no regular expressions). Reachable only through the
F1 command menu's `Find` submenu — no dedicated key bindings (see
[KEYBINDINGS.md](KEYBINDINGS.md#find)). See
[ARCHITECTURE.md](ARCHITECTURE.md#find-and-replace) for how the input/notice overlays and
`EditorCore`'s search methods fit together.

| Command          | Effect                                                                 |
|-------------------|--------------------------------------------------------------------------|
| `find.search`      | Search from the start of the buffer for `context.argument`; remembers it as the last search term. The menu entry opens the search input box first and supplies the typed text as the argument. |
| `find.next`        | Find the next occurrence of the last search term, forward from the cursor, no wrap |
| `find.previous`     | Find the previous occurrence of the last search term, backward from the cursor, no wrap |

All three set `context.message`; on failure (`false` return) the message is shown in a
`NoticeBox` overlay in addition to the status bar, dismissed by any key. `find.next` /
`find.previous` fail immediately with "no search term" if no search has been run yet in
this session.

`file.save` returns `false` and sets a status message when the write is refused by the
write-protect flag (test builds) or fails for another reason (see `io::SaveResult`).

## Macros

Reachable through the F1 menu's **Makra** submenu; `macro.new` and `macro.stop-recording`
are additionally hardcoded to F5/F6 (see [KEYBINDINGS.md](KEYBINDINGS.md#editing)). See
[MACROS.md](MACROS.md) for recording/playback semantics and
[KEYBINDINGS.md](KEYBINDINGS.md#macros) for the hotkey shapes.

| Command                 | Argument | Effect |
|---------------------------|----------|--------|
| `macro.new`                | hotkey id | Starts recording onto that hotkey (overwrites any existing macro there — no confirmation prompt at this level). Fails if the id is reserved (F1, F5, F6, Ctrl+S, Ctrl+Y — see `IsReservedHotkeyId`). |
| `macro.stop-recording`     | — | Stops the current recording. Fails with a status message if nothing is recording. |
| `macro.delete`             | hotkey id | Deletes the macro on that hotkey, if any. |
| `macro.save`                | file path | Writes all in-memory macros to that path exactly as given (no automatic `.mac` extension at this level). Subject to the write-protect flag. |
| `macro.load`                | file path | Loads macros from that path (always allowed — read, not write). |

All five **are** registered in `CommandEngine` — see `CommandContext::macros` in
`command_engine.h` and their handlers in `command_engine.cpp`. This is what lets a macro
step call them directly, typically (for the four that take an argument) via a
`<TEXT>`/`<SHORTKEY>` placeholder that pauses playback to ask the user for the value on
that one invocation — see [MACROS.md](MACROS.md#placeholder-arguments). `macro.stop-
recording` takes no argument, so a macro step just names it plain, e.g. a step
`macro.stop-recording	` bound to its own hotkey ends whatever recording is in progress.

The F1 menu still reaches all five through its own multi-step UI flow (hotkey capture,
filename prompt, and for stop-recording a direct `MacroEngine::StopRecording()` call) in
`OnKeyDown`'s F1-menu handling in `src/ui/window.cpp`, bypassing `CommandEngine` entirely
for the same reason as before — the four that need an argument have to *collect* it
interactively before they have one, which `CommandEngine::Execute` doesn't support, and
`macro.stop-recording`'s menu entry additionally needs the disabled/dimmed state
(`CommandMenu::SetEnabled`) that only the menu tracks. That interactive path also keeps
the overwrite confirmation and automatic `.mac` extension that the raw commands above
don't have.

## Command menu

Besides the hardcoded key bindings above, every command is also reachable through the
**F1 command menu** (`src/ui/command_menu.h/cpp`): a static tree of submenus (Cursor,
Edit, Blok, File, Find, Makra — mirroring the groups on this page) navigated with
Up/Down/Left/Right and confirmed with Enter. See [KEYBINDINGS.md](KEYBINDINGS.md#command-menu)
for the key mapping and [ARCHITECTURE.md](ARCHITECTURE.md) for how it plugs into
`WindowProc`.

The menu is a fixed, hand-authored tree — it does not walk `CommandEngine::Names()`, so
adding a command to `RegisterBuiltinCommands` does not automatically add it to the menu;
`BuildDefaultMenu()` in `command_menu.cpp` needs a matching entry. Leaf items can also be
individually disabled (`MenuItem::enabled`, toggled via `CommandMenu::SetEnabled`) — used
for `macro.stop-recording`, which is dimmed and ignores Enter/Right while nothing is
recording.

## Not yet implemented

`delete` (block/range), `pretty`, and other commands named in CLAUDE.md's expected
`src/commands/` directory do not exist yet beyond `find.*` above. There is also no
general way to invoke a command by typed name (a `:command` line) — the F1 menu covers
discovery/navigation, and `find.search` is the one command wired to a purpose-built text
input.
