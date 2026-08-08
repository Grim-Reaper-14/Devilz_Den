#include "GTA_Patterns.hpp"

namespace Devilz::Integrations::GTA5_Enhanced
{
GTA_Pattern_Set GTA_Patterns::Core(const Build_Info& build)
{
    GTA_Pattern_Set set;
    set.buildFingerprint = build.fingerprint;

    // Intentionally no guessed signatures here. Exact Enhanced signatures are
    // data for a verified build and should be added only after validation.
    // The pointer manager will fail closed when a required definition is absent.
    return set;
}
}
