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
#include <sigc++/signal.h>

typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;

namespace unity 
{
namespace lockscreen
{

class BackgroundSettings
{
public:
  virtual ~BackgroundSettings() {};

  virtual BaseTexturePtr GetBackgroundTexture(nux::Size const& size) = 0;

  sigc::signal<void> bg_changed;
};

} // lockscreen
} // unity

#endif // UNITY_BACKGROUND_SETTINGS_H
