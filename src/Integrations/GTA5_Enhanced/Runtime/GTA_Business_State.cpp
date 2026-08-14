#include "GTA_Business_State.hpp"

namespace Devilz::Integrations::GTA5_Enhanced
{
const char* GTA_Nightclub_Good_Name(GTA_Nightclub_Good good) noexcept
{
    switch (good) {
    case GTA_Nightclub_Good::Cargo: return "Cargo and Shipments";
    case GTA_Nightclub_Good::SportingGoods: return "Sporting Goods";
    case GTA_Nightclub_Good::SouthAmericanImports: return "South American Imports";
    case GTA_Nightclub_Good::PharmaceuticalResearch: return "Pharmaceutical Research";
    case GTA_Nightclub_Good::OrganicProduce: return "Organic Produce";
    case GTA_Nightclub_Good::PrintingAndCopying: return "Printing and Copying";
    case GTA_Nightclub_Good::CashCreation: return "Cash Creation";
    default: return "Unknown";
    }
}
}
