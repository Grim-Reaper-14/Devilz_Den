#pragma once

#include "Backend/Error/Result.hpp"
#include "Backend/Memory/Module_Manager.hpp"
#include "Build_Info.hpp"

#include <string_view>

namespace Devilz::Integrations::GTA5_Enhanced
{
class Build_Info_Detector final
{
public:
    [[nodiscard]] Devilz::Backend::Result<Build_Info> DetectLoaded(
        std::string_view moduleName = "GTA5_Enhanced.exe") const;
};
}
