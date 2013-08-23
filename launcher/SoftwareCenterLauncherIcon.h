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

#include <string>
#include <UnityCore/GLibDBusProxy.h>
#include "ApplicationLauncherIcon.h"
#include "LauncherDragWindow.h"

namespace unity
{
namespace launcher
{
class Launcher;

class SoftwareCenterLauncherIcon : public ApplicationLauncherIcon
{
  NUX_DECLARE_OBJECT_TYPE(SoftwareCenterLauncherIcon, ApplicationLauncherIcon);
public:
  typedef nux::ObjectPtr<SoftwareCenterLauncherIcon> Ptr;

  SoftwareCenterLauncherIcon(ApplicationPtr const& app,
                             std::string const& aptdaemon_trans_id,
                             std::string const& icon_path);

  void Animate(nux::ObjectPtr<Launcher> const& launcher, int start_x, int start_y);

  std::string GetName() const;

protected:
  void ActivateLauncherIcon(ActionArg arg);

private:
  std::string GetActualDesktopFileAfterInstall();
  void OnFinished(GVariant *params);
  void OnPropertyChanged(GVariant* params);
  void OnDragAnimationFinished(std::string const& final_icon);

  glib::DBusProxy aptdaemon_trans_;

  nux::ObjectPtr<nux::IOpenGLBaseTexture> icon_texture_;
  nux::ObjectPtr<LauncherDragWindow> drag_window_;
  nux::ObjectPtr<Launcher> launcher_;
  bool finished_;
  bool needs_urgent_;
  std::string aptdaemon_trans_id_;

  friend class TestSoftwareCenterLauncherIcon;
};

}
}

#endif //SOFTWARE_CENTER_LAUNCHERICON_H
