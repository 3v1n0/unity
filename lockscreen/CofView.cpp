// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2013 Canonical Ltd
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
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#include "config.h"

#include "CofView.h"
#include "unity-shared/RawPixel.h"
#include "unity-shared/ThemeSettings.h"

namespace unity
{
namespace lockscreen
{
namespace
{
const std::string COF_FILE = "lockscreen_cof";
}

CofView::CofView()
  // FIXME (andy) if we get an svg cof we can make it resolution independent.
  : IconTexture(theme::Settings::Get()->ThemedFilePath(COF_FILE, {PKGDATADIR}), std::numeric_limits<unsigned>::max())
  , scale(1.0)
{
  scale.changed.connect([this] (double scale) {
    int w, h;
    gdk_pixbuf_get_file_info(theme::Settings::Get()->ThemedFilePath(COF_FILE, {PKGDATADIR}).c_str(), &w, &h);
    SetSize(RawPixel(std::max(w, h)).CP(scale));
    ReLoadIcon();
  });
}

nux::Area* CofView::FindAreaUnderMouse(nux::Point const& mouse_position,
                                       nux::NuxEventType event_type)
{
  return nullptr;
}

}
}
