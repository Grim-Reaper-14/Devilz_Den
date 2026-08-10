#include "GTA_Build_Target_Coordinator.hpp"

#include <algorithm>

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

std::size_t GTA_Build_Target_Report::LocatedCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(targets.begin(), targets.end(), [](const auto& target) {
        return target.status.state == GTA_Runtime_Target_State::Located ||
               target.status.state == GTA_Runtime_Target_State::StructurallyValidated ||
               target.status.state == GTA_Runtime_Target_State::Validated;
    }));
}

std::size_t GTA_Build_Target_Report::StructurallyValidatedCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(targets.begin(), targets.end(), [](const auto& target) {
        return target.status.state == GTA_Runtime_Target_State::StructurallyValidated;
    }));
}

std::size_t GTA_Build_Target_Report::ValidatedCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(targets.begin(), targets.end(), [](const auto& target) {
        return target.status.state == GTA_Runtime_Target_State::Validated;
    }));
}

Result<GTA_Build_Target_Report> GTA_Build_Target_Coordinator::Resolve(
    std::uint32_t pid,
    std::uint64_t fingerprint) const
{
    if (pid == 0 || fingerprint == 0)
        return Result<GTA_Build_Target_Report>::Failure(
            Error(ErrorCode::InvalidArgument, ErrorCategory::Runtime, "Invalid GTA build target resolution request"));

    auto set = m_registry.Find(fingerprint);
    if (!set)
        return Result<GTA_Build_Target_Report>::Failure(
            Error(ErrorCode::NotFound, ErrorCategory::Runtime, "No target set is registered for this GTA build"));

    GTA_Build_Target_Report report{};
    report.fingerprint = fingerprint;
    report.targets.reserve(set->targets.size());

    GTA_Target_Resolver resolver(pid, fingerprint);
    for (const auto& definition : set->targets) {
        auto resolution = resolver.Resolve(definition);
        if (!resolution)
            return Result<GTA_Build_Target_Report>::Failure(resolution.Failure());
        report.targets.push_back(std::move(resolution.Value()));
    }

    return Result<GTA_Build_Target_Report>::Success(std::move(report));
}
}
