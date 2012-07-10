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

#include <boost/algorithm/string.hpp>

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <NuxCore/Logger.h>

#include <UnityCore/Variant.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/DesktopUtilities.h>

#include "BamfLauncherIcon.h"
#include "FavoriteStore.h"
#include "Launcher.h"
#include "MultiMonitor.h"
#include "unity-shared/WindowManager.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/ubus-server.h"

#include <glib/gi18n-lib.h>
#include <gio/gdesktopappinfo.h>

namespace unity
{
namespace launcher
{
namespace
{
nux::logging::Logger logger("unity.launcher");

  // We use the "bamf-" prefix since the manager is protected, to avoid name clash
  const std::string WINDOW_MOVE_TIMEOUT = "bamf-window-move";
  const std::string ICON_REMOVE_TIMEOUT = "bamf-icon-remove";
  //const std::string ICON_DND_OVER_TIMEOUT = "bamf-icon-dnd-over";
}

NUX_IMPLEMENT_OBJECT_TYPE(BamfLauncherIcon);

BamfLauncherIcon::BamfLauncherIcon(BamfApplication* app)
  : SimpleLauncherIcon()
  , _bamf_app(app, glib::AddRef())
  , _supported_types_filled(false)
  , use_custom_bg_color_(false)
  , bg_color_(nux::color::White)
{
  g_object_set_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen"),
                     GUINT_TO_POINTER(1));
  auto bamf_view = glib::object_cast<BamfView>(_bamf_app);

  glib::String icon(bamf_view_get_icon(bamf_view));

  tooltip_text = BamfName();
  icon_name = icon.Str();
  SetIconType(TYPE_APPLICATION);

  if (IsSticky())
    SetQuirk(QUIRK_VISIBLE, true);
  else
    SetQuirk(QUIRK_VISIBLE, bamf_view_user_visible(bamf_view));

  SetQuirk(QUIRK_ACTIVE, bamf_view_is_active(bamf_view));
  SetQuirk(QUIRK_RUNNING, bamf_view_is_running(bamf_view));

  glib::SignalBase* sig;

  sig = new glib::Signal<void, BamfView*, BamfView*>(bamf_view, "child-added",
                          [&] (BamfView*, BamfView*) {
                            EnsureWindowState();
                            UpdateMenus();
                            UpdateIconGeometries(GetCenters());
                          });
  _gsignals.Add(sig);

  sig = new glib::Signal<void, BamfView*, BamfView*>(bamf_view, "child-removed",
                          [&] (BamfView*, BamfView*) { EnsureWindowState(); });
  _gsignals.Add(sig);

  sig = new glib::Signal<void, BamfView*, gboolean>(bamf_view, "urgent-changed",
                          [&] (BamfView*, gboolean urgent) {
                            SetQuirk(QUIRK_URGENT, urgent);
                          });
  _gsignals.Add(sig);

  sig = new glib::Signal<void, BamfView*, gboolean>(bamf_view, "active-changed",
                          [&] (BamfView*, gboolean active) {
                            SetQuirk(QUIRK_ACTIVE, active);
                          });
  _gsignals.Add(sig);

  sig = new glib::Signal<void, BamfView*, gboolean>(bamf_view, "running-changed",
                          [&] (BamfView*, gboolean running) {
                            SetQuirk(QUIRK_RUNNING, running);

                            if (running)
                            {
                              EnsureWindowState();
                              UpdateIconGeometries(GetCenters());
                              _source_manager.Remove(ICON_REMOVE_TIMEOUT);
                            }
                          });
  _gsignals.Add(sig);

  sig = new glib::Signal<void, BamfView*, gboolean>(bamf_view, "user-visible-changed",
                          [&] (BamfView*, gboolean visible) {
                            if (!IsSticky())
                              SetQuirk(QUIRK_VISIBLE, visible);
                          });
  _gsignals.Add(sig);

  sig = new glib::Signal<void, BamfView*>(bamf_view, "closed",
                          [&] (BamfView*) {
                            if (!IsSticky())
                            {
                              SetQuirk(QUIRK_VISIBLE, false);

                              /* Use a timeout to remove the icon, this avoids
                               * that we remove an application that is going
                               * to be reopened soon. So applications that
                               * have a splash screen won't be removed from
                               * the launcher while the splash is closed and
                               * a new window is opened. */
                              _source_manager.AddTimeoutSeconds(1, [&] {
                                Remove();
                                return false;
                              }, ICON_REMOVE_TIMEOUT);
                            }
                          });
  _gsignals.Add(sig);

  WindowManager::Default()->window_minimized.connect(sigc::mem_fun(this, &BamfLauncherIcon::OnWindowMinimized));
  WindowManager::Default()->window_moved.connect(sigc::mem_fun(this, &BamfLauncherIcon::OnWindowMoved));
  WindowManager::Default()->compiz_screen_viewport_switch_ended.connect(sigc::mem_fun(this, &BamfLauncherIcon::EnsureWindowState));
  WindowManager::Default()->terminate_expo.connect(sigc::mem_fun(this, &BamfLauncherIcon::EnsureWindowState));

  EnsureWindowState();
  UpdateMenus();
  UpdateDesktopFile();
  UpdateBackgroundColor();

  // hack
  SetProgress(0.0f);

  // Calls when there are no higher priority events pending to the default main loop.
  _source_manager.AddIdle([&] { FillSupportedTypes(); return false; });
}

BamfLauncherIcon::~BamfLauncherIcon()
{
  if (_bamf_app)
    g_object_set_qdata(G_OBJECT(_bamf_app.RawPtr()),
                       g_quark_from_static_string("unity-seen"), nullptr);
}

void BamfLauncherIcon::Remove()
{
  /* Removing the unity-seen flag to the wrapped bamf application, on remove
   * request we make sure that if the bamf application is re-opened while
   * the removal process is still ongoing, the application will be shown
   * on the launcher. Disconnecting from signals and nullifying the _bamf_app
   * we make sure that this icon won't be reused (no duplicated icon). */
  _gsignals.Disconnect(_bamf_app);
  g_object_set_qdata(G_OBJECT(_bamf_app.RawPtr()),
                     g_quark_from_static_string("unity-seen"), nullptr);
  _bamf_app = nullptr;

  SimpleLauncherIcon::Remove();
}

bool BamfLauncherIcon::IsSticky() const
{
  if (!BAMF_IS_VIEW(_bamf_app.RawPtr()))
    return false;
  else
    return bamf_view_is_sticky(BAMF_VIEW(_bamf_app.RawPtr()));
}

bool BamfLauncherIcon::IsVisible() const
{
  return GetQuirk(QUIRK_VISIBLE);
}

bool BamfLauncherIcon::IsActive() const
{
  return GetQuirk(QUIRK_ACTIVE);
}

bool BamfLauncherIcon::IsRunning() const
{
  return GetQuirk(QUIRK_RUNNING);
}

bool BamfLauncherIcon::IsUrgent() const
{
  return GetQuirk(QUIRK_URGENT);
}

void BamfLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  SimpleLauncherIcon::ActivateLauncherIcon(arg);
  WindowManager* wm = WindowManager::Default();
  bool scaleWasActive = wm->IsScaleActive();

  bool active = IsActive();
  bool user_visible = IsRunning();

  if (arg.target && OwnsWindow(arg.target))
  {
    wm->Activate(arg.target);
    return;
  }

  /* We should check each child to see if there is
   * an unmapped (!= minimized) window around and
   * if so force "Focus" behaviour */

  if (arg.source != ActionArg::SWITCHER)
  {
    auto bamf_view = glib::object_cast<BamfView>(_bamf_app);
    user_visible = bamf_view_user_visible(bamf_view);

    if (active)
    {
      bool any_visible = false;
      bool any_mapped = false;
      bool any_on_top = false;
      bool any_on_monitor = (arg.monitor < 0);
      int active_monitor = arg.monitor;
      GList* children = bamf_view_get_children(bamf_view);

      for (GList* l = children; l; l = l->next)
      {
        if (!BAMF_IS_WINDOW(l->data))
          continue;

        auto view = static_cast<BamfView*>(l->data);
        auto win = static_cast<BamfWindow*>(l->data);
        Window xid = bamf_window_get_xid(win);

        if (!any_visible && wm->IsWindowOnCurrentDesktop(xid))
        {
          any_visible = true;
        }

        if (!any_mapped && wm->IsWindowMapped(xid))
        {
          any_mapped = true;
        }

        if (!any_on_top && wm->IsWindowOnTop(xid))
        {
          any_on_top = true;
        }

        if (!any_on_monitor && bamf_window_get_monitor(win) == arg.monitor &&
            wm->IsWindowMapped(xid) && wm->IsWindowVisible(xid))
        {
          any_on_monitor = true;
        }

        if (bamf_view_is_active(view))
        {
          active_monitor = bamf_window_get_monitor(win);
        }
      }

      g_list_free(children);

      if (!any_visible || !any_mapped || !any_on_top)
        active = false;

      if (any_on_monitor && arg.monitor >= 0 && active_monitor != arg.monitor)
        active = false;
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
    if (GetQuirk(QUIRK_STARTING))
      return;

    if (scaleWasActive)
    {
      wm->TerminateScale();
    }

    SetQuirk(QUIRK_STARTING, true);
    OpenInstanceLauncherIcon(ActionArg());
  }
  else // app is running
  {
    if (active)
    {
      if (scaleWasActive) // #5 above
      {
        wm->TerminateScale();
        Focus(arg);
      }
      else // #2 above
      {
        if (arg.source != ActionArg::SWITCHER)
        {
          Spread(true, 0, false);
        }
      }
    }
    else
    {
      if (scaleWasActive) // #4 above
      {
        wm->TerminateScale();
        Focus(arg);
        if (arg.source != ActionArg::SWITCHER)
          Spread(true, 0, false);
      }
      else // #3 above
      {
        Focus(arg);
      }
    }
  }

  if (arg.source != ActionArg::SWITCHER)
    ubus_server_send_message(ubus_server_get_default(), UBUS_LAUNCHER_ACTION_DONE, nullptr);
}

std::vector<Window> BamfLauncherIcon::GetWindows(WindowFilterMask filter, int monitor)
{
  WindowManager* wm = WindowManager::Default();
  std::vector<Window> results;

  if (!BAMF_IS_VIEW(_bamf_app.RawPtr()))
  {
    if (_bamf_app)
    {
      LOG_WARNING(logger) << "Not a view but not null.";
    }
    return results;
  }

  monitor = (filter & WindowFilter::ON_ALL_MONITORS) ? -1 : monitor;
  bool mapped = (filter & WindowFilter::MAPPED);
  bool user_visible = (filter & WindowFilter::USER_VISIBLE);
  bool current_desktop = (filter & WindowFilter::ON_CURRENT_DESKTOP);

  GList* children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));
  for (GList* l = children; l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
      continue;

    auto window = static_cast<BamfWindow*>(l->data);
    auto view = static_cast<BamfView*>(l->data);

    if ((monitor >= 0 && bamf_window_get_monitor(window) == monitor) || monitor < 0)
    {
      if ((user_visible && bamf_view_user_visible(view)) || !user_visible)
      {
        guint32 xid = bamf_window_get_xid(window);

        if ((mapped && wm->IsWindowMapped(xid)) || !mapped)
        {
          if ((current_desktop && wm->IsWindowOnCurrentDesktop(xid)) || !current_desktop)
          {
            results.push_back(xid);
          }
        }
      }
    }
  }

  g_list_free(children);
  return results;
}

std::vector<Window> BamfLauncherIcon::Windows()
{
  return GetWindows(WindowFilter::MAPPED|WindowFilter::ON_ALL_MONITORS);
}

std::vector<Window> BamfLauncherIcon::WindowsOnViewport()
{
  WindowFilterMask filter = 0;
  filter |= WindowFilter::MAPPED;
  filter |= WindowFilter::USER_VISIBLE;
  filter |= WindowFilter::ON_CURRENT_DESKTOP;
  filter |= WindowFilter::ON_ALL_MONITORS;

  return GetWindows(filter);
}

std::vector<Window> BamfLauncherIcon::WindowsForMonitor(int monitor)
{
  WindowFilterMask filter = 0;
  filter |= WindowFilter::MAPPED;
  filter |= WindowFilter::USER_VISIBLE;
  filter |= WindowFilter::ON_CURRENT_DESKTOP;

  return GetWindows(filter, monitor);
}

std::string BamfLauncherIcon::NameForWindow(Window window)
{
  std::string result;
  GList* children, *l;

  children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));
  for (l = children; l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
      continue;

    if (bamf_window_get_xid(static_cast<BamfWindow*>(l->data)) == window)
    {
      auto view = static_cast<BamfView*>(l->data);
      result = glib::String(bamf_view_get_name(view)).Str();
      break;
    }
  }

  g_list_free(children);
  return result;
}

void BamfLauncherIcon::OnWindowMinimized(guint32 xid)
{
  if (!OwnsWindow(xid))
    return;

  Present(0.5f, 600);
  UpdateQuirkTimeDelayed(300, QUIRK_SHIMMER);
}

void BamfLauncherIcon::OnWindowMoved(guint32 moved_win)
{
  if (!OwnsWindow(moved_win))
    return;

  _source_manager.AddTimeout(250, [&] {
    EnsureWindowState();
    UpdateIconGeometries(GetCenters());

    return false;
  }, WINDOW_MOVE_TIMEOUT);
}

void BamfLauncherIcon::UpdateDesktopFile()
{
  if (!BAMF_IS_APPLICATION(_bamf_app.RawPtr()))
  {
    if (_bamf_app)
    {
      LOG_WARNING(logger) << "Not an app but not null.";
    }
    return;
  }
  const char* filename = bamf_application_get_desktop_file(_bamf_app);

  if (filename != nullptr && filename[0] != '\0' && _desktop_file != filename)
  {
    _desktop_file = filename;

    // add a file watch to the desktop file so that if/when the app is removed
    // we can remove ourself from the launcher and when it's changed
    // we can update the quicklist.
    if (_desktop_file_monitor)
      _gsignals.Disconnect(_desktop_file_monitor, "changed");

    glib::Object<GFile> desktop_file(g_file_new_for_path(_desktop_file.c_str()));
    _desktop_file_monitor = g_file_monitor_file(desktop_file, G_FILE_MONITOR_NONE,
                                                nullptr, nullptr);
    g_file_monitor_set_rate_limit(_desktop_file_monitor, 1000);

    auto sig = new glib::Signal<void, GFileMonitor*, GFile*, GFile*, GFileMonitorEvent>(_desktop_file_monitor, "changed",
                                [&] (GFileMonitor*, GFile*, GFile*, GFileMonitorEvent event_type) {
                                  switch (event_type)
                                  {
                                    case G_FILE_MONITOR_EVENT_DELETED:
                                      UnStick();
                                      break;
                                    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
                                      UpdateDesktopQuickList();
                                      UpdateBackgroundColor();
                                      break;
                                    default:
                                      break;
                                  }
                                });
    _gsignals.Add(sig);
  }
}

std::string BamfLauncherIcon::DesktopFile()
{
  UpdateDesktopFile();
  return _desktop_file;
}

std::string BamfLauncherIcon::BamfName() const
{
  glib::String name(bamf_view_get_name(BAMF_VIEW(_bamf_app.RawPtr())));
  return name.Str();
}

void BamfLauncherIcon::AddProperties(GVariantBuilder* builder)
{
  SimpleLauncherIcon::AddProperties(builder);

  GVariantBuilder xids_builder;
  g_variant_builder_init(&xids_builder, G_VARIANT_TYPE ("au"));

  for (auto xid : GetWindows())
    g_variant_builder_add(&xids_builder, "u", xid);

  variant::BuilderWrapper(builder)
    .add("desktop_file", DesktopFile())
    .add("desktop_id", GetDesktopID())
    .add("application_id", GPOINTER_TO_UINT(_bamf_app.RawPtr()))
    .add("xids", g_variant_builder_end(&xids_builder))
    .add("sticky", IsSticky());
}

bool BamfLauncherIcon::OwnsWindow(Window xid) const
{
  GList* children, *l;
  bool owns = false;

  if (!xid || !_bamf_app)
    return owns;

  children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));

  for (l = children; l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
      continue;

    if (bamf_window_get_xid(static_cast<BamfWindow*>(l->data)) == xid)
    {
      owns = true;
      break;
    }
  }

  g_list_free(children);
  return owns;
}

void BamfLauncherIcon::OpenInstanceWithUris(std::set<std::string> uris)
{
  glib::Error error;
  glib::Object<GDesktopAppInfo> desktopInfo(g_desktop_app_info_new_from_filename(DesktopFile().c_str()));
  auto appInfo = glib::object_cast<GAppInfo>(desktopInfo);

  if (g_app_info_supports_uris(appInfo))
  {
    GList* list = nullptr;

    for (auto  it : uris)
      list = g_list_prepend(list, g_strdup(it.c_str()));

    g_app_info_launch_uris(appInfo, list, nullptr, &error);
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

    g_app_info_launch(appInfo, list, nullptr, &error);
    g_list_free_full(list, g_object_unref);
  }
  else
  {
    g_app_info_launch(appInfo, nullptr, nullptr, &error);
  }

  if (error)
    g_warning("%s\n", error.Message().c_str());

  UpdateQuirkTime(QUIRK_STARTING);
}

void BamfLauncherIcon::OpenInstanceLauncherIcon(ActionArg arg)
{
  std::set<std::string> empty;
  OpenInstanceWithUris(empty);
  ubus_server_send_message(ubus_server_get_default(), UBUS_LAUNCHER_ACTION_DONE, nullptr);
}

void BamfLauncherIcon::Focus(ActionArg arg)
{
  bool any_urgent = false;
  bool any_visible = false;
  bool any_user_visible = false;
  WindowManager* wm = WindowManager::Default();

  std::vector<Window> windows;
  GList* children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));

  /* get the list of windows */
  for (GList* l = children; l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
      continue;

    auto view = static_cast<BamfView*>(l->data);

    Window xid = bamf_window_get_xid(static_cast<BamfWindow*>(l->data));
    bool urgent = bamf_view_is_urgent(view);
    bool user_visible = bamf_view_user_visible(view);

    if (any_urgent)
    {
      if (urgent)
        windows.push_back(xid);
    }
    else if (any_user_visible && !urgent)
    {
      if (user_visible)
        windows.push_back(xid);
    }
    else
    {
      if (urgent || user_visible)
      {
        windows.clear();
        any_visible = false;
        any_urgent = (any_urgent || urgent);
        any_user_visible = (any_user_visible || user_visible);
      }

      windows.push_back(xid);
    }

    if (wm->IsWindowOnCurrentDesktop(xid) && wm->IsWindowVisible(xid))
    {
      any_visible = true;
    }
  }

  g_list_free(children);

  auto visibility = WindowManager::FocusVisibility::OnlyVisible;

  if (arg.source != ActionArg::SWITCHER)
  {
    if (any_visible)
    {
      visibility = WindowManager::FocusVisibility::ForceUnminimizeInvisible;
    }
    else
    {
      visibility = WindowManager::FocusVisibility::ForceUnminimizeOnCurrentDesktop;
    }
  }

  bool only_top_win = !any_urgent;
  wm->FocusWindowGroup(windows, visibility, arg.monitor, only_top_win);
}

bool BamfLauncherIcon::Spread(bool current_desktop, int state, bool force)
{
  auto windows = GetWindows(current_desktop ? WindowFilter::ON_CURRENT_DESKTOP : 0);
  return WindowManager::Default()->ScaleWindowGroup(windows, state, force);
}

void BamfLauncherIcon::EnsureWindowState()
{
  std::vector<bool> monitors;
  monitors.resize(max_num_monitors);

  if (BAMF_IS_VIEW(_bamf_app.RawPtr()))
  {
    GList* children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));
    for (GList* l = children; l; l = l->next)
    {
      if (!BAMF_IS_WINDOW(l->data))
        continue;

      auto window = static_cast<BamfWindow*>(l->data);
      Window xid = bamf_window_get_xid(window);
      int monitor = bamf_window_get_monitor(window);

      if (monitor >= 0 && WindowManager::Default()->IsWindowOnCurrentDesktop(xid))
        monitors[monitor] = true;
    }

    g_list_free(children);
  }
  else
  {
    if (_bamf_app)
    {
      LOG_WARNING(logger) << "Not a view but not null.";
    }
  }
  for (int i = 0; i < max_num_monitors; i++)
    SetWindowVisibleOnMonitor(monitors[i], i);

  EmitNeedsRedraw();
}

void BamfLauncherIcon::UpdateDesktopQuickList()
{
  std::string const& desktop_file = DesktopFile();

  if (desktop_file.empty())
    return;

  if (_menu_desktop_shortcuts)
  {
    for (GList *l = dbusmenu_menuitem_get_children(_menu_desktop_shortcuts); l; l = l->next)
    {
      _gsignals.Disconnect(l->data, "item-activated");
    }
  }

  _menu_desktop_shortcuts = dbusmenu_menuitem_new();
  dbusmenu_menuitem_set_root(_menu_desktop_shortcuts, TRUE);

  // Build a desktop shortcuts object and tell it that our
  // environment is Unity to handle the filtering
  _desktop_shortcuts = indicator_desktop_shortcuts_new(desktop_file.c_str(), "Unity");
  // This will get us a list of the nicks available, it should
  // always be at least one entry of NULL if there either aren't
  // any or they're filtered for the environment we're in
  const gchar** nicks = indicator_desktop_shortcuts_get_nicks(_desktop_shortcuts);

  int index = 0;
  while (nicks[index])
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
    dbusmenu_menuitem_property_set(item, "shortcut-nick", nicks[index]);

    auto sig = new glib::Signal<void, DbusmenuMenuitem*, gint>(item, "item-activated",
                                [&] (DbusmenuMenuitem* item, gint) {
                                  const gchar *nick;
                                  nick = dbusmenu_menuitem_property_get(item, "shortcut-nick");
                                  indicator_desktop_shortcuts_nick_exec(_desktop_shortcuts, nick);
                                });
    _gsignals.Add(sig);

    dbusmenu_menuitem_child_append(_menu_desktop_shortcuts, item);
    index++;
  }
}

void BamfLauncherIcon::UpdateBackgroundColor()
{
  bool last_use_custom_bg_color = use_custom_bg_color_;
  nux::Color last_bg_color(bg_color_);

  std::string const& color = DesktopUtilities::GetBackgroundColor(DesktopFile());

  use_custom_bg_color_ = !color.empty();

  if (use_custom_bg_color_)
    bg_color_ = nux::Color(color);

  if (last_use_custom_bg_color != use_custom_bg_color_ ||
      last_bg_color != bg_color_)
    EmitNeedsRedraw();
}

void BamfLauncherIcon::UpdateMenus()
{
  GList* children, *l;

  children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));
  for (l = children; l; l = l->next)
  {
    if (!BAMF_IS_INDICATOR(l->data))
      continue;

    auto indicator = static_cast<BamfIndicator*>(l->data);
    std::string path = bamf_indicator_get_dbus_menu_path(indicator);

    // we already have this
    if (_menu_clients.find(path) != _menu_clients.end())
      continue;

    std::string address = bamf_indicator_get_remote_address(indicator);
    DbusmenuClient* client = dbusmenu_client_new(address.c_str(), path.c_str());
    _menu_clients[path] = glib::Object<DbusmenuClient>(client);
  }

  g_list_free(children);

  // add dynamic quicklist
  if (_menuclient_dynamic_quicklist && DBUSMENU_IS_CLIENT(_menuclient_dynamic_quicklist.RawPtr()))
  {
    if (_menu_clients["dynamicquicklist"] != _menuclient_dynamic_quicklist)
    {
      _menu_clients["dynamicquicklist"] = _menuclient_dynamic_quicklist;
    }
  }
  else if (_menu_clients["dynamicquicklist"])
  {
    _menu_clients.erase("dynamicquicklist");
    _menuclient_dynamic_quicklist = nullptr;
  }

  // make a client for desktop file actions
  if (!DBUSMENU_IS_MENUITEM(_menu_desktop_shortcuts.RawPtr()))
  {
    UpdateDesktopQuickList();
  }
}

void BamfLauncherIcon::Quit()
{
  GList* children, *l;

  children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));

  for (l = children; l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
      continue;

    Window xid = bamf_window_get_xid(static_cast<BamfWindow*>(l->data));
    WindowManager::Default()->Close(xid);
  }

  g_list_free(children);
}

void BamfLauncherIcon::Stick(bool save)
{
  if (IsSticky())
    return;

  std::string const& desktop_file = DesktopFile();
  bamf_view_set_sticky(BAMF_VIEW(_bamf_app.RawPtr()), true);

  if (save && !desktop_file.empty())
    FavoriteStore::Instance().AddFavorite(desktop_file, -1);
}

void BamfLauncherIcon::UnStick()
{
  if (!IsSticky())
    return;

  std::string const& desktop_file = DesktopFile();
  BamfView* view = BAMF_VIEW(_bamf_app.RawPtr());
  bamf_view_set_sticky(view, false);

  if (bamf_view_is_closed(view) || !bamf_view_user_visible(view))
    Remove();

  if (!desktop_file.empty())
    FavoriteStore::Instance().RemoveFavorite(desktop_file);
}

void BamfLauncherIcon::ToggleSticky()
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

void BamfLauncherIcon::EnsureMenuItemsReady()
{
  DbusmenuMenuitem* menu_item;

  /* Pin */
  if (_menu_items.find("Pin") == _menu_items.end())
  {
    menu_item = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    _gsignals.Add(new glib::Signal<void, DbusmenuMenuitem*, int>(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                                    [&] (DbusmenuMenuitem*, int) {
                                      ToggleSticky();
                                    }));

    _menu_items["Pin"] = glib::Object<DbusmenuMenuitem>(menu_item);
  }

  const char* label = !IsSticky() ? _("Lock to Launcher") : _("Unlock from Launcher");

  dbusmenu_menuitem_property_set(_menu_items["Pin"], DBUSMENU_MENUITEM_PROP_LABEL, label);


  /* Quit */
  if (_menu_items.find("Quit") == _menu_items.end())
  {
    menu_item = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Quit"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    _gsignals.Add(new glib::Signal<void, DbusmenuMenuitem*, int>(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                                    [&] (DbusmenuMenuitem*, int) {
                                      Quit();
                                    }));

    _menu_items["Quit"] = glib::Object<DbusmenuMenuitem>(menu_item);
  }
}

std::list<DbusmenuMenuitem*> BamfLauncherIcon::GetMenus()
{
  std::list<DbusmenuMenuitem*> result;
  bool first_separator_needed = false;
  DbusmenuMenuitem* item = nullptr;

  // FIXME for O: hack around the wrong abstraction
  UpdateMenus();

  for (auto it = _menu_clients.begin(); it != _menu_clients.end(); ++it)
  {
    GList* child = nullptr;
    DbusmenuClient* client = it->second;
    if (!client)
      continue;
    DbusmenuMenuitem* root = dbusmenu_client_get_root(client);

    if (!root || !dbusmenu_menuitem_property_get_bool(root, DBUSMENU_MENUITEM_PROP_VISIBLE))
      continue;

    for (child = dbusmenu_menuitem_get_children(root); child; child = child->next)
    {
      DbusmenuMenuitem* item = (DbusmenuMenuitem*) child->data;

      if (!item || !DBUSMENU_IS_MENUITEM(item))
        continue;

      if (dbusmenu_menuitem_property_get_bool(item, DBUSMENU_MENUITEM_PROP_VISIBLE))
      {
        first_separator_needed = true;
        dbusmenu_menuitem_property_set_bool(item, "unity-use-markup", FALSE);

        result.push_back(item);
      }
    }
  }

  // FIXME: this should totally be added as a _menu_client
  if (DBUSMENU_IS_MENUITEM(_menu_desktop_shortcuts.RawPtr()))
  {
    GList* child = nullptr;

    for (child = dbusmenu_menuitem_get_children(_menu_desktop_shortcuts); child; child = child->next)
    {
      DbusmenuMenuitem* item = (DbusmenuMenuitem*) child->data;

      if (!item)
        continue;

      first_separator_needed = true;

      result.push_back(item);
    }
  }

  if (first_separator_needed)
  {
    auto first_sep = _menu_items_extra.find("FirstSeparator");
    if (first_sep != _menu_items_extra.end())
    {
      item = first_sep->second;
    }
    else
    {
      item = dbusmenu_menuitem_new();
      dbusmenu_menuitem_property_set(item,
                                     DBUSMENU_MENUITEM_PROP_TYPE,
                                     DBUSMENU_CLIENT_TYPES_SEPARATOR);
      _menu_items_extra["FirstSeparator"] = glib::Object<DbusmenuMenuitem>(item);
    }
    result.push_back(item);
  }

  auto app_name_item = _menu_items_extra.find("AppName");
  if (app_name_item != _menu_items_extra.end())
  {
    item = app_name_item->second;
  }
  else
  {
    glib::String app_name(g_markup_escape_text(BamfName().c_str(), -1));
    std::ostringstream bold_app_name;
    bold_app_name << "<b>" << app_name << "</b>";

    item = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(item,
                                   DBUSMENU_MENUITEM_PROP_LABEL,
                                   bold_app_name.str().c_str());
    dbusmenu_menuitem_property_set_bool(item,
                                        DBUSMENU_MENUITEM_PROP_ENABLED,
                                        true);
    dbusmenu_menuitem_property_set_bool(item,
                                        "unity-use-markup",
                                        true);

    _gsignals.Add(new glib::Signal<void, DbusmenuMenuitem*, int>(item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                                    [&] (DbusmenuMenuitem*, int) {
                                      _source_manager.AddIdle([&] {
                                        ActivateLauncherIcon(ActionArg());
                                        return false;
                                      });
                                    }));

    _menu_items_extra["AppName"] = glib::Object<DbusmenuMenuitem>(item);
  }
  result.push_back(item);

  auto second_sep = _menu_items_extra.find("SecondSeparator");
  if (second_sep != _menu_items_extra.end())
  {
    item = second_sep->second;
  }
  else
  {
    item = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(item,
                                   DBUSMENU_MENUITEM_PROP_TYPE,
                                   DBUSMENU_CLIENT_TYPES_SEPARATOR);
    _menu_items_extra["SecondSeparator"] = glib::Object<DbusmenuMenuitem>(item);
  }
  result.push_back(item);

  EnsureMenuItemsReady();

  for (auto it_m = _menu_items.begin(); it_m != _menu_items.end(); ++it_m)
  {
    if (!IsRunning() && it_m->first == "Quit")
      continue;

    bool exists = false;
    std::string label_default(dbusmenu_menuitem_property_get(it_m->second, DBUSMENU_MENUITEM_PROP_LABEL));

    for (auto menu_item : result)
    {
      const gchar* type = dbusmenu_menuitem_property_get(menu_item, DBUSMENU_MENUITEM_PROP_TYPE);
      if (type == nullptr)//(g_strcmp0 (type, DBUSMENU_MENUITEM_PROP_LABEL) == 0)
      {
        std::string label_menu(dbusmenu_menuitem_property_get(menu_item, DBUSMENU_MENUITEM_PROP_LABEL));
        if (label_menu == label_default)
        {
          exists = true;
          break;
        }
      }
    }

    if (!exists)
      result.push_back(it_m->second);
  }

  return result;
}

void BamfLauncherIcon::UpdateIconGeometries(std::vector<nux::Point3> center)
{
  if (!BAMF_IS_VIEW(_bamf_app.RawPtr()))
  {
    if (_bamf_app)
    {
      LOG_WARNING(logger) << "Not a view but not null.";
    }
    return;
  }

  nux::Geometry geo;
  geo.width = 48;
  geo.height = 48;

  GList* children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));

  for (GList* l = children; l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
      continue;

    Window xid = bamf_window_get_xid(static_cast<BamfWindow*>(l->data));
    int monitor = bamf_window_get_monitor(static_cast<BamfWindow*>(l->data));
    monitor = std::max<int>(0, std::min<int>(center.size() - 1, monitor));

    geo.x = center[monitor].x - 24;
    geo.y = center[monitor].y - 24;
    WindowManager::Default()->SetWindowIconGeometry(xid, geo);
  }

  g_list_free(children);
}

void BamfLauncherIcon::OnCenterStabilized(std::vector<nux::Point3> center)
{
  UpdateIconGeometries(center);
}

std::string BamfLauncherIcon::GetDesktopID()
{
  std::string const& desktop_file = DesktopFile();

  return DesktopUtilities::GetDesktopID(desktop_file);
}

std::string BamfLauncherIcon::GetRemoteUri()
{
  if (_remote_uri.empty())
  {
    const std::string prefix = "application://";
    std::string const& desktop_id = GetDesktopID();

    if (!desktop_id.empty())
    {
      _remote_uri = prefix + desktop_id;
    }
  }

  return _remote_uri;
}

std::set<std::string> BamfLauncherIcon::ValidateUrisForLaunch(DndData const& uris)
{
  std::set<std::string> result;

  for (auto uri : uris.Uris())
    result.insert(uri);

  return result;
}

void BamfLauncherIcon::OnDndHovered()
{
  // for now, let's not do this, it turns out to be quite buggy
  //if (IsRunning())
  //  Spread(CompAction::StateInitEdgeDnd, true);
}

void BamfLauncherIcon::OnDndEnter()
{
  /* Disabled, since the DND code is currently disabled as well.
  _source_manager.AddTimeout(1000, [&] {
    OnDndHovered();
    return false;
  }, ICON_DND_OVER_TIMEOUT);
  */
}

void BamfLauncherIcon::OnDndLeave()
{
  /* Disabled, since the DND code is currently disabled as well.
  _source_manager.Remove(ICON_DND_OVER_TIMEOUT);
  */
}

bool BamfLauncherIcon::OnShouldHighlightOnDrag(DndData const& dnd_data)
{
  bool is_home_launcher = boost::algorithm::ends_with(DesktopFile(), "nautilus-home.desktop") ||
                          boost::algorithm::ends_with(DesktopFile(), "nautilus.desktop");

  if (is_home_launcher)
  {
    return true;
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

nux::DndAction BamfLauncherIcon::OnQueryAcceptDrop(DndData const& dnd_data)
{
  return ValidateUrisForLaunch(dnd_data).empty() ? nux::DNDACTION_NONE : nux::DNDACTION_COPY;
}

void BamfLauncherIcon::OnAcceptDrop(DndData const& dnd_data)
{
  OpenInstanceWithUris(ValidateUrisForLaunch(dnd_data));
}

bool BamfLauncherIcon::ShowInSwitcher(bool current)
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
      for (int i = 0; i < max_num_monitors; i++)
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

unsigned long long BamfLauncherIcon::SwitcherPriority()
{
  GList* children, *l;
  unsigned long long result = 0;

  children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));

  /* get the list of windows */
  for (l = children; l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
      continue;

    Window xid = bamf_window_get_xid(static_cast<BamfWindow*>(l->data));
    result = std::max(result, WindowManager::Default()->GetWindowActiveNumber(xid));
  }

  g_list_free(children);
  return result;
}

nux::Color BamfLauncherIcon::BackgroundColor() const
{
  if (use_custom_bg_color_)
    return bg_color_;

  return SimpleLauncherIcon::BackgroundColor();
}

const std::set<std::string>& BamfLauncherIcon::GetSupportedTypes()
{
  if (!_supported_types_filled)
    FillSupportedTypes();

  return _supported_types;
}

void BamfLauncherIcon::FillSupportedTypes()
{
  if (!_supported_types_filled)
  {
    _supported_types_filled = true;
    _supported_types.clear();

    std::string const& desktop_file = DesktopFile();

    if (desktop_file.empty())
      return;

    GKeyFile* key_file = g_key_file_new();
    glib::Error error;

    g_key_file_load_from_file(key_file, desktop_file.c_str(), (GKeyFileFlags) 0, &error);

    if (error)
    {
      g_key_file_free(key_file);
      return;
    }

    char** mimes = g_key_file_get_string_list(key_file, "Desktop Entry", "MimeType", nullptr, nullptr);
    if (!mimes)
    {
      g_key_file_free(key_file);
      return;
    }

    for (int i = 0; mimes[i]; i++)
    {
      unity::glib::String super_type(g_content_type_from_mime_type(mimes[i]));
      _supported_types.insert(super_type.Str());
    }

    g_key_file_free(key_file);
    g_strfreev(mimes);
  }
}

std::string BamfLauncherIcon::GetName() const
{
  return "BamfLauncherIcon";
}

} // namespace launcher
} // namespace unity
