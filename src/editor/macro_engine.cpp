#include "macro_engine.h"

#include "../config/config.h"

#include <windows.h>

namespace qed {
namespace {

const wchar_t kMacFileHeader[] = L"QEDITNA-MACRO v1";

std::wstring LastErrorText(DWORD code) {
    LPWSTR buffer = nullptr;
    const DWORD len = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    std::wstring text = (len && buffer) ? std::wstring(buffer, len) : L"unknown error";
    if (buffer) LocalFree(buffer);
    while (!text.empty() && (text.back() == L'\n' || text.back() == L'\r')) text.pop_back();
    return text;
}

std::wstring Utf8ToWide(const char* data, int bytes) {
    if (bytes <= 0) return std::wstring();
    const int need = MultiByteToWideChar(CP_UTF8, 0, data, bytes, nullptr, 0);
    if (need <= 0) return std::wstring();
    std::wstring out(static_cast<size_t>(need), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, data, bytes, &out[0], need);
    return out;
}

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) return std::string();
    const int need = WideCharToMultiByte(CP_UTF8, 0, text.c_str(),
                                         static_cast<int>(text.size()),
                                         nullptr, 0, nullptr, nullptr);
    if (need <= 0) return std::string();
    std::string out(static_cast<size_t>(need), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                        &out[0], need, nullptr, nullptr);
    return out;
}

// One step per line: "command\targument". A line with no tab has an empty
// argument. Sections start with "[hotkeyId]".
std::wstring SerializeMacros(const std::map<std::wstring, Macro>& macros) {
    std::wstring out = kMacFileHeader;
    out += L"\r\n";
    for (const auto& entry : macros) {
        out += L"[" + entry.first + L"]\r\n";
        for (const auto& step : entry.second) {
            out += step.command + L"\t" + step.argument + L"\r\n";
        }
    }
    return out;
}

bool ParseMacros(const std::wstring& text, std::map<std::wstring, Macro>* outMacros) {
    std::vector<std::wstring> lines;
    std::wstring current;
    for (wchar_t ch : text) {
        if (ch == L'\r') continue;
        if (ch == L'\n') {
            lines.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) lines.push_back(current);

    if (lines.empty() || lines.front() != kMacFileHeader) return false;

    std::map<std::wstring, Macro> macros;
    std::wstring currentId;
    bool haveSection = false;
    for (size_t i = 1; i < lines.size(); ++i) {
        const std::wstring& line = lines[i];
        if (line.empty()) continue;
        if (line.front() == L'[' && line.back() == L']') {
            currentId = line.substr(1, line.size() - 2);
            macros[currentId];   // ensure the section exists even with zero steps
            haveSection = true;
            continue;
        }
        if (!haveSection) continue;   // stray line before any section header
        const size_t tab = line.find(L'\t');
        MacroStep step;
        if (tab == std::wstring::npos) {
            step.command = line;
        } else {
            step.command  = line.substr(0, tab);
            step.argument = line.substr(tab + 1);
        }
        macros[currentId].push_back(step);
    }

    if (macros.empty()) return false;
    *outMacros = std::move(macros);
    return true;
}

} // namespace

void MacroEngine::StartRecording(const std::wstring& hotkeyId) {
    recording_ = true;
    recordingHotkeyId_ = hotkeyId;
    recordingBuffer_.clear();
}

void MacroEngine::StopRecording() {
    if (!recording_) return;
    macros_[recordingHotkeyId_] = std::move(recordingBuffer_);
    recording_ = false;
    recordingHotkeyId_.clear();
    recordingBuffer_.clear();
}

void MacroEngine::RecordStep(const std::wstring& command, const std::wstring& argument) {
    if (!recording_) return;
    recordingBuffer_.push_back(MacroStep{command, argument});
}

bool MacroEngine::HasMacro(const std::wstring& hotkeyId) const {
    return macros_.find(hotkeyId) != macros_.end();
}

const Macro* MacroEngine::GetMacro(const std::wstring& hotkeyId) const {
    auto it = macros_.find(hotkeyId);
    return it == macros_.end() ? nullptr : &it->second;
}

void MacroEngine::DeleteMacro(const std::wstring& hotkeyId) {
    macros_.erase(hotkeyId);
}

bool MacroEngine::SaveToFile(const std::wstring& path, std::wstring* errorOut) const {
    if (!config::kWriteEnabled) {
        if (errorOut) *errorOut = L"Write protection is active (build without QEDITNA_ENABLE_WRITE).";
        return false;
    }
    if (macros_.empty()) {
        if (errorOut) *errorOut = L"No macros to save.";
        return false;
    }

    const std::string payload = WideToUtf8(SerializeMacros(macros_));

    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        if (errorOut) *errorOut = LastErrorText(GetLastError());
        return false;
    }
    DWORD written = 0;
    const BOOL ok = payload.empty() ||
                    WriteFile(file, payload.data(), static_cast<DWORD>(payload.size()),
                             &written, nullptr);
    if (!ok && errorOut) *errorOut = LastErrorText(GetLastError());
    CloseHandle(file);

    return ok && written == payload.size();
}

bool MacroEngine::LoadFromFile(const std::wstring& path, std::wstring* errorOut) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        if (errorOut) *errorOut = LastErrorText(GetLastError());
        return false;
    }

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart > 0x7FFFFFFF) {
        if (errorOut) *errorOut = L"Cannot read macro file.";
        CloseHandle(file);
        return false;
    }

    std::string raw(static_cast<size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    if (size.QuadPart > 0 &&
        !ReadFile(file, &raw[0], static_cast<DWORD>(raw.size()), &read, nullptr)) {
        if (errorOut) *errorOut = LastErrorText(GetLastError());
        CloseHandle(file);
        return false;
    }
    CloseHandle(file);
    raw.resize(read);

    size_t offset = 0;
    if (raw.size() >= 3 && static_cast<unsigned char>(raw[0]) == 0xEF &&
        static_cast<unsigned char>(raw[1]) == 0xBB &&
        static_cast<unsigned char>(raw[2]) == 0xBF) {
        offset = 3;
    }
    const std::wstring text = Utf8ToWide(raw.data() + offset, static_cast<int>(raw.size() - offset));

    std::map<std::wstring, Macro> parsed;
    if (!ParseMacros(text, &parsed)) {
        if (errorOut) *errorOut = L"Not a valid QEditNA macro file, or it contains no macros.";
        return false;
    }

    for (auto& entry : parsed) macros_[entry.first] = std::move(entry.second);
    return true;
}

bool ResolveMacroHotkey(unsigned key, bool ctrl, bool alt,
                        std::wstring* idOut, std::wstring* labelOut) {
    if (ctrl && alt) return false;

    if (!ctrl && !alt) {
        if (key >= VK_F2 && key <= VK_F12) {
            const std::wstring label = L"F" + std::to_wstring(key - VK_F1 + 1);
            if (idOut) *idOut = label;
            if (labelOut) *labelOut = label;
            return true;
        }
        return false;
    }

    const bool isLetter = key >= 'A' && key <= 'Z';
    const bool isDigit  = key >= '0' && key <= '9';
    if (!isLetter && !isDigit) return false;

    const wchar_t letter = static_cast<wchar_t>(key);
    if (ctrl && (letter == L'S' || letter == L'Y')) return false;   // already bound

    const std::wstring id = (ctrl ? L"Ctrl+" : L"Alt+") + std::wstring(1, letter);
    if (idOut) *idOut = id;
    if (labelOut) *labelOut = id;
    return true;
}

} // namespace qed
