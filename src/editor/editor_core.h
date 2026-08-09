// editor_core.h — text buffer, cursor and viewport state
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "../io/file_io.h"

namespace qed {

struct Cursor {
    size_t line = 0;   // 0-based index into the buffer
    size_t col  = 0;   // 0-based character offset within the line
};

enum class BlockMode { None, Line, Column };

// A marked block. `anchor` is where marking started; `end` tracks the cursor
// while marking is in progress and freezes once the block is locked
// (BlockMarkEnd). Line blocks cover whole lines between anchor.line and
// end.line; column blocks cover the rectangle between the two positions.
struct Selection {
    BlockMode mode   = BlockMode::None;
    Cursor    anchor;
    Cursor    end;
    bool      locked = false;
};

// Line-based buffer. Simple and predictable; a gap buffer can replace it later
// behind this same interface.
class EditorCore {
public:
    EditorCore();

    bool LoadFromFile(const std::wstring& path, std::wstring* errorOut);
    bool SaveToFile(std::wstring* messageOut);

    const std::vector<std::wstring>& Lines() const { return lines_; }
    const std::wstring& Line(size_t index) const;
    size_t LineCount() const { return lines_.size(); }

    const Cursor& GetCursor() const { return cursor_; }
    const std::wstring& FilePath() const { return path_; }
    bool IsModified() const { return modified_; }
    unsigned long long ByteSize() const { return byteSize_; }

    // --- cursor movement (all clamp to valid positions) ---
    void MoveLeft();
    void MoveRight();
    void MoveUp();
    void MoveDown();
    void MoveLineStart();
    void MoveLineEnd();
    void MoveFileStart();
    void MoveFileEnd();
    void MovePage(int direction, size_t pageLines);
    void GotoLine(size_t oneBasedLine);

    // --- editing ---
    void InsertChar(wchar_t ch);
    void InsertNewline();
    void DeleteBack();      // Backspace
    void DeleteForward();   // Delete
    void DeleteLine();

    // --- search (case-insensitive substring, no regex) ---
    // Searches the whole buffer from the start, moves the cursor to the first
    // match, and remembers `term` as the last search term (used by FindNext /
    // FindPrevious). Returns false and leaves the cursor unchanged if not found.
    bool FindFirst(const std::wstring& term, std::wstring* messageOut);
    // Searches forward from just after the cursor, using the last search
    // term. Does not wrap around the end of the buffer.
    bool FindNext(std::wstring* messageOut);
    // Searches backward from just before the cursor, using the last search
    // term. Does not wrap around the start of the buffer.
    bool FindPrevious(std::wstring* messageOut);
    const std::wstring& LastSearchTerm() const { return lastSearchTerm_; }

    // --- block selection ---
    // Cancels any existing selection and starts marking a new one of the
    // given mode, anchored at the cursor.
    void BlockMarkStart(BlockMode mode);
    // Freezes the current selection so further cursor movement no longer
    // resizes it. No-op if nothing is being marked.
    void BlockMarkEnd();
    // Clears the selection without touching the buffer.
    void BlockCancel();

    bool HasSelection() const { return selection_.mode != BlockMode::None; }
    const Selection& CurrentSelection() const { return selection_; }

    // Inserts a copy of the block's content at the cursor. Selection is left
    // in place (repeatable paste). Fails if there is no selection.
    bool BlockCopy(std::wstring* messageOut);
    // Moves the block's content to the cursor (copy + erase original).
    // Cancels the selection. Fails if there is no selection.
    bool BlockMove(std::wstring* messageOut);
    // Erases the block's content in place. Cancels the selection. Fails if
    // there is no selection.
    bool BlockDelete(std::wstring* messageOut);

    // --- viewport ---
    size_t TopLine() const { return topLine_; }
    size_t LeftCol() const { return leftCol_; }
    // Scrolls just enough to keep the cursor inside a visibleLines x visibleCols box.
    void EnsureCursorVisible(size_t visibleLines, size_t visibleCols);
    void ScrollBy(int lines, size_t visibleLines);

private:
    void ClampCursor();
    void UpdateSelectionExtent();   // called after every cursor move
    std::pair<Cursor, Cursor> NormalizedSelection() const;   // (min, max)
    std::vector<std::wstring> ExtractBlock() const;
    void EraseBlock();

    std::vector<std::wstring> lines_;
    Cursor                    cursor_;
    Selection                 selection_;
    std::wstring              path_;
    bool                      modified_   = false;
    unsigned long long        byteSize_   = 0;
    io::LineEnding            lineEnding_ = io::LineEnding::CRLF;
    bool                      hadBom_     = false;
    size_t                    topLine_    = 0;
    size_t                    leftCol_    = 0;
    std::wstring              lastSearchTerm_;
};

} // namespace qed
