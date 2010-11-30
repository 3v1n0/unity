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

#include <gio/gdesktopappinfo.h>

#include <core/core.h>
#include <core/atoms.h>

BamfLauncherIcon::BamfLauncherIcon (Launcher* IconManager, BamfApplication *app, CompScreen *screen)
:   SimpleLauncherIcon(IconManager)
{
  m_App = app;
  m_Screen = screen;
  char *icon_name = bamf_view_get_icon (BAMF_VIEW (m_App));

  SetTooltipText (bamf_view_get_name (BAMF_VIEW (app)));
  SetIconName (icon_name);
  SetIconType (LAUNCHER_ICON_TYPE_APPLICATION);
  
  if (bamf_view_is_sticky (BAMF_VIEW (m_App)))
    SetQuirk (LAUNCHER_ICON_QUIRK_VISIBLE, true);
  else
    SetQuirk (LAUNCHER_ICON_QUIRK_VISIBLE, bamf_view_user_visible (BAMF_VIEW (m_App)));
  
  SetQuirk (LAUNCHER_ICON_QUIRK_ACTIVE, bamf_view_is_active (BAMF_VIEW (m_App)));
  SetQuirk (LAUNCHER_ICON_QUIRK_RUNNING, bamf_view_is_running (BAMF_VIEW (m_App)));
  
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
}

BamfLauncherIcon::~BamfLauncherIcon()
{
  g_object_unref (m_App);
}

bool
BamfLauncherIcon::IconOwnsWindow (Window w)
{
  GList *children, *l;
  BamfView *view;

  children = bamf_view_get_children (BAMF_VIEW (m_App));

  for (l = children; l; l = l->next)
  {
    view = (BamfView *) l->data;

    if (BAMF_IS_WINDOW (view))
    {
      guint32 xid = bamf_window_get_xid (BAMF_WINDOW (view));
        
      if (xid == w)
        return true;
    }
  }
  
  return false;
}

void
BamfLauncherIcon::OnMouseClick (int button)
{
  if (button != 1)
    return;
  
  BamfView *view;
  GList *children, *l;
  bool active, running;
  GDesktopAppInfo *appInfo;
  
  children = bamf_view_get_children (BAMF_VIEW (m_App));
  active = bamf_view_is_active (BAMF_VIEW (m_App));
  running = bamf_view_is_running (BAMF_VIEW (m_App));
  
  if (!running)
  {
    appInfo = g_desktop_app_info_new_from_filename (bamf_application_get_desktop_file (BAMF_APPLICATION (m_App)));
    g_app_info_launch (G_APP_INFO (appInfo), NULL, NULL, NULL);
    g_object_unref (appInfo);
    
    UpdateQuirkTime (LAUNCHER_ICON_QUIRK_STARTING);
    
    return;
  }
  
  if (active)
  {
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
    
    if (windowList.size () > 1)
    {
      std::string *match = PluginAdapter::Default ()->MatchStringForXids (&windowList);
      PluginAdapter::Default ()->InitiateScale (match);
      delete match;
    }
  }
  else
  {
    for (l = children; l; l = l->next)
    {
      view = (BamfView *) l->data;
  
      if (BAMF_IS_WINDOW (view))
      {
        guint32 xid = bamf_window_get_xid (BAMF_WINDOW (view));
    
        CompWindow *window = m_Screen->findWindow ((Window) xid);
    
        if (window)
          window->activate ();
      }
    }
  }
}

void
BamfLauncherIcon::OnClosed (BamfView *view, gpointer data)
{
  BamfLauncherIcon *self = (BamfLauncherIcon *) data;
  
  if (!bamf_view_is_sticky (BAMF_VIEW (self->m_App)))
    self->Remove ();
}

void
BamfLauncherIcon::OnUserVisibleChanged (BamfView *view, gboolean visible, gpointer data)
{
  BamfLauncherIcon *self = (BamfLauncherIcon *) data;
  
  if (!bamf_view_is_sticky (BAMF_VIEW (self->m_App)))
    self->SetQuirk (LAUNCHER_ICON_QUIRK_VISIBLE, visible);
}

void
BamfLauncherIcon::OnRunningChanged (BamfView *view, gboolean running, gpointer data)
{
  BamfLauncherIcon *self = (BamfLauncherIcon *) data;
  self->SetQuirk (LAUNCHER_ICON_QUIRK_RUNNING, running);
  
  if (running)
  {
    self->EnsureWindowState ();
    self->UpdateIconGeometries (self->GetCenter ());
  }
}

void
BamfLauncherIcon::OnActiveChanged (BamfView *view, gboolean active, gpointer data)
{
  BamfLauncherIcon *self = (BamfLauncherIcon *) data;
  self->SetQuirk (LAUNCHER_ICON_QUIRK_ACTIVE, active);
}

void
BamfLauncherIcon::OnUrgentChanged (BamfView *view, gboolean urgent, gpointer data)
{
  BamfLauncherIcon *self = (BamfLauncherIcon *) data;
  self->SetQuirk (LAUNCHER_ICON_QUIRK_URGENT, urgent);
}

void
BamfLauncherIcon::EnsureWindowState ()
{
  GList *children, *l;
  int count = 0;
  
  children = bamf_view_get_children (BAMF_VIEW (m_App));
  for (l = children; l; l = l->next)
  {
    if (BAMF_IS_WINDOW (l->data))
      count++;
  }
  
  SetRelatedWindows (count);
}

void
BamfLauncherIcon::OnChildAdded (BamfView *view, BamfView *child, gpointer data)
{
  BamfLauncherIcon *self = (BamfLauncherIcon*) data;
  self->EnsureWindowState ();
  self->UpdateMenus ();
  self->UpdateIconGeometries (self->GetCenter ());
}

void
BamfLauncherIcon::OnChildRemoved (BamfView *view, BamfView *child, gpointer data)
{
  BamfLauncherIcon *self = (BamfLauncherIcon*) data;
  self->EnsureWindowState ();
}

void
BamfLauncherIcon::UpdateMenus ()
{
  GList *children, *l;
  
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
}

std::list<DbusmenuMenuitem *>
BamfLauncherIcon::GetMenus ()
{
  std::map<std::string, DbusmenuClient *>::iterator it;
  std::list<DbusmenuMenuitem *> result;
  DbusmenuMenuitem *menu_item;
  
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
      
      result.push_back (item);
    }
  }
  
  if (_menu_items.find ("Pin") == _menu_items.end ())
  {
    menu_item = dbusmenu_menuitem_new ();
    dbusmenu_menuitem_property_set (menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_CHECK);
    dbusmenu_menuitem_property_set (menu_item, DBUSMENU_MENUITEM_PROP_LABEL, "Pin To Launcher");
    dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_int (menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED);
    
    _menu_items["Pin"] = menu_item;
  }
  
  result.push_back (_menu_items["Pin"]);
  
  if (_menu_items.find ("Quit") == _menu_items.end ())
  {
    menu_item = dbusmenu_menuitem_new ();
    dbusmenu_menuitem_property_set (menu_item, DBUSMENU_MENUITEM_PROP_LABEL, "Quit");
    dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    
    _menu_items["Quit"] = menu_item;
  }
  
  result.push_back (_menu_items["Quit"]);
  
  return result;
}


void
BamfLauncherIcon::UpdateIconGeometries (nux::Point3 center)
{
  GList *children, *l;
  BamfView *view;
  long data[4];

  data[0] = center.x - 24;
  data[1] = center.y - 24;
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
}

void 
BamfLauncherIcon::OnCenterStabilized (nux::Point3 center)
{
  UpdateIconGeometries (center);
}
