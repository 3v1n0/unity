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

#include "Nux/Nux.h"
#include "Nux/BaseWindow.h"

#include "BamfLauncherIcon.h"
#include "Launcher.h"
#include "PluginAdapter.h"
#include "FavoriteStore.h"

#include "ubus-server.h"
#include "UBusMessages.h"

#include <glib/gi18n-lib.h>
#include <gio/gdesktopappinfo.h>
#include <libindicator/indicator-desktop-shortcuts.h>
#include <core/core.h>
#include <core/atoms.h>

using unity::FavoriteStore;

struct _ShortcutData
{
  BamfLauncherIcon *self;
  IndicatorDesktopShortcuts *shortcuts;
  char *nick;
};
typedef struct _ShortcutData ShortcutData;
static void shortcut_data_destroy (ShortcutData *data)
{
  g_object_unref (data->shortcuts);
  g_free (data->nick);
  g_slice_free (ShortcutData, data);
}

static void shortcut_activated (DbusmenuMenuitem* _sender, guint timestamp, gpointer userdata)
{
  ShortcutData *data = (ShortcutData *)userdata;
  indicator_desktop_shortcuts_nick_exec (data->shortcuts, data->nick);
}

void BamfLauncherIcon::Activate ()
{
  ActivateLauncherIcon ();
}


void BamfLauncherIcon::ActivateLauncherIcon ()
{
  bool scaleWasActive = PluginAdapter::Default ()->IsScaleActive ();

  bool active, running;
  active = bamf_view_is_active (BAMF_VIEW (m_App));
  running = bamf_view_is_running (BAMF_VIEW (m_App));

  /* Behaviour:
   * 1) Nothing running -> launch application
   * 2) Running and active -> spread application
   * 3) Running and not active -> focus application
   * 4) Spread is active and different icon pressed -> change spread
   * 5) Spread is active -> Spread de-activated, and fall through
   */

  if (!running) // #1 above
  {
    if (GetQuirk (QUIRK_STARTING))
      return;

    if (scaleWasActive)
    {
      PluginAdapter::Default ()->TerminateScale ();
    }

    SetQuirk (QUIRK_STARTING, true);
    OpenInstanceLauncherIcon ();
  }
  else // app is running
  {
    if (active)
    {
      if (scaleWasActive) // #5 above
      {
        PluginAdapter::Default ()->TerminateScale ();
        Focus ();
      }
      else // #2 above
      {
        Spread (0, false);
      }
    }
    else
    {
      if (scaleWasActive) // #4 above
      {
        PluginAdapter::Default ()->TerminateScale ();
        Focus ();
        Spread (0, false);
      }
      else // #3 above
      {
        Focus ();
      }
    }
  }

  ubus_server_send_message (ubus_server_get_default (), UBUS_LAUNCHER_ACTION_DONE, NULL);
}

BamfLauncherIcon::BamfLauncherIcon (Launcher* IconManager, BamfApplication *app, CompScreen *screen)
:   SimpleLauncherIcon(IconManager)
{
  _cached_desktop_file = NULL;
  _cached_name = NULL;
  m_App = app;
  m_Screen = screen;
  _remote_uri = 0;
  _dnd_hover_timer = 0;
  _dnd_hovered = false;
  _launcher = IconManager;
  _menu_desktop_shortcuts = NULL;
  char *icon_name = bamf_view_get_icon (BAMF_VIEW (m_App));

  SetTooltipText (BamfName ());
  SetIconName (icon_name);
  SetIconType (TYPE_APPLICATION);

  if (bamf_view_is_sticky (BAMF_VIEW (m_App)))
    SetQuirk (QUIRK_VISIBLE, true);
  else
    SetQuirk (QUIRK_VISIBLE, bamf_view_user_visible (BAMF_VIEW (m_App)));

  SetQuirk (QUIRK_ACTIVE, bamf_view_is_active (BAMF_VIEW (m_App)));
  SetQuirk (QUIRK_RUNNING, bamf_view_is_running (BAMF_VIEW (m_App)));

  g_free (icon_name);

  g_signal_connect (app, "child-removed", (GCallback) &BamfLauncherIcon::OnChildRemoved, this);
  g_signal_connect (app, "child-added", (GCallback) &BamfLauncherIcon::OnChildAdded, this);
  g_signal_connect (app, "urgent-changed", (GCallback) &BamfLauncherIcon::OnUrgentChanged, this);
  g_signal_connect (app, "running-changed", (GCallback) &BamfLauncherIcon::OnRunningChanged, this);
  g_signal_connect (app, "active-changed", (GCallback) &BamfLauncherIcon::OnActiveChanged, this);
  g_signal_connect (app, "user-visible-changed", (GCallback) &BamfLauncherIcon::OnUserVisibleChanged, this);
  g_signal_connect (app, "closed", (GCallback) &BamfLauncherIcon::OnClosed, this);

  g_object_ref (m_App);

  EnsureWindowState ();
  UpdateMenus ();

  // add a file watch to the desktop file so that if/when the app is removed we can remove ourself from the launcher.
  GFile *_desktop_file = g_file_new_for_path (DesktopFile ());
  _desktop_file_monitor = g_file_monitor_file (_desktop_file,
                                               G_FILE_MONITOR_NONE,
                                               NULL,
                                               NULL);

  _on_desktop_file_changed_handler_id = g_signal_connect (_desktop_file_monitor,
                                                          "changed",
                                                          G_CALLBACK (&BamfLauncherIcon::OnDesktopFileChanged),
                                                          this);

  _on_window_minimized_connection = (sigc::connection) PluginAdapter::Default ()->window_minimized.connect (sigc::mem_fun (this, &BamfLauncherIcon::OnWindowMinimized));
  _hidden_changed_connection = (sigc::connection) IconManager->hidden_changed.connect (sigc::mem_fun (this, &BamfLauncherIcon::OnLauncherHiddenChanged));

  /* hack */
  SetProgress (0.0f);
  
}

BamfLauncherIcon::~BamfLauncherIcon()
{
  g_object_set_qdata (G_OBJECT (m_App), g_quark_from_static_string ("unity-seen"), GINT_TO_POINTER (0));

  // We might not have created the menu items yet
  if (_menu_items.find ("Pin") != _menu_items.end ()) {
    g_signal_handler_disconnect ((gpointer) _menu_items["Pin"],
                                 _menu_callbacks["Pin"]);
  }

  if (_menu_items.find ("Quit") != _menu_items.end ()) {
    g_signal_handler_disconnect ((gpointer) _menu_items["Quit"],
                                 _menu_callbacks["Quit"]);
  }

  if (_on_desktop_file_changed_handler_id != 0)
    g_signal_handler_disconnect ((gpointer) _desktop_file_monitor,
                                 _on_desktop_file_changed_handler_id);

  if (_on_window_minimized_connection.connected ())
    _on_window_minimized_connection.disconnect ();
  
  if (_hidden_changed_connection.connected ())
    _hidden_changed_connection.disconnect ();

  g_signal_handlers_disconnect_by_func (m_App, (void *) &BamfLauncherIcon::OnChildRemoved,       this);
  g_signal_handlers_disconnect_by_func (m_App, (void *) &BamfLauncherIcon::OnChildAdded,         this);
  g_signal_handlers_disconnect_by_func (m_App, (void *) &BamfLauncherIcon::OnUrgentChanged,      this);
  g_signal_handlers_disconnect_by_func (m_App, (void *) &BamfLauncherIcon::OnRunningChanged,     this);
  g_signal_handlers_disconnect_by_func (m_App, (void *) &BamfLauncherIcon::OnActiveChanged,      this);
  g_signal_handlers_disconnect_by_func (m_App, (void *) &BamfLauncherIcon::OnUserVisibleChanged, this);
  g_signal_handlers_disconnect_by_func (m_App, (void *) &BamfLauncherIcon::OnClosed,             this);

  g_object_unref (m_App);
  g_object_unref (_desktop_file_monitor);

  g_free (_cached_desktop_file);
  g_free (_cached_name);
}

void BamfLauncherIcon::OnLauncherHiddenChanged ()
{
  UpdateIconGeometries (GetCenter ());
}

void BamfLauncherIcon::OnWindowMinimized (guint32 xid)
{
  if (!OwnsWindow (xid))
    return;

  Present (0.5f, 600);
  UpdateQuirkTimeDelayed (300, QUIRK_SHIMMER);
}

bool BamfLauncherIcon::IsSticky ()
{
  return bamf_view_is_sticky (BAMF_VIEW (m_App));
}

const char* BamfLauncherIcon::DesktopFile ()
{
  char *filename = NULL;
  filename = (char*) bamf_application_get_desktop_file (m_App);

  if (filename != NULL)
  {
    if (_cached_desktop_file != NULL)
      g_free (_cached_desktop_file);
    
    _cached_desktop_file = g_strdup (filename);
  }
  
  return _cached_desktop_file;
}

const char* BamfLauncherIcon::BamfName ()
{
  char *name = NULL;
  name = (char *)bamf_view_get_name (BAMF_VIEW (m_App));

  if (name != NULL)
  {
    if (_cached_name != NULL)
      g_free (_cached_name);

    _cached_name = g_strdup (name);
  }

  return _cached_name;
}

void BamfLauncherIcon::AddProperties (GVariantBuilder *builder)
{
  LauncherIcon::AddProperties (builder);

  g_variant_builder_add (builder, "{sv}", "desktop-file", g_variant_new_string (bamf_application_get_desktop_file (m_App)));

  GList *children, *l;
  BamfView *view;

  children = bamf_view_get_children (BAMF_VIEW (m_App));
  GVariant* xids[(int) g_list_length (children)];

  int i = 0;
  for (l = children; l; l = l->next)
  {
    view = (BamfView *) l->data;

    if (BAMF_IS_WINDOW (view))
    {
      xids[i++] = g_variant_new_uint32 (bamf_window_get_xid (BAMF_WINDOW (view)));
    }
  }
  g_list_free (children);
  g_variant_builder_add (builder, "{sv}", "xids", g_variant_new_array (G_VARIANT_TYPE_UINT32, xids, i));
}

bool BamfLauncherIcon::OwnsWindow (Window w)
{
  GList *children, *l;
  BamfView *view;
  bool owns = false;

  children = bamf_view_get_children (BAMF_VIEW (m_App));

  for (l = children; l; l = l->next)
  {
    view = (BamfView *) l->data;

    if (BAMF_IS_WINDOW (view))
    {
      guint32 xid = bamf_window_get_xid (BAMF_WINDOW (view));

      if (xid == w)
      {
        owns = true;
        break;
      }
    }
  }

  g_list_free (children);
  return owns;
}

void BamfLauncherIcon::OpenInstanceWithUris (std::list<char *> uris)
{
  GDesktopAppInfo *appInfo;
  GError *error = NULL;
  std::list<char *>::iterator it;

  appInfo = g_desktop_app_info_new_from_filename (bamf_application_get_desktop_file (BAMF_APPLICATION (m_App)));
  
  if (g_app_info_supports_uris (G_APP_INFO (appInfo)))
  {
    GList *list = NULL;
    
    for (it = uris.begin (); it != uris.end (); it++)
      list = g_list_prepend (list, *it);
    
    g_app_info_launch_uris (G_APP_INFO (appInfo), list, NULL, &error);
    g_list_free (list);
  }
  else if (g_app_info_supports_files (G_APP_INFO (appInfo)))
  {
    GList *list = NULL, *l;
    for (it = uris.begin (); it != uris.end (); it++)
    {
      GFile *file = g_file_new_for_uri (*it);
      list = g_list_prepend (list, file);
    }
    g_app_info_launch (G_APP_INFO (appInfo), list, NULL, &error);
    
    for (l = list; l; l = l->next)
      g_object_unref (G_FILE (list->data));
      
    g_list_free (list);
  }
  else
  {
    g_app_info_launch (G_APP_INFO (appInfo), NULL, NULL, &error);
  }

  g_object_unref (appInfo);

  if (error)
  {
    g_warning ("%s\n", error->message);
    g_error_free (error);
  }

  UpdateQuirkTime (QUIRK_STARTING);
}

void BamfLauncherIcon::OpenInstanceLauncherIcon ()
{
  std::list<char *> empty;
  OpenInstanceWithUris (empty);
  ubus_server_send_message (ubus_server_get_default (), UBUS_LAUNCHER_ACTION_DONE, NULL);
}

void BamfLauncherIcon::Focus ()
{
  GList *children, *l;
  BamfView *view;
  bool any_urgent = false;
  bool any_on_current = false;

  children = bamf_view_get_children (BAMF_VIEW (m_App));

  CompWindowList windows;

  /* get the list of windows */
  for (l = children; l; l = l->next)
  {
    view = (BamfView *) l->data;

    if (BAMF_IS_WINDOW (view))
    {
      guint32 xid = bamf_window_get_xid (BAMF_WINDOW (view));

      CompWindow *window = m_Screen->findWindow ((Window) xid);

      if (window)
      {
        if (bamf_view_is_urgent (view))
          any_urgent = true;
        windows.push_back (window);
      }
    }
  }

  // not a good sign
  if (windows.empty ())
  {
    g_list_free (children);
    return;
  }

  /* sort the list */
  CompWindowList tmp;
  CompWindowList::iterator it;
  for (it = m_Screen->windows ().begin (); it != m_Screen->windows ().end (); it++)
  {
    if (std::find (windows.begin (), windows.end (), *it) != windows.end ())
      tmp.push_back (*it);
  }
  windows = tmp;

  /* filter based on workspace */
  for (it = windows.begin (); it != windows.end (); it++)
  {
    if ((*it)->defaultViewport () == m_Screen->vp ())
    {
      any_on_current = true;
      break;
    }
  }

  if (any_urgent)
  {
    // we cant use the compiz tracking since it is currently broken
    /*for (it = windows.begin (); it != windows.end (); it++)
    {
      if ((*it)->state () & CompWindowStateDemandsAttentionMask)
      {
        (*it)->activate ();
        break;
      }
    }*/
    for (l = children; l; l = l->next)
    {
      view = (BamfView *) l->data;

      if (BAMF_IS_WINDOW (view))
      {
        guint32 xid = bamf_window_get_xid (BAMF_WINDOW (view));

        CompWindow *window = m_Screen->findWindow ((Window) xid);

        if (window && bamf_view_is_urgent (view))
        {
          window->activate ();
          break;
        }
      }
    }
  }
  else if (any_on_current)
  {
    for (it = windows.begin (); it != windows.end (); it++)
    {
      if ((*it)->defaultViewport () == m_Screen->vp ())
      {
        (*it)->activate ();
      }
    }
  }
  else
  {
    (*(windows.rbegin ()))->activate ();
  }

  g_list_free (children);
}

bool BamfLauncherIcon::Spread (int state, bool force)
{
  BamfView *view;
  GList *children, *l;
  children = bamf_view_get_children (BAMF_VIEW (m_App));

  std::list<Window> windowList;
  for (l = children; l; l = l->next)
  {
    view = (BamfView *) l->data;

    if (BAMF_IS_WINDOW (view))
    {
      guint32 xid = bamf_window_get_xid (BAMF_WINDOW (view));

      windowList.push_back ((Window) xid);
    }
  }

  if (windowList.size () > 1 || (windowList.size () > 0 && force))
  {
    std::string match = PluginAdapter::Default ()->MatchStringForXids (&windowList);
    PluginAdapter::Default ()->InitiateScale (match, state);
    g_list_free (children);
    return true;
  }

  g_list_free (children);

  return false;  
}

void BamfLauncherIcon::OnClosed (BamfView *view, gpointer data)
{
  BamfLauncherIcon *self = (BamfLauncherIcon *) data;

  if (!bamf_view_is_sticky (BAMF_VIEW (self->m_App)))
    self->Remove ();
}

void BamfLauncherIcon::OnUserVisibleChanged (BamfView *view, gboolean visible, 
                                             gpointer data)
{
  BamfLauncherIcon *self = (BamfLauncherIcon *) data;

  if (!bamf_view_is_sticky (BAMF_VIEW (self->m_App)))
    self->SetQuirk (QUIRK_VISIBLE, visible);
}

void BamfLauncherIcon::OnRunningChanged (BamfView *view, gboolean running, 
                                         gpointer data)
{
  BamfLauncherIcon *self = (BamfLauncherIcon *) data;
  self->SetQuirk (QUIRK_RUNNING, running);

  if (running)
  {
    self->EnsureWindowState ();
    self->UpdateIconGeometries (self->GetCenter ());
  }
}

void BamfLauncherIcon::OnActiveChanged (BamfView *view, gboolean active, gpointer data)
{
  BamfLauncherIcon *self = (BamfLauncherIcon *) data;
  self->SetQuirk (QUIRK_ACTIVE, active);
}

void BamfLauncherIcon::OnUrgentChanged (BamfView *view, gboolean urgent, gpointer data)
{
  BamfLauncherIcon *self = (BamfLauncherIcon *) data;
  self->SetQuirk (QUIRK_URGENT, urgent);
}

void BamfLauncherIcon::EnsureWindowState ()
{
  GList *children, *l;
  int count = 0;

  bool has_visible = false;
  
  children = bamf_view_get_children (BAMF_VIEW (m_App));
  for (l = children; l; l = l->next)
  {
    if (!BAMF_IS_WINDOW (l->data))
      continue;
      
    count++;
    
    guint32 xid = bamf_window_get_xid (BAMF_WINDOW (l->data));
    CompWindow *window = m_Screen->findWindow ((Window) xid);
    
    if (window && window->defaultViewport () == m_Screen->vp ())
      has_visible = true;
  }

  SetRelatedWindows (count);
  SetHasVisibleWindow (has_visible);

  g_list_free (children);
}

void BamfLauncherIcon::OnChildAdded (BamfView *view, BamfView *child, gpointer data)
{
  BamfLauncherIcon *self = (BamfLauncherIcon*) data;
  self->EnsureWindowState ();
  self->UpdateMenus ();
  self->UpdateIconGeometries (self->GetCenter ());
}

void BamfLauncherIcon::OnChildRemoved (BamfView *view, BamfView *child, gpointer data)
{
  BamfLauncherIcon *self = (BamfLauncherIcon*) data;
  self->EnsureWindowState ();
}

void BamfLauncherIcon::UpdateMenus ()
{
  GList *children, *l;
  IndicatorDesktopShortcuts *desktop_shortcuts;

  children = bamf_view_get_children (BAMF_VIEW (m_App));
  for (l = children; l; l = l->next)
  {
    if (!BAMF_IS_INDICATOR (l->data))
      continue;

    BamfIndicator *indicator = BAMF_INDICATOR (l->data);
    std::string path = bamf_indicator_get_dbus_menu_path (indicator);

    // we already have this
    if (_menu_clients.find (path) != _menu_clients.end ())
      continue;

    DbusmenuClient *client = dbusmenu_client_new (bamf_indicator_get_remote_address (indicator), path.c_str ());
    _menu_clients[path] = client;
  }

  g_list_free (children);
  
  // add dynamic quicklist
  if (_menuclient_dynamic_quicklist != NULL)
    _menu_clients["dynamicquicklist"] = _menuclient_dynamic_quicklist;

  // make a client for desktop file actions
  if (!DBUSMENU_IS_MENUITEM (_menu_desktop_shortcuts) &&
      g_strcmp0 (DesktopFile (), """"))
  {
    GKeyFile *keyfile;
    GError *error = NULL;

    // check that we have the X-Ayatana-Desktop-Shortcuts flag
    // not sure if we should do this or if libindicator should shut up
    // and not report errors when it can't find the key.
    // so FIXME when ted is around
    keyfile = g_key_file_new ();
    g_key_file_load_from_file (keyfile, DesktopFile (), G_KEY_FILE_NONE, &error);

    if (error != NULL)
    {
      g_warning ("Could not load desktop file for: %s" , DesktopFile ());
      g_error_free (error);
      return;
    }

    if (g_key_file_has_key (keyfile, G_KEY_FILE_DESKTOP_GROUP,
                            "X-Ayatana-Desktop-Shortcuts", NULL))
    {
      DbusmenuMenuitem *root = dbusmenu_menuitem_new ();
      dbusmenu_menuitem_set_root (root, TRUE);
      desktop_shortcuts = indicator_desktop_shortcuts_new (bamf_application_get_desktop_file (m_App),
                                                           "Unity");
      const gchar **nicks = indicator_desktop_shortcuts_get_nicks (desktop_shortcuts);

      int index = 0;
      if (nicks)
      {
        while (((gpointer*) nicks)[index])
        {
          const char* name;
          DbusmenuMenuitem *item;
          name = g_strdup (indicator_desktop_shortcuts_nick_get_name (desktop_shortcuts,
                                                                      nicks[index]));
          ShortcutData *data = g_slice_new0 (ShortcutData);
          data->self = this;
          data->shortcuts = INDICATOR_DESKTOP_SHORTCUTS (g_object_ref (desktop_shortcuts));
          data->nick = g_strdup (nicks[index]);

          item = dbusmenu_menuitem_new ();
          dbusmenu_menuitem_property_set (item, DBUSMENU_MENUITEM_PROP_LABEL, name);
          dbusmenu_menuitem_property_set_bool (item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
          dbusmenu_menuitem_property_set_bool (item, DBUSMENU_MENUITEM_PROP_VISIBLE, TRUE);
          g_signal_connect_data (item, "item-activated",
                                (GCallback) shortcut_activated, (gpointer) data,
                                (GClosureNotify) shortcut_data_destroy, (GConnectFlags)0);

          dbusmenu_menuitem_child_append (root, item);

          index++;

          g_free ((void *)name);
        }
      }

      _menu_desktop_shortcuts = root;
      g_key_file_free (keyfile);

    }
  }

}

void BamfLauncherIcon::OnQuit (DbusmenuMenuitem *item, int time, BamfLauncherIcon *self)
{
  GList *children, *l;
  BamfView *view;

  children = bamf_view_get_children (BAMF_VIEW (self->m_App));

  for (l = children; l; l = l->next)
  {
    view = (BamfView *) l->data;

    if (BAMF_IS_WINDOW (view))
    {
      guint32 xid = bamf_window_get_xid (BAMF_WINDOW (view));
      CompWindow *window = self->m_Screen->findWindow ((Window) xid);

      if (window)
        window->close (self->m_Screen->getCurrentTime ());
    }
  }

  g_list_free (children);
}

void BamfLauncherIcon::UnStick (void)
{
  BamfView *view = BAMF_VIEW (this->m_App);
  
  if (!bamf_view_is_sticky (view))
    return;

  const gchar *desktop_file = bamf_application_get_desktop_file (this->m_App);
  bamf_view_set_sticky (view, false);
  if (bamf_view_is_closed (view))
    this->Remove ();

  if (desktop_file && strlen (desktop_file) > 0)
    FavoriteStore::GetDefault().RemoveFavorite(desktop_file);
}

void BamfLauncherIcon::OnTogglePin (DbusmenuMenuitem *item, int time, BamfLauncherIcon *self)
{
  BamfView *view = BAMF_VIEW (self->m_App);
  bool sticky = bamf_view_is_sticky (view);
  const gchar *desktop_file = bamf_application_get_desktop_file (self->m_App);

  if (sticky)
  {
    self->UnStick ();
  }
  else
  {
    bamf_view_set_sticky (view, true);

    if (desktop_file && strlen (desktop_file) > 0)
      FavoriteStore::GetDefault().AddFavorite(desktop_file, -1);
  }
}

void BamfLauncherIcon::EnsureMenuItemsReady ()
{
  DbusmenuMenuitem *menu_item;

  /* Pin */
  if (_menu_items.find ("Pin") == _menu_items.end ())
  {
    menu_item = dbusmenu_menuitem_new ();
    g_object_ref (menu_item);

    dbusmenu_menuitem_property_set (menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_CHECK);
    dbusmenu_menuitem_property_set (menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Keep in launcher"));
    dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    _menu_callbacks["Pin"] = g_signal_connect (menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, (GCallback) &BamfLauncherIcon::OnTogglePin, this);

    _menu_items["Pin"] = menu_item;
  }
  int checked = !bamf_view_is_sticky (BAMF_VIEW (m_App)) ?
                 DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED : DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED;

  dbusmenu_menuitem_property_set_int (_menu_items["Pin"],
                                      DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
                                      checked);


  /* Quit */
  if (_menu_items.find ("Quit") == _menu_items.end ())
  {
    menu_item = dbusmenu_menuitem_new ();
    g_object_ref (menu_item);

    dbusmenu_menuitem_property_set (menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Quit"));
    dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    _menu_callbacks["Quit"] = g_signal_connect (menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, (GCallback) &BamfLauncherIcon::OnQuit, this);

    _menu_items["Quit"] = menu_item;
  }
}

static void OnAppLabelActivated (DbusmenuMenuitem* sender,
                                 guint             timestamp,
                                 gpointer data)
{
  BamfLauncherIcon* self = NULL;

  if (!data)
    return;
    
  self = (BamfLauncherIcon*) data;
  self->ActivateLauncherIcon ();
}

std::list<DbusmenuMenuitem *> BamfLauncherIcon::GetMenus ()
{
  std::map<std::string, DbusmenuClient *>::iterator it;
  std::list<DbusmenuMenuitem *> result;
  bool first_separator_needed = false;
  DbusmenuMenuitem* item = NULL;
  
  // FIXME for O: hack around the wrong abstraction
  UpdateMenus ();
  
  for (it = _menu_clients.begin (); it != _menu_clients.end (); it++)
  {
    GList * child = NULL;
    DbusmenuClient *client = (*it).second;
    DbusmenuMenuitem *root = dbusmenu_client_get_root (client);

    for (child = dbusmenu_menuitem_get_children(root); child != NULL; child = g_list_next(child))
    {
      DbusmenuMenuitem *item = (DbusmenuMenuitem *) child->data;

      if (!item)
        continue;

      first_separator_needed = true;

      result.push_back (item);
    }
  }

  // FIXME: this should totally be added as a _menu_client
  if (DBUSMENU_IS_MENUITEM (_menu_desktop_shortcuts))
  {
    GList * child = NULL;
    DbusmenuMenuitem *root = _menu_desktop_shortcuts;

    for (child = dbusmenu_menuitem_get_children(root); child != NULL; child = g_list_next(child))
    {
      DbusmenuMenuitem *item = (DbusmenuMenuitem *) child->data;

      if (!item)
        continue;

      first_separator_needed = true;

      result.push_back (item);
    }
  }

  if (first_separator_needed)
  {
    item = dbusmenu_menuitem_new ();
    dbusmenu_menuitem_property_set (item,
                                    DBUSMENU_MENUITEM_PROP_TYPE,
                                    DBUSMENU_CLIENT_TYPES_SEPARATOR);
    result.push_back (item);
  }

  gchar *app_name;
  app_name = g_markup_escape_text (BamfName (), -1);

  item = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_LABEL,
                                  app_name);
  dbusmenu_menuitem_property_set_bool (item,
                                       DBUSMENU_MENUITEM_PROP_ENABLED,
                                       true);
  g_signal_connect (item, "item-activated", (GCallback) OnAppLabelActivated, this);
  result.push_back (item);
  g_free (app_name);

  item = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set (item,
                                  DBUSMENU_MENUITEM_PROP_TYPE,
                                  DBUSMENU_CLIENT_TYPES_SEPARATOR);
  result.push_back (item);

  EnsureMenuItemsReady ();

  std::map<std::string, DbusmenuMenuitem *>::iterator it_m;
  std::list<DbusmenuMenuitem *>::iterator it_l;
  bool exists;
  for (it_m = _menu_items.begin (); it_m != _menu_items.end (); it_m++)
  {
    const char* key = ((*it_m).first).c_str();
    if (g_strcmp0 (key , "Quit") == 0 && !bamf_view_is_running (BAMF_VIEW (m_App)))
      continue;

    exists = false;
    std::string label_default = dbusmenu_menuitem_property_get ((*it_m).second, DBUSMENU_MENUITEM_PROP_LABEL);
    for(it_l = result.begin(); it_l != result.end(); it_l++)
    {
      const gchar* type = dbusmenu_menuitem_property_get (*it_l, DBUSMENU_MENUITEM_PROP_TYPE);
      if (type == NULL)//(g_strcmp0 (type, DBUSMENU_MENUITEM_PROP_LABEL) == 0)
      {
        std::string label_menu = dbusmenu_menuitem_property_get (*it_l, DBUSMENU_MENUITEM_PROP_LABEL);
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


void BamfLauncherIcon::UpdateIconGeometries (nux::Point3 center)
{
  GList *children, *l;
  BamfView *view;
  long data[4];

  if (_launcher->Hidden () && !_launcher->ShowOnEdge ())
  {
    data[0] = 0;
    data[1] = 0;
  }
  else
  {
    data[0] = center.x - 24;
    data[1] = center.y - 24;
  }
  data[2] = 48;
  data[3] = 48;

  children = bamf_view_get_children (BAMF_VIEW (m_App));

  for (l = children; l; l = l->next)
  {
    view = (BamfView *) l->data;

    if (BAMF_IS_WINDOW (view))
    {
      guint32 xid = bamf_window_get_xid (BAMF_WINDOW (view));

      XChangeProperty (m_Screen->dpy (), xid, Atoms::wmIconGeometry,
                       XA_CARDINAL, 32, PropModeReplace,
                       (unsigned char *) data, 4);
    }
  }

  g_list_free (children);
}

void BamfLauncherIcon::OnCenterStabilized (nux::Point3 center)
{
  UpdateIconGeometries (center);
}

const gchar* BamfLauncherIcon::GetRemoteUri ()
{
  if (!_remote_uri)
  {
    const gchar * desktop_file = bamf_application_get_desktop_file (BAMF_APPLICATION (m_App));
    _remote_uri = g_strdup_printf ("application://%s", g_path_get_basename (desktop_file));
  }
  
  return _remote_uri;
}

std::list<char *> BamfLauncherIcon::ValidateUrisForLaunch (std::list<char *> uris)
{
  GKeyFile *key_file;
  const char *desktop_file;
  GError *error = NULL;
  std::list<char *> results;
  
  desktop_file = DesktopFile ();
  
  if (!desktop_file || strlen (desktop_file) <= 1)
    return results;
  
  key_file = g_key_file_new ();
  g_key_file_load_from_file (key_file, desktop_file, (GKeyFileFlags) 0, &error);
  
  if (error)
  {
    g_error_free (error);
    g_key_file_free (key_file);
    return results;
  }

  char **mimes = g_key_file_get_string_list (key_file, "Desktop Entry", "MimeType", NULL, NULL);
  if (!mimes)
  {
    g_key_file_free (key_file);
    return results;
  }
  
  std::list<char *>::iterator it;
  for (it = uris.begin (); it != uris.end (); it++)
  {
    GFile *file = g_file_new_for_uri (*it);
    GFileInfo *info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
    const char *content_type = g_file_info_get_content_type (info);

    int i = 0;
    for (; mimes[i]; i++)
    {
      char *super_type = g_content_type_from_mime_type (mimes[i]);
      if (g_content_type_is_a (content_type, super_type))
      {
        results.push_back (*it);
        break;
      }
      g_free (super_type);
    }
    
    
    g_object_unref (file);
    g_object_unref (info);
  }
  
  g_strfreev (mimes);
  g_key_file_free (key_file);
  return results;
}

gboolean BamfLauncherIcon::OnDndHoveredTimeout (gpointer data)
{
  BamfLauncherIcon *self = (BamfLauncherIcon*) data;
  if (self->_dnd_hovered && bamf_view_is_running (BAMF_VIEW (self->m_App)))
    self->Spread (CompAction::StateInitEdgeDnd, true);
  
  self->_dnd_hover_timer = 0;
  return false;
}

void BamfLauncherIcon::OnDndEnter ()
{
  _dnd_hovered = true;
  _dnd_hover_timer = g_timeout_add (1000, &BamfLauncherIcon::OnDndHoveredTimeout, this);
}

void BamfLauncherIcon::OnDndLeave ()
{
  _dnd_hovered = false;
  
  if (_dnd_hover_timer)
    g_source_remove (_dnd_hover_timer);
  _dnd_hover_timer = 0;
}

nux::DndAction BamfLauncherIcon::OnQueryAcceptDrop (std::list<char *> uris)
{
  return ValidateUrisForLaunch (uris).empty () ? nux::DNDACTION_NONE : nux::DNDACTION_COPY;
}

void  BamfLauncherIcon::OnAcceptDrop (std::list<char *> uris)
{
  OpenInstanceWithUris (ValidateUrisForLaunch (uris));
}

void BamfLauncherIcon::OnDesktopFileChanged (GFileMonitor        *monitor,
                                             GFile               *file,
                                             GFile               *other_file,
                                             GFileMonitorEvent    event_type,
                                             gpointer             data)
{
  BamfLauncherIcon *self = static_cast<BamfLauncherIcon*> (data);
  switch (event_type) {
  case G_FILE_MONITOR_EVENT_DELETED:
    self->UnStick ();
    break;
  default:
    break;
  }
}
