#include "Pattern_Cache_Persistence.hpp"

#include <charconv>
#include <sstream>

namespace Devilz::Backend
{
namespace
{
std::uint64_t Fingerprint(const Module_Fingerprint& fp) noexcept
{
    return fp.imageHash ^ (static_cast<std::uint64_t>(fp.timeDateStamp) << 32) ^ fp.sizeOfImage;
}
}

Result<void> Pattern_Cache_Persistence::Save(const Pattern_Cache& cache, const std::filesystem::path& path) const
{
    if (!m_files)
        return Result<void>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Filesystem, "Pattern cache persistence has no filesystem manager"));

    std::ostringstream out;
    out << "DDPC1\n";
    for (const auto& entry : cache.Snapshot()) {
        out << entry.key.module << '\t'
            << entry.key.name << '\t'
            << entry.key.moduleFingerprint << '\t'
            << entry.relativeOffset << '\n';
    }
    return m_files->WriteText(path, out.str(), true);
}

Result<std::size_t> Pattern_Cache_Persistence::LoadForModule(Pattern_Cache& cache,
                                                             const Module_Info& module,
                                                             const std::filesystem::path& path) const
{
    if (!m_files)
        return Result<std::size_t>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Filesystem, "Pattern cache persistence has no filesystem manager"));

    auto exists = m_files->Exists(path);
    if (!exists) return Result<std::size_t>::Failure(exists.Failure());
    if (!exists.Value()) return Result<std::size_t>::Success(0);

    auto text = m_files->ReadText(path);
    if (!text) return Result<std::size_t>::Failure(text.Failure());

    std::istringstream input(text.Value());
    std::string line;
    if (!std::getline(input, line) || line != "DDPC1")
        return Result<std::size_t>::Failure(Error(ErrorCode::FileReadFailed, ErrorCategory::Filesystem, "Unsupported pattern cache format").With("Path", path.string()));

    const auto validFingerprint = Fingerprint(module.fingerprint);
    std::size_t loaded = 0;
    while (std::getline(input, line)) {
        const auto a = line.find('\t');
        const auto b = a == std::string::npos ? a : line.find('\t', a + 1);
        const auto c = b == std::string::npos ? b : line.find('\t', b + 1);
        if (a == std::string::npos || b == std::string::npos || c == std::string::npos) continue;

        const std::string moduleName = line.substr(0, a);
        if (moduleName != module.name) continue;
        const std::string name = line.substr(a + 1, b - a - 1);
        std::uint64_t fingerprint = 0;
        std::size_t offset = 0;
        const auto fpText = std::string_view(line).substr(b + 1, c - b - 1);
        const auto offText = std::string_view(line).substr(c + 1);
        if (std::from_chars(fpText.data(), fpText.data() + fpText.size(), fingerprint).ec != std::errc{} ||
            std::from_chars(offText.data(), offText.data() + offText.size(), offset).ec != std::errc{}) continue;
        if (fingerprint != validFingerprint || offset >= module.size) continue;

        cache.Store({module.name, name, fingerprint}, Pointer(module.base + offset), module.base);
        ++loaded;
    }
    return Result<std::size_t>::Success(loaded);
}
}
