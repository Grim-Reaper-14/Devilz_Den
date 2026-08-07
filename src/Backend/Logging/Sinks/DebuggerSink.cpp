#include "DebuggerSink.hpp"

#include <Windows.h>

#include <sstream>

namespace Devilz::Backend
{
void DebuggerSink::Write(const LogRecord& record)
{
    std::ostringstream out;
    out << '[' << record.sequence << "] [" << record.service << "] " << record.message;
    if (record.error) out << " | " << record.error->DetailedDescription();
    out << '\n';
    ::OutputDebugStringA(out.str().c_str());
}
}
