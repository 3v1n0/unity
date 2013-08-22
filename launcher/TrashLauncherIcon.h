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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#ifndef TRASHLAUNCHERICON_H
#define TRASHLAUNCHERICON_H

#include <gio/gio.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>

#include "DndData.h"
#include "SimpleLauncherIcon.h"
#include "unity-shared/FileManager.h"

namespace unity
{
namespace launcher
{

class TrashLauncherIcon : public SimpleLauncherIcon
{
public:
  TrashLauncherIcon(FileManager::Ptr const& = nullptr);

protected:
  void UpdateTrashIcon();

  nux::DndAction OnQueryAcceptDrop(DndData const& dnd_data);
  bool OnShouldHighlightOnDrag(DndData const& dnd_data);
  void OnAcceptDrop(DndData const& dnd_data);

  std::string GetName() const;

private:
  void ActivateLauncherIcon(ActionArg arg);
  void OnOpenedLocationsChanged();
  MenuItemsVector GetMenus();

  static void UpdateTrashIconCb(GObject* source, GAsyncResult* res, gpointer data);

  bool empty_;
  FileManager::Ptr file_manager_;
  glib::Cancellable cancellable_;
  glib::Object<GFileMonitor> trash_monitor_;
  glib::Signal<void, GFileMonitor*, GFile*, GFile*, GFileMonitorEvent> trash_changed_signal_;
  glib::Signal<void, DbusmenuMenuitem*, unsigned> empty_activated_signal_;
};

}
} // namespace unity

#endif // TRASHLAUNCHERICON_H
