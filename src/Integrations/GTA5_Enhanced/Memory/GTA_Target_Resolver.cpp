#include "GTA_Target_Resolver.hpp"

#include "Backend/Process/Process_Memory_Reader.hpp"
#include "GTA_Address_Resolver.hpp"

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

Result<GTA_Target_Resolution> GTA_Target_Resolver::Resolve(const GTA_Target_Definition& definition) const
{
    GTA_Target_Resolution output{};
    output.status.id = definition.id;
    output.status.name = definition.name;
    output.status.required = definition.required;
    output.candidateKind = definition.candidateKind;

    if (m_pid == 0 || definition.pattern.empty()) {
        output.status.state = GTA_Runtime_Target_State::Unknown;
        output.status.detail = "No verified pattern is registered for this target";
        return Result<GTA_Target_Resolution>::Success(std::move(output));
    }

    auto pattern = Process_Pattern::Parse(definition.pattern);
    if (!pattern) {
        output.status.state = GTA_Runtime_Target_State::Failed;
        output.status.detail = "Registered target pattern is invalid";
        return Result<GTA_Target_Resolution>::Success(std::move(output));
    }

    Process_Module_Manager modules(m_pid);
    auto module = modules.Find(definition.module);
    if (!module) {
        output.status.state = GTA_Runtime_Target_State::Failed;
        output.status.detail = "Target module could not be resolved in the GTA process";
        return Result<GTA_Target_Resolution>::Success(std::move(output));
    }

    Process_Pattern_Scanner scanner(m_pid);
    auto matches = scanner.Scan(module.Value(), *pattern, 8);
    if (!matches)
        return Result<GTA_Target_Resolution>::Failure(matches.Failure());

    if (matches.Value().empty()) {
        output.status.state = GTA_Runtime_Target_State::Missing;
        output.status.detail = "Verified pattern produced no matches in the target module";
        return Result<GTA_Target_Resolution>::Success(std::move(output));
    }

    Process_Memory_Reader reader(m_pid);
    for (const auto& match : matches.Value()) {
        auto address = GTA_Address_Resolver::Resolve(reader, match.address, definition.resolve);
        if (address && address.Value() != 0)
            output.candidates.push_back(address.Value());
    }

    if (output.candidates.empty()) {
        output.status.state = GTA_Runtime_Target_State::Failed;
        output.status.detail = "Pattern matched, but no candidate address could be decoded";
        return Result<GTA_Target_Resolution>::Success(std::move(output));
    }

    if (output.candidates.size() != 1) {
        output.status.state = GTA_Runtime_Target_State::Failed;
        output.status.detail = "Pattern is ambiguous; multiple candidate addresses were decoded";
        return Result<GTA_Target_Resolution>::Success(std::move(output));
    }

    output.address = output.candidates.front();
    output.status.state = GTA_Runtime_Target_State::Located;
    output.status.detail = "Unique candidate address resolved; semantic validation is still required";
    return Result<GTA_Target_Resolution>::Success(std::move(output));
}
}
