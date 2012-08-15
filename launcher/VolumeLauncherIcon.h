// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *              Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#ifndef UNITYSHELL_VOLUME_LAUNCHER_ICON_H
#define UNITYSHELL_VOLUME_LAUNCHER_ICON_H

#include "SimpleLauncherIcon.h"

namespace unity
{
namespace launcher
{

class Volume;
class DevicesSettings;

class VolumeLauncherIcon : public SimpleLauncherIcon
{
public:
  typedef nux::ObjectPtr<VolumeLauncherIcon> Ptr;

  VolumeLauncherIcon(std::shared_ptr<Volume> const& volume,
                     std::shared_ptr<DevicesSettings> const& devices_settings);
  virtual ~VolumeLauncherIcon();

  bool CanEject(); // TODO: rename to public virtual bool IsTrashable();
  void EjectAndShowNotification(); // TODO: rename to private virtual void DoDropToTrash();
  virtual std::list<DbusmenuMenuitem*> GetMenus();
  void OnRemoved(); // TODO: make virtual if create RemovableVolumeLauncherIcon too ;)

protected:
  virtual void ActivateLauncherIcon(ActionArg arg);

  // Introspection
  virtual std::string GetName() const;

private:
  class Impl;
  std::shared_ptr<Impl> pimpl_;
};

}
}

#endif
