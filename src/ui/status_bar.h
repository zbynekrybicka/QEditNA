// status_bar.h — single-line status bar at the top of the window
#pragma once

#include <string>
#include <windows.h>

namespace qed {

class EditorCore;

namespace ui {

// Builds the status text: filename, size, cursor position, flags.
std::wstring BuildStatusText(const EditorCore& editor, const std::wstring& transient);

// Paints the status bar into rect using the supplied font.
void DrawStatusBar(HDC dc, const RECT& rect, const std::wstring& text);

} // namespace ui
} // namespace qed
