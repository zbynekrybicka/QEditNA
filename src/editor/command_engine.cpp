#include "command_engine.h"

#include "editor_core.h"

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

    engine.Register(L"file.save", L"Write the buffer to disk", [](CommandContext& ctx) {
        std::wstring message;
        const bool ok = ctx.editor->SaveToFile(&message);
        ctx.message = message;
        return ok;
    });
}

} // namespace qed
