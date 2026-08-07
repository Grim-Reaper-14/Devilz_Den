#pragma once

#include "LogRecord.hpp"

namespace Devilz::Backend
{
class ILogSink
{
public:
    virtual ~ILogSink() = default;
    virtual void Write(const LogRecord& record) = 0;
    virtual void Flush() = 0;
};
}
