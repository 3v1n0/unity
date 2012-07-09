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
#include <UnityCore/GLibDBusProxy.h>
#include <UnityCore/GLibWrapper.h>

#include "DndData.h"
#include "SimpleLauncherIcon.h"

namespace unity
{
namespace launcher
{

class TrashLauncherIcon : public SimpleLauncherIcon
{

public:
  TrashLauncherIcon();
  ~TrashLauncherIcon();

protected:
  void UpdateTrashIcon();

  nux::DndAction OnQueryAcceptDrop(DndData const& dnd_data);
  bool OnShouldHighlightOnDrag(DndData const& dnd_data);
  void OnAcceptDrop(DndData const& dnd_data);

  std::string GetName() const;

private:
  gulong on_trash_changed_handler_id_;
  glib::Object<GFileMonitor> trash_monitor_;
  gboolean empty_;
  glib::DBusProxy proxy_;

  void ActivateLauncherIcon(ActionArg arg);
  std::list<DbusmenuMenuitem*> GetMenus();

  static void UpdateTrashIconCb(GObject* source, GAsyncResult* res, gpointer data);
  static void OnTrashChanged(GFileMonitor* monitor, GFile* file, GFile* other_file,
                             GFileMonitorEvent event_type, gpointer data);
  static void OnEmptyTrash(DbusmenuMenuitem* item, int time, TrashLauncherIcon* self);
};

}
} // namespace unity

#endif // TRASHLAUNCHERICON_H
