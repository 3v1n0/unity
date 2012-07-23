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
 *              Andrea Azzarone <azzaronea@gmail.com>
 */

#include "TrashLauncherIcon.h"

#include <glib/gi18n-lib.h>
#include <Nux/WindowCompositor.h>
#include <NuxCore/Logger.h>

#include "Launcher.h"
#include "QuicklistManager.h"
#include "QuicklistMenuItemLabel.h"

namespace unity
{
namespace launcher
{
namespace
{
  nux::logging::Logger logger("unity.launcher.TrashLauncherIcon");
}

TrashLauncherIcon::TrashLauncherIcon()
  : SimpleLauncherIcon()
  , on_trash_changed_handler_id_(0)
  , proxy_("org.gnome.Nautilus", "/org/gnome/Nautilus", "org.gnome.Nautilus.FileOperations")
{
  tooltip_text = _("Trash");
  icon_name = "user-trash";
  SetQuirk(QUIRK_VISIBLE, true);
  SetQuirk(QUIRK_RUNNING, false);
  SetIconType(TYPE_TRASH);
  SetShortcut('t');

  glib::Object<GFile> location(g_file_new_for_uri("trash:///"));

  glib::Error err;
  trash_monitor_ = g_file_monitor_directory(location,
                                            G_FILE_MONITOR_NONE,
                                            NULL,
                                            &err);

  if (err)
  {
    LOG_ERROR(logger) << "Could not create file monitor for trash uri: " << err;
  }
  else
  {
    on_trash_changed_handler_id_ = g_signal_connect(trash_monitor_,
                                                  "changed",
                                                  G_CALLBACK(&TrashLauncherIcon::OnTrashChanged),
                                                  this);
  }

  UpdateTrashIcon();
}

TrashLauncherIcon::~TrashLauncherIcon()
{
  if (on_trash_changed_handler_id_ != 0)
    g_signal_handler_disconnect((gpointer) trash_monitor_,
                                on_trash_changed_handler_id_);
}

std::list<DbusmenuMenuitem*> TrashLauncherIcon::GetMenus()
{
  std::list<DbusmenuMenuitem*> result;
  DbusmenuMenuitem* menu_item;

  /* Empty Trash */
  menu_item = dbusmenu_menuitem_new();
  g_object_ref(menu_item);

  dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Empty Trash..."));
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, !empty_);
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

  g_signal_connect(menu_item,
                   DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                   (GCallback)&TrashLauncherIcon::OnEmptyTrash, this);
  result.push_back(menu_item);

  return result;
}

void TrashLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  SimpleLauncherIcon::ActivateLauncherIcon(arg);

  glib::Error error;
  g_spawn_command_line_async("xdg-open trash://", &error);
}

void TrashLauncherIcon::OnEmptyTrash(DbusmenuMenuitem* item, int time, TrashLauncherIcon* self)
{
  self->proxy_.Call("EmptyTrash");
}

void TrashLauncherIcon::UpdateTrashIcon()
{
  glib::Object<GFile> location(g_file_new_for_uri("trash:///"));

  g_file_query_info_async(location,
                          G_FILE_ATTRIBUTE_STANDARD_ICON,
                          G_FILE_QUERY_INFO_NONE,
                          0,
                          NULL,
                          &TrashLauncherIcon::UpdateTrashIconCb,
                          this);

}

void TrashLauncherIcon::UpdateTrashIconCb(GObject* source,
                                          GAsyncResult* res,
                                          gpointer data)
{
  TrashLauncherIcon* self = (TrashLauncherIcon*) data;

  // FIXME: should use the generic LoadIcon function (not taking from the unity theme)
  glib::Object<GFileInfo> info(g_file_query_info_finish(G_FILE(source), res, NULL));

  if (info)
  {
    glib::Object<GIcon> icon(g_file_info_get_icon(info));
    glib::String icon_string(g_icon_to_string(icon));

    self->icon_name = icon_string.Str();

    self->empty_ = (self->icon_name == "user-trash");
  }
}

void TrashLauncherIcon::OnTrashChanged(GFileMonitor* monitor,
                                       GFile* file,
                                       GFile* other_file,
                                       GFileMonitorEvent event_type,
                                       gpointer data)
{
  TrashLauncherIcon* self = (TrashLauncherIcon*) data;
  self->UpdateTrashIcon();
}


nux::DndAction TrashLauncherIcon::OnQueryAcceptDrop(DndData const& dnd_data)
{
  return nux::DNDACTION_MOVE;
}

bool TrashLauncherIcon::OnShouldHighlightOnDrag(DndData const& dnd_data)
{
  return true;
}


void TrashLauncherIcon::OnAcceptDrop(DndData const& dnd_data)
{
  for (auto it : dnd_data.Uris())
  {
    glib::Object<GFile> file(g_file_new_for_uri(it.c_str()));
    g_file_trash(file, NULL, NULL);
  }

  SetQuirk(LauncherIcon::QUIRK_PULSE_ONCE, true);
}

std::string TrashLauncherIcon::GetName() const
{
  return "TrashLauncherIcon";
}

} // namespace launcher
} // namespace unity
