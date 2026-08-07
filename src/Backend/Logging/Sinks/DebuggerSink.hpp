#pragma once

#include "Backend/Logging/ILogSink.hpp"

namespace Devilz::Backend
{
class DebuggerSink final : public ILogSink
{
public:
    void Write(const LogRecord& record) override;
    void Flush() override {}
};
}
