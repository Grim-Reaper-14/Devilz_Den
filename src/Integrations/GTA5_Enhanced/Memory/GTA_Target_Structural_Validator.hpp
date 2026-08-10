#pragma once

#include "GTA_Target_Definition.hpp"
#include "GTA_Target_Evidence.hpp"

#include <string>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct GTA_Target_Structural_Validation
{
    bool applicable = false;
    bool passed = false;
    std::string detail;
};

class GTA_Target_Structural_Validator final
{
public:
    [[nodiscard]] static GTA_Target_Structural_Validation Validate(
        const GTA_Target_Definition& definition,
        const GTA_Target_Evidence& evidence);
};
}
