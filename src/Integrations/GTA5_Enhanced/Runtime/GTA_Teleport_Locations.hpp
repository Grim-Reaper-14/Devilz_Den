#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace Devilz::Integrations::GTA5_Enhanced
{
enum class GTA_Teleport_Location_Id : std::uint8_t
{
    None,
    LsInternationalAirport,
    DelPerroPier,
    SandyShoresAirfield,
    McKenzieAirfield,
    MountChiliadSummit,
    PaletoBayPier,
    FortZancudo,
    PoliceStation,
    LosSantosCustoms,
    TrevorsMethLab,
    PacificStandardBank,
    MazeBankRoof
};

struct GTA_Teleport_Location
{
    GTA_Teleport_Location_Id id{};
    std::string_view label;
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

inline constexpr std::array<GTA_Teleport_Location, 12> GTA_Teleport_Locations{{
    {GTA_Teleport_Location_Id::LsInternationalAirport, "LS INTERNATIONAL AIRPORT", -1034.6F, -2733.6F, 13.8F},
    {GTA_Teleport_Location_Id::DelPerroPier, "DEL PERRO PIER", -1850.127F, -1231.751F, 13.017F},
    {GTA_Teleport_Location_Id::SandyShoresAirfield, "SANDY SHORES AIRFIELD", 1747.0F, 3273.7F, 41.1F},
    {GTA_Teleport_Location_Id::McKenzieAirfield, "MCKENZIE AIRFIELD", 2121.7F, 4796.3F, 41.1F},
    {GTA_Teleport_Location_Id::MountChiliadSummit, "MOUNT CHILIAD SUMMIT", 450.718F, 5566.614F, 806.183F},
    {GTA_Teleport_Location_Id::PaletoBayPier, "PALETO BAY PIER", -275.522F, 6635.835F, 7.425F},
    {GTA_Teleport_Location_Id::FortZancudo, "FORT ZANCUDO", -2047.4F, 3132.1F, 32.8F},
    {GTA_Teleport_Location_Id::PoliceStation, "POLICE STATION", 436.491F, -982.172F, 30.699F},
    {GTA_Teleport_Location_Id::LosSantosCustoms, "LOS SANTOS CUSTOMS", -365.425F, -131.809F, 37.873F},
    {GTA_Teleport_Location_Id::TrevorsMethLab, "TREVOR'S METH LAB", 1391.773F, 3608.716F, 38.942F},
    {GTA_Teleport_Location_Id::PacificStandardBank, "PACIFIC STANDARD BANK", 235.046F, 216.434F, 106.287F},
    {GTA_Teleport_Location_Id::MazeBankRoof, "MAZE BANK ROOF", -75.015F, -818.215F, 326.176F}
}};

[[nodiscard]] constexpr const GTA_Teleport_Location* FindTeleportLocation(
    GTA_Teleport_Location_Id id) noexcept
{
    for (const auto& location : GTA_Teleport_Locations) {
        if (location.id == id)
            return &location;
    }
    return nullptr;
}
}
