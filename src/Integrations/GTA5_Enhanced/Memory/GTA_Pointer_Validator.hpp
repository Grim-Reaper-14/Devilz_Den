#pragma once

#include "GTA_Pointers.hpp"
#include "../Runtime/GTA_Runtime_Target.hpp"

#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
class GTA_Pointer_Validator final
{
public:
    [[nodiscard]] static std::vector<GTA_Runtime_Target_Status> Validate(const GTA_Pointers& pointers);
};
}
