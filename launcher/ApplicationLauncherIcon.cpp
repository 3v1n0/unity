// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
#include <boost/algorithm/string.hpp>

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <NuxCore/Logger.h>

#include <UnityCore/Variant.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/DesktopUtilities.h>

#include "ApplicationLauncherIcon.h"
#include "FavoriteStore.h"
#include "MultiMonitor.h"
#include "unity-shared/DesktopApplicationManager.h"
#include "unity-shared/GnomeFileManager.h"
#include "unity-shared/UBusMessages.h"

#include <glib/gi18n-lib.h>
#include <gio/gdesktopappinfo.h>

#include <libbamf/bamf-tab.h>

namespace unity
{
namespace launcher
{
DECLARE_LOGGER(logger, "unity.launcher.icon.application");
namespace
{
// We use the "bamf-" prefix since the manager is protected, to avoid name clash
const std::string WINDOW_MOVE_TIMEOUT = "bamf-window-move";
const std::string ICON_REMOVE_TIMEOUT = "bamf-icon-remove";
//const std::string ICON_DND_OVER_TIMEOUT = "bamf-icon-dnd-over";
const std::string DEFAULT_ICON = "application-default-icon";
const int MAXIMUM_QUICKLIST_WIDTH = 300;

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
  : SimpleLauncherIcon(IconType::APPLICATION)
  , _startup_notification_timestamp(0)
  , _last_scroll_timestamp(0)
  , _last_scroll_direction(ScrollDirection::DOWN)
  , _progressive_scroll(0)
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

  WindowManager& wm = WindowManager::Default();
  wm.window_minimized.connect(sigc::mem_fun(this, &ApplicationLauncherIcon::OnWindowMinimized));
  wm.window_moved.connect(sigc::mem_fun(this, &ApplicationLauncherIcon::OnWindowMoved));
  wm.screen_viewport_switch_ended.connect(sigc::mem_fun(this, &ApplicationLauncherIcon::EnsureWindowState));
  wm.terminate_expo.connect(sigc::mem_fun(this, &ApplicationLauncherIcon::EnsureWindowState));

  EnsureWindowState();
}

ApplicationLauncherIcon::~ApplicationLauncherIcon()
{
  SetApplication(nullptr);
}

ApplicationPtr ApplicationLauncherIcon::GetApplication() const
{
  return app_;
}

void ApplicationLauncherIcon::SetApplication(ApplicationPtr const& app)
{
  if (app_ == app)
    return;

  if (app_)
  {
    app_->sticky = false;
    app_->seen = false;
  }

  signals_conn_.Clear();
  app_ = app;

  if (!app)
    return;

  app_->seen = true;
  SetupApplicationSignalsConnections();

  // Let's update the icon properties to match the new application ones
  app_->title.changed.emit(app_->title());
  app_->icon.changed.emit(app_->icon());
  app_->visible.changed.emit(app_->visible());
  app_->active.changed.emit(app_->active());
  app_->running.changed.emit(app_->running());
  app_->desktop_file.changed.emit(app_->desktop_file());

  // Make sure we set the LauncherIcon stick bit too...
  if (app_->sticky())
    Stick(false); // don't emit the signal
}

void ApplicationLauncherIcon::SetupApplicationSignalsConnections()
{
  // Lambda functions should be fine here because when the application the icon
  // is only ever removed when the application is closed.
  signals_conn_.Add(app_->window_opened.connect([this](ApplicationWindow const&) {
    EnsureWindowState();
    UpdateIconGeometries(GetCenters());
  }));

  signals_conn_.Add(app_->window_closed.connect(sigc::mem_fun(this, &ApplicationLauncherIcon::EnsureWindowState)));
  signals_conn_.Add(app_->window_moved.connect(sigc::hide(sigc::mem_fun(this, &ApplicationLauncherIcon::EnsureWindowState))));

  signals_conn_.Add(app_->urgent.changed.connect([this](bool const& urgent) {
    LOG_DEBUG(logger) << tooltip_text() << " urgent now " << (urgent ? "true" : "false");
    SetQuirk(Quirk::URGENT, urgent);
  }));

  signals_conn_.Add(app_->active.changed.connect([this](bool const& active) {
    LOG_DEBUG(logger) << tooltip_text() << " active now " << (active ? "true" : "false");
    SetQuirk(Quirk::ACTIVE, active);
  }));

  signals_conn_.Add(app_->desktop_file.changed.connect([this](std::string const& desktop_file) {
    LOG_DEBUG(logger) << tooltip_text() << " desktop_file now " << desktop_file;
    UpdateDesktopFile();
  }));

  signals_conn_.Add(app_->title.changed.connect([this](std::string const& name) {
    LOG_DEBUG(logger) << tooltip_text() << " name now " << name;
    if (_menu_items.size() == MenuItemType::SIZE)
      _menu_items[MenuItemType::APP_NAME] = nullptr;
    tooltip_text = name;
  }));

  signals_conn_.Add(app_->icon.changed.connect([this](std::string const& icon) {
    LOG_DEBUG(logger) << tooltip_text() << " icon now " << icon;
    icon_name = (icon.empty() ? DEFAULT_ICON : icon);
  }));

  signals_conn_.Add(app_->running.changed.connect([this](bool const& running) {
    LOG_DEBUG(logger) << tooltip_text() << " running now " << (running ? "true" : "false");
    SetQuirk(Quirk::RUNNING, running);

    if (running)
    {
      _source_manager.Remove(ICON_REMOVE_TIMEOUT);

      EnsureWindowState();
      UpdateIconGeometries(GetCenters());
    }
  }));

  signals_conn_.Add(app_->visible.changed.connect([this](bool const& visible) {
    SetQuirk(Quirk::VISIBLE, IsSticky() ? true : visible);
  }));

  signals_conn_.Add(app_->closed.connect([this]() {
    if (!IsSticky())
    {
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
  }));
}

bool ApplicationLauncherIcon::GetQuirk(AbstractLauncherIcon::Quirk quirk) const
{
  if (quirk == Quirk::ACTIVE)
  {
    if (!SimpleLauncherIcon::GetQuirk(Quirk::ACTIVE))
      return false;

    if (app_->type() == "webapp")
      return true;

    // Sometimes BAMF is not fast enough to update the active application
    // while quickly switching between apps, so we double check that the
    // real active window is part of the selection (see bug #1111620)
    return app_->OwnsWindow(WindowManager::Default().GetActiveWindow());
  }

  return SimpleLauncherIcon::GetQuirk(quirk);
}

void ApplicationLauncherIcon::Remove()
{
  LogUnityEvent(ApplicationEventType::LEAVE);
  /* Removing the unity-seen flag to the wrapped bamf application, on remove
   * request we make sure that if the application is re-opened while the
   * removal process is still ongoing, the application will be shown on the
   * launcher. Disconnecting from signals we make sure that this icon won't be
   * reused (no duplicated icon). */
  app_->seen = false;
  // Disconnect all our callbacks.
  notify_callbacks(); // This is from sigc++::trackable
  SimpleLauncherIcon::Remove();
}

bool ApplicationLauncherIcon::IsSticky() const
{
  if (app_)
    return app_->sticky() && SimpleLauncherIcon::IsSticky();

  return false;
}

bool ApplicationLauncherIcon::IsActive() const
{
  return GetQuirk(Quirk::ACTIVE);
}

bool ApplicationLauncherIcon::IsRunning() const
{
  return GetQuirk(Quirk::RUNNING);
}

bool ApplicationLauncherIcon::IsUrgent() const
{
  return GetQuirk(Quirk::URGENT);
}

void ApplicationLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  SimpleLauncherIcon::ActivateLauncherIcon(arg);
  WindowManager& wm = WindowManager::Default();

  // This is a little awkward as the target is only set from the switcher.
  if (arg.target)
  {
    // thumper: should we Raise too? should the WM raise?
    wm.Activate(arg.target);
    return;
  }

  bool scaleWasActive = wm.IsScaleActive();
  if (scaleWasActive)
  {
    wm.TerminateScale();
  }

  bool active = IsActive();
  bool user_visible = IsRunning();
  /* We should check each child to see if there is
   * an unmapped (!= minimized) window around and
   * if so force "Focus" behaviour */

  if (arg.source != ActionArg::Source::SWITCHER)
  {
    user_visible = app_->visible();

    if (active)
    {
      bool any_visible = false;
      bool any_mapped = false;
      bool any_on_top = false;
      bool any_on_monitor = (arg.monitor < 0);
      int active_monitor = arg.monitor;

      for (auto const& window : app_->GetWindows())
      {
        Window xid = window->window_id();

        if (!any_visible && wm.IsWindowOnCurrentDesktop(xid))
        {
          any_visible = true;
        }

        if (!any_mapped && wm.IsWindowMapped(xid))
        {
          any_mapped = true;
        }

        if (!any_on_top && wm.IsWindowOnTop(xid))
        {
          any_on_top = true;
        }

        if (!any_on_monitor && window->monitor() == arg.monitor &&
            wm.IsWindowMapped(xid) && wm.IsWindowVisible(xid))
        {
          any_on_monitor = true;
        }

        if (window->active())
        {
          active_monitor = window->monitor();
        }
      }

      if (!any_visible || !any_mapped || !any_on_top)
        active = false;

      if (any_on_monitor && arg.monitor >= 0 && active_monitor != arg.monitor)
        active = false;
    }

    if (user_visible && IsSticky() && IsFileManager())
    {
      // See bug #753938
      unsigned minimum_windows = 0;
      auto const& file_manager = GnomeFileManager::Get();

      if (file_manager->IsTrashOpened())
        ++minimum_windows;

      if (file_manager->IsDeviceOpened())
        ++minimum_windows;

      if (minimum_windows > 0)
      {
        if (file_manager->OpenedLocations().size() == minimum_windows &&
            GetWindows(WindowFilter::USER_VISIBLE|WindowFilter::MAPPED).size() == minimum_windows)
        {
          user_visible = false;
        }
      }
    }
  }

  /* Behaviour:
   * 1) Nothing running, or nothing visible -> launch application
   * 2) Running and active -> spread application
   * 3) Running and not active -> focus application
   * 4) Spread is active and different icon pressed -> change spread
   * 5) Spread is active -> Spread de-activated, and fall through
   */

  if (!IsRunning() || (IsRunning() && !user_visible)) // #1 above
  {
    if (GetQuirk(Quirk::STARTING))
      return;

    SetQuirk(Quirk::STARTING, true);
    OpenInstanceLauncherIcon(arg.timestamp);
  }
  else // app is running
  {
    if (active)
    {
      if (scaleWasActive) // #5 above
      {
        Focus(arg);
      }
      else // #2 above
      {
        if (arg.source != ActionArg::Source::SWITCHER)
        {
          Spread(true, 0, false);
        }
      }
    }
    else
    {
      if (scaleWasActive) // #4 above
      {
        Focus(arg);
        if (arg.source != ActionArg::Source::SWITCHER)
          Spread(true, 0, false);
      }
      else // #3 above
      {
        Focus(arg);
      }
    }
  }
}

WindowList ApplicationLauncherIcon::GetWindows(WindowFilterMask filter, int monitor)
{
  WindowManager& wm = WindowManager::Default();
  WindowList results;

  monitor = (filter & WindowFilter::ON_ALL_MONITORS) ? -1 : monitor;
  bool mapped = (filter & WindowFilter::MAPPED);
  bool user_visible = (filter & WindowFilter::USER_VISIBLE);
  bool current_desktop = (filter & WindowFilter::ON_CURRENT_DESKTOP);

  for (auto& window : app_->GetWindows())
  {
    if ((monitor >= 0 && window->monitor() == monitor) || monitor < 0)
    {
      if ((user_visible && window->visible()) || !user_visible)
      {
        Window xid = window->window_id();

        if ((mapped && wm.IsWindowMapped(xid)) || !mapped)
        {
          if ((current_desktop && wm.IsWindowOnCurrentDesktop(xid)) || !current_desktop)
          {
            results.push_back(window);
          }
        }
      }
    }
  }

  return results;
}

WindowList ApplicationLauncherIcon::Windows()
{
  return GetWindows(WindowFilter::MAPPED|WindowFilter::ON_ALL_MONITORS);
}

std::vector<Window> ApplicationLauncherIcon::WindowsOnViewport()
{
  WindowFilterMask filter = 0;
  filter |= WindowFilter::MAPPED;
  filter |= WindowFilter::USER_VISIBLE;
  filter |= WindowFilter::ON_CURRENT_DESKTOP;
  filter |= WindowFilter::ON_ALL_MONITORS;

  std::vector<Window> windows;
  for (auto& window : GetWindows(filter))
  {
    windows.push_back(window->window_id());
  }
  return windows;
}

std::vector<Window> ApplicationLauncherIcon::WindowsForMonitor(int monitor)
{
  WindowFilterMask filter = 0;
  filter |= WindowFilter::MAPPED;
  filter |= WindowFilter::USER_VISIBLE;
  filter |= WindowFilter::ON_CURRENT_DESKTOP;

  std::vector<Window> windows;
  for (auto& window : GetWindows(filter, monitor))
  {
    windows.push_back(window->window_id());
  }
  return windows;
}

void ApplicationLauncherIcon::OnWindowMinimized(guint32 xid)
{
  if (!app_->OwnsWindow(xid))
    return;

  Present(0.5f, 600);
  UpdateQuirkTimeDelayed(300, Quirk::SHIMMER);
}

void ApplicationLauncherIcon::OnWindowMoved(guint32 moved_win)
{
  if (!app_->OwnsWindow(moved_win))
    return;

  _source_manager.AddTimeout(250, [this] {
    EnsureWindowState();
    UpdateIconGeometries(GetCenters());

    return false;
  }, WINDOW_MOVE_TIMEOUT);
}

void ApplicationLauncherIcon::UpdateDesktopFile()
{
  std::string const& filename = app_->desktop_file();

  if (_desktop_file_monitor)
    _gsignals.Disconnect(_desktop_file_monitor, "changed");

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
    _desktop_file_monitor = g_file_monitor_file(desktop_file, G_FILE_MONITOR_NONE,
                                                nullptr, nullptr);
    g_file_monitor_set_rate_limit(_desktop_file_monitor, 2000);

    _gsignals.Add<void, GFileMonitor*, GFile*, GFile*, GFileMonitorEvent>(_desktop_file_monitor, "changed",
      [this] (GFileMonitor*, GFile* f, GFile*, GFileMonitorEvent event_type) {
      switch (event_type)
      {
        case G_FILE_MONITOR_EVENT_DELETED:
        {
          glib::Object<GFile> file(f, glib::AddRef());
          _source_manager.AddTimeoutSeconds(1, [this, file] {
            if (!g_file_query_exists(file, nullptr))
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
      SimpleLauncherIcon::UnStick();

    uri_changed.emit(new_uri);

    if (update_saved_uri)
      Stick();
  }
}

std::string ApplicationLauncherIcon::DesktopFile() const
{
  return app_->desktop_file();
}

void ApplicationLauncherIcon::AddProperties(GVariantBuilder* builder)
{
  SimpleLauncherIcon::AddProperties(builder);

  GVariantBuilder xids_builder;
  g_variant_builder_init(&xids_builder, G_VARIANT_TYPE ("au"));

  for (auto const& window : GetWindows())
    g_variant_builder_add(&xids_builder, "u", window->window_id());

  variant::BuilderWrapper(builder)
    .add("desktop_file", DesktopFile())
    .add("desktop_id", app_->desktop_id())
    .add("xids", g_variant_builder_end(&xids_builder))
    .add("sticky", IsSticky())
    .add("startup_notification_timestamp", (uint64_t)_startup_notification_timestamp);
}

void ApplicationLauncherIcon::OpenInstanceWithUris(std::set<std::string> const& uris, Time timestamp)
{
  glib::Error error;
  glib::Object<GDesktopAppInfo> desktopInfo(g_desktop_app_info_new_from_filename(DesktopFile().c_str()));
  auto appInfo = glib::object_cast<GAppInfo>(desktopInfo);

  GdkDisplay* display = gdk_display_get_default();
  glib::Object<GdkAppLaunchContext> app_launch_context(gdk_display_get_app_launch_context(display));

  _startup_notification_timestamp = timestamp;
  if (_startup_notification_timestamp > 0)
    gdk_app_launch_context_set_timestamp(app_launch_context, _startup_notification_timestamp);

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

  UpdateQuirkTime(Quirk::STARTING);
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
  else if (app_->type() == "webapp")
  {
    // Webapps are again special.
    OpenInstanceLauncherIcon(arg.timestamp);
    return;
  }

  bool show_only_visible = arg.source == ActionArg::Source::SWITCHER;
  app_->Focus(show_only_visible, arg.monitor);
}

bool ApplicationLauncherIcon::Spread(bool current_desktop, int state, bool force)
{
  std::vector<Window> windows;
  for (auto& window : GetWindows(current_desktop ? WindowFilter::ON_CURRENT_DESKTOP : 0))
  {
    windows.push_back(window->window_id());
  }
  return WindowManager::Default().ScaleWindowGroup(windows, state, force);
}

void ApplicationLauncherIcon::EnsureWindowState()
{
  std::vector<bool> monitors(monitors::MAX);

  for (auto& window: app_->GetWindows())
  {
    int monitor = window->monitor();
    Window window_id = window->window_id();

    if (WindowManager::Default().IsWindowOnCurrentDesktop(window_id))
    {
      // If monitor is -1 (or negative), show on all monitors.
      if (monitor < 0)
      {
        for (unsigned j = 0; j < monitors::MAX; j++)
          monitors[j] = true;
      }
      else
        monitors[monitor] = true;
    }
  }

  for (unsigned i = 0; i < monitors::MAX; i++)
    SetWindowVisibleOnMonitor(monitors[i], i);

  EmitNeedsRedraw();
}

void ApplicationLauncherIcon::UpdateDesktopQuickList()
{
  std::string const& desktop_file = DesktopFile();

  if (_menu_desktop_shortcuts)
  {
    for (GList *l = dbusmenu_menuitem_get_children(_menu_desktop_shortcuts); l; l = l->next)
    {
      _gsignals.Disconnect(l->data, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED);
    }

    _menu_desktop_shortcuts = nullptr;
  }

  if (desktop_file.empty())
    return;

  _menu_desktop_shortcuts = dbusmenu_menuitem_new();
  dbusmenu_menuitem_set_root(_menu_desktop_shortcuts, TRUE);

  // Build a desktop shortcuts object and tell it that our
  // environment is Unity to handle the filtering
  _desktop_shortcuts = indicator_desktop_shortcuts_new(desktop_file.c_str(), "Unity");
  // This will get us a list of the nicks available, it should
  // always be at least one entry of NULL if there either aren't
  // any or they're filtered for the environment we're in
  const gchar** nicks = indicator_desktop_shortcuts_get_nicks(_desktop_shortcuts);

  for (int index = 0; nicks[index]; ++index)
  {
    // Build a dbusmenu item for each nick that is the desktop
    // file that is built from it's name and includes a callback
    // to the desktop shortcuts object to execute the nick
    glib::String name(indicator_desktop_shortcuts_nick_get_name(_desktop_shortcuts,
                                                                nicks[index]));
    glib::Object<DbusmenuMenuitem> item(dbusmenu_menuitem_new());
    dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, name);
    dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
    dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
    auto nick = glib::gchar_to_string(nicks[index]);

    _gsignals.Add<void, DbusmenuMenuitem*, gint>(item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
    [this, nick] (DbusmenuMenuitem* item, unsigned timestamp) {
      GdkDisplay* display = gdk_display_get_default();
      glib::Object<GdkAppLaunchContext> context(gdk_display_get_app_launch_context(display));
      gdk_app_launch_context_set_timestamp(context, timestamp);
      auto gcontext = glib::object_cast<GAppLaunchContext>(context);
      indicator_desktop_shortcuts_nick_exec_with_context(_desktop_shortcuts, nick.c_str(), gcontext);
    });

    dbusmenu_menuitem_child_append(_menu_desktop_shortcuts, item);
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

void ApplicationLauncherIcon::EnsureMenuItemsWindowsReady()
{
  // delete all menu items for windows
  _menu_items_windows.clear();

  auto const& windows = Windows();

  // We only add quicklist menu-items for windows if we have more than one window
  if (windows.size() < 2)
    return;

  Window active = WindowManager::Default().GetActiveWindow();

  // add menu items for all open windows
  for (auto const& w : windows)
  {
    auto const& title = w->title();

    if (title.empty())
      continue;

    glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());
    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, title.c_str());
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
    dbusmenu_menuitem_property_set_bool(menu_item, QuicklistMenuItem::MARKUP_ACCEL_DISABLED_PROPERTY, true);
    dbusmenu_menuitem_property_set_int(menu_item, QuicklistMenuItem::MAXIMUM_LABEL_WIDTH_PROPERTY, MAXIMUM_QUICKLIST_WIDTH);

    Window xid = w->window_id();
    _gsignals.Add<void, DbusmenuMenuitem*, unsigned>(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
      [xid] (DbusmenuMenuitem*, unsigned) {
        WindowManager& wm = WindowManager::Default();
        wm.Activate(xid);
        wm.Raise(xid);
    });

    if (xid == active)
    {
      dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_RADIO);
      dbusmenu_menuitem_property_set_int(menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED);
    }

    _menu_items_windows.push_back(menu_item);
  }
}

void ApplicationLauncherIcon::EnsureMenuItemsStaticQuicklist()
{
  // make a client for desktop file actions
  if (!_menu_desktop_shortcuts.IsType(DBUSMENU_TYPE_MENUITEM))
  {
    UpdateDesktopQuickList();
  }
}

void ApplicationLauncherIcon::Quit()
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
    SimpleLauncherIcon::Stick(save);

    if (save)
      LogUnityEvent(ApplicationEventType::ACCESS);
  }
}

void ApplicationLauncherIcon::UnStick()
{
  if (!IsSticky())
    return;

  LogUnityEvent(ApplicationEventType::ACCESS);
  SimpleLauncherIcon::UnStick();
  SetQuirk(Quirk::VISIBLE, app_->visible());
  app_->sticky = false;

  if (!app_->running())
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
  if (_menu_items.size() == MenuItemType::SIZE)
    return;

  _menu_items.resize(MenuItemType::SIZE);

  /* (Un)Stick to Launcher */
  glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());
  const char* label = !IsSticky() ? _("Lock to Launcher") : _("Unlock from Launcher");
  dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, label);
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

  _gsignals.Add<void, DbusmenuMenuitem*, unsigned>(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
    [this] (DbusmenuMenuitem*, unsigned) {
      ToggleSticky();
  });

  _menu_items[MenuItemType::STICK] = menu_item;

  /* Quit */
  menu_item = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Quit"));
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

  _gsignals.Add<void, DbusmenuMenuitem*, unsigned>(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
    [this] (DbusmenuMenuitem*, unsigned) {
      Quit();
  });

  _menu_items[MenuItemType::QUIT] = menu_item;

  /* Separator */
  menu_item = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_CLIENT_TYPES_SEPARATOR);
  _menu_items[MenuItemType::SEPARATOR] = menu_item;
}

AbstractLauncherIcon::MenuItemsVector ApplicationLauncherIcon::GetMenus()
{
  MenuItemsVector result;
  glib::Object<DbusmenuMenuitem> quit_item;
  bool separator_needed = false;

  EnsureMenuItemsDefaultReady();
  EnsureMenuItemsStaticQuicklist();

  for (auto const& menus : {GetRemoteMenus(), _menu_desktop_shortcuts})
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
    result.push_back(_menu_items[MenuItemType::SEPARATOR]);
    separator_needed = false;
  }

  if (!_menu_items[MenuItemType::APP_NAME])
  {
    glib::String app_name(g_markup_escape_text(app_->title().c_str(), -1));
    std::string bold_app_name("<b>"+app_name.Str()+"</b>");

    glib::Object<DbusmenuMenuitem> item(dbusmenu_menuitem_new());
    dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, bold_app_name.c_str());
    dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
    dbusmenu_menuitem_property_set_bool(item, QuicklistMenuItem::MARKUP_ENABLED_PROPERTY, TRUE);

    _gsignals.Add<void, DbusmenuMenuitem*, unsigned>(item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
      [this] (DbusmenuMenuitem*, unsigned timestamp) {
        _source_manager.AddIdle([this, timestamp] {
          ActivateLauncherIcon(ActionArg(ActionArg::Source::LAUNCHER, 0, timestamp));
          return false;
        });
    });

    _menu_items[MenuItemType::APP_NAME] = item;
  }

  result.push_back(_menu_items[MenuItemType::APP_NAME]);
  result.push_back(_menu_items[MenuItemType::SEPARATOR]);

  EnsureMenuItemsWindowsReady();

  if (!_menu_items_windows.empty())
  {
    for (auto const& it : _menu_items_windows)
      result.push_back(it);

    result.push_back(_menu_items[MenuItemType::SEPARATOR]);
  }

  const char* label = !IsSticky() ? _("Lock to Launcher") : _("Unlock from Launcher");
  dbusmenu_menuitem_property_set(_menu_items[MenuItemType::STICK], DBUSMENU_MENUITEM_PROP_LABEL, label);
  result.push_back(_menu_items[MenuItemType::STICK]);

  if (IsRunning())
  {
    if (!quit_item)
      quit_item = _menu_items[MenuItemType::QUIT];

    dbusmenu_menuitem_property_set(quit_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Quit"));
    result.push_back(quit_item);
  }

  return result;
}

void ApplicationLauncherIcon::UpdateIconGeometries(std::vector<nux::Point3> center)
{
  if (app_->type() == "webapp")
    return;

  nux::Geometry geo(0, 0, icon_size, icon_size);

  for (auto& window : app_->GetWindows())
  {
    Window xid = window->window_id();
    int monitor = window->monitor();
    monitor = std::max<int>(0, std::min<int>(center.size() - 1, monitor));

    geo.x = center[monitor].x - icon_size / 2;
    geo.y = center[monitor].y - icon_size / 2;
    WindowManager::Default().SetWindowIconGeometry(xid, geo);
  }
}

void ApplicationLauncherIcon::OnCenterStabilized(std::vector<nux::Point3> center)
{
  UpdateIconGeometries(center);
}

void ApplicationLauncherIcon::UpdateRemoteUri()
{
  std::string const& desktop_id = app_->desktop_id();

  if (!desktop_id.empty())
  {
    _remote_uri = FavoriteStore::URI_PREFIX_APP + desktop_id;
  }
  else
  {
    _remote_uri.clear();
  }
}

std::string ApplicationLauncherIcon::GetRemoteUri() const
{
  return _remote_uri;
}

std::set<std::string> ApplicationLauncherIcon::ValidateUrisForLaunch(DndData const& uris)
{
  std::set<std::string> result;

  for (auto uri : uris.Uris())
    result.insert(uri);

  return result;
}

void ApplicationLauncherIcon::OnDndHovered()
{
  // for now, let's not do this, it turns out to be quite buggy
  //if (IsRunning())
  //  Spread(CompAction::StateInitEdgeDnd, true);
}

void ApplicationLauncherIcon::OnDndEnter()
{
  /* Disabled, since the DND code is currently disabled as well.
  _source_manager.AddTimeout(1000, [this] {
    OnDndHovered();
    return false;
  }, ICON_DND_OVER_TIMEOUT);
  */
}

void ApplicationLauncherIcon::OnDndLeave()
{
  /* Disabled, since the DND code is currently disabled as well.
  _source_manager.Remove(ICON_DND_OVER_TIMEOUT);
  */
}

bool ApplicationLauncherIcon::IsFileManager()
{
  auto const& desktop_file = DesktopFile();

  return boost::algorithm::ends_with(desktop_file, "nautilus.desktop") ||
         boost::algorithm::ends_with(desktop_file, "nautilus-folder-handler.desktop") ||
         boost::algorithm::ends_with(desktop_file, "nautilus-home.desktop");
}

bool ApplicationLauncherIcon::OnShouldHighlightOnDrag(DndData const& dnd_data)
{
  if (IsFileManager())
  {
    for (auto uri : dnd_data.Uris())
    {
      if (boost::algorithm::starts_with(uri, "file://"))
        return true;
    }
    return false;
  }

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
  return ValidateUrisForLaunch(dnd_data).empty() ? nux::DNDACTION_NONE : nux::DNDACTION_COPY;
#else
  return nux::DNDACTION_NONE;
#endif
}

void ApplicationLauncherIcon::OnAcceptDrop(DndData const& dnd_data)
{
  auto timestamp = nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp;
  OpenInstanceWithUris(ValidateUrisForLaunch(dnd_data), timestamp);
}

bool ApplicationLauncherIcon::ShowInSwitcher(bool current)
{
  bool result = false;

  if (IsRunning() && IsVisible())
  {
    // If current is true, we only want to show the current workspace.
    if (!current)
    {
      result = true;
    }
    else
    {
      for (unsigned i = 0; i < monitors::MAX; ++i)
      {
        if (WindowVisibleOnMonitor(i))
        {
          result = true;
          break;
        }
      }
    }
  }

  return result;
}

bool ApplicationLauncherIcon::AllowDetailViewInSwitcher() const
{
  return app_->type() != "webapp";
}

uint64_t ApplicationLauncherIcon::SwitcherPriority()
{
  uint64_t result = 0;
  // Webapps always go at the back.
  if (app_->type() == "webapp")
    return result;

  for (auto& window : app_->GetWindows())
  {
    Window xid = window->window_id();
    result = std::max(result, WindowManager::Default().GetWindowActiveNumber(xid));
  }

  return result;
}

nux::Color ApplicationLauncherIcon::BackgroundColor() const
{
  if (use_custom_bg_color_)
    return bg_color_;

  return SimpleLauncherIcon::BackgroundColor();
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

void PerformScrollUp(WindowList const& windows, unsigned int progressive_scroll)
{
  if (!progressive_scroll)
  {
    windows.at(1)->Focus();
    return;
  }
  else if (progressive_scroll == 1)
  {
    windows.back()->Focus();
    return;
  }

  WindowManager::Default().RestackBelow(windows.at(0)->window_id(), windows.at(windows.size() - progressive_scroll + 1)->window_id());
  windows.at(windows.size() - progressive_scroll + 1)->Focus();
}

void PerformScrollDown(WindowList const& windows, unsigned int progressive_scroll)
{
  if (!progressive_scroll)
  {
    WindowManager::Default().RestackBelow(windows.at(0)->window_id(), windows.back()->window_id());
    windows.at(1)->Focus();
    return;
  }

  WindowManager::Default().RestackBelow(windows.at(0)->window_id(), windows.at(progressive_scroll)->window_id());
  windows.at(progressive_scroll)->Focus();
}

void ApplicationLauncherIcon::PerformScroll(ScrollDirection direction, Time timestamp)
{
  if (timestamp - _last_scroll_timestamp < 150)
    return;
  else if (timestamp - _last_scroll_timestamp > 1500 || direction != _last_scroll_direction)
    _progressive_scroll = 0;

  _last_scroll_timestamp = timestamp;
  _last_scroll_direction = direction;

  auto const& windows = GetWindowsOnCurrentDesktopInStackingOrder();
  if (windows.empty())
    return;

  if (!IsActive())
  {
    windows.at(0)->Focus();
    return;
  }

  if (windows.size() <= 1)
    return;

  ++_progressive_scroll;
  _progressive_scroll %= windows.size();

  switch(direction)
  {
  case ScrollDirection::UP:
    PerformScrollUp(windows, _progressive_scroll);
    break;
  case ScrollDirection::DOWN:
    PerformScrollDown(windows, _progressive_scroll);
    break;
  }
}

WindowList ApplicationLauncherIcon::GetWindowsOnCurrentDesktopInStackingOrder()
{
  auto windows = GetWindows(WindowFilter::ON_CURRENT_DESKTOP | WindowFilter::ON_ALL_MONITORS);
  auto sorted_windows = WindowManager::Default().GetWindowsInStackingOrder();

  // Order the windows
  std::sort(windows.begin(), windows.end(), [&sorted_windows] (ApplicationWindowPtr const& win1, ApplicationWindowPtr const& win2) {
    for (auto const& window : sorted_windows)
    {
      if (window == win1->window_id())
        return false;
      else if (window == win2->window_id())
        return true;
    }

    return true;
  });

  return windows;
}

std::string ApplicationLauncherIcon::GetName() const
{
  return "ApplicationLauncherIcon";
}

} // namespace launcher
} // namespace unity
