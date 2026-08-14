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

const char* GTA_Nightclub_Action_Name(GTA_Nightclub_Action_Kind kind) noexcept
{
    switch (kind) {
    case GTA_Nightclub_Action_Kind::SetCoreValues: return "Apply core values";
    case GTA_Nightclub_Action_Kind::SetPopularity: return "Set popularity";
    case GTA_Nightclub_Action_Kind::SetSafeCash: return "Set safe cash";
    case GTA_Nightclub_Action_Kind::SetEntryCost: return "Set entry cost";
    case GTA_Nightclub_Action_Kind::SetProductStocks: return "Apply warehouse stock";
    case GTA_Nightclub_Action_Kind::FillProductStocks: return "Fill warehouse";
    case GTA_Nightclub_Action_Kind::ClearProductStocks: return "Clear warehouse";
    case GTA_Nightclub_Action_Kind::SetSaleValues: return "Apply sale values";
    case GTA_Nightclub_Action_Kind::SetMissionState: return "Apply mission state";
    case GTA_Nightclub_Action_Kind::ResetMissionState: return "Reset mission state";
    default: return "Nightclub action";
    }
}

const char* GTA_Nightclub_Action_Status_Name(GTA_Nightclub_Action_Status status) noexcept
{
    switch (status) {
    case GTA_Nightclub_Action_Status::Idle: return "IDLE";
    case GTA_Nightclub_Action_Status::Queued: return "QUEUED";
    case GTA_Nightclub_Action_Status::Succeeded: return "SUCCEEDED";
    case GTA_Nightclub_Action_Status::RuntimeUnavailable: return "RUNTIME UNAVAILABLE";
    case GTA_Nightclub_Action_Status::UnsupportedBuild: return "UNSUPPORTED BUILD";
    case GTA_Nightclub_Action_Status::NoNightclub: return "NO NIGHTCLUB";
    case GTA_Nightclub_Action_Status::InvalidValue: return "INVALID VALUE";
    case GTA_Nightclub_Action_Status::Failed: return "FAILED";
    default: return "UNKNOWN";
    }
}
}
