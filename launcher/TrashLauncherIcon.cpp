// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2015 Canonical Ltd
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
#include <UnityCore/DesktopUtilities.h>
#include <zeitgeist.h>

#include "QuicklistMenuItemLabel.h"
#include "unity-shared/DesktopApplicationManager.h"
#include "unity-shared/GnomeFileManager.h"

namespace unity
{
namespace launcher
{
namespace
{
  DECLARE_LOGGER(logger, "unity.launcher.icon.trash");
  const std::string ZEITGEIST_UNITY_ACTOR = "application://compiz.desktop";
  const std::string TRASH_URI = "trash:";
  const std::string TRASH_PATH = "file://" + DesktopUtilities::GetUserTrashDirectory();
}

TrashLauncherIcon::TrashLauncherIcon(FileManager::Ptr const& fm)
  : WindowedLauncherIcon(IconType::TRASH)
  , StorageLauncherIcon(GetIconType(), fm ? fm : GnomeFileManager::Get())
  , empty_(true)
{
  tooltip_text = _("Trash");
  icon_name = "user-trash";
  position = Position::END;
  SetQuirk(Quirk::VISIBLE, true);
  SkipQuirkAnimation(Quirk::VISIBLE);
  SetShortcut('t');

  _source_manager.AddIdle([this]{
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
      glib_signals_.Add<void, GFileMonitor*, GFile*, GFile*, GFileMonitorEvent>(trash_monitor_, "changed",
      [this] (GFileMonitor*, GFile*, GFile*, GFileMonitorEvent) {
        UpdateTrashIcon();
      });
    }

    return false;
  });

  UpdateTrashIcon();
  UpdateStorageWindows();
}

WindowList TrashLauncherIcon::GetStorageWindows() const
{
  auto windows = file_manager_->WindowsForLocation(TRASH_URI);
  auto const& path_wins = file_manager_->WindowsForLocation(TRASH_PATH);
  windows.insert(end(windows), begin(path_wins), end(path_wins));
  return windows;
}

void TrashLauncherIcon::OpenInstanceLauncherIcon(Time timestamp)
{
  file_manager_->OpenTrash(timestamp);
}

AbstractLauncherIcon::MenuItemsVector TrashLauncherIcon::GetMenus()
{
  MenuItemsVector result;

  /* Empty Trash */
  glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());
  dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Empty Trash…"));
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, !empty_);
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
  glib_signals_.Add<void, DbusmenuMenuitem*, unsigned>(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
  [this] (DbusmenuMenuitem*, unsigned timestamp) {
    file_manager_->EmptyTrash(timestamp);
  });

  result.push_back(menu_item);

  if (IsRunning())
  {
    auto const& windows_items = GetWindowsMenuItems();
    if (!windows_items.empty())
    {
      menu_item = dbusmenu_menuitem_new();
      dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_CLIENT_TYPES_SEPARATOR);
      result.push_back(menu_item);

      result.insert(end(result), begin(windows_items), end(windows_items));
    }

    menu_item = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_CLIENT_TYPES_SEPARATOR);
    result.push_back(menu_item);

    menu_item = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Quit"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
    result.push_back(menu_item);

    glib_signals_.Add<void, DbusmenuMenuitem*, unsigned>(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
      Quit();
    });
  }

  return result;
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
    if (file_manager_->TrashFile(uri))
    {
      /* Log ZG event when moving file to trash; this is requred by File Scope.
       * See https://bugs.launchpad.net/unity/+bug/870150 */
      auto const& unity_app = ApplicationManager::Default().GetUnityApplication();
      auto subject = std::make_shared<desktop::ApplicationSubject>();
      subject->uri = uri;
      subject->origin = glib::String(g_path_get_dirname(uri.c_str())).Str();
      glib::Object<GFile> file(g_file_new_for_uri(uri.c_str()));
      glib::String parse_name(g_file_get_parse_name(file));
      subject->text = glib::String(g_path_get_basename(parse_name)).Str();
      unity_app->LogEvent(ApplicationEventType::DELETE, subject);
    }
  }

  SetQuirk(LauncherIcon::Quirk::PULSE_ONCE, true);
  FullyAnimateQuirkDelayed(100, LauncherIcon::Quirk::SHIMMER);
}

std::string TrashLauncherIcon::GetName() const
{
  return "TrashLauncherIcon";
}

} // namespace launcher
} // namespace unity
