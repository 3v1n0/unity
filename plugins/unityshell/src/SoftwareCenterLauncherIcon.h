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

#include <UnityCore/GLibDBusProxy.h>
#include "BamfLauncherIcon.h"

class LauncherDragWindow;

namespace unity
{
namespace launcher
{
class Launcher;

class SoftwareCenterLauncherIcon : public BamfLauncherIcon
{
  NUX_DECLARE_OBJECT_TYPE(SoftwareCenterLauncherIcon, BamfLauncherIcon);
public:
  typedef nux::ObjectPtr<SoftwareCenterLauncherIcon> Ptr;

  SoftwareCenterLauncherIcon(BamfApplication* app,
                             std::string const& aptdaemon_trans_id,
                             std::string const& icon_path);
  ~SoftwareCenterLauncherIcon();

  void Animate(nux::ObjectPtr<Launcher> launcher, int icon_x, int icon_y, int icon_size);

  std::string GetName() const;

protected:
  void ActivateLauncherIcon(ActionArg arg);

private:
  void OnPropertyChanged(GVariant* params);
  void OnDragAnimationFinished();

  glib::DBusProxy _aptdaemon_trans;

  nux::ObjectPtr<nux::IOpenGLBaseTexture> _icon_texture;
  LauncherDragWindow* _drag_window;
  nux::ObjectPtr<Launcher> _launcher;
  bool _finished;
  bool _finished_just_now;
};

}
}

#endif //SOFTWARE_CENTER_LAUNCHERICON_H
