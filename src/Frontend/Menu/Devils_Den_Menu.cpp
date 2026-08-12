#include "Integrations/GTA5_Enhanced/Runtime/GTA_Self_Online_Extension.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Teleport_Extension.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_Vehicle_Garage_Save.hpp"
#include "Integrations/GTA5_Enhanced/Runtime/GTA_World_Environment_Extension.hpp"

#include "Devils_Den_Menu_part00.inc"

#define DrawSelfPage DrawSelfPageLegacy
#include "Devils_Den_Menu_part01.inc"
#undef DrawSelfPage

#include "Devils_Den_Menu_self.inc"
#include "Devils_Den_Menu_part02.inc"
#include "Devils_Den_Menu_part03.inc"
#include "Devils_Den_Menu_vehicle_garage.inc"
#include "Devils_Den_Menu_world.inc"

#define DrawTeleportPage DrawTeleportPageLegacy
#define DrawPlaceholderPage DrawPlaceholderPageLegacy
#include "Devils_Den_Menu_part04.inc"
#undef DrawPlaceholderPage
#undef DrawTeleportPage

#include "Devils_Den_Menu_teleport.inc"
