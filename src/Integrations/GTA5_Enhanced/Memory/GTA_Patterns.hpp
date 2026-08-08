#pragma once

#include "Backend/Memory/Pattern_Batch_Scanner.hpp"
#include "../Runtime/Build_Info.hpp"

#include <string>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct GTA_Pattern_Set
{
    std::uint64_t buildFingerprint = 0;
    std::vector<Devilz::Backend::Pattern_Request> requests;
};

class GTA_Patterns final
{
public:
    static GTA_Pattern_Set Core(const Build_Info& build);
};
}
