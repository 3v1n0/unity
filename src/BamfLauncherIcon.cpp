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
    
    if (bamf_view_is_sticky (BAMF_VIEW (m_App)))
      SetVisible (true);
    else
      SetVisible (bamf_view_user_visible (BAMF_VIEW (m_App)));
    
    SetActive (bamf_view_is_active (BAMF_VIEW (m_App)));
    SetRunning (bamf_view_is_running (BAMF_VIEW (m_App)));
    
    g_free (icon_name);
    
    g_signal_connect (app, "urgent-changed", (GCallback) &BamfLauncherIcon::OnUrgentChanged, this);
    g_signal_connect (app, "running-changed", (GCallback) &BamfLauncherIcon::OnRunningChanged, this);
    g_signal_connect (app, "active-changed", (GCallback) &BamfLauncherIcon::OnActiveChanged, this);
    g_signal_connect (app, "user-visible-changed", (GCallback) &BamfLauncherIcon::OnUserVisibleChanged, this);
    g_signal_connect (app, "closed", (GCallback) &BamfLauncherIcon::OnClosed, this);
    
    g_object_ref (m_App);
}

BamfLauncherIcon::~BamfLauncherIcon()
{
    g_object_unref (m_App);
}

void
BamfLauncherIcon::OnMouseClick ()
{
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
      self->SetVisible (visible);
}

void
BamfLauncherIcon::OnRunningChanged (BamfView *view, gboolean running, gpointer data)
{
    BamfLauncherIcon *self = (BamfLauncherIcon *) data;
    self->SetRunning (running);
}

void
BamfLauncherIcon::OnActiveChanged (BamfView *view, gboolean active, gpointer data)
{
    BamfLauncherIcon *self = (BamfLauncherIcon *) data;
    self->SetActive (active);
}

void
BamfLauncherIcon::OnUrgentChanged (BamfView *view, gboolean urgent, gpointer data)
{
    BamfLauncherIcon *self = (BamfLauncherIcon *) data;
    self->SetUrgent (urgent);
}
