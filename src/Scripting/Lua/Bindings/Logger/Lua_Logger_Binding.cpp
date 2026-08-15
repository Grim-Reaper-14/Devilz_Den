#include "Lua_Logger_Binding.hpp"

#include "Scripting/Lua/Bindings/Lua_Binding_Context.hpp"
#include "Scripting/Lua/Lua_Binding_Library.hpp"
#include "Scripting/Lua/Lua_Engine.hpp"

#include <sol/sol.hpp>

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

namespace Devilz::Scripting::Lua::Bindings::Logger
{
namespace
{
class Logger_Library final : public Lua_Binding_Library
{
public:
    [[nodiscard]] std::string_view Name() const noexcept override
    {
        return "logger";
    }

    bool Register(Lua_Engine& engine, const Lua_Binding_Context& context) override
    {
        return RegisterLogger(engine, context);
    }
};

std::string MakeServiceName(
    std::uint64_t ownerScriptId,
    const sol::optional<std::string>& channel)
{
    std::string service = ownerScriptId == 0
        ? "Lua"
        : "Lua.Script." + std::to_string(ownerScriptId);

    if (channel && !channel->empty()) {
        service.push_back('.');
        service += *channel;
    }

    return service;
}

bool EmitLog(
    const Lua_Log_Callback& logger,
    Backend::LogLevel level,
    std::uint64_t ownerScriptId,
    std::string message,
    const sol::optional<std::string>& channel)
{
    if (!logger)
        return false;

    logger(level, std::move(message), MakeServiceName(ownerScriptId, channel));
    return true;
}

bool ParseLevel(std::string level, Backend::LogLevel& result)
{
    std::transform(
        level.begin(),
        level.end(),
        level.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });

    if (level == "trace") result = Backend::LogLevel::Trace;
    else if (level == "debug") result = Backend::LogLevel::Debug;
    else if (level == "info") result = Backend::LogLevel::Info;
    else if (level == "notice") result = Backend::LogLevel::Notice;
    else if (level == "warning" || level == "warn") result = Backend::LogLevel::Warning;
    else if (level == "error") result = Backend::LogLevel::Error;
    else if (level == "critical") result = Backend::LogLevel::Critical;
    else if (level == "fatal") result = Backend::LogLevel::Fatal;
    else return false;

    return true;
}
}

bool RegisterLogger(Lua_Engine& engine, const Lua_Binding_Context& context)
{
    if (!engine.Ready())
        return false;

    auto& state = engine.State();
    const sol::object devilzObject = state["devilz"];
    if (!devilzObject.is<sol::table>())
        return false;

    auto devilz = devilzObject.as<sol::table>();
    auto log = state.create_table();
    const auto ownerScriptId = engine.OwnerScriptId();
    const auto logger = context.logger;

    log["available"] = static_cast<bool>(logger);

    log.set_function(
        "write",
        [logger, ownerScriptId](
            std::string level,
            std::string message,
            sol::optional<std::string> channel) {
            Backend::LogLevel parsed{};
            if (!ParseLevel(std::move(level), parsed))
                return false;
            return EmitLog(logger, parsed, ownerScriptId, std::move(message), channel);
        });

    log.set_function(
        "trace",
        [logger, ownerScriptId](std::string message, sol::optional<std::string> channel) {
            return EmitLog(logger, Backend::LogLevel::Trace, ownerScriptId, std::move(message), channel);
        });
    log.set_function(
        "debug",
        [logger, ownerScriptId](std::string message, sol::optional<std::string> channel) {
            return EmitLog(logger, Backend::LogLevel::Debug, ownerScriptId, std::move(message), channel);
        });
    log.set_function(
        "info",
        [logger, ownerScriptId](std::string message, sol::optional<std::string> channel) {
            return EmitLog(logger, Backend::LogLevel::Info, ownerScriptId, std::move(message), channel);
        });
    log.set_function(
        "notice",
        [logger, ownerScriptId](std::string message, sol::optional<std::string> channel) {
            return EmitLog(logger, Backend::LogLevel::Notice, ownerScriptId, std::move(message), channel);
        });
    log.set_function(
        "warning",
        [logger, ownerScriptId](std::string message, sol::optional<std::string> channel) {
            return EmitLog(logger, Backend::LogLevel::Warning, ownerScriptId, std::move(message), channel);
        });
    log.set_function(
        "error",
        [logger, ownerScriptId](std::string message, sol::optional<std::string> channel) {
            return EmitLog(logger, Backend::LogLevel::Error, ownerScriptId, std::move(message), channel);
        });
    log.set_function(
        "critical",
        [logger, ownerScriptId](std::string message, sol::optional<std::string> channel) {
            return EmitLog(logger, Backend::LogLevel::Critical, ownerScriptId, std::move(message), channel);
        });
    log.set_function(
        "fatal",
        [logger, ownerScriptId](std::string message, sol::optional<std::string> channel) {
            return EmitLog(logger, Backend::LogLevel::Fatal, ownerScriptId, std::move(message), channel);
        });

    devilz["log"] = log;
    devilz["logger"] = log;
    return true;
}

std::unique_ptr<Lua_Binding_Library> CreateLoggerLibrary()
{
    return std::make_unique<Logger_Library>();
}
}
