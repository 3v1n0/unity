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
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include "config.h"

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/DesktopUtilities.h>

#include "ApplicationLauncherIcon.h"
#include "FavoriteStore.h"
#include "unity-shared/DesktopApplicationManager.h"

#include <glib/gi18n-lib.h>
#include <gio/gdesktopappinfo.h>

namespace unity
{
namespace launcher
{
namespace
{
DECLARE_LOGGER(logger, "unity.launcher.icon.application");

// We use the "application-" prefix since the manager is protected, to avoid name clash
const std::string DEFAULT_ICON = "application-default-icon";

enum MenuItemType
{
  STICK = 0,
  QUIT,
  APP_NAME,
  SEPARATOR,
  SIZE
};
}

NUX_IMPLEMENT_OBJECT_TYPE(ApplicationLauncherIcon);

ApplicationLauncherIcon::ApplicationLauncherIcon(ApplicationPtr const& app)
  : WindowedLauncherIcon(IconType::APPLICATION)
  , startup_notification_timestamp_(0)
  , use_custom_bg_color_(false)
  , bg_color_(nux::color::White)
{
  LOG_INFO(logger) << "Created ApplicationLauncherIcon: "
    << tooltip_text()
    << ", icon: " << icon_name()
    << ", sticky: " << (app->sticky() ? "yes" : "no")
    << ", visible: " << (app->visible() ? "yes" : "no")
    << ", active: " << (app->active() ? "yes" : "no")
    << ", running: " << (app->running() ? "yes" : "no");

  SetApplication(app);
  EnsureWindowsLocation();
}

ApplicationLauncherIcon::~ApplicationLauncherIcon()
{
  UnsetApplication();
}

ApplicationPtr ApplicationLauncherIcon::GetApplication() const
{
  return app_;
}

void ApplicationLauncherIcon::SetApplication(ApplicationPtr const& app)
{
  if (app_ == app)
    return;

  if (!app)
  {
    Remove();
    return;
  }

  bool was_sticky = IsSticky();
  UnsetApplication();

  app_ = app;
  app_->seen = true;
  SetupApplicationSignalsConnections();

  // Let's update the icon properties to match the new application ones
  app_->title.changed.emit(app_->title());
  app_->icon.changed.emit(app_->icon());
  app_->visible.changed.emit(app_->visible());
  app_->active.changed.emit(app_->active());
  app_->running.changed.emit(app_->running());
  app_->urgent.changed.emit(app_->urgent());
  app_->starting.changed.emit(app_->starting() || GetQuirk(Quirk::STARTING));
  app_->desktop_file.changed.emit(app_->desktop_file());

  // Make sure we set the LauncherIcon stick bit too...
  if (app_->sticky() || was_sticky)
    Stick(false); // don't emit the signal
}

void ApplicationLauncherIcon::UnsetApplication()
{
  if (!app_ || removed())
    return;

  /* Removing the unity-seen flag to the wrapped bamf application, on remove
   * request we make sure that if the application is re-opened while the
   * removal process is still ongoing, the application will be shown on the
   * launcher. Disconnecting from signals we make sure that this icon won't be
   * updated or will change visibility (no duplicated icon). */

  signals_conn_.Clear();
  app_->sticky = false;
  app_->seen = false;
}

void ApplicationLauncherIcon::SetupApplicationSignalsConnections()
{
  // Lambda functions should be fine here because when the application the icon
  // is only ever removed when the application is closed.
  signals_conn_.Add(app_->window_opened.connect([this](ApplicationWindowPtr const& win) {
    signals_conn_.Add(win->monitor.changed.connect([this] (int) { EnsureWindowsLocation(); }));
    EnsureWindowsLocation();
  }));

  signals_conn_.Add(app_->window_closed.connect([this] (ApplicationWindowPtr const&) { EnsureWindowsLocation(); }));

  for (auto& win : app_->GetWindows())
    signals_conn_.Add(win->monitor.changed.connect([this] (int) { EnsureWindowsLocation(); }));

  signals_conn_.Add(app_->urgent.changed.connect([this](bool urgent) {
    LOG_DEBUG(logger) << tooltip_text() << " urgent now " << (urgent ? "true" : "false");
    SetQuirk(Quirk::URGENT, urgent);
  }));

  signals_conn_.Add(app_->starting.changed.connect([this](bool starting) {
    LOG_DEBUG(logger) << tooltip_text() << " starting now " << (starting ? "true" : "false");
    SetQuirk(Quirk::STARTING, starting);
  }));

  signals_conn_.Add(app_->active.changed.connect([this](bool active) {
    LOG_DEBUG(logger) << tooltip_text() << " active now " << (active ? "true" : "false");
    SetQuirk(Quirk::ACTIVE, active);
  }));

  signals_conn_.Add(app_->desktop_file.changed.connect([this](std::string const& desktop_file) {
    LOG_DEBUG(logger) << tooltip_text() << " desktop_file now " << desktop_file;
    UpdateDesktopFile();
  }));

  signals_conn_.Add(app_->title.changed.connect([this](std::string const& name) {
    LOG_DEBUG(logger) << tooltip_text() << " name now " << name;
    if (menu_items_.size() == MenuItemType::SIZE)
      menu_items_[MenuItemType::APP_NAME] = nullptr;
    tooltip_text = name;
  }));

  signals_conn_.Add(app_->icon.changed.connect([this](std::string const& icon) {
    LOG_DEBUG(logger) << tooltip_text() << " icon now " << icon;
    icon_name = (icon.empty() ? DEFAULT_ICON : icon);
  }));

  signals_conn_.Add(app_->running.changed.connect([this](bool running) {
    LOG_DEBUG(logger) << tooltip_text() << " running now " << (running ? "true" : "false");
    SetQuirk(Quirk::RUNNING, running);

    if (running)
    {
      _source_manager.Remove(ICON_REMOVE_TIMEOUT);

      EnsureWindowState();
      UpdateIconGeometries(GetCenters());
    }
  }));

  signals_conn_.Add(app_->visible.changed.connect([this](bool visible) {
    SetQuirk(Quirk::VISIBLE, IsSticky() ? true : visible);
  }));

  signals_conn_.Add(app_->closed.connect([this] {
    LOG_DEBUG(logger) << tooltip_text() << " closed";
    OnApplicationClosed();
  }));
}

WindowList ApplicationLauncherIcon::GetManagedWindows() const
{
  return app_ ? app_->GetWindows() : WindowList();
}

void ApplicationLauncherIcon::OnApplicationClosed()
{
  if (IsSticky())
    return;

  SetQuirk(Quirk::VISIBLE, false);
  HideTooltip();

  /* Use a timeout to remove the icon, this avoids
   * that we remove an application that is going
   * to be reopened soon. So applications that
   * have a splash screen won't be removed from
   * the launcher while the splash is closed and
   * a new window is opened. */
  _source_manager.AddTimeoutSeconds(1, [this] {
    Remove();
    return false;
  }, ICON_REMOVE_TIMEOUT);
}

// Move to WindowedLauncherIcon?!
bool ApplicationLauncherIcon::GetQuirk(AbstractLauncherIcon::Quirk quirk, int monitor) const
{
  if (quirk == Quirk::ACTIVE)
  {
    if (!WindowedLauncherIcon::GetQuirk(Quirk::ACTIVE, monitor))
      return false;

    if (app_->type() == AppType::WEBAPP)
      return true;

    // Sometimes BAMF is not fast enough to update the active application
    // while quickly switching between apps, so we double check that the
    // real active window is part of the selection (see bug #1111620)
    return app_->OwnsWindow(WindowManager::Default().GetActiveWindow());
  }

  return WindowedLauncherIcon::GetQuirk(quirk, monitor);
}

void ApplicationLauncherIcon::Remove()
{
  LogUnityEvent(ApplicationEventType::LEAVE);
  UnsetApplication();
  WindowedLauncherIcon::Remove();
}

bool ApplicationLauncherIcon::IsSticky() const
{
  if (app_)
    return app_->sticky() && WindowedLauncherIcon::IsSticky();

  return false;
}

bool ApplicationLauncherIcon::IsUserVisible() const
{
  return app_ ? app_->visible() : false;
}

void ApplicationLauncherIcon::UpdateDesktopFile()
{
  std::string const& filename = app_->desktop_file();

  if (desktop_file_monitor_)
    glib_signals_.Disconnect(desktop_file_monitor_, "changed");

  auto old_uri = RemoteUri();
  UpdateRemoteUri();
  UpdateDesktopQuickList();
  UpdateBackgroundColor();
  auto const& new_uri = RemoteUri();

  if (!filename.empty())
  {
    // add a file watch to the desktop file so that if/when the app is removed
    // we can remove ourself from the launcher and when it's changed
    // we can update the quicklist.
    glib::Object<GFile> desktop_file(g_file_new_for_path(filename.c_str()));
    desktop_file_monitor_ = g_file_monitor_file(desktop_file, G_FILE_MONITOR_NONE,
                                                nullptr, nullptr);
    g_file_monitor_set_rate_limit(desktop_file_monitor_, 2000);

    glib_signals_.Add<void, GFileMonitor*, GFile*, GFile*, GFileMonitorEvent>(desktop_file_monitor_, "changed",
      [this, desktop_file] (GFileMonitor*, GFile*,  GFile*, GFileMonitorEvent event_type) {
      switch (event_type)
      {
        case G_FILE_MONITOR_EVENT_DELETED:
        {
          _source_manager.AddTimeoutSeconds(1, [this, desktop_file] {
            if (!g_file_query_exists(desktop_file, nullptr))
            {
              UnStick();
              LogUnityEvent(ApplicationEventType::DELETE);
            }
            return false;
          });
          break;
        }
        case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
        {
          UpdateDesktopQuickList();
          UpdateBackgroundColor();
          break;
        }
        default:
          break;
      }
    });
  }
  else if (app_->sticky())
  {
    UnStick();
  }

  if (old_uri != new_uri)
  {
    bool update_saved_uri = (!filename.empty() && app_->sticky());

    if (update_saved_uri)
      WindowedLauncherIcon::UnStick();

    uri_changed.emit(new_uri);

    if (update_saved_uri)
      Stick();
  }
}

std::string ApplicationLauncherIcon::DesktopFile() const
{
  return app_->desktop_file();
}

void ApplicationLauncherIcon::OpenInstanceWithUris(std::set<std::string> const& uris, Time timestamp)
{
  glib::Error error;
  glib::Object<GDesktopAppInfo> desktopInfo(g_desktop_app_info_new_from_filename(DesktopFile().c_str()));
  auto appInfo = glib::object_cast<GAppInfo>(desktopInfo);

  GdkDisplay* display = gdk_display_get_default();
  glib::Object<GdkAppLaunchContext> app_launch_context(gdk_display_get_app_launch_context(display));

  startup_notification_timestamp_ = timestamp;
  if (startup_notification_timestamp_ > 0)
    gdk_app_launch_context_set_timestamp(app_launch_context, startup_notification_timestamp_);

  if (g_app_info_supports_uris(appInfo))
  {
    GList* list = nullptr;

    for (auto  it : uris)
      list = g_list_prepend(list, g_strdup(it.c_str()));

    g_app_info_launch_uris(appInfo, list, glib::object_cast<GAppLaunchContext>(app_launch_context), &error);
    g_list_free_full(list, g_free);
  }
  else if (g_app_info_supports_files(appInfo))
  {
    GList* list = nullptr;

    for (auto it : uris)
    {
      GFile* file = g_file_new_for_uri(it.c_str());
      list = g_list_prepend(list, file);
    }

    g_app_info_launch(appInfo, list, glib::object_cast<GAppLaunchContext>(app_launch_context), &error);
    g_list_free_full(list, g_object_unref);
  }
  else
  {
    g_app_info_launch(appInfo, nullptr, glib::object_cast<GAppLaunchContext>(app_launch_context), &error);
  }

  if (error)
  {
    LOG_WARN(logger) << error;
  }

  FullyAnimateQuirk(Quirk::STARTING);
}

void ApplicationLauncherIcon::OpenInstanceLauncherIcon(Time timestamp)
{
  std::set<std::string> empty;
  OpenInstanceWithUris(empty, timestamp);
}

void ApplicationLauncherIcon::Focus(ActionArg arg)
{
  ApplicationWindowPtr window = app_->GetFocusableWindow();
  if (window)
  {
    // If we have a window, try to focus it.
    if (window->Focus())
      return;
  }
  else if (app_->type() == AppType::WEBAPP)
  {
    // Webapps are again special.
    OpenInstanceLauncherIcon(arg.timestamp);
    return;
  }

  bool show_only_visible = arg.source == ActionArg::Source::SWITCHER;
  app_->Focus(show_only_visible, arg.monitor);
}

void ApplicationLauncherIcon::UpdateDesktopQuickList()
{
  std::string const& desktop_file = DesktopFile();

  if (menu_desktop_shortcuts_)
  {
    for (GList *l = dbusmenu_menuitem_get_children(menu_desktop_shortcuts_); l; l = l->next)
    {
      glib_signals_.Disconnect(l->data, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED);
    }

    menu_desktop_shortcuts_ = nullptr;
  }

  if (desktop_file.empty())
    return;

  menu_desktop_shortcuts_ = dbusmenu_menuitem_new();
  dbusmenu_menuitem_set_root(menu_desktop_shortcuts_, TRUE);

  // Build a desktop shortcuts object and tell it that our
  // environment is Unity to handle the filtering
  desktop_shortcuts_ = indicator_desktop_shortcuts_new(desktop_file.c_str(), "Unity");
  // This will get us a list of the nicks available, it should
  // always be at least one entry of NULL if there either aren't
  // any or they're filtered for the environment we're in
  const gchar** nicks = indicator_desktop_shortcuts_get_nicks(desktop_shortcuts_);

  for (int index = 0; nicks[index]; ++index)
  {
    // Build a dbusmenu item for each nick that is the desktop
    // file that is built from it's name and includes a callback
    // to the desktop shortcuts object to execute the nick
    glib::String name(indicator_desktop_shortcuts_nick_get_name(desktop_shortcuts_,
                                                                nicks[index]));
    glib::Object<DbusmenuMenuitem> item(dbusmenu_menuitem_new());
    dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, name);
    dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
    dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
    auto nick = glib::gchar_to_string(nicks[index]);

    glib_signals_.Add<void, DbusmenuMenuitem*, gint>(item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
    [this, nick] (DbusmenuMenuitem* item, unsigned timestamp) {
      GdkDisplay* display = gdk_display_get_default();
      glib::Object<GdkAppLaunchContext> context(gdk_display_get_app_launch_context(display));
      gdk_app_launch_context_set_timestamp(context, timestamp);
      auto gcontext = glib::object_cast<GAppLaunchContext>(context);
      indicator_desktop_shortcuts_nick_exec_with_context(desktop_shortcuts_, nick.c_str(), gcontext);
    });

    dbusmenu_menuitem_child_append(menu_desktop_shortcuts_, item);
  }
}

void ApplicationLauncherIcon::UpdateBackgroundColor()
{
  bool last_use_custom_bg_color = use_custom_bg_color_;
  nux::Color last_bg_color(bg_color_);

  std::string const& color = DesktopUtilities::GetBackgroundColor(DesktopFile());

  use_custom_bg_color_ = !color.empty();

  if (use_custom_bg_color_)
    bg_color_ = nux::Color(color);

  if (last_use_custom_bg_color != use_custom_bg_color_ ||
      last_bg_color != bg_color_)
  {
    EmitNeedsRedraw();
  }
}

void ApplicationLauncherIcon::EnsureMenuItemsStaticQuicklist()
{
  // make a client for desktop file actions
  if (!menu_desktop_shortcuts_.IsType(DBUSMENU_TYPE_MENUITEM))
  {
    UpdateDesktopQuickList();
  }
}

void ApplicationLauncherIcon::Quit() const
{
  app_->Quit();
}

void ApplicationLauncherIcon::AboutToRemove()
{
  UnStick();
  Quit();
}

void ApplicationLauncherIcon::Stick(bool save)
{
  if (IsSticky() && !save)
    return;

  app_->sticky = true;

  if (RemoteUri().empty())
  {
    if (save)
      app_->CreateLocalDesktopFile();
  }
  else
  {
    WindowedLauncherIcon::Stick(save);

    if (save)
      LogUnityEvent(ApplicationEventType::ACCESS);
  }
}

void ApplicationLauncherIcon::UnStick()
{
  if (!IsSticky())
    return;

  LogUnityEvent(ApplicationEventType::ACCESS);
  WindowedLauncherIcon::UnStick();
  SetQuirk(Quirk::VISIBLE, app_->visible());
  app_->sticky = false;

  if (!IsRunning())
    Remove();
}

void ApplicationLauncherIcon::ToggleSticky()
{
  if (IsSticky())
  {
    UnStick();
  }
  else
  {
    Stick();
  }
}

void ApplicationLauncherIcon::LogUnityEvent(ApplicationEventType type)
{
  if (RemoteUri().empty())
    return;

  auto const& unity_app = ApplicationManager::Default().GetUnityApplication();
  unity_app->LogEvent(type, GetSubject());
}

ApplicationSubjectPtr ApplicationLauncherIcon::GetSubject()
{
  auto subject = std::make_shared<desktop::ApplicationSubject>();
  subject->uri = RemoteUri();
  subject->current_uri = subject->uri();
  subject->interpretation = ZEITGEIST_NFO_SOFTWARE;
  subject->manifestation = ZEITGEIST_NFO_SOFTWARE_ITEM;
  subject->mimetype = "application/x-desktop";
  subject->text = tooltip_text();

  return subject;
}

void ApplicationLauncherIcon::EnsureMenuItemsDefaultReady()
{
  if (menu_items_.size() == MenuItemType::SIZE)
    return;

  menu_items_.resize(MenuItemType::SIZE);

  /* (Un)Stick to Launcher */
  glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());
  const char* label = !IsSticky() ? _("Lock to Launcher") : _("Unlock from Launcher");
  dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, label);
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

  glib_signals_.Add<void, DbusmenuMenuitem*, unsigned>(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
    [this] (DbusmenuMenuitem*, unsigned) {
      ToggleSticky();
  });

  menu_items_[MenuItemType::STICK] = menu_item;

  /* Quit */
  menu_item = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Quit"));
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

  glib_signals_.Add<void, DbusmenuMenuitem*, unsigned>(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
    [this] (DbusmenuMenuitem*, unsigned) {
      Quit();
  });

  menu_items_[MenuItemType::QUIT] = menu_item;

  /* Separator */
  menu_item = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_CLIENT_TYPES_SEPARATOR);
  menu_items_[MenuItemType::SEPARATOR] = menu_item;
}

AbstractLauncherIcon::MenuItemsVector ApplicationLauncherIcon::GetMenus()
{
  MenuItemsVector result;
  glib::Object<DbusmenuMenuitem> quit_item;
  bool separator_needed = false;

  EnsureMenuItemsDefaultReady();
  EnsureMenuItemsStaticQuicklist();

  for (auto const& menus : {GetRemoteMenus(), menu_desktop_shortcuts_})
  {
    if (!menus.IsType(DBUSMENU_TYPE_MENUITEM))
      continue;

    for (GList* l = dbusmenu_menuitem_get_children(menus); l; l = l->next)
    {
      glib::Object<DbusmenuMenuitem> item(static_cast<DbusmenuMenuitem*>(l->data), glib::AddRef());

      if (!item.IsType(DBUSMENU_TYPE_MENUITEM))
        continue;

      if (dbusmenu_menuitem_property_get_bool(item, DBUSMENU_MENUITEM_PROP_VISIBLE))
      {
        dbusmenu_menuitem_property_set_bool(item, QuicklistMenuItem::MARKUP_ENABLED_PROPERTY, FALSE);

        const gchar* type = dbusmenu_menuitem_property_get(item, DBUSMENU_MENUITEM_PROP_TYPE);

        if (!type) // (g_strcmp0 (type, DBUSMENU_MENUITEM_PROP_LABEL) == 0)
        {
          if (dbusmenu_menuitem_property_get_bool(item, QuicklistMenuItem::QUIT_ACTION_PROPERTY))
          {
            quit_item = item;
            continue;
          }

          const gchar* l = dbusmenu_menuitem_property_get(item, DBUSMENU_MENUITEM_PROP_LABEL);
          auto const& label = glib::gchar_to_string(l);

          if (label == _("Quit")  || label == "Quit"  ||
              label == _("Exit")  || label == "Exit"  ||
              label == _("Close") || label == "Close")
          {
            quit_item = item;
            continue;
          }
        }

        separator_needed = true;
        result.push_back(item);
      }
    }
  }

  if (separator_needed)
  {
    result.push_back(menu_items_[MenuItemType::SEPARATOR]);
    separator_needed = false;
  }

  if (!menu_items_[MenuItemType::APP_NAME])
  {
    glib::String app_name(g_markup_escape_text(app_->title().c_str(), -1));
    std::string bold_app_name("<b>"+app_name.Str()+"</b>");

    glib::Object<DbusmenuMenuitem> item(dbusmenu_menuitem_new());
    dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, bold_app_name.c_str());
    dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_ACCESSIBLE_DESC, app_name.Str().c_str());
    dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
    dbusmenu_menuitem_property_set_bool(item, QuicklistMenuItem::MARKUP_ENABLED_PROPERTY, TRUE);

    glib_signals_.Add<void, DbusmenuMenuitem*, unsigned>(item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
      [this] (DbusmenuMenuitem*, unsigned timestamp) {
        _source_manager.AddIdle([this, timestamp] {
          ActivateLauncherIcon(ActionArg(ActionArg::Source::LAUNCHER, 0, timestamp));
          return false;
        });
    });

    menu_items_[MenuItemType::APP_NAME] = item;
  }

  result.push_back(menu_items_[MenuItemType::APP_NAME]);
  result.push_back(menu_items_[MenuItemType::SEPARATOR]);

  auto const& windows_menu_items = GetWindowsMenuItems();

  if (!windows_menu_items.empty())
  {
    result.insert(end(result), begin(windows_menu_items), end(windows_menu_items));
    result.push_back(menu_items_[MenuItemType::SEPARATOR]);
  }

  const char* label = !IsSticky() ? _("Lock to Launcher") : _("Unlock from Launcher");
  dbusmenu_menuitem_property_set(menu_items_[MenuItemType::STICK], DBUSMENU_MENUITEM_PROP_LABEL, label);
  result.push_back(menu_items_[MenuItemType::STICK]);

  if (IsRunning())
  {
    if (DesktopFile().empty() && !IsSticky())
    {
      /* Add to Dash */
      glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());
      dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Add to Dash"));
      dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
      dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

      glib_signals_.Add<void, DbusmenuMenuitem*, unsigned>(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
        [this] (DbusmenuMenuitem*, unsigned) {
          app_->CreateLocalDesktopFile();
      });

      result.push_back(menu_item);
    }

    if (!quit_item)
      quit_item = menu_items_[MenuItemType::QUIT];

    dbusmenu_menuitem_property_set(quit_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Quit"));
    result.push_back(quit_item);
  }

  return result;
}

void ApplicationLauncherIcon::UpdateIconGeometries(std::vector<nux::Point3> const& centers)
{
  if (app_->type() == AppType::WEBAPP)
    return;

  return WindowedLauncherIcon::UpdateIconGeometries(centers);
}

void ApplicationLauncherIcon::UpdateRemoteUri()
{
  std::string const& desktop_id = app_->desktop_id();

  if (!desktop_id.empty())
  {
    remote_uri_ = FavoriteStore::URI_PREFIX_APP + desktop_id;
  }
  else
  {
    remote_uri_.clear();
  }
}

std::string ApplicationLauncherIcon::GetRemoteUri() const
{
  return remote_uri_;
}

bool ApplicationLauncherIcon::OnShouldHighlightOnDrag(DndData const& dnd_data)
{
  for (auto type : dnd_data.Types())
  {
    for (auto supported_type : GetSupportedTypes())
    {
      if (g_content_type_is_a(type.c_str(), supported_type.c_str()))
      {
        if (!dnd_data.UrisByType(type).empty())
          return true;
      }
    }
  }

  return false;
}

nux::DndAction ApplicationLauncherIcon::OnQueryAcceptDrop(DndData const& dnd_data)
{
#ifdef USE_X11
  return dnd_data.Uris().empty() ? nux::DNDACTION_NONE : nux::DNDACTION_COPY;
#else
  return nux::DNDACTION_NONE;
#endif
}

void ApplicationLauncherIcon::OnAcceptDrop(DndData const& dnd_data)
{
  auto timestamp = nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp;
  OpenInstanceWithUris(dnd_data.Uris(), timestamp);
}

bool ApplicationLauncherIcon::AllowDetailViewInSwitcher() const
{
  return app_->type() != AppType::WEBAPP;
}

uint64_t ApplicationLauncherIcon::SwitcherPriority()
{
  // Webapps always go at the back.
  if (app_->type() == AppType::WEBAPP)
    return 0;

  return WindowedLauncherIcon::SwitcherPriority();
}

nux::Color ApplicationLauncherIcon::BackgroundColor() const
{
  if (use_custom_bg_color_)
    return bg_color_;

  return WindowedLauncherIcon::BackgroundColor();
}

const std::set<std::string> ApplicationLauncherIcon::GetSupportedTypes()
{
  std::set<std::string> supported_types;

  for (auto& type : app_->GetSupportedMimeTypes())
  {
    unity::glib::String super_type(g_content_type_from_mime_type(type.c_str()));
    supported_types.insert(super_type.Str());
  }

  return supported_types;
}

std::string ApplicationLauncherIcon::GetName() const
{
  return "ApplicationLauncherIcon";
}

void ApplicationLauncherIcon::AddProperties(debug::IntrospectionData& introspection)
{
  WindowedLauncherIcon::AddProperties(introspection);

  introspection.add("desktop_file", DesktopFile())
               .add("desktop_id", app_->desktop_id());
}

} // namespace launcher
} // namespace unity
