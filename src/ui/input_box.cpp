#include "input_box.h"

namespace qed {
namespace ui {

void InputBox::Open(std::wstring label) {
    active_ = true;
    label_  = std::move(label);
    text_.clear();
}

void InputBox::Cancel() {
    active_ = false;
    text_.clear();
}

InputAction InputBox::HandleChar(wchar_t ch) {
    if (!active_) return InputAction::kNone;

    if (ch == L'\r' || ch == L'\n') {
        active_ = false;   // Text() still holds the typed value for the caller
        return InputAction::kConfirmed;
    }
    if (ch == L'\b') {
        if (!text_.empty()) text_.pop_back();
        return InputAction::kNone;
    }
    if (ch < 0x20) return InputAction::kNone;   // ignore control codes, incl. Esc

    text_.push_back(ch);
    return InputAction::kNone;
}

void InputBox::Draw(HDC dc, const RECT& clientRect, HFONT font, int lineHeight) const {
    if (!active_) return;

    const int padX = 6;
    const int y     = lineHeight;   // sits just below the status bar
    RECT bar = {0, y, clientRect.right, y + lineHeight + 4};

    HBRUSH background = CreateSolidBrush(RGB(40, 40, 40));
    FillRect(dc, &bar, background);
    DeleteObject(background);
    FrameRect(dc, &bar, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));

    HFONT oldFont = static_cast<HFONT>(SelectObject(dc, font));
    const int oldMode = SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 0));

    const std::wstring line = label_ + text_ + L"_";
    RECT textRect = bar;
    textRect.left += padX;
    DrawTextW(dc, line.c_str(), static_cast<int>(line.size()), &textRect,
              DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX);

    SetBkMode(dc, oldMode);
    SelectObject(dc, oldFont);
}

} // namespace ui
} // namespace qed
