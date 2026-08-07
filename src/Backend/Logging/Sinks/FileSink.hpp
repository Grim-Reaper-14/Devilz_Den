#pragma once

#include "Backend/Logging/ILogSink.hpp"

#include <filesystem>
#include <fstream>
#include <mutex>

namespace Devilz::Backend
{
class FileSink final : public ILogSink
{
public:
    explicit FileSink(const std::filesystem::path& path);
    void Write(const LogRecord& record) override;
    void Flush() override;

private:
    std::mutex m_mutex;
    std::ofstream m_stream;
};
}
