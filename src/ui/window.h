// window.h — main window creation and message loop
#pragma once

#include <string>
#include <windows.h>

namespace qed {
namespace ui {

// Registers the window class, creates the window and runs the message loop.
// Returns the process exit code.
// macroToLoad, when non-empty, is loaded via macro.load right after startup
// (equivalent to running the "Načíst makra" menu command with that filename).
int RunEditor(HINSTANCE instance, int showCommand, const std::wstring& fileToOpen,
             const std::wstring& macroToLoad = L"");

} // namespace ui
} // namespace qed
