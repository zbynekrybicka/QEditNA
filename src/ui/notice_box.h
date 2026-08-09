// notice_box.h — modal-ish notice overlay ("term not found", etc.)
//
// Shown with Show(text); dismissed by any key. While active, the caller
// should route all keyboard input to Dismiss() instead of normal handling,
// so the message is guaranteed to be seen before anything else happens.
#pragma once

#include <string>
#include <windows.h>

namespace qed {
namespace ui {

class NoticeBox {
public:
    bool IsActive() const { return active_; }

    void Show(std::wstring text);
    void Dismiss();

    void Draw(HDC dc, const RECT& clientRect, HFONT font, int lineHeight) const;

private:
    bool         active_ = false;
    std::wstring text_;
};

} // namespace ui
} // namespace qed
