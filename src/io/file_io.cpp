#include "file_io.h"

#include "../config/config.h"

#include <windows.h>

namespace qed {
namespace io {
namespace {

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

} // namespace

LoadResult LoadFile(const std::wstring& path) {
    LoadResult result;

    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        const DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
            result.ok = true;              // treat as a brand new, empty file
            result.lines.emplace_back();
            return result;
        }
        result.error = LastErrorText(err);
        return result;
    }

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size)) {
        result.error = LastErrorText(GetLastError());
        CloseHandle(file);
        return result;
    }
    if (size.QuadPart > 0x7FFFFFFF) {
        result.error = L"File is larger than 2 GB.";
        CloseHandle(file);
        return result;
    }

    std::string raw(static_cast<size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    if (size.QuadPart > 0 &&
        !ReadFile(file, &raw[0], static_cast<DWORD>(raw.size()), &read, nullptr)) {
        result.error = LastErrorText(GetLastError());
        CloseHandle(file);
        return result;
    }
    CloseHandle(file);
    raw.resize(read);
    result.byteSize = read;

    size_t offset = 0;
    if (raw.size() >= 3 && static_cast<unsigned char>(raw[0]) == 0xEF &&
        static_cast<unsigned char>(raw[1]) == 0xBB &&
        static_cast<unsigned char>(raw[2]) == 0xBF) {
        offset = 3;
        result.hadBom = true;
    }

    const std::wstring text =
        Utf8ToWide(raw.data() + offset, static_cast<int>(raw.size() - offset));

    result.lineEnding = (text.find(L"\r\n") != std::wstring::npos) ? LineEnding::CRLF
                                                                  : LineEnding::LF;

    std::wstring current;
    for (wchar_t ch : text) {
        if (ch == L'\r') continue;         // normalise CRLF -> LF in the buffer
        if (ch == L'\n') {
            result.lines.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    result.lines.push_back(current);

    result.ok = true;
    return result;
}

SaveResult SaveFile(const std::wstring& path,
                    const std::vector<std::wstring>& lines,
                    LineEnding lineEnding,
                    bool writeBom) {
    SaveResult result;

    // Hard gate: in a write-protected build we never touch the disk.
    if (!config::kWriteEnabled) {
        result.refusedByWriteProtect = true;
        result.error = L"Write protection is active (build without QEDITNA_ENABLE_WRITE).";
        return result;
    }

    if (config::kBackupOnWrite &&
        GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
        const std::wstring backup = path + L".bak";
        if (!CopyFileW(path.c_str(), backup.c_str(), FALSE)) {
            // Backup failure aborts the write: the existing file stays intact.
            result.error = L"Backup failed: " + LastErrorText(GetLastError());
            return result;
        }
    }

    std::string payload;
    if (writeBom) payload.append("\xEF\xBB\xBF");
    const char* eol = (lineEnding == LineEnding::CRLF) ? "\r\n" : "\n";
    for (size_t i = 0; i < lines.size(); ++i) {
        payload += WideToUtf8(lines[i]);
        if (i + 1 < lines.size()) payload += eol;
    }

    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        result.error = LastErrorText(GetLastError());
        return result;
    }
    DWORD written = 0;
    const BOOL ok = payload.empty() ||
                    WriteFile(file, payload.data(),
                              static_cast<DWORD>(payload.size()), &written, nullptr);
    if (!ok) result.error = LastErrorText(GetLastError());
    CloseHandle(file);

    result.ok = ok && written == payload.size();
    return result;
}

} // namespace io
} // namespace qed
