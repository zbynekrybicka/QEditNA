// window.h — main window creation and message loop
#pragma once

#include <string>
#include <windows.h>

namespace qed {
namespace ui {

// Registers the window class, creates the window and runs the message loop.
// Returns the process exit code.
int RunEditor(HINSTANCE instance, int showCommand, const std::wstring& fileToOpen);

} // namespace ui
} // namespace qed
