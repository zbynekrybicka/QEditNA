// main.cpp — QEditNA entry point
#include "config/constants.h"
#include "ui/window.h"

#include <string>
#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int showCommand) {
    std::wstring fileToOpen;

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        if (argc > 1) fileToOpen = argv[1];
        LocalFree(argv);
    }

    return qed::ui::RunEditor(instance, showCommand, fileToOpen);
}
