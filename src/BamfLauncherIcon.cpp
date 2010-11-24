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

BamfLauncherIcon::BamfLauncherIcon (Launcher* IconManager, BamfApplication *app, CompScreen *screen, NUX_FILE_LINE_DECL)
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
}

BamfLauncherIcon::~BamfLauncherIcon()
{
    g_object_unref (m_App);
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
      self->EnsureWindowState ();
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
}

void
BamfLauncherIcon::OnChildRemoved (BamfView *view, BamfView *child, gpointer data)
{
    BamfLauncherIcon *self = (BamfLauncherIcon*) data;
    self->EnsureWindowState ();
}

std::list<DbusmenuClient *>
BamfLauncherIcon::GetMenus ()
{
    GList *children, *l;
    std::list<DbusmenuClient *> result;
    
    children = bamf_view_get_children (BAMF_VIEW (m_App));
    for (l = children; l; l = l->next)
    {
        if (BAMF_IS_INDICATOR (l->data))
        {
            BamfIndicator *indicator = BAMF_INDICATOR (l->data);
            DbusmenuClient *client = dbusmenu_client_new (bamf_indicator_get_remote_address (indicator), bamf_indicator_get_dbus_menu_path (indicator));
            
            result.push_back (client);
        }
    }
    
    return result;
}
