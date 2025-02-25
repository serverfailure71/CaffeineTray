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
#include "CaffeineAppSO.hpp"
#include "CaffeineApp.hpp"

namespace CaffeineTake {

auto CaffeineAppSO::EnableCaffeine () -> void
{
    if (mApp)
    {
        mApp->EnableCaffeine();
    }
}

auto CaffeineAppSO::DisableCaffeine () -> void
{
    if (mApp)
    {
        mApp->DisableCaffeine();
    }
}

auto CaffeineAppSO::GetSettings () const -> SettingsPtr
{
    if (mApp)
    {
        return mApp->mSettings;
    }

    return nullptr;
}

auto CaffeineAppSO::GetLang () const -> LangPtr
{
    if (mApp)
    {
        return mApp->mLang;
    }

    return nullptr;
}

auto CaffeineAppSO::GetIcons () const -> CaffeineIconsPtr
{
    if (mApp)
    {
        return mApp->mIcons;
    }

    return nullptr;
}
} // namespace CaffeineTake
    