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

#include "Launcher.h"
#include "Nux/WindowCompositor.h"
#include "QuicklistManager.h"
#include "QuicklistMenuItemLabel.h"

namespace unity {

TrashLauncherIcon::TrashLauncherIcon(Launcher* IconManager)
  : SimpleLauncherIcon(IconManager)
  , proxy_("org.gnome.Nautilus", "/org/gnome/Nautilus", "org.gnome.Nautilus.FileOperations")
{
  tooltip_text = _("Trash");
  SetIconName("user-trash");
  SetQuirk(QUIRK_VISIBLE, true);
  SetQuirk(QUIRK_RUNNING, false);
  SetIconType(TYPE_TRASH);
  SetShortcut('t');
  
  glib::Object<GFile> location(g_file_new_for_uri("trash:///"));

  trash_monitor_ = g_file_monitor_directory(location,
                                            G_FILE_MONITOR_NONE,
                                            NULL,
                                            NULL);

  on_trash_changed_handler_id_ = g_signal_connect(trash_monitor_,
                                                  "changed",
                                                  G_CALLBACK(&TrashLauncherIcon::OnTrashChanged),
                                                  this);

  UpdateTrashIcon();
}

TrashLauncherIcon::~TrashLauncherIcon()
{
  if (on_trash_changed_handler_id_ != 0)
    g_signal_handler_disconnect((gpointer) trash_monitor_,
                                on_trash_changed_handler_id_);
}

nux::Color TrashLauncherIcon::BackgroundColor()
{
  return nux::Color(0xFF333333);
}

nux::Color TrashLauncherIcon::GlowColor()
{
  return nux::Color(0xFF333333);
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
    glib::String icon_name(g_icon_to_string(icon));
    
    self->SetIconName(icon_name.Value());

    self->empty_ = g_strcmp0(icon_name.Value(), "user-trash") == 0;
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

nux::DndAction TrashLauncherIcon::OnQueryAcceptDrop(std::list<char*> uris)
{
  return nux::DNDACTION_MOVE;
}

void TrashLauncherIcon::OnAcceptDrop(std::list<char*> uris)
{
  for (auto it : uris)
  {
    glib::Object<GFile> file(g_file_new_for_uri(it));
    g_file_trash(file, NULL, NULL);
  }
  
  SetQuirk(LauncherIcon::QUIRK_PULSE_ONCE, true);
}

} // namespace unity
