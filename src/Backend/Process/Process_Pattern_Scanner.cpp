#include "Process_Pattern_Scanner.hpp"

namespace Devilz::Backend
{
Result<std::vector<Process_Pattern_Match>> Process_Pattern_Scanner::Scan(
    const Process_Module_Info& module,
    const Process_Pattern& pattern,
    std::size_t maxMatches) const
{
    if (!module.Valid() || pattern.Empty() || pattern.Size() > module.imageSize || maxMatches == 0)
        return Result<std::vector<Process_Pattern_Match>>::Failure(
            Error(ErrorCode::InvalidArgument, ErrorCategory::Runtime, "Invalid external pattern scan request"));

    auto image = m_reader.Read(module.baseAddress, module.imageSize);
    if (!image)
        return Result<std::vector<Process_Pattern_Match>>::Failure(image.Failure());

    const auto& bytes = image.Value();
    const auto& signature = pattern.Bytes();
    std::vector<Process_Pattern_Match> matches;

    const std::size_t last = bytes.size() - signature.size();
    for (std::size_t offset = 0; offset <= last && matches.size() < maxMatches; ++offset) {
        bool matched = true;
        for (std::size_t i = 0; i < signature.size(); ++i) {
            if (signature[i].wildcard)
                continue;
            if (std::to_integer<std::uint8_t>(bytes[offset + i]) != signature[i].value) {
                matched = false;
                break;
            }
        }
        if (matched)
            matches.push_back({module.baseAddress + offset, offset});
    }

    return Result<std::vector<Process_Pattern_Match>>::Success(std::move(matches));
}
}
