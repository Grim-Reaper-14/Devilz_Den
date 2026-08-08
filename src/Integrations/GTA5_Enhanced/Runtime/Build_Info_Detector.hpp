#pragma once

#include "Backend/Error/Result.hpp"
#include "Backend/Process/Process_Info.hpp"
#include "Build_Info.hpp"

namespace Devilz::Integrations::GTA5_Enhanced
{
class Build_Info_Detector final
{
public:
    [[nodiscard]] Devilz::Backend::Result<Build_Info> Detect(
        const Devilz::Backend::Process_Info& process) const;
};
}
