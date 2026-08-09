# Keybindings

All bindings are hardcoded in `OnKeyDown`/`OnChar` in `src/ui/window.cpp` — there is no
configurable keymap yet.

## Navigation

| Key                  | Command             |
|-----------------------|----------------------|
| Left                   | `cursor.left`        |
| Ctrl+Left              | `cursor.file-start`  |
| Right                  | `cursor.right`       |
| Ctrl+Right             | `cursor.file-end`    |
| Up                     | `cursor.up`          |
| Down                   | `cursor.down`        |
| Home                   | `cursor.line-start`  |
| Ctrl+Home              | `cursor.file-start`  |
| End                    | `cursor.line-end`    |
| Ctrl+End               | `cursor.file-end`    |
| Page Up                | `cursor.page-up`     |
| Page Down              | `cursor.page-down`   |
| Mouse wheel            | scroll (3 lines per notch, not routed through the command engine) |

## Editing

| Key         | Command                |
|-------------|--------------------------|
| Enter        | `edit.newline`           |
| Backspace    | `edit.delete-back`       |
| Delete       | `edit.delete-forward`    |
| Ctrl+Y       | `edit.delete-line`       |
| Ctrl+S       | `file.save`              |
| Esc          | clear the status-bar message (not a registered command) |
| any printable character | inserted directly at the cursor |

## Find

`find.search`, `find.next`, and `find.previous` have no dedicated key bindings — they are
reachable only through the **F1 command menu** (`Find` submenu). See
[COMMANDS.md](COMMANDS.md#find) for what each does.

Choosing `Find > Search...` opens the search input box; Enter runs the search, Esc
cancels. While it's open it swallows all keyboard input except those two keys — same
swallow-everything rule as the F1 menu itself. If a search or `find.next`/`find.previous`
fails, a notice overlay appears with the reason; **any key** dismisses it and normal
editing resumes. See [input_box.h](../src/ui/input_box.h) and
[notice_box.h](../src/ui/notice_box.h).

## Command menu

| Key         | Effect                                                     |
|-------------|--------------------------------------------------------------|
| F1          | Toggle the command menu open/closed                          |
| Up / Down   | Move selection within the current menu level                 |
| Right, Enter| Open the highlighted submenu, or execute a highlighted command and close the menu |
| Left        | Go back one submenu level (no-op at the root)                |
| F1 (again)  | Close the menu without executing anything                    |

While the menu is open it swallows all keyboard input (`OnKeyDown`/`OnChar` return early
in `src/ui/window.cpp`) so editing keys and typed characters have no effect until the menu
is closed. See [command_menu.h](../src/ui/command_menu.h).

## Reserved but unimplemented

CLAUDE.md reserves F2–F12 and Ctrl+\* / Alt+\* for the macro engine. No macro engine
exists yet, so none of those keys are currently bound to anything; unhandled keys fall
through to `DefWindowProcW`.
