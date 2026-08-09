#include "command_engine.h"

#include "editor_core.h"
#include "macro_engine.h"

namespace qed {

void CommandEngine::Register(std::wstring name, std::wstring description,
                             CommandHandler handler) {
    Command command;
    command.name        = name;
    command.description = std::move(description);
    command.handler     = std::move(handler);
    commands_[std::move(name)] = std::move(command);
}

bool CommandEngine::Execute(const std::wstring& name, CommandContext& context) {
    auto it = commands_.find(name);
    if (it == commands_.end()) {
        context.message = L"Unknown command: " + name;
        return false;
    }
    if (!context.editor) {
        context.message = L"No editor in context.";
        return false;
    }
    return it->second.handler(context);
}

bool CommandEngine::Has(const std::wstring& name) const {
    return commands_.find(name) != commands_.end();
}

std::vector<std::wstring> CommandEngine::Names() const {
    std::vector<std::wstring> names;
    names.reserve(commands_.size());
    for (const auto& entry : commands_) names.push_back(entry.first);
    return names;
}

void RegisterBuiltinCommands(CommandEngine& engine) {
    auto simple = [&engine](const wchar_t* name, const wchar_t* description,
                            void (EditorCore::*method)()) {
        engine.Register(name, description, [method](CommandContext& ctx) {
            (ctx.editor->*method)();
            return true;
        });
    };

    simple(L"cursor.left",       L"Move cursor one character left",  &EditorCore::MoveLeft);
    simple(L"cursor.right",      L"Move cursor one character right", &EditorCore::MoveRight);
    simple(L"cursor.up",         L"Move cursor one line up",         &EditorCore::MoveUp);
    simple(L"cursor.down",       L"Move cursor one line down",       &EditorCore::MoveDown);
    simple(L"cursor.line-start", L"Move to start of line",           &EditorCore::MoveLineStart);
    simple(L"cursor.line-end",   L"Move to end of line",             &EditorCore::MoveLineEnd);
    simple(L"cursor.file-start", L"Move to start of file",           &EditorCore::MoveFileStart);
    simple(L"cursor.file-end",   L"Move to end of file",             &EditorCore::MoveFileEnd);

    engine.Register(L"cursor.page-up", L"Move one page up", [](CommandContext& ctx) {
        ctx.editor->MovePage(-1, ctx.visibleLines);
        return true;
    });
    engine.Register(L"cursor.page-down", L"Move one page down", [](CommandContext& ctx) {
        ctx.editor->MovePage(1, ctx.visibleLines);
        return true;
    });

    engine.Register(L"edit.newline", L"Insert a line break", [](CommandContext& ctx) {
        ctx.editor->InsertNewline();
        return true;
    });
    engine.Register(L"edit.insert-char", L"Insert a character at the cursor (argument)",
                    [](CommandContext& ctx) {
                        if (ctx.argument.empty()) return false;
                        ctx.editor->InsertChar(ctx.argument[0]);
                        return true;
                    });
    engine.Register(L"edit.delete-back", L"Delete character before cursor",
                    [](CommandContext& ctx) {
                        ctx.editor->DeleteBack();
                        return true;
                    });
    engine.Register(L"edit.delete-forward", L"Delete character under cursor",
                    [](CommandContext& ctx) {
                        ctx.editor->DeleteForward();
                        return true;
                    });
    engine.Register(L"edit.delete-line", L"Delete the current line",
                    [](CommandContext& ctx) {
                        ctx.editor->DeleteLine();
                        return true;
                    });

    engine.Register(L"find.search", L"Search for a term (argument)", [](CommandContext& ctx) {
        std::wstring message;
        const bool ok = ctx.editor->FindFirst(ctx.argument, &message);
        ctx.message = message;
        return ok;
    });
    engine.Register(L"find.next", L"Find next occurrence of the last search term",
                    [](CommandContext& ctx) {
                        std::wstring message;
                        const bool ok = ctx.editor->FindNext(&message);
                        ctx.message = message;
                        return ok;
                    });
    engine.Register(L"find.previous", L"Find previous occurrence of the last search term",
                    [](CommandContext& ctx) {
                        std::wstring message;
                        const bool ok = ctx.editor->FindPrevious(&message);
                        ctx.message = message;
                        return ok;
                    });

    engine.Register(L"block.mark-line-start", L"Start marking a line block at the cursor",
                    [](CommandContext& ctx) {
                        ctx.editor->BlockMarkStart(BlockMode::Line);
                        ctx.message = L"Označování řádkového bloku spuštěno.";
                        return true;
                    });
    engine.Register(L"block.mark-line-end", L"Lock the line block being marked",
                    [](CommandContext& ctx) {
                        if (!ctx.editor->HasSelection()) {
                            ctx.message = L"Není spuštěno žádné označování.";
                            return false;
                        }
                        ctx.editor->BlockMarkEnd();
                        ctx.message = L"Řádkový blok označen.";
                        return true;
                    });
    engine.Register(L"block.mark-column-start", L"Start marking a column block at the cursor",
                    [](CommandContext& ctx) {
                        ctx.editor->BlockMarkStart(BlockMode::Column);
                        ctx.message = L"Označování sloupcového bloku spuštěno.";
                        return true;
                    });
    engine.Register(L"block.mark-column-end", L"Lock the column block being marked",
                    [](CommandContext& ctx) {
                        if (!ctx.editor->HasSelection()) {
                            ctx.message = L"Není spuštěno žádné označování.";
                            return false;
                        }
                        ctx.editor->BlockMarkEnd();
                        ctx.message = L"Sloupcový blok označen.";
                        return true;
                    });
    engine.Register(L"block.copy", L"Copy the marked block to the cursor",
                    [](CommandContext& ctx) {
                        std::wstring message;
                        const bool ok = ctx.editor->BlockCopy(&message);
                        ctx.message = message;
                        return ok;
                    });
    engine.Register(L"block.move", L"Move the marked block to the cursor",
                    [](CommandContext& ctx) {
                        std::wstring message;
                        const bool ok = ctx.editor->BlockMove(&message);
                        ctx.message = message;
                        return ok;
                    });
    engine.Register(L"block.delete", L"Delete the marked block's content",
                    [](CommandContext& ctx) {
                        std::wstring message;
                        const bool ok = ctx.editor->BlockDelete(&message);
                        ctx.message = message;
                        return ok;
                    });
    engine.Register(L"block.cancel", L"Cancel the current block selection",
                    [](CommandContext& ctx) {
                        if (!ctx.editor->HasSelection()) {
                            ctx.message = L"Není označen žádný blok.";
                            return false;
                        }
                        ctx.editor->BlockCancel();
                        ctx.message = L"Označení bloku zrušeno.";
                        return true;
                    });

    engine.Register(L"file.save", L"Write the buffer to disk", [](CommandContext& ctx) {
        std::wstring message;
        const bool ok = ctx.editor->SaveToFile(&message);
        ctx.message = message;
        return ok;
    });

    // Macro management as ordinary commands (argument = hotkey id or
    // filename), so a macro step can drive them — typically via the
    // <SHORTKEY>/<TEXT> placeholder that pauses playback to ask the user for
    // the argument (see window.cpp's ContinuePlayback). The F1 menu still
    // reaches these interactively through its own multi-step flow instead of
    // through CommandEngine, since it needs to prompt before it has an
    // argument at all.
    engine.Register(L"macro.new", L"Start recording onto a hotkey (argument: hotkey id)",
                    [](CommandContext& ctx) {
                        if (!ctx.macros || ctx.argument.empty()) return false;
                        if (IsReservedHotkeyId(ctx.argument)) {
                            ctx.message = ctx.argument + L" je pevně přiřazená klávesa, makro na ni nelze uložit.";
                            return false;
                        }
                        ctx.macros->StartRecording(ctx.argument);
                        ctx.message = L"Nahrávání makra (" + ctx.argument + L") spuštěno.";
                        return true;
                    });
    engine.Register(L"macro.stop-recording", L"Stop the current recording, if any",
                    [](CommandContext& ctx) {
                        if (!ctx.macros) return false;
                        if (!ctx.macros->IsRecording()) {
                            ctx.message = L"Žádné nahrávání neprobíhá.";
                            return false;
                        }
                        ctx.macros->StopRecording();
                        ctx.message = L"Nahrávání makra ukončeno.";
                        return true;
                    });
    engine.Register(L"macro.delete", L"Delete the macro on a hotkey (argument: hotkey id)",
                    [](CommandContext& ctx) {
                        if (!ctx.macros || ctx.argument.empty()) return false;
                        if (!ctx.macros->HasMacro(ctx.argument)) {
                            ctx.message = L"Žádné makro pro " + ctx.argument + L".";
                            return false;
                        }
                        ctx.macros->DeleteMacro(ctx.argument);
                        ctx.message = L"Makro " + ctx.argument + L" smazáno.";
                        return true;
                    });
    engine.Register(L"macro.save", L"Save all macros to a file (argument: path)",
                    [](CommandContext& ctx) {
                        if (!ctx.macros || ctx.argument.empty()) return false;
                        std::wstring error;
                        const bool ok = ctx.macros->SaveToFile(ctx.argument, &error);
                        ctx.message = ok ? (L"Makra uložena do " + ctx.argument) : error;
                        return ok;
                    });
    engine.Register(L"macro.load", L"Load macros from a file (argument: path)",
                    [](CommandContext& ctx) {
                        if (!ctx.macros || ctx.argument.empty()) return false;
                        std::wstring error;
                        const bool ok = ctx.macros->LoadFromFile(ctx.argument, &error);
                        ctx.message = ok ? (L"Makra načtena ze souboru " + ctx.argument) : error;
                        return ok;
                    });
}

} // namespace qed
