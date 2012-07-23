// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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

#ifndef UNITYSHELL_BFBLAUNCHERICON_H
#define UNITYSHELL_BFBLAUNCHERICON_H

#include "SimpleLauncherIcon.h"

#include <UnityCore/FilesystemLenses.h>

#include "LauncherOptions.h"
#include "unity-shared/UBusWrapper.h"

namespace unity
{
namespace launcher
{

class BFBLauncherIcon : public SimpleLauncherIcon
{

public:
  BFBLauncherIcon(LauncherHideMode hide_mode);

  virtual nux::Color BackgroundColor() const;
  virtual nux::Color GlowColor();

  void ActivateLauncherIcon(ActionArg arg);
  void SetHideMode(LauncherHideMode hide_mode);

protected:
  std::list<DbusmenuMenuitem*> GetMenus();
  std::string GetName() const;

private:
  void OnOverlayShown(GVariant *data, bool visible);
  static void OnMenuitemActivated(DbusmenuMenuitem* item, int time, gchar* lens);

  static unity::UBusManager ubus_manager_;
  nux::Color background_color_;
  dash::LensDirectoryReader::Ptr reader_;
  LauncherHideMode launcher_hide_mode_;
};

}
}

#endif // UNITYSHELL_BFBLAUNCHERICON_H
