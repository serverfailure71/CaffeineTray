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

#pragma once

#include "ForwardDeclaration.hpp"

namespace CaffeineTake {

class CaffeineApp;

// App shared object.
class CaffeineAppSO final
{
    CaffeineApp* mApp;

public:
    constexpr CaffeineAppSO (CaffeineApp* app)
        : mApp (app)
    {
    }

    ~CaffeineAppSO ()
    {
        mApp = nullptr;
    }

    auto EnableCaffeine  () -> void;
    auto DisableCaffeine () -> void;

    auto GetSettings () const -> SettingsPtr;
    auto GetLang     () const -> LangPtr;
    auto GetIcons    () const -> CaffeineIconsPtr;
};

} // namespace CaffeineTake
