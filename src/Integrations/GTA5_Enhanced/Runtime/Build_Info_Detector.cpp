#include "Build_Info_Detector.hpp"

#include <sstream>

namespace Devilz::Integrations::GTA5_Enhanced
{
using namespace Devilz::Backend;

Result<Build_Info> Build_Info_Detector::DetectLoaded(std::string_view moduleName) const
{
    Module_Manager modules;
    auto module = modules.FindLoaded(moduleName);
    if (!module)
        return Result<Build_Info>::Failure(module.Failure().With("RequestedModule", std::string(moduleName)));

    const auto& info = module.Value();
    Build_Info build{};
    build.peTimestamp = info.fingerprint.timeDateStamp;
    build.imageSize = info.fingerprint.sizeOfImage;
    build.fingerprint = info.fingerprint.imageHash ^
        (static_cast<std::uint64_t>(info.fingerprint.timeDateStamp) << 32) ^
        info.fingerprint.sizeOfImage;
    build.executablePath = info.path;

    std::ostringstream version;
    version << "PE-" << build.peTimestamp << '-' << build.imageSize;
    build.version = version.str();

    if (build.fingerprint == 0)
        return Result<Build_Info>::Failure(Error(ErrorCode::RuntimeFailure, ErrorCategory::Runtime,
            "Loaded GTA Enhanced module produced an invalid build fingerprint"));

    return Result<Build_Info>::Success(std::move(build));
}
}
