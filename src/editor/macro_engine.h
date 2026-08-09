// macro_engine.h — macro recording, storage, and .mac file persistence.
//
// A macro is a flat sequence of (command, argument) pairs, identical to what
// CommandEngine::Execute already takes — recording taps the same choke point
// every key/menu action already runs through (see window.cpp's Run/RunFind).
// A step whose command is "macro.play" is itself a recorded macro hotkey
// invocation (playing one macro from inside another), not a CommandEngine
// command — RunMacro() in window.cpp special-cases it instead of dispatching
// it to CommandEngine.
#pragma once

#include <map>
#include <string>
#include <vector>

namespace qed {

struct MacroStep {
    std::wstring command;
    std::wstring argument;
};
using Macro = std::vector<MacroStep>;

class MacroEngine {
public:
    bool IsRecording() const { return recording_; }
    const std::wstring& RecordingHotkeyId() const { return recordingHotkeyId_; }

    // Begins recording into an in-progress buffer for `hotkeyId`. Any
    // previous unfinished recording is discarded.
    void StartRecording(const std::wstring& hotkeyId);

    // Commits the in-progress recording as the macro for its hotkey
    // (overwriting any previous macro on that hotkey), even if it recorded
    // zero steps. No-op if not currently recording.
    void StopRecording();

    // Appends one step to the in-progress recording. No-op if not recording.
    void RecordStep(const std::wstring& command, const std::wstring& argument);

    bool HasMacro(const std::wstring& hotkeyId) const;
    const Macro* GetMacro(const std::wstring& hotkeyId) const;
    void DeleteMacro(const std::wstring& hotkeyId);

    // Writes every stored macro to `path` as a .mac file. Refuses to touch
    // disk under the same write-protect gate as document saves
    // (config::kWriteEnabled) — sets *errorOut and returns false in that
    // case, exactly like io::SaveFile's refusal path.
    bool SaveToFile(const std::wstring& path, std::wstring* errorOut) const;

    // Reads macros from `path`, replacing any in-memory macro with the same
    // hotkey id. Fails (false + *errorOut) if the file does not exist, is
    // not a recognised .mac file, or contains no macros. Reading is always
    // allowed, matching io::LoadFile's read-is-always-allowed rule.
    bool LoadFromFile(const std::wstring& path, std::wstring* errorOut);

private:
    bool                          recording_ = false;
    std::wstring                  recordingHotkeyId_;
    Macro                         recordingBuffer_;
    std::map<std::wstring, Macro> macros_;
};

// Resolves a raw virtual-key code plus modifier state into a canonical macro
// hotkey id ("F2".."F12", "Ctrl+A", "Alt+G", ...) and a matching display
// label (currently identical to the id). Returns false when the combination
// is outside the reserved F2-F12 / Ctrl+<letter-or-digit> / Alt+<letter-or-
// digit> ranges, or collides with a key already bound elsewhere in the app
// (Ctrl+S, Ctrl+Y).
bool ResolveMacroHotkey(unsigned key, bool ctrl, bool alt,
                        std::wstring* idOut, std::wstring* labelOut);

} // namespace qed
