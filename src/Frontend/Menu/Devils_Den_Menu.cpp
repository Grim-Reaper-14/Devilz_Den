#include "Frontend/Menu/Devils_Den_Menu.hpp"
#include "Frontend/Menu/Menu_Appearance.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Self_Online_Extension.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Teleport_Extension.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Vehicle_Garage_Save.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_World_Environment_Extension.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Ped_Control.hpp"

#define Draw DrawLegacy
#define DrawBanner DrawBannerLegacy
#define DrawNavigation DrawNavigationLegacy
#include "Devils_Den_Menu_part00.inc"
#undef DrawNavigation
#undef DrawBanner
#undef Draw

#define DrawWeaponsPage DrawWeaponsPageLegacy
#include "Devils_Den_Menu_part01.inc"
#undef DrawWeaponsPage
#include "Devils_Den_Menu_part02.inc"
#include "Devils_Den_Menu_part03.inc"
#include "Devils_Den_Menu_vehicle_garage.inc"

#define DrawTeleportPage DrawTeleportPageLegacy
#define DrawPlaceholderPage DrawPlaceholderPageLegacy
#define DrawSettingsPage DrawSettingsPageLegacy
#include "Devils_Den_Menu_part04.inc"
#undef DrawSettingsPage
#undef DrawPlaceholderPage
#undef DrawTeleportPage

#include "Devils_Den_Menu_draw.inc"
#include "Devils_Den_Menu_banner.inc"
#include "Devils_Den_Menu_navigation.inc"
#include "Devils_Den_Menu_settings.inc"
#include "Devils_Den_Menu_world.inc"
#include "Devils_Den_Menu_teleport.inc"
#include "Weapons/Devils_Aimbot_Menu.inc"
