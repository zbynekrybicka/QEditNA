// main.cpp — QEditNA entry point
#include "config/constants.h"
#include "ui/window.h"

#include <string>
#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int showCommand) {
    std::wstring fileToOpen;
    std::wstring macroToLoad;

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        for (int i = 1; i < argc; ++i) {
            if (std::wstring(argv[i]) == L"-m" && i + 1 < argc) {
                macroToLoad = argv[++i];
            } else if (fileToOpen.empty()) {
                fileToOpen = argv[i];
            }
        }
        LocalFree(argv);
    }

    return qed::ui::RunEditor(instance, showCommand, fileToOpen, macroToLoad);
}
