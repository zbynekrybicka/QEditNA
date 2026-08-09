#include "editor_core.h"

#include <cwctype>

#include "../config/config.h"

namespace qed {
namespace {
const std::wstring kEmptyLine;

std::wstring ToLowerCopy(const std::wstring& text) {
    std::wstring out(text);
    for (wchar_t& ch : out) ch = static_cast<wchar_t>(std::towlower(ch));
    return out;
}
}

EditorCore::EditorCore() {
    lines_.emplace_back();
}

const std::wstring& EditorCore::Line(size_t index) const {
    return index < lines_.size() ? lines_[index] : kEmptyLine;
}

bool EditorCore::LoadFromFile(const std::wstring& path, std::wstring* errorOut) {
    io::LoadResult loaded = io::LoadFile(path);
    if (!loaded.ok) {
        if (errorOut) *errorOut = loaded.error;
        return false;
    }
    lines_      = std::move(loaded.lines);
    lineEnding_ = loaded.lineEnding;
    hadBom_     = loaded.hadBom;
    byteSize_   = loaded.byteSize;
    path_       = path;
    modified_   = false;
    cursor_     = Cursor{};
    topLine_    = 0;
    leftCol_    = 0;
    if (lines_.empty()) lines_.emplace_back();
    return true;
}

bool EditorCore::SaveToFile(std::wstring* messageOut) {
    if (path_.empty()) {
        if (messageOut) *messageOut = L"No filename — save is not available yet.";
        return false;
    }
    io::SaveResult saved = io::SaveFile(path_, lines_, lineEnding_, hadBom_);
    if (saved.ok) {
        modified_ = false;
        if (messageOut) *messageOut = L"Saved.";
        return true;
    }
    if (messageOut) *messageOut = saved.error;
    return false;
}

// --- cursor movement -------------------------------------------------------

void EditorCore::ClampCursor() {
    if (lines_.empty()) lines_.emplace_back();
    if (cursor_.line >= lines_.size()) cursor_.line = lines_.size() - 1;
    if (cursor_.col > lines_[cursor_.line].size()) cursor_.col = lines_[cursor_.line].size();
}

void EditorCore::MoveLeft() {
    if (cursor_.col > 0) {
        --cursor_.col;
    } else if (cursor_.line > 0) {
        --cursor_.line;
        cursor_.col = lines_[cursor_.line].size();
    }
}

void EditorCore::MoveRight() {
    if (cursor_.col < lines_[cursor_.line].size()) {
        ++cursor_.col;
    } else if (cursor_.line + 1 < lines_.size()) {
        ++cursor_.line;
        cursor_.col = 0;
    }
}

void EditorCore::MoveUp() {
    if (cursor_.line > 0) --cursor_.line;
    ClampCursor();
}

void EditorCore::MoveDown() {
    if (cursor_.line + 1 < lines_.size()) ++cursor_.line;
    ClampCursor();
}

void EditorCore::MoveLineStart() { cursor_.col = 0; }
void EditorCore::MoveLineEnd()   { cursor_.col = lines_[cursor_.line].size(); }

void EditorCore::MoveFileStart() { cursor_ = Cursor{}; }

void EditorCore::MoveFileEnd() {
    cursor_.line = lines_.size() - 1;
    cursor_.col  = lines_[cursor_.line].size();
}

void EditorCore::MovePage(int direction, size_t pageLines) {
    if (pageLines == 0) pageLines = 1;
    if (direction < 0) {
        cursor_.line = (cursor_.line > pageLines) ? cursor_.line - pageLines : 0;
    } else {
        cursor_.line += pageLines;
    }
    ClampCursor();
}

void EditorCore::GotoLine(size_t oneBasedLine) {
    cursor_.line = (oneBasedLine == 0) ? 0 : oneBasedLine - 1;
    cursor_.col  = 0;
    ClampCursor();
}

// --- editing ---------------------------------------------------------------

void EditorCore::InsertChar(wchar_t ch) {
    lines_[cursor_.line].insert(cursor_.col, 1, ch);
    ++cursor_.col;
    modified_ = true;
}

void EditorCore::InsertNewline() {
    std::wstring& line = lines_[cursor_.line];
    std::wstring tail  = line.substr(cursor_.col);
    line.erase(cursor_.col);
    lines_.insert(lines_.begin() + static_cast<long>(cursor_.line) + 1, tail);
    ++cursor_.line;
    cursor_.col = 0;
    modified_   = true;
}

void EditorCore::DeleteBack() {
    if (cursor_.col > 0) {
        lines_[cursor_.line].erase(cursor_.col - 1, 1);
        --cursor_.col;
        modified_ = true;
    } else if (cursor_.line > 0) {
        const size_t joinAt = lines_[cursor_.line - 1].size();
        lines_[cursor_.line - 1] += lines_[cursor_.line];
        lines_.erase(lines_.begin() + static_cast<long>(cursor_.line));
        --cursor_.line;
        cursor_.col = joinAt;
        modified_   = true;
    }
}

void EditorCore::DeleteForward() {
    std::wstring& line = lines_[cursor_.line];
    if (cursor_.col < line.size()) {
        line.erase(cursor_.col, 1);
        modified_ = true;
    } else if (cursor_.line + 1 < lines_.size()) {
        line += lines_[cursor_.line + 1];
        lines_.erase(lines_.begin() + static_cast<long>(cursor_.line) + 1);
        modified_ = true;
    }
}

void EditorCore::DeleteLine() {
    if (lines_.size() == 1) {
        lines_[0].clear();
    } else {
        lines_.erase(lines_.begin() + static_cast<long>(cursor_.line));
    }
    cursor_.col = 0;
    ClampCursor();
    modified_ = true;
}

// --- search ------------------------------------------------------------------

bool EditorCore::FindFirst(const std::wstring& term, std::wstring* messageOut) {
    lastSearchTerm_ = term;
    if (term.empty()) {
        if (messageOut) *messageOut = L"Prázdný hledaný výraz.";
        return false;
    }
    const std::wstring needle = ToLowerCopy(term);
    for (size_t line = 0; line < lines_.size(); ++line) {
        const std::wstring hay = ToLowerCopy(lines_[line]);
        const size_t pos = hay.find(needle);
        if (pos != std::wstring::npos) {
            cursor_.line = line;
            cursor_.col  = pos;
            ClampCursor();
            if (messageOut) *messageOut = L"Nalezeno: " + term;
            return true;
        }
    }
    if (messageOut) *messageOut = L"Hledaný výraz nebyl nalezen: " + term;
    return false;
}

bool EditorCore::FindNext(std::wstring* messageOut) {
    if (lastSearchTerm_.empty()) {
        if (messageOut) *messageOut = L"Není zadán žádný hledaný výraz.";
        return false;
    }
    const std::wstring needle = ToLowerCopy(lastSearchTerm_);
    for (size_t line = cursor_.line; line < lines_.size(); ++line) {
        const std::wstring hay = ToLowerCopy(lines_[line]);
        const size_t startCol = (line == cursor_.line) ? cursor_.col + 1 : 0;
        if (startCol > hay.size()) continue;
        const size_t pos = hay.find(needle, startCol);
        if (pos != std::wstring::npos) {
            cursor_.line = line;
            cursor_.col  = pos;
            ClampCursor();
            if (messageOut) *messageOut = L"Nalezeno: " + lastSearchTerm_;
            return true;
        }
    }
    if (messageOut) *messageOut = L"Další výskyt nebyl nalezen.";
    return false;
}

bool EditorCore::FindPrevious(std::wstring* messageOut) {
    if (lastSearchTerm_.empty()) {
        if (messageOut) *messageOut = L"Není zadán žádný hledaný výraz.";
        return false;
    }
    const std::wstring needle = ToLowerCopy(lastSearchTerm_);
    for (size_t offset = 0; offset <= cursor_.line; ++offset) {
        const size_t line = cursor_.line - offset;
        const std::wstring hay = ToLowerCopy(lines_[line]);
        size_t pos;
        if (line == cursor_.line) {
            if (cursor_.col == 0) continue;
            pos = hay.rfind(needle, cursor_.col - 1);
        } else {
            pos = hay.rfind(needle);
        }
        if (pos != std::wstring::npos) {
            cursor_.line = line;
            cursor_.col  = pos;
            ClampCursor();
            if (messageOut) *messageOut = L"Nalezeno: " + lastSearchTerm_;
            return true;
        }
    }
    if (messageOut) *messageOut = L"Předchozí výskyt nebyl nalezen.";
    return false;
}

// --- viewport --------------------------------------------------------------

void EditorCore::EnsureCursorVisible(size_t visibleLines, size_t visibleCols) {
    if (visibleLines == 0) visibleLines = 1;
    if (visibleCols == 0)  visibleCols  = 1;

    if (cursor_.line < topLine_) topLine_ = cursor_.line;
    else if (cursor_.line >= topLine_ + visibleLines)
        topLine_ = cursor_.line - visibleLines + 1;

    if (cursor_.col < leftCol_) leftCol_ = cursor_.col;
    else if (cursor_.col >= leftCol_ + visibleCols)
        leftCol_ = cursor_.col - visibleCols + 1;
}

void EditorCore::ScrollBy(int lines, size_t visibleLines) {
    const size_t maxTop = (lines_.size() > visibleLines) ? lines_.size() - visibleLines : 0;
    if (lines < 0) {
        const size_t up = static_cast<size_t>(-lines);
        topLine_ = (topLine_ > up) ? topLine_ - up : 0;
    } else {
        topLine_ += static_cast<size_t>(lines);
    }
    if (topLine_ > maxTop) topLine_ = maxTop;
}

} // namespace qed
