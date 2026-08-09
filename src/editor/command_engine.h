// command_engine.h — command registry and dispatch
//
// Every user action goes through a named command. This is the single hook the
// macro engine will later tap into for recording and playback.
#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace qed {

class EditorCore;
class MacroEngine;

struct CommandContext {
    EditorCore*  editor       = nullptr;
    MacroEngine* macros       = nullptr;   // needed by macro.new/delete/save/load
    size_t       visibleLines = 1;   // current viewport height, for page moves
    size_t       visibleCols  = 1;
    std::wstring argument;           // optional textual argument (e.g. goto line)
    std::wstring message;            // filled by the command, shown in status bar
};

using CommandHandler = std::function<bool(CommandContext&)>;

struct Command {
    std::wstring   name;
    std::wstring   description;
    CommandHandler handler;
};

class CommandEngine {
public:
    void Register(std::wstring name, std::wstring description, CommandHandler handler);

    // Returns false when the command is unknown or its handler failed.
    bool Execute(const std::wstring& name, CommandContext& context);

    bool Has(const std::wstring& name) const;
    std::vector<std::wstring> Names() const;

private:
    std::map<std::wstring, Command> commands_;
};

// Populates the engine with the built-in v0.1 command set.
void RegisterBuiltinCommands(CommandEngine& engine);

} // namespace qed
