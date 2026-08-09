// command_menu.h — F1 command menu: a static tree of commands and submenus,
// navigated with the keyboard (drilldown style, one level visible at a time).
#pragma once

#include <string>
#include <vector>
#include <windows.h>

namespace qed {
namespace ui {

struct MenuItem {
    std::wstring          label;
    std::wstring          command;    // non-empty for leaf items (executable commands)
    std::vector<MenuItem> children;   // non-empty for submenus

    bool IsSubmenu() const { return !children.empty(); }
};

enum class MenuAction {
    kNone,      // key consumed, menu still open
    kClosed,    // menu closed without choosing a command
    kExecute,   // a leaf command was chosen; outCommand is filled and the menu closed
};

// Owns the static command tree and the navigation state (a stack of levels,
// one per submenu depth, each remembering its own selected index).
class CommandMenu {
public:
    CommandMenu();

    bool IsActive() const { return active_; }

    // Opens the menu at the root if closed, or closes it if open.
    void Toggle();
    void Close();

    // Handles a key while the menu is active. Only call this when IsActive().
    MenuAction HandleKey(WPARAM key, std::wstring* outCommand);

    void Draw(HDC dc, const RECT& clientRect, HFONT font, int lineHeight) const;

private:
    struct Level {
        const std::vector<MenuItem>* items;
        size_t                       selected;
    };

    std::vector<MenuItem> root_;
    std::vector<Level>    stack_;
    bool                  active_ = false;
};

} // namespace ui
} // namespace qed
