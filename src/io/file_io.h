// file_io.h — file reading/writing with the write-protect guard
#pragma once

#include <string>
#include <vector>

namespace qed {
namespace io {

enum class LineEnding { CRLF, LF };

struct LoadResult {
    bool                      ok = false;
    std::wstring              error;          // human readable, empty when ok
    std::vector<std::wstring> lines;
    LineEnding                lineEnding = LineEnding::CRLF;
    bool                      hadBom = false;
    unsigned long long        byteSize = 0;
};

// Reads a UTF-8 (with or without BOM) file and splits it into lines.
// A non-existent path yields ok == true with a single empty line (new file).
LoadResult LoadFile(const std::wstring& path);

// Writes lines back as UTF-8. Honours qed::config::kWriteEnabled — when write
// protection is on this performs no disk access at all and reports refusal.
struct SaveResult {
    bool         ok = false;
    bool         refusedByWriteProtect = false;
    std::wstring error;
};

SaveResult SaveFile(const std::wstring& path,
                    const std::vector<std::wstring>& lines,
                    LineEnding lineEnding,
                    bool writeBom);

} // namespace io
} // namespace qed
