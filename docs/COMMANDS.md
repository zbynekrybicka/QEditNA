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

Printable characters are inserted directly via `EditorCore::InsertChar` from `WM_CHAR`,
bypassing the command engine's named dispatch (there is no `edit.insert-char` command;
insertion is high-frequency enough that it's handled as a direct call — see
`OnChar` in `window.cpp`).

## File

| Command       | Effect                                      | Key     |
|---------------|----------------------------------------------|---------|
| `file.save`    | Write the buffer to disk (subject to the write-protect flag — see [ARCHITECTURE.md](ARCHITECTURE.md)) | Ctrl+S |

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

## Command menu

Besides the hardcoded key bindings above, every command is also reachable through the
**F1 command menu** (`src/ui/command_menu.h/cpp`): a static tree of submenus (Cursor,
Edit, File — mirroring the groups on this page) navigated with Up/Down/Left/Right and
confirmed with Enter. See [KEYBINDINGS.md](KEYBINDINGS.md#command-menu) for the key
mapping and [ARCHITECTURE.md](ARCHITECTURE.md) for how it plugs into `WindowProc`.

The menu is a fixed, hand-authored tree — it does not walk `CommandEngine::Names()`, so
adding a command to `RegisterBuiltinCommands` does not automatically add it to the menu;
`BuildDefaultMenu()` in `command_menu.cpp` needs a matching entry.

## Not yet implemented

`delete` (block/range), `pretty`, and other commands named in CLAUDE.md's expected
`src/commands/` directory do not exist yet beyond `find.*` above. There is also no
general way to invoke a command by typed name (a `:command` line) — the F1 menu covers
discovery/navigation, and `find.search` is the one command wired to a purpose-built text
input.
