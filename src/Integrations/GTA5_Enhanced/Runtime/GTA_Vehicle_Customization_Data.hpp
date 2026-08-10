#pragma once

#include <array>
#include <string_view>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct GTA_Vehicle_Named_Value
{
    int value = 0;
    std::string_view name;
};

inline constexpr std::array<GTA_Vehicle_Named_Value, 13> GTA_Vehicle_Wheel_Types{{
    {0, "Sport"},
    {1, "Muscle"},
    {2, "Lowrider"},
    {3, "SUV"},
    {4, "Offroad"},
    {5, "Tuner"},
    {6, "Bike Wheels"},
    {7, "High End"},
    {8, "Benny's Originals"},
    {9, "Benny's Bespoke"},
    {10, "Open Wheel"},
    {11, "Street"},
    {12, "Track"}
}};

inline constexpr std::array<GTA_Vehicle_Named_Value, 13> GTA_Vehicle_Plate_Styles{{
    {0, "Blue on White 1"},
    {1, "Yellow on Black"},
    {2, "Yellow on Blue"},
    {3, "Blue on White 2"},
    {4, "Blue on White 3"},
    {5, "Yankton"},
    {6, "Ecola"},
    {7, "Las Venturas"},
    {8, "Liberty City"},
    {9, "Los Santos Car Meet"},
    {10, "Los Santos Panic"},
    {11, "Los Santos Pounders"},
    {12, "Sprunk"}
}};

inline constexpr std::array<GTA_Vehicle_Named_Value, 7> GTA_Vehicle_Window_Tints{{
    {0, "None"},
    {1, "Pure Black"},
    {2, "Dark Smoke"},
    {3, "Light Smoke"},
    {4, "Stock"},
    {5, "Limo"},
    {6, "Green"}
}};

[[nodiscard]] inline constexpr std::string_view GTA_Fallback_Mod_Slot_Name(int slot) noexcept
{
    switch (slot) {
    case 0: return "Spoilers";
    case 1: return "Front Bumper";
    case 2: return "Rear Bumper";
    case 3: return "Side Skirts";
    case 4: return "Exhaust";
    case 5: return "Frame";
    case 6: return "Grille";
    case 7: return "Hood";
    case 8: return "Left Fender";
    case 9: return "Right Fender";
    case 10: return "Roof";
    case 11: return "Engine";
    case 12: return "Brakes";
    case 13: return "Transmission";
    case 14: return "Horns";
    case 15: return "Suspension";
    case 16: return "Armor";
    case 17: return "Nitrous";
    case 18: return "Turbo";
    case 19: return "Subwoofer";
    case 20: return "Tire Smoke";
    case 21: return "Hydraulics";
    case 22: return "Xenon";
    case 23: return "Front Wheels";
    case 24: return "Rear Wheels";
    case 25: return "Plate Holder";
    case 26: return "Vanity Plate";
    case 27: return "Trim Design";
    case 28: return "Ornaments";
    case 29: return "Dashboard";
    case 30: return "Dials";
    case 31: return "Door Speakers";
    case 32: return "Seats";
    case 33: return "Steering Wheel";
    case 34: return "Shifter";
    case 35: return "Plaques";
    case 36: return "Speakers";
    case 37: return "Trunk";
    case 38: return "Hydraulics";
    case 39: return "Engine Block";
    case 40: return "Air Filter";
    case 41: return "Struts";
    case 42: return "Arch Covers";
    case 43: return "Aerials";
    case 44: return "Interior Trim";
    case 45: return "Tank";
    case 46: return "Windows";
    case 47: return "Doors";
    case 48: return "Livery";
    case 49: return "Lightbar";
    default: return "Modification";
    }
}
}
