#include "notice_box.h"

namespace qed {
namespace ui {

void NoticeBox::Show(std::wstring text) {
    active_ = true;
    text_   = std::move(text);
}

void NoticeBox::Dismiss() {
    active_ = false;
    text_.clear();
}

void NoticeBox::Draw(HDC dc, const RECT& clientRect, HFONT font, int lineHeight) const {
    if (!active_) return;

    const int padX = 14;
    const int padY = 8;

    HFONT oldFontMeasure = static_cast<HFONT>(SelectObject(dc, font));
    SIZE size{};
    GetTextExtentPoint32W(dc, text_.c_str(), static_cast<int>(text_.size()), &size);
    SelectObject(dc, oldFontMeasure);

    const int boxWidth  = size.cx + padX * 2;
    const int boxHeight = lineHeight + padY * 2;
    const int x = (clientRect.right - boxWidth) / 2;
    const int y = (clientRect.bottom - boxHeight) / 3;

    RECT box = {x, y, x + boxWidth, y + boxHeight};

    HBRUSH background = CreateSolidBrush(RGB(128, 0, 0));
    FillRect(dc, &box, background);
    DeleteObject(background);
    FrameRect(dc, &box, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));

    HFONT oldFont = static_cast<HFONT>(SelectObject(dc, font));
    const int oldMode = SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));

    RECT textRect = box;
    DrawTextW(dc, text_.c_str(), static_cast<int>(text_.size()), &textRect,
              DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);

    SetBkMode(dc, oldMode);
    SelectObject(dc, oldFont);
}

} // namespace ui
} // namespace qed
