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

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>

#include "BamfLauncherIcon.h"
#include "FavoriteStore.h"
#include "Launcher.h"
#include "WindowManager.h"
#include "UBusMessages.h"
#include "ubus-server.h"

#include <glib/gi18n-lib.h>
#include <gio/gdesktopappinfo.h>

#include <UnityCore/GLibWrapper.h>

namespace unity
{
namespace launcher
{

BamfLauncherIcon::BamfLauncherIcon(Launcher* IconManager, BamfApplication* app)
  : SimpleLauncherIcon(IconManager)
  , _bamf_app(app, glib::AddRef())
  , _launcher(IconManager)
  , _desktop_file(nullptr)
  , _dnd_hovered(false)
  , _dnd_hover_timer(0)
  , _supported_types_filled(false)
  , _fill_supported_types_id(0)
  , _window_moved_id(0)
{
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
                            UpdateIconGeometries(GetCenter());
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
                              UpdateIconGeometries(GetCenter());
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
                              Remove();
                          });
  _gsignals.Add(sig);

  WindowManager::Default()->window_minimized.connect(sigc::mem_fun(this, &BamfLauncherIcon::OnWindowMinimized));
  WindowManager::Default()->window_moved.connect(sigc::mem_fun(this, &BamfLauncherIcon::OnWindowMoved));
  WindowManager::Default()->compiz_screen_viewport_switch_ended.connect(sigc::mem_fun(this, &BamfLauncherIcon::EnsureWindowState));
  WindowManager::Default()->terminate_expo.connect(sigc::mem_fun(this, &BamfLauncherIcon::EnsureWindowState));
  IconManager->hidden_changed.connect(sigc::mem_fun(this, &BamfLauncherIcon::OnLauncherHiddenChanged));

  EnsureWindowState();
  UpdateMenus();
  UpdateDesktopFile();

  // hack
  SetProgress(0.0f);

  // Calls when there are no higher priority events pending to the default main loop.
  _fill_supported_types_id = g_idle_add((GSourceFunc)FillSupportedTypes, this);
}

BamfLauncherIcon::~BamfLauncherIcon()
{
  g_object_set_qdata(G_OBJECT(_bamf_app.RawPtr()),
                     g_quark_from_static_string("unity-seen"),
                     GINT_TO_POINTER(0));

  // We might not have created the menu items yet
  for (auto it = _menu_clients.begin(); it != _menu_clients.end(); it++)
  {
    g_object_unref(G_OBJECT(it->second));
  }

  if (_fill_supported_types_id != 0)
    g_source_remove(_fill_supported_types_id);

  if (_window_moved_id != 0)
    g_source_remove(_window_moved_id);
}

bool BamfLauncherIcon::IsSticky()
{
  return bamf_view_is_sticky(BAMF_VIEW(_bamf_app.RawPtr()));
}

bool BamfLauncherIcon::IsVisible()
{
  return GetQuirk(QUIRK_VISIBLE);
}

bool BamfLauncherIcon::IsActive()
{
  return GetQuirk(QUIRK_ACTIVE);
}

bool BamfLauncherIcon::IsRunning()
{
  return GetQuirk(QUIRK_RUNNING);
}

bool BamfLauncherIcon::IsUrgent()
{
  return GetQuirk(QUIRK_URGENT);
}

void BamfLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  SimpleLauncherIcon::ActivateLauncherIcon(arg);
  bool scaleWasActive = WindowManager::Default()->IsScaleActive();
  GList    *l;
  BamfView *view;

  bool active = IsActive();
  bool user_visible = IsRunning();

  if (arg.target && OwnsWindow (arg.target))
  {
    WindowManager::Default()->Activate(arg.target);
    return;
  }

  /* We should check each child to see if there is
   * an unmapped (!= minimized) window around and
   * if so force "Focus" behaviour */

  if (arg.source != ActionArg::SWITCHER)
  {
    user_visible = bamf_view_user_visible(BAMF_VIEW(_bamf_app.RawPtr()));

    bool any_visible = false;
    for (l = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr())); l; l = l->next)
    {
      view = BAMF_VIEW(l->data);

      if (BAMF_IS_WINDOW(view))
      {
        Window xid = bamf_window_get_xid(BAMF_WINDOW(view));

        if (!any_visible && WindowManager::Default()->IsWindowOnCurrentDesktop(xid))
        {
          any_visible = true;
        }
        if (active && !WindowManager::Default()->IsWindowMapped(xid))
        {
          active = false;
        }
      }
    }

    if (!any_visible)
      active = false;
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
      WindowManager::Default()->TerminateScale();
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
        WindowManager::Default()->TerminateScale();
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
        WindowManager::Default()->TerminateScale();
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

std::vector<Window> BamfLauncherIcon::RelatedXids ()
{
  std::vector<Window> results;
  GList* children, *l;
  BamfView* view;
  WindowManager *wm = WindowManager::Default();

  children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));
  for (l = children; l; l = l->next)
  {
    view = BAMF_VIEW(l->data);
    if (BAMF_IS_WINDOW(view))
    {
      guint32 xid = bamf_window_get_xid(BAMF_WINDOW(view));

      if (wm->IsWindowMapped(xid))
      {
        results.push_back (xid);
      }
    }
  }

  g_list_free(children);
  return results;
}

std::string BamfLauncherIcon::NameForWindow(Window window)
{
  std::string result;
  GList* children, *l;
  BamfView* view;

  children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));
  for (l = children; l; l = l->next)
  {
    view = BAMF_VIEW(l->data);
    if (BAMF_IS_WINDOW(view) && bamf_window_get_xid(BAMF_WINDOW(view)) == window)
    {
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

  if (_window_moved_id != 0)
    g_source_remove(_window_moved_id);

  _window_moved_id = g_timeout_add(250, [] (gpointer data) -> gboolean
  {
    BamfLauncherIcon* self = static_cast<BamfLauncherIcon*>(data);
    self->EnsureWindowState();
    self->_window_moved_id = 0;
    return FALSE;
  }, this);
}

void BamfLauncherIcon::OnLauncherHiddenChanged()
{
  UpdateIconGeometries(GetCenter());
}

void BamfLauncherIcon::UpdateDesktopFile()
{
  char* filename = nullptr;
  filename = (char*) bamf_application_get_desktop_file(_bamf_app);

  if (filename != nullptr && _desktop_file != filename)
  {
    _desktop_file = filename;

    // add a file watch to the desktop file so that if/when the app is removed
    // we can remove ourself from the launcher and when it's changed
    // we can update the quicklist.
    if (_desktop_file_monitor)
      _gsignals.Disconnect(_desktop_file_monitor, "changed");

    glib::Object<GFile> desktop_file(g_file_new_for_path(DesktopFile()));
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
                                      break;
                                    default:
                                      break;
                                  }
                                });
    _gsignals.Add(sig);
  }
}

const char* BamfLauncherIcon::DesktopFile()
{
  UpdateDesktopFile();
  return _desktop_file;
}

std::string BamfLauncherIcon::BamfName()
{
  glib::String name(bamf_view_get_name(BAMF_VIEW(_bamf_app.RawPtr())));
  return name.Str();
}

void BamfLauncherIcon::AddProperties(GVariantBuilder* builder)
{
  LauncherIcon::AddProperties(builder);

  g_variant_builder_add(builder, "{sv}", "desktop-file", g_variant_new_string(DesktopFile()));

  GList* children, *l;
  BamfView* view;

  children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));
  GVariant* xids[(int) g_list_length(children)];

  int i = 0;
  for (l = children; l; l = l->next)
  {
    view = BAMF_VIEW(l->data);

    if (BAMF_IS_WINDOW(view))
    {
      xids[i++] = g_variant_new_uint32(bamf_window_get_xid(BAMF_WINDOW(view)));
    }
  }
  g_list_free(children);
  g_variant_builder_add(builder, "{sv}", "xids", g_variant_new_array(G_VARIANT_TYPE_UINT32, xids, i));
}

bool BamfLauncherIcon::OwnsWindow(Window w)
{
  GList* children, *l;
  BamfView* view;
  bool owns = false;

  if (!w) return owns;

  children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));

  for (l = children; l; l = l->next)
  {
    view = BAMF_VIEW(l->data);

    if (BAMF_IS_WINDOW(view))
    {
      guint32 xid = bamf_window_get_xid(BAMF_WINDOW(view));

      if (xid == w)
      {
        owns = true;
        break;
      }
    }
  }

  g_list_free(children);
  return owns;
}

void BamfLauncherIcon::OpenInstanceWithUris(std::set<std::string> uris)
{
  GDesktopAppInfo* appInfo;
  GError* error = nullptr;

  appInfo = g_desktop_app_info_new_from_filename(DesktopFile());

  if (g_app_info_supports_uris(G_APP_INFO(appInfo)))
  {
    GList* list = nullptr;

    for (auto  it : uris)
      list = g_list_prepend(list, g_strdup(it.c_str()));

    g_app_info_launch_uris(G_APP_INFO(appInfo), list, nullptr, &error);
    g_list_free_full(list, g_free);
  }
  else if (g_app_info_supports_files(G_APP_INFO(appInfo)))
  {
    GList* list = nullptr, *l;

    for (auto it : uris)
    {
      GFile* file = g_file_new_for_uri(it.c_str());
      list = g_list_prepend(list, file);
    }
    g_app_info_launch(G_APP_INFO(appInfo), list, nullptr, &error);

    for (l = list; l; l = l->next)
      g_object_unref(G_FILE(list->data));

    g_list_free(list);
  }
  else
  {
    g_app_info_launch(G_APP_INFO(appInfo), nullptr, nullptr, &error);
  }

  g_object_unref(appInfo);

  if (error)
  {
    g_warning("%s\n", error->message);
    g_error_free(error);
  }

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
  GList* children, *l;
  BamfView* view;
  bool any_urgent = false;
  bool any_visible = false;
  bool any_user_visible = false;

  children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));

  std::vector<Window> windows;

  /* get the list of windows */
  for (l = children; l; l = l->next)
  {
    view = BAMF_VIEW(l->data);

    if (BAMF_IS_WINDOW(view))
    {
      Window xid = bamf_window_get_xid(BAMF_WINDOW(view));
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

      if (WindowManager::Default()->IsWindowOnCurrentDesktop(xid) &&
          WindowManager::Default()->IsWindowVisible(xid))
      {
        any_visible = true;
      }
    }
  }
  g_list_free(children);

  if (arg.source != ActionArg::SWITCHER)
  {
    if (any_visible)
    {
      WindowManager::Default()->FocusWindowGroup(windows,
       WindowManager::FocusVisibility::ForceUnminimizeInvisible);
    }
    else
    {
      WindowManager::Default()->FocusWindowGroup(windows,
       WindowManager::FocusVisibility::ForceUnminimizeOnCurrentDesktop);
    }
  }
  else
  {
    WindowManager::Default()->FocusWindowGroup(windows,
      WindowManager::FocusVisibility::OnlyVisible);
  }
}

bool BamfLauncherIcon::Spread(bool current_desktop, int state, bool force)
{
  BamfView* view;
  GList* children, *l;
  children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));
  WindowManager* wm = WindowManager::Default();

  std::vector<Window> windowList;
  for (l = children; l; l = l->next)
  {
    view = BAMF_VIEW(l->data);

    if (BAMF_IS_WINDOW(view))
    {
      guint32 xid = bamf_window_get_xid(BAMF_WINDOW(view));

      if (!current_desktop || (current_desktop && wm->IsWindowOnCurrentDesktop(xid)))
      {
        windowList.push_back((Window) xid);
      }
    }
  }

  g_list_free(children);
  return WindowManager::Default()->ScaleWindowGroup(windowList, state, force);
}

void BamfLauncherIcon::EnsureWindowState()
{
  GList* children, *l;
  bool has_win_on_current_vp = false;
  unsigned int user_visible_count = 0;
  unsigned int children_count = 0;

  children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));
  for (l = children; l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
      continue;

    Window xid = bamf_window_get_xid(BAMF_WINDOW(l->data));
    if (WindowManager::Default()->IsWindowOnCurrentDesktop(xid))
    {
      has_win_on_current_vp = true;

      if (bamf_view_user_visible (BAMF_VIEW (l->data)))
      {
        user_visible_count++;
      }
    }

    children_count++;
  }

  if (user_visible_count > 0)
  {
    SetRelatedWindows(user_visible_count);
  }
  else if (children_count > 0)
  {
    SetRelatedWindows(1);
  }

  SetHasWindowOnViewport(has_win_on_current_vp);

  g_list_free(children);
}

void BamfLauncherIcon::UpdateDesktopQuickList()
{
  GKeyFile* keyfile;
  GError* error = nullptr;
  const char *desktop_file;

  desktop_file = DesktopFile();

  if (!desktop_file || desktop_file[0] == '\0')
    return;

  // check that we have the X-Ayatana-Desktop-Shortcuts flag
  // not sure if we should do this or if libindicator should shut up
  // and not report errors when it can't find the key.
  // so FIXME when ted is around
  keyfile = g_key_file_new();
  g_key_file_load_from_file(keyfile, desktop_file, G_KEY_FILE_NONE, &error);

  if (error != nullptr)
  {
    g_warning("Could not load desktop file for: %s", desktop_file);
    g_key_file_free(keyfile);
    g_error_free(error);
    return;
  }

  if (g_key_file_has_key(keyfile, G_KEY_FILE_DESKTOP_GROUP,
                         "X-Ayatana-Desktop-Shortcuts", nullptr))
  {
    for (GList *l = dbusmenu_menuitem_get_children(_menu_desktop_shortcuts); l; l = l->next)
      _gsignals.Disconnect(l->data, "item-activated");

    _menu_desktop_shortcuts = dbusmenu_menuitem_new();
    dbusmenu_menuitem_set_root(_menu_desktop_shortcuts, TRUE);

    _desktop_shortcuts = indicator_desktop_shortcuts_new(desktop_file, "Unity");
    const gchar** nicks = indicator_desktop_shortcuts_get_nicks(_desktop_shortcuts);

    int index = 0;
    if (nicks)
    {
      while (nicks[index])
      {
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
  }

  g_key_file_free(keyfile);
}

void BamfLauncherIcon::UpdateMenus()
{
  GList* children, *l;

  children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));
  for (l = children; l; l = l->next)
  {
    if (!BAMF_IS_INDICATOR(l->data))
      continue;

    BamfIndicator* indicator = BAMF_INDICATOR(l->data);
    std::string path = bamf_indicator_get_dbus_menu_path(indicator);

    // we already have this
    if (_menu_clients.find(path) != _menu_clients.end())
      continue;

    DbusmenuClient* client = dbusmenu_client_new(bamf_indicator_get_remote_address(indicator), path.c_str());
    _menu_clients[path] = client;
  }

  g_list_free(children);

  // add dynamic quicklist
  if (_menuclient_dynamic_quicklist != nullptr)
  {
    auto menu_client = DBUSMENU_CLIENT(g_object_ref(_menuclient_dynamic_quicklist));
    _menu_clients["dynamicquicklist"] = menu_client;
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
  BamfView* view;

  children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));

  for (l = children; l; l = l->next)
  {
    view = BAMF_VIEW(l->data);

    if (BAMF_IS_WINDOW(view))
    {
      guint32 xid = bamf_window_get_xid(BAMF_WINDOW(view));
      WindowManager::Default()->Close(xid);
    }
  }

  g_list_free(children);
}

void BamfLauncherIcon::Stick()
{
  if (IsSticky())
    return;

  const gchar* desktop_file = DesktopFile();
  bamf_view_set_sticky(BAMF_VIEW(_bamf_app.RawPtr()), true);

  if (desktop_file && desktop_file[0] != '\0')
    FavoriteStore::GetDefault().AddFavorite(desktop_file, -1);
}

void BamfLauncherIcon::UnStick()
{
  if (!IsSticky())
    return;

  const gchar* desktop_file = DesktopFile();
  BamfView* view = BAMF_VIEW(_bamf_app.RawPtr());
  bamf_view_set_sticky(view, false);

  if (bamf_view_is_closed(view) || !bamf_view_user_visible(view))
    Remove();

  if (desktop_file && desktop_file[0] != '\0')
    FavoriteStore::GetDefault().RemoveFavorite(desktop_file);
}

void BamfLauncherIcon::ToggleSticky()
{
  if (IsSticky())
  {
    UnStick();
  }
  else
  {
    bamf_view_set_sticky(BAMF_VIEW(_bamf_app.RawPtr()), true);
    const gchar* desktop_file = DesktopFile();

    if (desktop_file && strlen(desktop_file) > 0)
      FavoriteStore::GetDefault().AddFavorite(desktop_file, -1);
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

  const char* label = !IsSticky() ? _("Lock to launcher") : _("Unlock from launcher");

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
    DbusmenuClient* client = (*it).second;
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
                                      ActivateLauncherIcon(ActionArg(ActionArg::OTHER, 0));
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

void BamfLauncherIcon::UpdateIconGeometries(nux::Point3 center)
{
  GList* children, *l;
  BamfView* view;
  nux::Geometry geo;

  if (_launcher->Hidden() && !_launcher->ShowOnEdge())
  {
    geo.x = 0;
    geo.y = 0;
  }
  else
  {
    geo.x = center.x - 24;
    geo.y = center.y - 24;
  }
  geo.width = 48;
  geo.height = 48;

  children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));

  for (l = children; l; l = l->next)
  {
    view = BAMF_VIEW(l->data);

    if (BAMF_IS_WINDOW(view))
    {
      guint32 xid = bamf_window_get_xid(BAMF_WINDOW(view));
      WindowManager::Default()->SetWindowIconGeometry((Window)xid, geo);
    }
  }

  g_list_free(children);
}

void BamfLauncherIcon::OnCenterStabilized(nux::Point3 center)
{
  UpdateIconGeometries(center);
}

const gchar* BamfLauncherIcon::GetRemoteUri()
{
  if (_remote_uri.empty())
  {
    std::ostringstream remote_uri_stream;
    const gchar* desktop_file = DesktopFile();
    glib::String basename(g_path_get_basename(desktop_file));

    remote_uri_stream << "application://" << basename.Str();
    _remote_uri = remote_uri_stream.str();
  }

  return _remote_uri.c_str();
}

std::set<std::string> BamfLauncherIcon::ValidateUrisForLaunch(unity::DndData& uris)
{
  std::set<std::string> result;
  gboolean is_home_launcher = g_str_has_suffix(DesktopFile(), "nautilus-home.desktop");

  if (is_home_launcher)
  {
    for (auto k : uris.Uris())
      result.insert(k);
    return result;
  }

  for (auto i : uris.Types())
  {
    for (auto j : GetSupportedTypes())
    {
      if (g_content_type_is_a(i.c_str(), j.c_str()))
      {
        for (auto k : uris.UrisByType(i))
          result.insert(k);

        break;
      }
    }
  }

  return result;
}

gboolean BamfLauncherIcon::OnDndHoveredTimeout(gpointer data)
{
  BamfLauncherIcon* self = static_cast <BamfLauncherIcon*> (data);

  // for now, let's not do this, it turns out to be quite buggy
  //if (self->_dnd_hovered && self->IsRunning())
  //  self->Spread(CompAction::StateInitEdgeDnd, true);

  self->_dnd_hover_timer = 0;
  return false;
}

void BamfLauncherIcon::OnDndEnter()
{
  _dnd_hovered = true;
  _dnd_hover_timer = g_timeout_add(1000, &BamfLauncherIcon::OnDndHoveredTimeout, this);
}

void BamfLauncherIcon::OnDndLeave()
{
  _dnd_hovered = false;

  if (_dnd_hover_timer)
    g_source_remove(_dnd_hover_timer);
  _dnd_hover_timer = 0;
}

nux::DndAction BamfLauncherIcon::OnQueryAcceptDrop(unity::DndData& dnd_data)
{
  return ValidateUrisForLaunch(dnd_data).empty() ? nux::DNDACTION_NONE : nux::DNDACTION_COPY;
}

void BamfLauncherIcon::OnAcceptDrop(unity::DndData& dnd_data)
{
  OpenInstanceWithUris(ValidateUrisForLaunch(dnd_data));
}

bool BamfLauncherIcon::ShowInSwitcher()
{
  return IsRunning() && IsVisible();
}

unsigned long long BamfLauncherIcon::SwitcherPriority()
{
  GList* children, *l;
  BamfView* view;
  unsigned long long result = 0;

  children = bamf_view_get_children(BAMF_VIEW(_bamf_app.RawPtr()));

  /* get the list of windows */
  for (l = children; l; l = l->next)
  {
    view = BAMF_VIEW(l->data);

    if (BAMF_IS_WINDOW(view))
    {
      guint32 xid = bamf_window_get_xid(BAMF_WINDOW(view));
      result = std::max(result, WindowManager::Default()->GetWindowActiveNumber(xid));
    }
  }

  g_list_free(children);
  return result;
}

const std::set<std::string>& BamfLauncherIcon::GetSupportedTypes()
{
  if (!_supported_types_filled)
    FillSupportedTypes(this);

  return _supported_types;
}

gboolean BamfLauncherIcon::FillSupportedTypes(gpointer data)
{
  BamfLauncherIcon* self = static_cast <BamfLauncherIcon*>(data);

  if (self->_fill_supported_types_id)
  {
    g_source_remove(self->_fill_supported_types_id);
    self->_fill_supported_types_id = 0;
  }

  if (!self->_supported_types_filled)
  {
    self->_supported_types_filled = true;

    self->_supported_types.clear();

    const char* desktop_file = self->DesktopFile();

    if (!desktop_file || strlen(desktop_file) <= 1)
      return false;

    GKeyFile* key_file = g_key_file_new();
    unity::glib::Error error;

    g_key_file_load_from_file(key_file, desktop_file, (GKeyFileFlags) 0, &error);

    if (error)
    {
      g_key_file_free(key_file);
      return false;
    }

    char** mimes = g_key_file_get_string_list(key_file, "Desktop Entry", "MimeType", nullptr, nullptr);
    if (!mimes)
    {
      g_key_file_free(key_file);
      return false;
    }

    for (int i = 0; mimes[i]; i++)
    {
      unity::glib::String super_type(g_content_type_from_mime_type(mimes[i]));
      self->_supported_types.insert(super_type.Str());
    }

    g_key_file_free(key_file);
    g_strfreev(mimes);
  }

  return false;
}

} // namespace launcher
} // namespace unity
