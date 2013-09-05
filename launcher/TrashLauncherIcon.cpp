// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2013 Canonical Ltd
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
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "TrashLauncherIcon.h"

#include "config.h"
#include <glib/gi18n-lib.h>
#include <NuxCore/Logger.h>

#include "QuicklistMenuItemLabel.h"
#include "unity-shared/DesktopApplicationManager.h"
#include "unity-shared/GnomeFileManager.h"
#include "unity-shared/ZeitgeistUtils.h"

namespace unity
{
namespace launcher
{
namespace
{
  DECLARE_LOGGER(logger, "unity.launcher.icon.trash");
  const std::string ZEITGEIST_UNITY_ACTOR = "application://compiz.desktop";
  const std::string TRASH_URI = "trash:";
}

TrashLauncherIcon::TrashLauncherIcon(FileManager::Ptr const& fmo)
  : SimpleLauncherIcon(IconType::TRASH)
  , empty_(true)
  , file_manager_(fmo ? fmo : GnomeFileManager::Get())
{
  tooltip_text = _("Trash");
  icon_name = "user-trash";
  position = Position::END;
  SetQuirk(Quirk::VISIBLE, true);
  SetQuirk(Quirk::RUNNING, file_manager_->IsTrashOpened());
  SetShortcut('t');

  glib::Object<GFile> location(g_file_new_for_uri(TRASH_URI.c_str()));

  glib::Error err;
  trash_monitor_ = g_file_monitor_directory(location, G_FILE_MONITOR_NONE, cancellable_, &err);
  g_file_monitor_set_rate_limit(trash_monitor_, 1000);

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

  file_manager_->locations_changed.connect(sigc::mem_fun(this, &TrashLauncherIcon::OnOpenedLocationsChanged));

  UpdateTrashIcon();
}

void TrashLauncherIcon::OnOpenedLocationsChanged()
{
  SetQuirk(Quirk::RUNNING, file_manager_->IsTrashOpened());
}

AbstractLauncherIcon::MenuItemsVector TrashLauncherIcon::GetMenus()
{
  MenuItemsVector result;

  /* Empty Trash */
  glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());
  dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Empty Trash…"));
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, !empty_);
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
  empty_activated_signal_.Connect(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
  [this] (DbusmenuMenuitem*, unsigned timestamp) {
    file_manager_->EmptyTrash(timestamp);
  });

  result.push_back(menu_item);

  return result;
}

void TrashLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  SimpleLauncherIcon::ActivateLauncherIcon(arg);
  file_manager_->OpenTrash(arg.timestamp);
}

void TrashLauncherIcon::UpdateTrashIcon()
{
  glib::Object<GFile> location(g_file_new_for_uri(TRASH_URI.c_str()));

  g_file_query_info_async(location,
                          G_FILE_ATTRIBUTE_STANDARD_ICON,
                          G_FILE_QUERY_INFO_NONE,
                          0,
                          cancellable_,
                          &TrashLauncherIcon::UpdateTrashIconCb,
                          this);
}

void TrashLauncherIcon::UpdateTrashIconCb(GObject* source, GAsyncResult* res, gpointer data)
{
  auto self = static_cast<TrashLauncherIcon*>(data);

  // FIXME: should use the generic LoadIcon function (not taking from the unity theme)
  glib::Object<GFileInfo> info(g_file_query_info_finish(G_FILE(source), res, nullptr));

  if (info)
  {
    glib::Object<GIcon> icon(g_file_info_get_icon(info), glib::AddRef());
    glib::String icon_string(g_icon_to_string(icon));

    self->icon_name = icon_string.Str();
    self->empty_ = (self->icon_name == "user-trash");
  }
}


nux::DndAction TrashLauncherIcon::OnQueryAcceptDrop(DndData const& dnd_data)
{
#ifdef USE_X11
  return nux::DNDACTION_MOVE;
#else
  return nux::DNDACTION_NONE;
#endif
}

bool TrashLauncherIcon::OnShouldHighlightOnDrag(DndData const& dnd_data)
{
  return true;
}

void TrashLauncherIcon::OnAcceptDrop(DndData const& dnd_data)
{
  for (auto const& uri : dnd_data.Uris())
  {
    glib::Object<GFile> file(g_file_new_for_uri(uri.c_str()));
    glib::Error error;

    /* Log ZG event when moving file to trash; this is requred by File Scope.
       See https://bugs.launchpad.net/unity/+bug/870150  */
    if (g_file_trash(file, cancellable_, &error))
    {
      auto const& unity_app = ApplicationManager::Default().GetUnityApplication();
      auto subject = std::make_shared<desktop::ApplicationSubject>();
      subject->uri = uri;
      subject->origin = glib::String(g_path_get_dirname(uri.c_str())).Str();
      glib::String parse_name(g_file_get_parse_name(file));
      subject->text = glib::String(g_path_get_basename(parse_name)).Str();
      unity_app->LogEvent(ApplicationEventType::DELETE, subject);
    }
    else
    {
      LOG_ERROR(logger) << "Impossible to trash file '" << uri << "': " << error;
    }
  }

  SetQuirk(LauncherIcon::Quirk::PULSE_ONCE, true);
}

std::string TrashLauncherIcon::GetName() const
{
  return "TrashLauncherIcon";
}

} // namespace launcher
} // namespace unity
