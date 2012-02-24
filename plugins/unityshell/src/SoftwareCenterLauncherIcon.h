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
 * Authored by: Bilal Akhtar <bilalakhtar@ubuntu.com>
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#ifndef SOFTWARE_CENTER_LAUNCHERICON_H
#define SOFTWARE_CENTER_LAUNCHERICON_H

#include "BamfLauncherIcon.h"
#include <UnityCore/GLibDBusProxy.h>

namespace unity
{
namespace launcher
{

class SoftwareCenterLauncherIcon : public BamfLauncherIcon
{
public:
  SoftwareCenterLauncherIcon(BamfApplication* app,
                             std::string const& aptdaemon_trans_id,
                             std::string const& icon_path,
                             gint32 icon_x,
                             gint32 icon_y,
                             gint32 icon_size);

private:
  void OnPropertyChanged(GVariant* params);
  void AnimateIcon(gint32 icon_x, gint32 icon_y, gint32 icon_size);

  glib::DBusProxy _aptdaemon_trans;
};

}
}

#endif //SOFTWARE_CENTER_LAUNCHERICON_H
