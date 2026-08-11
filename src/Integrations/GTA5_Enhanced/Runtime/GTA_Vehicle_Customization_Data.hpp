#pragma once

#include <array>
#include <span>
#include <string_view>

namespace Devilz::Integrations::GTA5_Enhanced
{
struct GTA_Vehicle_Named_Value
{
    int value = 0;
    std::string_view name;
};

struct GTA_Vehicle_Named_Rgb
{
    std::string_view name;
    std::array<int, 3> rgb{0, 0, 0};
};

inline constexpr std::array<GTA_Vehicle_Named_Value, 13> GTA_Vehicle_Wheel_Types{{
    {0, "Sport"}, {1, "Muscle"}, {2, "Lowrider"}, {3, "SUV"}, {4, "Offroad"},
    {5, "Tuner"}, {6, "Bike Wheels"}, {7, "High End"}, {8, "Benny's Originals"},
    {9, "Benny's Bespoke"}, {10, "Open Wheel"}, {11, "Street"}, {12, "Track"}
}};

inline constexpr std::array<GTA_Vehicle_Named_Value, 13> GTA_Vehicle_Plate_Styles{{
    {0, "Blue on White 1"}, {1, "Yellow on Black"}, {2, "Yellow on Blue"},
    {3, "Blue on White 2"}, {4, "Blue on White 3"}, {5, "Yankton"},
    {6, "Ecola"}, {7, "Las Venturas"}, {8, "Liberty City"},
    {9, "Los Santos Car Meet"}, {10, "Los Santos Panic"},
    {11, "Los Santos Pounders"}, {12, "Sprunk"}
}};

inline constexpr std::array<GTA_Vehicle_Named_Value, 7> GTA_Vehicle_Window_Tints{{
    {0, "None"}, {1, "Pure Black"}, {2, "Dark Smoke"}, {3, "Light Smoke"},
    {4, "Stock"}, {5, "Limo"}, {6, "Green"}
}};

inline constexpr std::array<GTA_Vehicle_Named_Value, 14> GTA_Vehicle_Headlight_Colors{{
    {-1, "Default"}, {0, "White"}, {1, "Blue"}, {2, "Electric Blue"},
    {3, "Mint Green"}, {4, "Lime Green"}, {5, "Yellow"}, {6, "Golden Shower"},
    {7, "Orange"}, {8, "Red"}, {9, "Pony Pink"}, {10, "Hot Pink"},
    {11, "Purple"}, {12, "Blacklight"}
}};

inline constexpr std::array<GTA_Vehicle_Named_Rgb, 13> GTA_Vehicle_Neon_Colors{{
    {"White", {222, 222, 255}}, {"Blue", {2, 21, 255}}, {"Electric Blue", {3, 83, 255}},
    {"Mint Green", {0, 255, 140}}, {"Lime Green", {94, 255, 1}}, {"Yellow", {255, 255, 0}},
    {"Golden Shower", {255, 150, 5}}, {"Orange", {255, 62, 0}}, {"Red", {255, 1, 1}},
    {"Pony Pink", {255, 50, 100}}, {"Hot Pink", {255, 5, 190}},
    {"Purple", {35, 1, 255}}, {"Blacklight", {15, 3, 255}}
}};

inline constexpr std::array<GTA_Vehicle_Named_Rgb, 11> GTA_Vehicle_Tyre_Smoke_Colors{{
    {"White", {255, 255, 255}}, {"Black", {20, 20, 20}}, {"Blue", {0, 174, 239}},
    {"Yellow", {252, 238, 0}}, {"Purple", {100, 79, 142}}, {"Orange", {255, 127, 0}},
    {"Green", {114, 204, 114}}, {"Red", {226, 6, 6}}, {"Pink", {203, 54, 148}},
    {"Brown", {180, 130, 97}}, {"Patriot", {0, 0, 0}}
}};

inline constexpr std::array<GTA_Vehicle_Named_Value, 6> GTA_Vehicle_Paint_Types{{
    {0, "Classic"}, {1, "Metallic"}, {3, "Matte"},
    {4, "Metal"}, {5, "Chrome"}, {6, "Chameleon"}
}};

inline constexpr GTA_Vehicle_Named_Value GTA_Vehicle_Classic_Colors[] = {
    {0, "Black"}, {147, "Carbon Black"}, {1, "Graphite"}, {11, "Anthracite Black"},
    {2, "Black Steel"}, {3, "Dark Steel"}, {4, "Silver"}, {5, "Bluish Silver"},
    {6, "Rolled Steel"}, {7, "Shadow Silver"}, {8, "Stone Silver"}, {9, "Midnight Silver"},
    {10, "Cast Iron Silver"}, {27, "Red"}, {28, "Torino Red"}, {29, "Formula Red"},
    {150, "Lava Red"}, {30, "Blaze Red"}, {31, "Grace Red"}, {32, "Garnet Red"},
    {33, "Sunset Red"}, {34, "Cabernet Red"}, {143, "Wine Red"}, {35, "Candy Red"},
    {135, "Hot Pink"}, {137, "Pfister Pink"}, {136, "Salmon Pink"}, {36, "Sunrise Orange"},
    {38, "Orange"}, {138, "Bright Orange"}, {99, "Gold"}, {90, "Bronze"},
    {88, "Yellow"}, {89, "Race Yellow"}, {91, "Dew Yellow"}, {49, "Dark Green"},
    {50, "Racing Green"}, {51, "Sea Green"}, {52, "Olive Green"}, {53, "Bright Green"},
    {54, "Gasoline Green"}, {92, "Lime Green"}, {141, "Midnight Blue"}, {61, "Galaxy Blue"},
    {62, "Dark Blue"}, {63, "Saxon Blue"}, {64, "Blue"}, {65, "Mariner Blue"},
    {66, "Harbor Blue"}, {67, "Diamond Blue"}, {68, "Surf Blue"}, {69, "Nautical Blue"},
    {73, "Racing Blue"}, {70, "Ultra Blue"}, {74, "Light Blue"}, {96, "Chocolate Brown"},
    {101, "Bison Brown"}, {95, "Creek Brown"}, {94, "Feltzer Brown"}, {97, "Maple Brown"},
    {103, "Beechwood Brown"}, {104, "Sienna Brown"}, {98, "Saddle Brown"}, {100, "Moss Brown"},
    {102, "Woodbeech Brown"}, {105, "Sandy Brown"}, {106, "Bleached Brown"},
    {71, "Schafter Purple"}, {72, "Spinnaker Purple"}, {142, "Midnight Purple"},
    {145, "Bright Purple"}, {107, "Cream"}, {111, "Ice White"}, {112, "Frost White"},
    {37, "Classic Gold"}, {139, "Green"}, {144, "Hunter Green"}, {125, "Securicor Green"},
    {157, "Epsilon Blue"}, {140, "Fluorescent Blue"}, {146, "V Dark Blue"},
    {127, "Police Blue"}, {93, "Champagne"}, {134, "Pure White"}, {156, "Default Alloy"},
    {160, "Secret Gold"}
};

inline constexpr GTA_Vehicle_Named_Value GTA_Vehicle_Matte_Colors[] = {
    {12, "Black"}, {13, "Gray"}, {14, "Light Gray"}, {131, "Ice White"},
    {83, "Blue"}, {82, "Dark Blue"}, {84, "Midnight Blue"}, {149, "Midnight Purple"},
    {148, "Schafter Purple"}, {39, "Red"}, {40, "Dark Red"}, {41, "Orange"},
    {42, "Yellow"}, {55, "Lime Green"}, {128, "Green"}, {151, "Forest Green"},
    {155, "Foliage Green"}, {152, "Olive Drab"}, {153, "Dark Earth"},
    {154, "Desert Tan"}, {129, "Brown"}
};

inline constexpr GTA_Vehicle_Named_Value GTA_Vehicle_Metal_Colors[] = {
    {117, "Brushed Steel"}, {118, "Brushed Black Steel"}, {119, "Brushed Aluminum"},
    {158, "Pure Gold"}, {159, "Brushed Gold"}
};

inline constexpr GTA_Vehicle_Named_Value GTA_Vehicle_Chrome_Colors[] = {
    {120, "Chrome"}
};

inline constexpr GTA_Vehicle_Named_Value GTA_Vehicle_Chameleon_Colors[] = {
    {161, "Anodized Red"}, {162, "Anodized Wine"}, {163, "Anodized Purple"},
    {164, "Anodized Blue"}, {165, "Anodized Green"}, {166, "Anodized Lime"},
    {167, "Anodized Copper"}, {168, "Anodized Bronze"}, {169, "Anodized Champagne"},
    {170, "Anodized Gold"}, {171, "Green Blue Flip"}, {172, "Green Red Flip"},
    {173, "Green Brown Flip"}, {174, "Green Turquoise Flip"}, {175, "Green Purple Flip"},
    {176, "Teal Purple Flip"}, {177, "Turquoise Red Flip"}, {178, "Turquoise Purple Flip"},
    {179, "Cyan Purple Flip"}, {180, "Blue Pink Flip"}, {181, "Blue Green Flip"},
    {182, "Purple Red Flip"}, {183, "Purple Green Flip"}, {184, "Magenta Green Flip"},
    {185, "Magenta Yellow Flip"}, {186, "Burgundy Green Flip"}, {187, "Magenta Cyan Flip"},
    {188, "Copper Purple Flip"}, {189, "Magenta Orange Flip"}, {190, "Red Orange Flip"},
    {191, "Orange Purple Flip"}, {192, "Orange Blue Flip"}, {193, "White Purple Flip"},
    {194, "Red Rainbow Flip"}, {195, "Blue Rainbow Flip"}, {196, "Dark Green Pearl"},
    {197, "Dark Teal Pearl"}, {198, "Dark Blue Pearl"}, {199, "Dark Purple Pearl"},
    {200, "Oil Slick Pearl"}, {201, "Light Green Pearl"}, {202, "Light Blue Pearl"},
    {203, "Light Purple Pearl"}, {204, "Light Pink Pearl"}, {205, "Off White Prismatic"},
    {206, "Pink Pearl"}, {207, "Yellow Pearl"}, {208, "Green Pearl"},
    {209, "Blue Pearl"}, {210, "Cream Pearl"}, {211, "White Prismatic"},
    {212, "Graphite Prismatic"}, {213, "Dark Blue Prismatic"}, {214, "Dark Purple Prismatic"},
    {215, "Hot Pink Prismatic"}, {216, "Red Prismatic"}, {217, "Green Prismatic"},
    {218, "Black Prismatic"}, {219, "Oil Slick Prismatic"}, {220, "Rainbow Prismatic"},
    {221, "Black Holographic"}, {222, "White Holographic"}
};

[[nodiscard]] inline constexpr std::span<const GTA_Vehicle_Named_Value> GTA_Vehicle_Paint_Palette(int paintType) noexcept
{
    switch (paintType) {
    case 3: return GTA_Vehicle_Matte_Colors;
    case 4: return GTA_Vehicle_Metal_Colors;
    case 5: return GTA_Vehicle_Chrome_Colors;
    case 6: return GTA_Vehicle_Chameleon_Colors;
    case 0:
    case 1:
    default: return GTA_Vehicle_Classic_Colors;
    }
}

template <std::size_t N>
[[nodiscard]] inline constexpr std::string_view GTA_Vehicle_Value_Name(
    const std::array<GTA_Vehicle_Named_Value, N>& values,
    int value,
    std::string_view fallback = "Unknown") noexcept
{
    for (const auto& entry : values)
        if (entry.value == value)
            return entry.name;
    return fallback;
}

[[nodiscard]] inline constexpr std::string_view GTA_Vehicle_Paint_Color_Name(
    int paintType,
    int color,
    std::string_view fallback = "Unknown") noexcept
{
    for (const auto& entry : GTA_Vehicle_Paint_Palette(paintType))
        if (entry.value == color)
            return entry.name;
    return fallback;
}

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
