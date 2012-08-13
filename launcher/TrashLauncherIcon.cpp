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
  , proxy_("org.gnome.Nautilus", "/org/gnome/Nautilus", "org.gnome.Nautilus.FileOperations")
  , cancellable_(g_cancellable_new())
{
  tooltip_text = _("Trash");
  icon_name = "user-trash";
  SetQuirk(Quirk::VISIBLE, true);
  SetQuirk(Quirk::RUNNING, false);
  SetIconType(IconType::TRASH);
  SetShortcut('t');

  glib::Object<GFile> location(g_file_new_for_uri("trash:///"));

  glib::Error err;
  trash_monitor_ = g_file_monitor_directory(location, G_FILE_MONITOR_NONE, nullptr, &err);

  if (err)
  {
    LOG_ERROR(logger) << "Could not create file monitor for trash uri: " << err;
  }
  else
  {
    trash_changed_signal_.Connect(trash_monitor_, "changed",
    [this] (GFileMonitor*, GFile*, GFile*, GFileMonitorEvent) {
      UpdateTrashIcon();
    });
  }

  UpdateTrashIcon();
}

TrashLauncherIcon::~TrashLauncherIcon()
{
  g_cancellable_cancel(cancellable_);
}

AbstractLauncherIcon::MenuItemsList TrashLauncherIcon::GetMenus()
{
  MenuItemsList result;

  /* Empty Trash */
  glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());
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
                          cancellable_,
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

  SetQuirk(LauncherIcon::Quirk::PULSE_ONCE, true);
}

std::string TrashLauncherIcon::GetName() const
{
  return "TrashLauncherIcon";
}

} // namespace launcher
} // namespace unity
