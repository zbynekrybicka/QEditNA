#include "command_menu.h"

#include <algorithm>

namespace qed {
namespace ui {
namespace {

MenuItem Leaf(const wchar_t* label, const wchar_t* command, bool enabled = true) {
    MenuItem item;
    item.label   = label;
    item.command = command;
    item.enabled = enabled;
    return item;
}

MenuItem Submenu(const wchar_t* label, std::vector<MenuItem> children) {
    MenuItem item;
    item.label    = label;
    item.children = std::move(children);
    return item;
}

std::vector<MenuItem> BuildDefaultMenu() {
    return {
        Submenu(L"Cursor", {
            Leaf(L"Left",        L"cursor.left"),
            Leaf(L"Right",       L"cursor.right"),
            Leaf(L"Up",          L"cursor.up"),
            Leaf(L"Down",        L"cursor.down"),
            Leaf(L"Line start",  L"cursor.line-start"),
            Leaf(L"Line end",    L"cursor.line-end"),
            Leaf(L"File start",  L"cursor.file-start"),
            Leaf(L"File end",    L"cursor.file-end"),
            Leaf(L"Page up",     L"cursor.page-up"),
            Leaf(L"Page down",   L"cursor.page-down"),
        }),
        Submenu(L"Edit", {
            Leaf(L"Newline",         L"edit.newline"),
            Leaf(L"Delete back",     L"edit.delete-back"),
            Leaf(L"Delete forward",  L"edit.delete-forward"),
            Leaf(L"Delete line",     L"edit.delete-line"),
        }),
        Submenu(L"File", {
            Leaf(L"Save", L"file.save"),
        }),
        Submenu(L"Find", {
            Leaf(L"Search...", L"find.search"),
            Leaf(L"Next",      L"find.next"),
            Leaf(L"Previous",  L"find.previous"),
        }),
        Submenu(L"Makra", {
            Leaf(L"Nové makro",         L"macro.new"),
            Leaf(L"Ukončit nahrávání",  L"macro.stop-recording", /*enabled=*/false),
            Leaf(L"Smazat makro",       L"macro.delete"),
            Leaf(L"Uložit makra",       L"macro.save"),
            Leaf(L"Načíst makra",       L"macro.load"),
        }),
    };
}

void SetEnabledRecursive(std::vector<MenuItem>& items, const std::wstring& command, bool enabled) {
    for (auto& item : items) {
        if (item.IsSubmenu()) {
            SetEnabledRecursive(item.children, command, enabled);
        } else if (item.command == command) {
            item.enabled = enabled;
        }
    }
}

} // namespace

CommandMenu::CommandMenu() : root_(BuildDefaultMenu()) {}

void CommandMenu::Toggle() {
    if (active_) {
        Close();
        return;
    }
    active_ = true;
    stack_.clear();
    stack_.push_back(Level{&root_, 0});
}

void CommandMenu::Close() {
    active_ = false;
    stack_.clear();
}

void CommandMenu::SetEnabled(const std::wstring& command, bool enabled) {
    SetEnabledRecursive(root_, command, enabled);
}

MenuAction CommandMenu::HandleKey(WPARAM key, std::wstring* outCommand) {
    if (!active_ || stack_.empty()) return MenuAction::kNone;

    Level& level = stack_.back();
    const size_t count = level.items->size();
    if (count == 0) return MenuAction::kNone;

    switch (key) {
        case VK_UP:
            level.selected = (level.selected == 0) ? count - 1 : level.selected - 1;
            return MenuAction::kNone;

        case VK_DOWN:
            level.selected = (level.selected + 1) % count;
            return MenuAction::kNone;

        case VK_RIGHT:
        case VK_RETURN: {
            const MenuItem& item = (*level.items)[level.selected];
            if (item.IsSubmenu()) {
                stack_.push_back(Level{&item.children, 0});
                return MenuAction::kNone;
            }
            if (!item.enabled) return MenuAction::kNone;
            if (outCommand) *outCommand = item.command;
            Close();
            return MenuAction::kExecute;
        }

        case VK_LEFT:
            if (stack_.size() > 1) stack_.pop_back();
            return MenuAction::kNone;

        default:
            return MenuAction::kNone;   // swallow everything else while the menu is open
    }
}

void CommandMenu::Draw(HDC dc, const RECT& clientRect, HFONT font, int lineHeight) const {
    if (!active_ || stack_.empty()) return;

    const Level& level = stack_.back();
    const std::vector<MenuItem>& items = *level.items;

    const int padX  = 10;
    const int padY  = 4;
    const int width = 260;

    int textWidth = 0;
    {
        HDC measure = dc;
        HFONT oldFont = static_cast<HFONT>(SelectObject(measure, font));
        for (const auto& item : items) {
            SIZE size{};
            std::wstring text = item.label + (item.IsSubmenu() ? L"  >" : L"");
            GetTextExtentPoint32W(measure, text.c_str(), static_cast<int>(text.size()), &size);
            textWidth = (std::max)(textWidth, static_cast<int>(size.cx));
        }
        SelectObject(measure, oldFont);
    }
    const int boxWidth  = (std::max)(width, textWidth + padX * 2);
    const int boxHeight = static_cast<int>(items.size()) * lineHeight + padY * 2;

    const int x = (clientRect.right - boxWidth) / 2;
    const int y = (clientRect.bottom - boxHeight) / 3;

    RECT box = {x, y, x + boxWidth, y + boxHeight};

    HBRUSH background = CreateSolidBrush(RGB(30, 30, 30));
    FillRect(dc, &box, background);
    DeleteObject(background);
    FrameRect(dc, &box, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));

    HFONT oldFont = static_cast<HFONT>(SelectObject(dc, font));
    const int oldMode = SetBkMode(dc, TRANSPARENT);

    for (size_t i = 0; i < items.size(); ++i) {
        RECT row = {x, static_cast<LONG>(y + padY + i * lineHeight),
                    x + boxWidth, static_cast<LONG>(y + padY + (i + 1) * lineHeight)};

        const bool selected = (i == level.selected);
        if (selected) {
            HBRUSH highlight = CreateSolidBrush(RGB(0, 0, 128));
            FillRect(dc, &row, highlight);
            DeleteObject(highlight);
        }

        const bool enabled = items[i].IsSubmenu() || items[i].enabled;
        SetTextColor(dc, !enabled ? RGB(120, 120, 120)
                                   : selected ? RGB(255, 255, 255) : RGB(220, 220, 220));

        RECT textRect = row;
        textRect.left += padX;
        std::wstring text = items[i].label + (items[i].IsSubmenu() ? L"  >" : L"");
        DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &textRect,
                  DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX);
    }

    SetBkMode(dc, oldMode);
    SelectObject(dc, oldFont);
}

} // namespace ui
} // namespace qed
