#include "window.h"

#include "command_menu.h"
#include "input_box.h"
#include "notice_box.h"
#include "status_bar.h"
#include "../config/config.h"
#include "../editor/command_engine.h"
#include "../editor/editor_core.h"

#include <string>
#include <windowsx.h>

namespace qed {
namespace ui {
namespace {

// Everything the window needs, stored once and reached via GWLP_USERDATA.
struct WindowState {
    EditorCore    editor;
    CommandEngine commands;
    CommandMenu   menu;
    InputBox      searchInput;
    NoticeBox     notice;
    bool          noticeJustDismissed = false;
    HFONT         font        = nullptr;
    int           charWidth   = 8;
    int           lineHeight  = 16;
    int           statusHeight = 18;
    size_t        visibleLines = 1;
    size_t        visibleCols  = 1;
    std::wstring  statusMessage;
};

WindowState* StateOf(HWND window) {
    return reinterpret_cast<WindowState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
}

// Expands tabs so the rendered column matches the logical layout.
std::wstring ExpandTabs(const std::wstring& line) {
    std::wstring out;
    out.reserve(line.size());
    for (wchar_t ch : line) {
        if (ch == L'\t') {
            const size_t pad = kTabWidth - (out.size() % kTabWidth);
            out.append(pad, L' ');
        } else {
            out.push_back(ch);
        }
    }
    return out;
}

void MeasureFont(HWND window, WindowState* state) {
    HDC dc = GetDC(window);
    HFONT previous = static_cast<HFONT>(SelectObject(dc, state->font));
    TEXTMETRICW metrics{};
    GetTextMetricsW(dc, &metrics);
    state->charWidth    = metrics.tmAveCharWidth > 0 ? metrics.tmAveCharWidth : 8;
    state->lineHeight   = metrics.tmHeight + metrics.tmExternalLeading;
    state->statusHeight = state->lineHeight + 2;
    SelectObject(dc, previous);
    ReleaseDC(window, dc);
}

void RecalcViewport(HWND window, WindowState* state) {
    RECT client{};
    GetClientRect(window, &client);
    const int textHeight = client.bottom - state->statusHeight;
    state->visibleLines = textHeight > 0
                              ? static_cast<size_t>(textHeight / state->lineHeight)
                              : 1;
    if (state->visibleLines == 0) state->visibleLines = 1;
    state->visibleCols = static_cast<size_t>(client.right / state->charWidth);
    if (state->visibleCols == 0) state->visibleCols = 1;
}

void UpdateCaret(HWND /*window*/, WindowState* state) {
    const Cursor& cursor = state->editor.GetCursor();
    if (cursor.line < state->editor.TopLine() ||
        cursor.line >= state->editor.TopLine() + state->visibleLines ||
        cursor.col < state->editor.LeftCol()) {
        SetCaretPos(-1, -1);
        return;
    }
    const int x = static_cast<int>(cursor.col - state->editor.LeftCol()) * state->charWidth;
    const int y = state->statusHeight +
                  static_cast<int>(cursor.line - state->editor.TopLine()) * state->lineHeight;
    SetCaretPos(x, y);
}

void Refresh(HWND window, WindowState* state) {
    state->editor.EnsureCursorVisible(state->visibleLines, state->visibleCols);
    UpdateCaret(window, state);
    InvalidateRect(window, nullptr, FALSE);
}

// Runs a named command and refreshes the view.
void Run(HWND window, WindowState* state, const wchar_t* command) {
    CommandContext context;
    context.editor       = &state->editor;
    context.visibleLines = state->visibleLines;
    context.visibleCols  = state->visibleCols;
    state->commands.Execute(command, context);
    state->statusMessage = context.message;
    Refresh(window, state);
}

// Runs a command that takes a textual argument (e.g. find.search) and, on
// failure, surfaces the message as a NoticeBox instead of only the status bar
// — search failures need to be impossible to miss.
void RunFind(HWND window, WindowState* state, const wchar_t* command,
            const std::wstring& argument) {
    CommandContext context;
    context.editor       = &state->editor;
    context.visibleLines = state->visibleLines;
    context.visibleCols  = state->visibleCols;
    context.argument     = argument;
    const bool ok = state->commands.Execute(command, context);
    state->statusMessage = context.message;
    if (!ok && !context.message.empty()) {
        state->notice.Show(context.message);
    }
    Refresh(window, state);
}

void OnPaint(HWND window, WindowState* state) {
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(window, &paint);

    RECT client{};
    GetClientRect(window, &client);

    // Off-screen buffer keeps the redraw flicker-free.
    HDC     memDc  = CreateCompatibleDC(dc);
    HBITMAP bitmap = CreateCompatibleBitmap(dc, client.right, client.bottom);
    HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memDc, bitmap));

    HBRUSH background = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(memDc, &client, background);
    DeleteObject(background);

    HFONT oldFont = static_cast<HFONT>(SelectObject(memDc, state->font));
    SetBkMode(memDc, TRANSPARENT);
    SetTextColor(memDc, RGB(0, 0, 0));

    const size_t top = state->editor.TopLine();
    const size_t left = state->editor.LeftCol();
    for (size_t row = 0; row < state->visibleLines; ++row) {
        const size_t index = top + row;
        if (index >= state->editor.LineCount()) break;
        const std::wstring expanded = ExpandTabs(state->editor.Line(index));
        if (expanded.size() <= left) continue;
        const std::wstring visible = expanded.substr(left);
        TextOutW(memDc, 0,
                 state->statusHeight + static_cast<int>(row) * state->lineHeight,
                 visible.c_str(), static_cast<int>(visible.size()));
    }

    RECT statusRect = {0, 0, client.right, state->statusHeight};
    DrawStatusBar(memDc, statusRect,
                  BuildStatusText(state->editor, state->statusMessage));

    state->menu.Draw(memDc, client, state->font, state->lineHeight);
    state->searchInput.Draw(memDc, client, state->font, state->lineHeight);
    state->notice.Draw(memDc, client, state->font, state->lineHeight);

    BitBlt(dc, 0, 0, client.right, client.bottom, memDc, 0, 0, SRCCOPY);

    SelectObject(memDc, oldFont);
    SelectObject(memDc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(memDc);

    EndPaint(window, &paint);
}

bool OnKeyDown(HWND window, WindowState* state, WPARAM key) {
    if (state->notice.IsActive()) {
        // Any key dismisses the notice. The keydown that dismisses it may
        // also generate a WM_CHAR (e.g. a letter) — noticeJustDismissed
        // tells OnChar to swallow exactly that one follow-up message so it
        // doesn't fall through to text insertion.
        state->notice.Dismiss();
        state->noticeJustDismissed = true;
        InvalidateRect(window, nullptr, FALSE);
        return true;
    }

    if (state->searchInput.IsActive()) {
        if (key == VK_ESCAPE) {
            state->searchInput.Cancel();
            InvalidateRect(window, nullptr, FALSE);
        }
        return true;   // swallow everything else while the input box is open
    }

    if (key == VK_F1) {
        state->menu.Toggle();
        InvalidateRect(window, nullptr, FALSE);
        return true;
    }

    if (state->menu.IsActive()) {
        std::wstring command;
        if (state->menu.HandleKey(key, &command) == MenuAction::kExecute) {
            if (command == L"find.search") {
                state->searchInput.Open(L"Hledat: ");
                InvalidateRect(window, nullptr, FALSE);
            } else if (command == L"find.next" || command == L"find.previous") {
                RunFind(window, state, command.c_str(), L"");
            } else {
                Run(window, state, command.c_str());
            }
        } else {
            InvalidateRect(window, nullptr, FALSE);
        }
        return true;
    }

    const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

    switch (key) {
        case VK_LEFT:   Run(window, state, ctrl ? L"cursor.file-start" : L"cursor.left"); return true;
        case VK_RIGHT:  Run(window, state, ctrl ? L"cursor.file-end"   : L"cursor.right"); return true;
        case VK_UP:     Run(window, state, L"cursor.up");         return true;
        case VK_DOWN:   Run(window, state, L"cursor.down");       return true;
        case VK_HOME:   Run(window, state, ctrl ? L"cursor.file-start" : L"cursor.line-start"); return true;
        case VK_END:    Run(window, state, ctrl ? L"cursor.file-end"   : L"cursor.line-end");   return true;
        case VK_PRIOR:  Run(window, state, L"cursor.page-up");    return true;
        case VK_NEXT:   Run(window, state, L"cursor.page-down");  return true;
        case VK_DELETE: Run(window, state, L"edit.delete-forward"); return true;
        case 'Y':
            if (ctrl) { Run(window, state, L"edit.delete-line"); return true; }
            return false;
        case 'S':
            if (ctrl) { Run(window, state, L"file.save"); return true; }
            return false;
        case VK_ESCAPE:
            state->statusMessage.clear();
            InvalidateRect(window, nullptr, FALSE);
            return true;
        default:
            return false;
    }
}

bool OnChar(HWND window, WindowState* state, WPARAM ch) {
    if (state->noticeJustDismissed) {
        state->noticeJustDismissed = false;   // consume the char that dismissed the notice
        return true;
    }
    if (state->notice.IsActive()) return true;

    if (state->searchInput.IsActive()) {
        const InputAction action =
            state->searchInput.HandleChar(static_cast<wchar_t>(ch));
        if (action == InputAction::kConfirmed) {
            const std::wstring term = state->searchInput.Text();
            RunFind(window, state, L"find.search", term);
        } else {
            InvalidateRect(window, nullptr, FALSE);
        }
        return true;
    }

    if (state->menu.IsActive()) return true;   // swallow while the F1 menu is open

    switch (ch) {
        case L'\r':
        case L'\n':
            Run(window, state, L"edit.newline");
            return true;
        case L'\b':
            Run(window, state, L"edit.delete-back");
            return true;
        case 0x1B:  // Esc — already handled in WM_KEYDOWN
            return true;
        default:
            break;
    }
    if (ch < 0x20 && ch != L'\t') return false;   // ignore other control codes
    state->editor.InsertChar(static_cast<wchar_t>(ch));
    Refresh(window, state);
    return true;
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(window, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return DefWindowProcW(window, message, wParam, lParam);
    }

    WindowState* state = StateOf(window);
    if (!state) return DefWindowProcW(window, message, wParam, lParam);

    switch (message) {
        case WM_CREATE:
            MeasureFont(window, state);
            RecalcViewport(window, state);
            return 0;

        case WM_SIZE:
            RecalcViewport(window, state);
            state->editor.EnsureCursorVisible(state->visibleLines, state->visibleCols);
            UpdateCaret(window, state);
            InvalidateRect(window, nullptr, FALSE);
            return 0;

        case WM_SETFOCUS:
            CreateCaret(window, nullptr, 2, state->lineHeight);
            UpdateCaret(window, state);
            ShowCaret(window);
            return 0;

        case WM_KILLFOCUS:
            DestroyCaret();
            return 0;

        case WM_ERASEBKGND:
            return 1;   // OnPaint fully covers the client area

        case WM_PAINT:
            OnPaint(window, state);
            return 0;

        case WM_KEYDOWN:
            if (OnKeyDown(window, state, wParam)) return 0;
            break;

        case WM_CHAR:
            if (OnChar(window, state, wParam)) return 0;
            break;

        case WM_MOUSEWHEEL: {
            const int notches = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            state->editor.ScrollBy(-notches * 3, state->visibleLines);
            UpdateCaret(window, state);
            InvalidateRect(window, nullptr, FALSE);
            return 0;
        }

        case WM_CLOSE:
            if (state->editor.IsModified()) {
                const int answer = MessageBoxW(
                    window,
                    L"The buffer has unsaved changes. Close anyway?",
                    QEDITNA_APP_NAME, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
                if (answer != IDYES) return 0;
            }
            DestroyWindow(window);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

} // namespace

int RunEditor(HINSTANCE instance, int showCommand, const std::wstring& fileToOpen) {
    WindowState state;
    RegisterBuiltinCommands(state.commands);

    if (!fileToOpen.empty()) {
        std::wstring error;
        if (!state.editor.LoadFromFile(fileToOpen, &error)) {
            MessageBoxW(nullptr, (L"Cannot open file:\n" + error).c_str(),
                        QEDITNA_APP_NAME, MB_OK | MB_ICONERROR);
            return 1;
        }
    }

    state.font = CreateFontW(config::kFontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN,
                             config::kFontFace);
    if (!state.font) state.font = static_cast<HFONT>(GetStockObject(ANSI_FIXED_FONT));

    WNDCLASSEXW windowClass{};
    windowClass.cbSize        = sizeof(windowClass);
    windowClass.style         = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc   = WindowProc;
    windowClass.hInstance     = instance;
    windowClass.hCursor       = LoadCursorW(nullptr, IDC_IBEAM);
    windowClass.hIcon         = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.lpszClassName = QEDITNA_WINDOW_CLASS;

    if (!RegisterClassExW(&windowClass)) {
        MessageBoxW(nullptr, L"Window class registration failed.",
                    QEDITNA_APP_NAME, MB_OK | MB_ICONERROR);
        return 1;
    }

    const std::wstring title =
        std::wstring(QEDITNA_APP_NAME) + L" " + QEDITNA_VERSION_STRING;

    HWND window = CreateWindowExW(0, QEDITNA_WINDOW_CLASS, title.c_str(),
                                  WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                  900, 600, nullptr, nullptr, instance, &state);
    if (!window) {
        MessageBoxW(nullptr, L"Window creation failed.",
                    QEDITNA_APP_NAME, MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    DeleteObject(state.font);
    return static_cast<int>(message.wParam);
}

} // namespace ui
} // namespace qed
