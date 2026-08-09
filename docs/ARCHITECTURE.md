# Architecture

Reflects the actual state of the codebase at v0.1.0. See [CLAUDE.md](../CLAUDE.md) for
the target/expected structure — this document describes what is currently implemented.

## Module map

```
src/
├── main.cpp                    wWinMain entry point, parses argv[1] as file to open
├── config/
│   ├── constants.h             version, app name, window class, layout constants
│   └── config.h                kWriteEnabled, kBackupOnWrite, font settings
├── editor/
│   ├── editor_core.h/cpp       EditorCore: line buffer, cursor, viewport
│   └── command_engine.h/cpp    CommandEngine: name -> handler registry + dispatch
├── io/
│   └── file_io.h/cpp           LoadFile / SaveFile, UTF-8, BOM & line-ending detection
└── ui/
    ├── window.h/cpp            window class, message loop, key/char handling, painting
    ├── status_bar.h/cpp        status line text + drawing
    ├── command_menu.h/cpp      F1 command menu: static tree + navigation + drawing
    ├── input_box.h/cpp         generic single-line text input overlay (used by search)
    └── notice_box.h/cpp        dismiss-on-any-key message overlay (used by search failures)
```

Not yet implemented: `commands/` (individual command files), `macro_engine`, undo/redo,
`backup.h/cpp` as a separate module (backup logic currently lives inline in `file_io.cpp`).

## Data flow

1. `wWinMain` (main.cpp) parses the command line and calls `ui::RunEditor`.
2. `RunEditor` (window.cpp) builds a single `WindowState` containing the `EditorCore`,
   the `CommandEngine`, and rendering/layout state (font metrics, viewport size, status
   message). This struct is stored via `GWLP_USERDATA` and is the only state in the app —
   no globals.
3. `RegisterBuiltinCommands` populates the `CommandEngine` with the v0.1 command set
   (see [COMMANDS.md](COMMANDS.md)).
4. The Win32 message loop dispatches to `WindowProc`. Keyboard input funnels through
   `OnKeyDown` (non-printable keys: arrows, Home/End, PageUp/Down, Delete, Ctrl+letter
   shortcuts) and `OnChar` (printable characters, Enter, Backspace).
5. Both handlers resolve a *command name* and call `Run()`, which builds a
   `CommandContext` (pointer to the `EditorCore`, current viewport size, output message),
   dispatches it through `CommandEngine::Execute`, and refreshes the view. This is the one
   choke point every editing/navigation action passes through — the future macro engine
   will hook here to record and replay commands.
6. `EditorCore` owns a `std::vector<std::wstring>` line buffer plus a `Cursor{line, col}`.
   All mutating methods clamp the cursor to valid positions internally.
7. Rendering (`OnPaint`) draws into an off-screen bitmap (flicker-free) then blits it:
   status bar first, then visible lines with tabs expanded to `kTabWidth`-column stops.

## Text buffer

`EditorCore` uses a plain line-based buffer (`std::vector<std::wstring>`), not a gap
buffer — the header notes a gap buffer could replace it later behind the same interface
if large-file performance requires it. Each line is a `std::wstring` (UTF-16, matching
Win32's native string type); files are read/written as UTF-8 and converted at the I/O
boundary (`file_io.cpp`).

`EditorCore` also tracks the line ending style (`CRLF`/`LF`) and whether the source file
had a BOM, so a round-tripped save preserves the original format.

## File I/O and the write-protect guard

- `io::LoadFile` always reads. A non-existent path returns `ok == true` with a single
  empty line, so opening a new filename behaves like "new file".
- `io::SaveFile` is gated by `config::kWriteEnabled` (`src/config/config.h`), which is
  `false` unless the binary is compiled with `-DQEDITNA_ENABLE_WRITE` (the `WRITE=1`
  make flag, or `build.bat WRITE`). When disabled, `SaveFile` performs **no disk access**
  and reports `refusedByWriteProtect = true` — this is the "test build" safety rule from
  CLAUDE.md.
- When writing is enabled and `config::kBackupOnWrite` is `true`, a `.bak` copy of the
  existing file is written before the primary file is overwritten.

## Command engine

`CommandEngine` is a `std::map<std::wstring, Command>` keyed by dotted command names
(`cursor.left`, `edit.newline`, `file.save`, ...). A `CommandHandler` is
`std::function<bool(CommandContext&)>`; handlers read/write `CommandContext::argument`
and `CommandContext::message` and return `false` on failure (e.g. unknown command, save
refused). `RegisterBuiltinCommands` wires up every command currently exposed by the UI —
see [COMMANDS.md](COMMANDS.md) for the full list.

There is no separate `commands/*.cpp` per command yet; all built-in handlers are small
lambdas defined inline in `command_engine.cpp`.

## Command menu (F1)

`ui::CommandMenu` (`src/ui/command_menu.h/cpp`) is a static, hand-authored tree of
`MenuItem`s: each node is either a submenu (non-empty `children`) or a leaf bound to a
command name (`command`) already registered in `CommandEngine`. It does not enumerate
`CommandEngine::Names()` — the tree is built once by `BuildDefaultMenu()` and has to be
kept in sync by hand when commands are added.

Navigation state is a `std::vector<Level>` stack, one entry per depth, each remembering
which item is currently selected at that level (drilldown style — only the current
level's items are shown, not a full cascading tree). `WindowProc` owns the `CommandMenu`
instance inside `WindowState` alongside `EditorCore` and `CommandEngine`.

- **F1** calls `CommandMenu::Toggle()`: opens the menu at the root if closed, closes it
  (discarding navigation state) if open. This is the *only* way to close the menu without
  choosing a command.
- While `CommandMenu::IsActive()` is true, `OnKeyDown` routes every key to
  `CommandMenu::HandleKey` instead of the normal cursor/edit dispatch, and `OnChar`
  returns immediately — so typing and editing are fully suspended while the menu is open.
- `HandleKey` returns `kNone` (still open), `kClosed` (only reachable via F1, handled
  separately), or `kExecute` with the chosen command name; `WindowProc`'s `Run()` helper
  then dispatches that command through `CommandEngine::Execute` exactly like a normal key
  binding, so a menu selection is indistinguishable from a hotkey to the rest of the
  system (and will be equally visible to the future macro engine).
- Up/Down move the selection within the current level (wrapping). Right or Enter on a
  submenu item pushes a new level; Right or Enter on a leaf item executes its command and
  closes the menu. Left pops back to the parent level (no-op at the root).
- `CommandMenu::Draw` paints a centered floating box listing only the current level, with
  the selected row highlighted; called from `OnPaint` after the status bar, into the same
  off-screen bitmap.

## Find and replace

Search is case-insensitive substring matching over the plain line buffer — no regular
expressions, no "replace" yet (only find is implemented; see CLAUDE.md's `find.h/cpp` for
the eventual scope).

- `EditorCore` owns the search state: `lastSearchTerm_` plus three methods —
  `FindFirst(term, message)`, `FindNext(message)`, `FindPrevious(message)`. All three
  lower-case both the haystack line and the needle with `towlower` per character (no
  locale-aware collation) and move the cursor to the match on success. `FindFirst`
  searches the whole buffer from the top and overwrites `lastSearchTerm_` unconditionally
  (even on failure, so a failed search still becomes the term `find.next` reuses).
  `FindNext`/`FindPrevious` reuse `lastSearchTerm_`, search strictly after/before the
  current cursor position, and do **not** wrap around the start/end of the buffer.
- `command_engine.cpp` wires these to `find.search` (takes the term via
  `CommandContext::argument`), `find.next`, and `find.previous` — see
  [COMMANDS.md](COMMANDS.md).
- Two new small UI overlay modules support this at the input layer, both reusable beyond
  search:
  - `ui::InputBox` (`src/ui/input_box.h/cpp`) — a single-line prompt. `Open(label)`
    activates it; `HandleChar` appends printable characters, handles Backspace, and
    returns `kConfirmed` on Enter (`Text()` then holds the typed value); `Cancel()` is
    called from `WM_KEYDOWN` on Esc, matching how Esc is handled everywhere else in this
    codebase (via `OnKeyDown`, not `OnChar`).
  - `ui::NoticeBox` (`src/ui/notice_box.h/cpp`) — a centered message box. `Show(text)`
    activates it; **any** key dismisses it (`Dismiss()`), after which normal
    editing/cursor keys resume as usual.
- `WindowState` (`window.cpp`) holds one `InputBox searchInput` and one `NoticeBox
  notice`, plus a `noticeJustDismissed` flag. That flag exists because `WM_KEYDOWN` and
  `WM_CHAR` are separate messages: `TranslateMessage` queues the `WM_CHAR` for a
  character-producing key *before* `WM_KEYDOWN` is dispatched (see the message loop in
  `RunEditor`), so dismissing the notice inside `OnKeyDown` isn't enough on its own — the
  very next `OnChar` call would otherwise insert that same character into the buffer.
  Setting `noticeJustDismissed = true` in `OnKeyDown` and consuming it (without inserting)
  at the top of `OnChar` closes that gap.
- Key routing in `OnKeyDown`/`OnChar` checks the notice first, then the search input box,
  then the F1 menu, then normal editing — each active overlay swallows all input until it
  closes, the same "swallow everything while active" pattern the F1 menu already
  established.
- `find.search`, `find.next`, and `find.previous` have **no dedicated key bindings** —
  they're reachable only through the F1 menu's `Find` submenu. F1's menu executor
  special-cases all three commands (see `OnKeyDown`'s menu-handling block) instead of
  routing them through the normal `Run()` helper every other menu leaf uses:
  - `find.search` → `searchInput.Open(...)`, which opens the input box; the search itself
    runs later, from `OnChar`, once Enter confirms the typed text.
  - `find.next` / `find.previous` → dispatched immediately via `RunFind()` (see below),
    since they need no argument.
- `window.cpp`'s `RunFind()` helper is a variant of `Run()` that also takes a
  `CommandContext::argument` and shows a `NoticeBox` when the command fails. It's used for
  all three find commands: from the input box's Enter-confirm path (`find.search` with
  the typed text) and from the F1 menu (`find.next` / `find.previous` with an empty
  argument).

## UI layout

- Single top-level window, no menu bar.
- A 1-line status bar is drawn at the **top** of the client area (matches CLAUDE.md's
  "Status Bar (1 line, top)"), built by `ui::BuildStatusText` and painted by
  `ui::DrawStatusBar`.
- The editing viewport occupies the remaining client area below the status bar; visible
  line/column counts are recalculated on `WM_SIZE` from font metrics.
- The caret is a native Win32 caret (`CreateCaret`/`SetCaretPos`), hidden when the cursor
  scrolls outside the visible viewport.

## Not yet implemented

- Macro engine (recording/playback), macro keybindings (F2–F12, Ctrl+*, Alt+*)
- Auto-generating the F1 command menu tree from `CommandEngine::Names()` (currently hand-authored)
- Undo/redo
- Replace (only find is implemented)
- Individual `commands/*.cpp` files (goto, delete, pretty, etc.) beyond what's in
  `command_engine.cpp`
- `io/backup.h/cpp` as a standalone module
- Automated test harness (`tests/`, `make test`)
