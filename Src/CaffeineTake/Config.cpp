// CaffeineTake - Keep your computer awake.
// 
// Copyright (c) 2020-2021 VacuityBox
// Copyright (c) 2022      serverfailure71
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
// 
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PCH.hpp"
#include "Config.hpp"

#if defined(FEATURE_CAFFEINETAKE_IMMERSIVE_CONTEXT_MENU)
#   pragma comment(lib, "ImmersiveNotifyIcon.lib")
#else
#   pragma comment(lib, "ClassicNotifyIcon.lib")
#endif

#if defined(FEATURE_CAFFEINETAKE_LOCKSCREEN_DETECTION)
#   pragma comment(lib, "Wtsapi32.lib")
#endif

#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_USB)
#   pragma comment(lib, "SetupAPI.lib")
#endif

#if defined(FEATURE_CAFFEINETAKE_AUTO_MODE_TRIGGER_BLUETOOTH)
#   pragma comment(lib, "Bthprops.lib")
#endif

#if defined(FEATURE_CAFFEINETAKE_NOTIFICATION_SOUND)
#   pragma comment(lib, "Winmm.lib")
#endif
