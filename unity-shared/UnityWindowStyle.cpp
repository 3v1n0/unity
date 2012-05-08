// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#include "UnityWindowStyle.h"
#include "config.h"

namespace unity {
namespace ui {

UnityWindowStyle::UnityWindowStyle()
{
  background_top_.Adopt(nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_top.png", -1, true));
  background_left_.Adopt(nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_left.png", -1, true));
  background_corner_.Adopt(nux::CreateTexture2DFromFile(PKGDATADIR"/switcher_corner.png", -1, true));
}

UnityWindowStyle::~UnityWindowStyle()
{
  // nothing to do
}

int UnityWindowStyle::GetBorderSize() const
{
  return 30; // as measured from textures
}

int UnityWindowStyle::GetInternalOffset() const
{
  return 20;
}

nux::BaseTexture* UnityWindowStyle::GetBackgroundTop() const
{
  return background_top_.GetPointer();
}

nux::BaseTexture* UnityWindowStyle::GetBackgroundLeft() const
{
  return background_left_.GetPointer();
}

nux::BaseTexture* UnityWindowStyle::GetBackgroundCorner() const
{
  return background_corner_.GetPointer();
}


}
}
