// input_box.h — single-line text input overlay (prompt + typed text).
//
// Generic, reusable module: Open() shows it with a label, typed characters
// are appended, Enter confirms (HandleChar returns kConfirmed), Esc cancels
// (call Cancel() from the WM_KEYDOWN handler, matching how Esc is already
// handled elsewhere in this codebase).
#pragma once

#include <string>
#include <windows.h>

namespace qed {
namespace ui {

enum class InputAction {
    kNone,        // key consumed, input box still open
    kConfirmed,   // Enter pressed; Text() holds the final value
    kCancelled,   // not returned by HandleChar; see Cancel()
};

class InputBox {
public:
    bool IsActive() const { return active_; }

    // Opens the box, empty, with the given prompt label.
    void Open(std::wstring label);

    // Cancels input (Esc). Clears the typed text.
    void Cancel();

    // Feeds one WM_CHAR code point while the box is active.
    // '\r'/'\n' confirms, '\b' deletes the last character, other control
    // codes (including Esc, 0x1B) are ignored — Esc is handled via Cancel()
    // from WM_KEYDOWN instead.
    InputAction HandleChar(wchar_t ch);

    const std::wstring& Text() const { return text_; }

    void Draw(HDC dc, const RECT& clientRect, HFONT font, int lineHeight) const;

private:
    bool         active_ = false;
    std::wstring label_;
    std::wstring text_;
};

} // namespace ui
} // namespace qed
