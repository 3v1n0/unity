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

#ifndef UNITY_BACKGROUND_SETTINGS_H
#define UNITY_BACKGROUND_SETTINGS_H

#include <NuxCore/Size.h>
#include <NuxGraphics/GLTextureResourceManager.h> // for nux::BaseTexture
#include <UnityCore/GLibWrapper.h>

class _GnomeBG;

namespace unity 
{

typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;

namespace lockscreen
{

class BackgroundSettings
{
public:
  BackgroundSettings();

  BaseTexturePtr GetBackgroundTexture(nux::Size const& size,
  	                                  bool draw_grid,
  	                                  bool draw_logo);

private:
  static int GetGridOffset(int size);

  glib::Object<_GnomeBG> gnome_bg_;
};

}
}

#endif
