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
#include <libindicator/indicator-desktop-shortcuts.h>

#include <UnityCore/GLibWrapper.h>

namespace unity
{
namespace launcher
{

struct _ShortcutData
{
  BamfLauncherIcon* self;
  IndicatorDesktopShortcuts* shortcuts;
  char* nick;
};
typedef struct _ShortcutData ShortcutData;
static void shortcut_data_destroy(ShortcutData* data)
{
  g_object_unref(data->shortcuts);
  g_free(data->nick);
  g_slice_free(ShortcutData, data);
}

static void shortcut_activated(DbusmenuMenuitem* _sender, guint timestamp, gpointer userdata)
{
  ShortcutData* data = (ShortcutData*)userdata;
  indicator_desktop_shortcuts_nick_exec(data->shortcuts, data->nick);
}

void BamfLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  SimpleLauncherIcon::ActivateLauncherIcon(arg);
  bool scaleWasActive = WindowManager::Default()->IsScaleActive();
  GList    *l;
  BamfView *view;

  bool active, running;
  active = bamf_view_is_active(BAMF_VIEW(m_App));
  running = bamf_view_is_running(BAMF_VIEW(m_App));

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
    bool any_visible = false;
    for (l = bamf_view_get_children(BAMF_VIEW(m_App)); l; l = l->next)
    {
      view = static_cast <BamfView*> (l->data);

      if (BAMF_IS_WINDOW(view))
      {
        Window xid = bamf_window_get_xid(BAMF_WINDOW(view));

        if (WindowManager::Default ()->IsWindowOnCurrentDesktop(xid))
          any_visible = true;
        if (!WindowManager::Default ()->IsWindowMapped(xid))
        {
          active = false;
        }
      }
    }

    if (!any_visible)
      active = false;
  }

  /* Behaviour:
   * 1) Nothing running -> launch application
   * 2) Running and active -> spread application
   * 3) Running and not active -> focus application
   * 4) Spread is active and different icon pressed -> change spread
   * 5) Spread is active -> Spread de-activated, and fall through
   */

  if (!running) // #1 above
  {
    if (GetQuirk(QUIRK_STARTING))
      return;

    if (scaleWasActive)
    {
      WindowManager::Default()->TerminateScale();
    }

    SetQuirk(QUIRK_STARTING, true);
    OpenInstanceLauncherIcon(ActionArg ());
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
          Spread(0, false);
      }
    }
    else
    {
      if (scaleWasActive) // #4 above
      {
        WindowManager::Default()->TerminateScale();
        Focus(arg);
        if (arg.source != ActionArg::SWITCHER)
          Spread(0, false);
      }
      else // #3 above
      {
        Focus(arg);
      }
    }
  }

  if (arg.source != ActionArg::SWITCHER)
    ubus_server_send_message(ubus_server_get_default(), UBUS_LAUNCHER_ACTION_DONE, NULL);
}

BamfLauncherIcon::BamfLauncherIcon(Launcher* IconManager, BamfApplication* app)
  : SimpleLauncherIcon(IconManager)
  , _supported_types_filled(false)
  , _fill_supported_types_id(0)
{
  _cached_desktop_file = NULL;
  _cached_name = NULL;
  m_App = app;
  _remote_uri = 0;
  _dnd_hover_timer = 0;
  _dnd_hovered = false;
  _launcher = IconManager;
  _desktop_file_monitor = NULL;
  _menu_desktop_shortcuts = NULL;
  _on_desktop_file_changed_handler_id = 0;
  _window_moved_id = 0;
  glib::String icon(bamf_view_get_icon(BAMF_VIEW(m_App)));

  tooltip_text = BamfName();
  icon_name = icon.Str();
  SetIconType(TYPE_APPLICATION);

  if (bamf_view_is_sticky(BAMF_VIEW(m_App)))
    SetQuirk(QUIRK_VISIBLE, true);
  else
    SetQuirk(QUIRK_VISIBLE, bamf_view_user_visible(BAMF_VIEW(m_App)));

  SetQuirk(QUIRK_ACTIVE, bamf_view_is_active(BAMF_VIEW(m_App)));
  SetQuirk(QUIRK_RUNNING, bamf_view_is_running(BAMF_VIEW(m_App)));

  g_signal_connect(app, "child-removed", (GCallback) &BamfLauncherIcon::OnChildRemoved, this);
  g_signal_connect(app, "child-added", (GCallback) &BamfLauncherIcon::OnChildAdded, this);
  g_signal_connect(app, "urgent-changed", (GCallback) &BamfLauncherIcon::OnUrgentChanged, this);
  g_signal_connect(app, "running-changed", (GCallback) &BamfLauncherIcon::OnRunningChanged, this);
  g_signal_connect(app, "active-changed", (GCallback) &BamfLauncherIcon::OnActiveChanged, this);
  g_signal_connect(app, "user-visible-changed", (GCallback) &BamfLauncherIcon::OnUserVisibleChanged, this);
  g_signal_connect(app, "closed", (GCallback) &BamfLauncherIcon::OnClosed, this);

  g_object_ref(m_App);

  EnsureWindowState();
  UpdateMenus();

  UpdateDesktopFile();

  WindowManager::Default()->window_minimized.connect(sigc::mem_fun(this, &BamfLauncherIcon::OnWindowMinimized));
  WindowManager::Default()->window_moved.connect(sigc::mem_fun(this, &BamfLauncherIcon::OnWindowMoved));
  WindowManager::Default()->compiz_screen_viewport_switch_ended.connect(sigc::mem_fun(this, &BamfLauncherIcon::OnViewPortSwitchEnded));
  WindowManager::Default()->terminate_expo.connect(sigc::mem_fun(this, &BamfLauncherIcon::OnViewPortSwitchEnded));
  IconManager->hidden_changed.connect(sigc::mem_fun(this, &BamfLauncherIcon::OnLauncherHiddenChanged));

  // hack
  SetProgress(0.0f);

  // Calls when there are no higher priority events pending to the default main loop.
  _fill_supported_types_id = g_idle_add((GSourceFunc)FillSupportedTypes, this);

}

BamfLauncherIcon::~BamfLauncherIcon()
{
  g_object_set_qdata(G_OBJECT(m_App), g_quark_from_static_string("unity-seen"), GINT_TO_POINTER(0));

  // We might not have created the menu items yet
  for (auto it = _menu_clients.begin(); it != _menu_clients.end(); it++)
  {
    g_object_unref(G_OBJECT(it->second));
  }

  if (_menu_items.find("Pin") != _menu_items.end())
  {
    g_signal_handler_disconnect(G_OBJECT(_menu_items["Pin"]),
                                _menu_callbacks["Pin"]);
  }

  if (_menu_items.find("Quit") != _menu_items.end())
  {
    g_signal_handler_disconnect(G_OBJECT(_menu_items["Quit"]),
                                _menu_callbacks["Quit"]);
  }

  for (auto it = _menu_items.begin(); it != _menu_items.end(); it++)
  {
    g_object_unref(G_OBJECT(it->second));
  }

  for (auto it = _menu_items_extra.begin(); it != _menu_items_extra.end(); it++)
  {
    g_object_unref(G_OBJECT(it->second));
  }

  if (G_IS_OBJECT(_menu_desktop_shortcuts))
    g_object_unref(G_OBJECT(_menu_desktop_shortcuts));

  if (_on_desktop_file_changed_handler_id != 0)
    g_signal_handler_disconnect(G_OBJECT(_desktop_file_monitor),
                                _on_desktop_file_changed_handler_id);

  if (_fill_supported_types_id != 0)
    g_source_remove(_fill_supported_types_id);
  
  if (_window_moved_id != 0)
    g_source_remove(_window_moved_id);

  g_signal_handlers_disconnect_by_func(m_App, (void*) &BamfLauncherIcon::OnChildRemoved,       this);
  g_signal_handlers_disconnect_by_func(m_App, (void*) &BamfLauncherIcon::OnChildAdded,         this);
  g_signal_handlers_disconnect_by_func(m_App, (void*) &BamfLauncherIcon::OnUrgentChanged,      this);
  g_signal_handlers_disconnect_by_func(m_App, (void*) &BamfLauncherIcon::OnRunningChanged,     this);
  g_signal_handlers_disconnect_by_func(m_App, (void*) &BamfLauncherIcon::OnActiveChanged,      this);
  g_signal_handlers_disconnect_by_func(m_App, (void*) &BamfLauncherIcon::OnUserVisibleChanged, this);
  g_signal_handlers_disconnect_by_func(m_App, (void*) &BamfLauncherIcon::OnClosed,             this);

  g_object_unref(m_App);
  g_object_unref(_desktop_file_monitor);

  g_free(_cached_desktop_file);
  g_free(_cached_name);
}

std::vector<Window> BamfLauncherIcon::RelatedXids ()
{
  std::vector<Window> results;
  GList* children, *l;
  BamfView* view;
  WindowManager *wm = WindowManager::Default ();

  children = bamf_view_get_children(BAMF_VIEW(m_App));
  for (l = children; l; l = l->next)
  {
    view = static_cast <BamfView*> (l->data);
    if (BAMF_IS_WINDOW(view))
    {
      guint32 xid = bamf_window_get_xid(BAMF_WINDOW(view));

      if (wm->IsWindowMapped(xid))
        results.push_back ((Window) xid);
    }
  }

  g_list_free(children);
  return results;
}

std::string BamfLauncherIcon::NameForWindow (Window window)
{
  std::string result;
  GList* children, *l;
  BamfView* view;

  children = bamf_view_get_children(BAMF_VIEW(m_App));
  for (l = children; l; l = l->next)
  {
    view = static_cast <BamfView*> (l->data);
    if (BAMF_IS_WINDOW(view) && (Window) bamf_window_get_xid(BAMF_WINDOW(view)) == window)
    {
      gchar *name = bamf_view_get_name (view);
      result = name;
      g_free (name);
      break;
    }
  }

  g_list_free(children);
  return result;
}

void BamfLauncherIcon::OnLauncherHiddenChanged()
{
  UpdateIconGeometries(GetCenter());
}

void BamfLauncherIcon::OnWindowMinimized(guint32 xid)
{
  if (!OwnsWindow(xid))
    return;

  Present(0.5f, 600);
  UpdateQuirkTimeDelayed(300, QUIRK_SHIMMER);
}

gboolean BamfLauncherIcon::OnWindowMovedTimeout(gpointer data)
{
  BamfLauncherIcon *self = static_cast <BamfLauncherIcon *> (data);
  GList *children = bamf_view_get_children(BAMF_VIEW(self->m_App));

  bool any_on_current = false;
  bool found_moved = (self->_window_moved_xid != 0 ? false : true);

  for (GList *l = children; l; l = l->next)
  {
    BamfView *view = BAMF_VIEW(l->data);

    if (BAMF_IS_WINDOW(view))
    {
      Window xid = bamf_window_get_xid(BAMF_WINDOW(view));

      if (self->_window_moved_xid == xid)
        found_moved = true;
      
      if (WindowManager::Default()->IsWindowOnCurrentDesktop(xid))
        any_on_current = true;

      if (found_moved && any_on_current)
        break;
    }
  }

  self->SetHasWindowOnViewport(any_on_current);
  self->_window_moved_id = 0;
  g_list_free(children);

  return FALSE;
}

void BamfLauncherIcon::OnWindowMoved(guint32 moved_win)
{
  if (_window_moved_id != 0)
    g_source_remove(_window_moved_id);

  _window_moved_xid = moved_win;

  if (_window_moved_xid == 0)
  {
    OnWindowMovedTimeout(this);
  }
  else
  {
    _window_moved_id = g_timeout_add(250,
                      (GSourceFunc)BamfLauncherIcon::OnWindowMovedTimeout, this);
  }
}

void BamfLauncherIcon::OnViewPortSwitchEnded()
{
  OnWindowMoved(0);
}

bool BamfLauncherIcon::IsSticky()
{
  return bamf_view_is_sticky(BAMF_VIEW(m_App));
}

void BamfLauncherIcon::UpdateDesktopFile()
{
  char* filename = NULL;
  filename = (char*) bamf_application_get_desktop_file(m_App);

  if (filename != NULL && g_strcmp0(_cached_desktop_file, filename) != 0)
  {
    if (_cached_desktop_file != NULL)
      g_free(_cached_desktop_file);

    _cached_desktop_file = g_strdup(filename);

    // add a file watch to the desktop file so that if/when the app is removed
    // we can remove ourself from the launcher and when it's changed
    // we can update the quicklist.
    if (_desktop_file_monitor)
    {
      if (_on_desktop_file_changed_handler_id != 0)
        g_signal_handler_disconnect(G_OBJECT(_desktop_file_monitor),
                                    _on_desktop_file_changed_handler_id);
      g_object_unref(_desktop_file_monitor);  
    }

    GFile* desktop_file = g_file_new_for_path(DesktopFile());
    _desktop_file_monitor = g_file_monitor_file(desktop_file, G_FILE_MONITOR_NONE,
                                                NULL, NULL);
    g_file_monitor_set_rate_limit (_desktop_file_monitor, 1000);
    _on_desktop_file_changed_handler_id = g_signal_connect(_desktop_file_monitor,
                                                           "changed",
                                                           G_CALLBACK(&BamfLauncherIcon::OnDesktopFileChanged),
                                                           this);
  }
}

const char* BamfLauncherIcon::DesktopFile()
{
  UpdateDesktopFile();
  return _cached_desktop_file;
}

const char* BamfLauncherIcon::BamfName()
{
  gchar* name = bamf_view_get_name(BAMF_VIEW(m_App));

  if (name == NULL)
    name = g_strdup("");

  if (_cached_name != NULL)
    g_free(_cached_name);

  _cached_name = name;

  return _cached_name;
}

void BamfLauncherIcon::AddProperties(GVariantBuilder* builder)
{
  LauncherIcon::AddProperties(builder);

  g_variant_builder_add(builder, "{sv}", "desktop-file", g_variant_new_string(DesktopFile()));

  GList* children, *l;
  BamfView* view;

  children = bamf_view_get_children(BAMF_VIEW(m_App));
  GVariant* xids[(int) g_list_length(children)];

  int i = 0;
  for (l = children; l; l = l->next)
  {
    view = static_cast <BamfView*> (l->data);

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

  children = bamf_view_get_children(BAMF_VIEW(m_App));

  for (l = children; l; l = l->next)
  {
    view = static_cast <BamfView*> (l->data);

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
  GError* error = NULL;

  appInfo = g_desktop_app_info_new_from_filename(DesktopFile());

  if (g_app_info_supports_uris(G_APP_INFO(appInfo)))
  {
    GList* list = NULL;

    for (auto  it : uris)
      list = g_list_prepend(list, g_strdup(it.c_str()));

    g_app_info_launch_uris(G_APP_INFO(appInfo), list, NULL, &error);
    g_list_free_full(list, g_free);
  }
  else if (g_app_info_supports_files(G_APP_INFO(appInfo)))
  {
    GList* list = NULL, *l;
    
    for (auto it : uris)
    {
      GFile* file = g_file_new_for_uri(it.c_str());
      list = g_list_prepend(list, file);
    }
    g_app_info_launch(G_APP_INFO(appInfo), list, NULL, &error);

    for (l = list; l; l = l->next)
      g_object_unref(G_FILE(list->data));

    g_list_free(list);
  }
  else
  {
    g_app_info_launch(G_APP_INFO(appInfo), NULL, NULL, &error);
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
  ubus_server_send_message(ubus_server_get_default(), UBUS_LAUNCHER_ACTION_DONE, NULL);
}

void BamfLauncherIcon::Focus(ActionArg arg)
{
  GList* children, *l;
  BamfView* view;
  bool any_urgent = false;
  bool any_visible = false;

  children = bamf_view_get_children(BAMF_VIEW(m_App));

  std::vector<Window> windows;

  /* get the list of windows */
  for (l = children; l; l = l->next)
  {
    view = static_cast <BamfView*> (l->data);

    if (BAMF_IS_WINDOW(view))
    {
      Window xid = bamf_window_get_xid(BAMF_WINDOW(view));
      bool urgent = bamf_view_is_urgent(view);

      if (WindowManager::Default ()->IsWindowOnCurrentDesktop (xid) &&
          WindowManager::Default ()->IsWindowVisible (xid))
        any_visible = true;

      if (any_urgent)
      {
        if (urgent)
          windows.push_back(xid);
      }
      else
      {
        if (urgent)
        {
          windows.clear();
          any_visible = false;
          any_urgent = true;
        }
      }
      windows.push_back(xid);
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
    WindowManager::Default()->FocusWindowGroup(windows,
      WindowManager::FocusVisibility::OnlyVisible);
}

bool BamfLauncherIcon::Spread(int state, bool force)
{
  BamfView* view;
  GList* children, *l;
  children = bamf_view_get_children(BAMF_VIEW(m_App));

  std::vector<Window> windowList;
  for (l = children; l; l = l->next)
  {
    view = static_cast <BamfView*> (l->data);

    if (BAMF_IS_WINDOW(view))
    {
      guint32 xid = bamf_window_get_xid(BAMF_WINDOW(view));
      windowList.push_back((Window) xid);
    }
  }

  g_list_free(children);
  return WindowManager::Default()->ScaleWindowGroup(windowList, state, force);
}

void BamfLauncherIcon::OnClosed(BamfView* view, gpointer data)
{
  BamfLauncherIcon* self = static_cast <BamfLauncherIcon*> (data);

  if (!bamf_view_is_sticky(BAMF_VIEW(self->m_App)))
    self->Remove();
}

void BamfLauncherIcon::OnUserVisibleChanged(BamfView* view, gboolean visible,
                                            gpointer data)
{
  BamfLauncherIcon* self = static_cast <BamfLauncherIcon*> (data);

  if (!bamf_view_is_sticky(BAMF_VIEW(self->m_App)))
    self->SetQuirk(QUIRK_VISIBLE, visible);
}

void BamfLauncherIcon::OnRunningChanged(BamfView* view, gboolean running,
                                        gpointer data)
{
  BamfLauncherIcon* self = static_cast <BamfLauncherIcon*> (data);
  self->SetQuirk(QUIRK_RUNNING, running);

  if (running)
  {
    self->EnsureWindowState();
    self->UpdateIconGeometries(self->GetCenter());
  }
}

void BamfLauncherIcon::OnActiveChanged(BamfView* view, gboolean active, gpointer data)
{
  BamfLauncherIcon* self = static_cast <BamfLauncherIcon*> (data);
  self->SetQuirk(QUIRK_ACTIVE, active);
}

void BamfLauncherIcon::OnUrgentChanged(BamfView* view, gboolean urgent, gpointer data)
{
  BamfLauncherIcon* self = static_cast <BamfLauncherIcon*> (data);
  self->SetQuirk(QUIRK_URGENT, urgent);
}

void BamfLauncherIcon::EnsureWindowState()
{
  GList* children, *l;
  int count = 0;
  bool has_visible = false;

  children = bamf_view_get_children(BAMF_VIEW(m_App));
  for (l = children; l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
      continue;

    if (!has_visible)
    {
      Window xid = bamf_window_get_xid(BAMF_WINDOW(l->data));
      if (WindowManager::Default()->IsWindowOnCurrentDesktop(xid))
        has_visible = true;
    }

    count++;
  }

  SetRelatedWindows(count);
  SetHasWindowOnViewport(has_visible);

  g_list_free(children);
}

void BamfLauncherIcon::OnChildAdded(BamfView* view, BamfView* child, gpointer data)
{
  BamfLauncherIcon* self = static_cast <BamfLauncherIcon*> (data);
  self->EnsureWindowState();
  self->UpdateMenus();
  self->UpdateIconGeometries(self->GetCenter());
}

void BamfLauncherIcon::OnChildRemoved(BamfView* view, BamfView* child, gpointer data)
{
  BamfLauncherIcon* self = static_cast <BamfLauncherIcon*> (data);
  self->EnsureWindowState();
}

void BamfLauncherIcon::UpdateDesktopQuickList()
{
  IndicatorDesktopShortcuts* desktop_shortcuts;
  GKeyFile* keyfile;
  GError* error = NULL;
  const char *desktop_file;

  desktop_file = DesktopFile();

  if (!desktop_file || g_strcmp0(desktop_file, "") == 0)
    return;

  // check that we have the X-Ayatana-Desktop-Shortcuts flag
  // not sure if we should do this or if libindicator should shut up
  // and not report errors when it can't find the key.
  // so FIXME when ted is around
  keyfile = g_key_file_new();
  g_key_file_load_from_file(keyfile, desktop_file, G_KEY_FILE_NONE, &error);

  if (error != NULL)
  {
    g_warning("Could not load desktop file for: %s", desktop_file);
    g_key_file_free(keyfile);
    g_error_free(error);
    return;
  }

  if (g_key_file_has_key(keyfile, G_KEY_FILE_DESKTOP_GROUP,
                         "X-Ayatana-Desktop-Shortcuts", NULL))
  {
    DbusmenuMenuitem* root = dbusmenu_menuitem_new();
    dbusmenu_menuitem_set_root(root, TRUE);
    desktop_shortcuts = indicator_desktop_shortcuts_new(desktop_file, "Unity");
    const gchar** nicks = indicator_desktop_shortcuts_get_nicks(desktop_shortcuts);

    int index = 0;
    if (nicks)
    {
      while (((gpointer*) nicks)[index])
      {
        gchar* name;
        DbusmenuMenuitem* item;
        name = indicator_desktop_shortcuts_nick_get_name(desktop_shortcuts,
                                                         nicks[index]);
        ShortcutData* data = g_slice_new0(ShortcutData);
        data->self = this;
        data->shortcuts = INDICATOR_DESKTOP_SHORTCUTS(g_object_ref(desktop_shortcuts));
        data->nick = g_strdup(nicks[index]);

        item = dbusmenu_menuitem_new();
        dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, name);
        dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
        dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
        g_signal_connect_data(item, "item-activated",
                              (GCallback) shortcut_activated, (gpointer) data,
                              (GClosureNotify) shortcut_data_destroy, (GConnectFlags)0);

        dbusmenu_menuitem_child_append(root, item);

        index++;

        g_free(name);
      }
    }

    if (G_IS_OBJECT(_menu_desktop_shortcuts))
      g_object_unref(G_OBJECT(_menu_desktop_shortcuts));

    _menu_desktop_shortcuts = root;
  }

  g_key_file_free(keyfile);
}

void BamfLauncherIcon::UpdateMenus()
{
  GList* children, *l;

  children = bamf_view_get_children(BAMF_VIEW(m_App));
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
  if (_menuclient_dynamic_quicklist != NULL)
  {
    auto menu_client = DBUSMENU_CLIENT(g_object_ref(_menuclient_dynamic_quicklist));
    _menu_clients["dynamicquicklist"] = menu_client;
  }

  // make a client for desktop file actions
  if (!DBUSMENU_IS_MENUITEM(_menu_desktop_shortcuts))
  {
    UpdateDesktopQuickList();
  }

}

void BamfLauncherIcon::OnQuit(DbusmenuMenuitem* item, int time, BamfLauncherIcon* self)
{
  GList* children, *l;
  BamfView* view;

  children = bamf_view_get_children(BAMF_VIEW(self->m_App));

  for (l = children; l; l = l->next)
  {
    view = static_cast <BamfView*> (l->data);

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
  BamfView* view = BAMF_VIEW(m_App);
  
  if (bamf_view_is_sticky(view))
    return;
  
  const gchar* desktop_file = DesktopFile();
  bamf_view_set_sticky(view, true);
  
  if (desktop_file && strlen(desktop_file) > 0)
    FavoriteStore::GetDefault().AddFavorite(desktop_file, -1);
}

void BamfLauncherIcon::UnStick(void)
{
  BamfView* view = BAMF_VIEW(this->m_App);

  if (!bamf_view_is_sticky(view))
    return;

  const gchar* desktop_file = DesktopFile();
  bamf_view_set_sticky(view, false);
  if (bamf_view_is_closed(view))
    this->Remove();

  if (desktop_file && strlen(desktop_file) > 0)
    FavoriteStore::GetDefault().RemoveFavorite(desktop_file);
}

void BamfLauncherIcon::OnTogglePin(DbusmenuMenuitem* item, int time, BamfLauncherIcon* self)
{
  BamfView* view = BAMF_VIEW(self->m_App);
  bool sticky = bamf_view_is_sticky(view);
  const gchar* desktop_file = self->DesktopFile();

  if (sticky)
  {
    self->UnStick();
  }
  else
  {
    bamf_view_set_sticky(view, true);

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

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_CHECK);
    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Keep in launcher"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    _menu_callbacks["Pin"] = g_signal_connect(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, (GCallback) &BamfLauncherIcon::OnTogglePin, this);

    _menu_items["Pin"] = menu_item;
  }
  int checked = !bamf_view_is_sticky(BAMF_VIEW(m_App)) ?
                DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED : DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED;

  dbusmenu_menuitem_property_set_int(_menu_items["Pin"],
                                     DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
                                     checked);


  /* Quit */
  if (_menu_items.find("Quit") == _menu_items.end())
  {
    menu_item = dbusmenu_menuitem_new();

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Quit"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    _menu_callbacks["Quit"] = g_signal_connect(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, (GCallback) &BamfLauncherIcon::OnQuit, this);

    _menu_items["Quit"] = menu_item;
  }
}

static void OnAppLabelActivated(DbusmenuMenuitem* sender,
                                guint             timestamp,
                                gpointer data)
{
  BamfLauncherIcon* self = NULL;

  if (!data)
    return;

  self = static_cast <BamfLauncherIcon*> (data);
  self->ActivateLauncherIcon(ActionArg(ActionArg::OTHER, 0));
}

std::list<DbusmenuMenuitem*> BamfLauncherIcon::GetMenus()
{
  std::map<std::string, DbusmenuClient*>::iterator it;
  std::list<DbusmenuMenuitem*> result;
  bool first_separator_needed = false;
  DbusmenuMenuitem* item = NULL;

  // FIXME for O: hack around the wrong abstraction
  UpdateMenus();

  for (it = _menu_clients.begin(); it != _menu_clients.end(); it++)
  {
    GList* child = NULL;
    DbusmenuClient* client = (*it).second;
    DbusmenuMenuitem* root = dbusmenu_client_get_root(client);

    if (!root || !dbusmenu_menuitem_property_get_bool(root, DBUSMENU_MENUITEM_PROP_VISIBLE))
      continue;

    for (child = dbusmenu_menuitem_get_children(root); child != NULL; child = g_list_next(child))
    {
      DbusmenuMenuitem* item = (DbusmenuMenuitem*) child->data;

      if (!item || !DBUSMENU_IS_MENUITEM(item))
        continue;

      if (dbusmenu_menuitem_property_get_bool(item, DBUSMENU_MENUITEM_PROP_VISIBLE))
      {
        first_separator_needed = true;

        result.push_back(item);
      }
    }
  }

  // FIXME: this should totally be added as a _menu_client
  if (DBUSMENU_IS_MENUITEM(_menu_desktop_shortcuts))
  {
    GList* child = NULL;
    DbusmenuMenuitem* root = _menu_desktop_shortcuts;

    for (child = dbusmenu_menuitem_get_children(root); child != NULL; child = g_list_next(child))
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
      _menu_items_extra["FirstSeparator"] = item;
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
    gchar* app_name;
    app_name = g_markup_escape_text(BamfName(), -1);

    item = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(item,
                                   DBUSMENU_MENUITEM_PROP_LABEL,
                                   app_name);
    dbusmenu_menuitem_property_set_bool(item,
                                        DBUSMENU_MENUITEM_PROP_ENABLED,
                                        true);
    g_signal_connect(item, "item-activated", (GCallback) OnAppLabelActivated, this);
    g_free(app_name);

    _menu_items_extra["AppName"] = item;
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
    _menu_items_extra["SecondSeparator"] = item;
  }
  result.push_back(item);

  EnsureMenuItemsReady();

  std::map<std::string, DbusmenuMenuitem*>::iterator it_m;
  std::list<DbusmenuMenuitem*>::iterator it_l;
  bool exists;
  for (it_m = _menu_items.begin(); it_m != _menu_items.end(); it_m++)
  {
    const char* key = ((*it_m).first).c_str();
    if (g_strcmp0(key , "Quit") == 0 && !bamf_view_is_running(BAMF_VIEW(m_App)))
      continue;

    exists = false;
    std::string label_default = dbusmenu_menuitem_property_get((*it_m).second, DBUSMENU_MENUITEM_PROP_LABEL);
    for (it_l = result.begin(); it_l != result.end(); it_l++)
    {
      const gchar* type = dbusmenu_menuitem_property_get(*it_l, DBUSMENU_MENUITEM_PROP_TYPE);
      if (type == NULL)//(g_strcmp0 (type, DBUSMENU_MENUITEM_PROP_LABEL) == 0)
      {
        std::string label_menu = dbusmenu_menuitem_property_get(*it_l, DBUSMENU_MENUITEM_PROP_LABEL);
        if (label_menu.compare(label_default) == 0)
        {
          exists = true;
          break;
        }
      }
    }

    if (!exists)
      result.push_back((*it_m).second);
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

  children = bamf_view_get_children(BAMF_VIEW(m_App));

  for (l = children; l; l = l->next)
  {
    view = static_cast <BamfView*> (l->data);

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
  if (!_remote_uri)
  {
    const gchar* desktop_file = DesktopFile();
    gchar* basename =  g_path_get_basename(desktop_file);

    _remote_uri = g_strdup_printf("application://%s", basename);

    g_free(basename);
  }

  return _remote_uri;
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
    for (auto j : GetSupportedTypes())
      if (g_content_type_is_a(i.c_str(), j.c_str()))
      {
        for (auto k : uris.UrisByType(i))
          result.insert(k);
          
        break;
      }
    
  return result;
}

gboolean BamfLauncherIcon::OnDndHoveredTimeout(gpointer data)
{
  BamfLauncherIcon* self = static_cast <BamfLauncherIcon*> (data);
  
  // for now, let's not do this, it turns out to be quite buggy
  //if (self->_dnd_hovered && bamf_view_is_running(BAMF_VIEW(self->m_App)))
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

void  BamfLauncherIcon::OnAcceptDrop(unity::DndData& dnd_data)
{
  OpenInstanceWithUris(ValidateUrisForLaunch(dnd_data));
}

void BamfLauncherIcon::OnDesktopFileChanged(GFileMonitor*        monitor,
                                            GFile*               file,
                                            GFile*               other_file,
                                            GFileMonitorEvent    event_type,
                                            gpointer             data)
{
  BamfLauncherIcon* self = static_cast<BamfLauncherIcon*>(data);
  switch (event_type)
  {
    case G_FILE_MONITOR_EVENT_DELETED:
      self->UnStick();
      break;
    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
      self->UpdateDesktopQuickList();
      break;
    default:
      break;
  }
}

bool
BamfLauncherIcon::ShowInSwitcher()
{
  return GetQuirk(QUIRK_RUNNING) && GetQuirk(QUIRK_VISIBLE);
}

unsigned long long
BamfLauncherIcon::SwitcherPriority()
{
  GList* children, *l;
  BamfView* view;
  unsigned long long result = 0;

  children = bamf_view_get_children(BAMF_VIEW(m_App));

  /* get the list of windows */
  for (l = children; l; l = l->next)
  {
    view = static_cast <BamfView*> (l->data);

    if (BAMF_IS_WINDOW(view))
    {
      guint32 xid = bamf_window_get_xid(BAMF_WINDOW(view));
      result = std::max(result, WindowManager::Default()->GetWindowActiveNumber (xid));
    }
  }

  g_list_free(children);
  return result;
}

const std::set<std::string>&
BamfLauncherIcon::GetSupportedTypes()
{
  if (!_supported_types_filled)
    FillSupportedTypes(this);
    
  return _supported_types;
}

gboolean
BamfLauncherIcon::FillSupportedTypes(gpointer data)
{
  BamfLauncherIcon* self = static_cast <BamfLauncherIcon*> (data);
  
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
    
    char** mimes = g_key_file_get_string_list(key_file, "Desktop Entry", "MimeType", NULL, NULL);
    if (!mimes)
    {
      g_key_file_free(key_file);
      return false;
    }
    
    for (int i=0; mimes[i]; i++)
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
