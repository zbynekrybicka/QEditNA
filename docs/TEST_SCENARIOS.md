# Test Scenarios

**Status: no automated test harness exists yet** (`tests/` directory and `make test` are
not implemented — the Makefile has no `test` target). These scenarios describe manual
verification of the currently implemented v0.1 behavior and should become automated
tests once a harness (likely a lightweight header-only framework, kept dependency-free)
is added.

## Data safety — write-protect flag (critical, per CLAUDE.md)

1. Build with plain `make release` / `build.bat` (no `WRITE=1`/`WRITE` arg).
2. Open an existing file, edit it, press Ctrl+S.
3. **Expect**: `io::SaveFile` returns `refusedByWriteProtect = true`; no bytes are
   written to disk (verify file mtime/contents unchanged); status bar shows the refusal
   message from `SaveToFile`.
4. Rebuild with `make release WRITE=1` / `build.bat WRITE`.
5. Repeat step 2. **Expect**: the file is overwritten, and if `config::kBackupOnWrite`
   is `true` (default), a `.bak` copy of the pre-edit content exists alongside it.

## File I/O

- Opening a non-existent path: **expect** `LoadFile` returns `ok == true` with a single
  empty line (new-file behavior), not an error.
- Opening a UTF-8 file with BOM vs. without: **expect** `hadBom` reflects the source and
  a write-enabled save round-trips the same BOM presence.
- Opening a CRLF file vs. an LF file: **expect** `lineEnding` is detected correctly and
  preserved on save.

## Cursor movement

- From the middle of a line, Left/Right move one character and clamp at line
  boundaries (no wraparound to adjacent lines currently — verify against
  `EditorCore::MoveLeft`/`MoveRight` behavior as implemented, not assumed).
- Up/Down preserve column where possible and clamp to shorter lines.
- Home/End go to line start/end; Ctrl+Home/Ctrl+Left and Ctrl+End/Ctrl+Right go to file
  start/end.
- Page Up/Page Down move by the current viewport's `visibleLines` (resize the window and
  confirm the page size adapts).

## Editing

- Enter splits the current line at the cursor (`InsertNewline`).
- Backspace at column 0 joins with the previous line; Backspace mid-line deletes the
  preceding character.
- Delete at end of line joins with the next line; mid-line deletes the following
  character.
- Ctrl+Y deletes the entire current line.

## Performance

- Open a 10MB+ file and confirm the window remains responsive to scrolling and typing
  (per CLAUDE.md's performance reminder). Not yet measured/benchmarked — no perf test
  exists.

## Not covered (features not yet implemented)

Macro recording/playback, undo/redo, `find`/`goto`/`pretty` commands — see
[ARCHITECTURE.md](ARCHITECTURE.md) and [MACROS.md](MACROS.md) for status.
