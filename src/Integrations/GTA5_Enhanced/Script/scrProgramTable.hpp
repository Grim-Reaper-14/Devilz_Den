#pragma once

#include "Backend/Error/Result.hpp"
#include "scrProgram.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace Devilz::Integrations::GTA5_Enhanced
{
class scrProgramTable final
{
public:
    Devilz::Backend::Result<void> Register(scrProgram* program);
    void Un