#include "status_bar.h"

#include "../config/config.h"
#include "../editor/editor_core.h"

namespace qed {
namespace ui {
namespace {

std::wstring BaseName(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    return (slash == std::wstring::npos) ? path : path.substr(slash + 1);
}

} // namespace

std::wstring BuildStatusText(const EditorCore& editor, const std::wstring& transient) {
    const std::wstring name =
        editor.FilePath().empty() ? L"[no file]" : BaseName(editor.FilePath());

    std::wstring text = name;
    if (editor.IsModified()) text += L" *";
    text += L"  |  " + std::to_wstring(editor.ByteSize()) + L" B";
    text += L"  |  " + std::to_wstring(editor.GetCursor().line + 1) + L":" +
            std::to_wstring(editor.GetCursor().col + 1);
    text += L"  |  " + std::to_wstring(editor.LineCount()) + L" lines";
    if (!config::kWriteEnabled) text += L"  |  READ-ONLY";
    if (!transient.empty()) text += L"  |  " + transient;
    return text;
}

void DrawStatusBar(HDC dc, const RECT& rect, const std::wstring& text) {
    const COLORREF background = RGB(0, 0, 128);
    const COLORREF foreground = RGB(255, 255, 255);

    HBRUSH brush = CreateSolidBrush(background);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);

    const int   oldMode = SetBkMode(dc, TRANSPARENT);
    const COLORREF oldColor = SetTextColor(dc, foreground);

    RECT textRect = rect;
    textRect.left += kStatusBarPadPx;
    DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &textRect,
              DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX | DT_END_ELLIPSIS);

    SetTextColor(dc, oldColor);
    SetBkMode(dc, oldMode);
}

} // namespace ui
} // namespace qed
