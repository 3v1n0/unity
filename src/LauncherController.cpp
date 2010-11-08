
#include "FavoriteStore.h"
#include "LauncherController.h"
#include "LauncherIcon.h"
#include "Launcher.h"
#include "PluginAdapter.h"


#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>

LauncherController::LauncherController(Launcher* launcher, CompScreen *screen, nux::BaseWindow* window, NUX_FILE_LINE_DECL)
{
    m_Launcher = launcher;
    m_Window = window;
    m_Screen = screen;
    _model = new LauncherModel ();
    
    m_Launcher->SetModel (_model);
    m_FavoriteStore = FavoriteStore::GetDefault ();
  
    g_timeout_add (5000, (GSourceFunc) &LauncherController::BamfTimerCallback, this);
    InsertExpoAction ();
}

LauncherController::~LauncherController()
{
  m_FavoriteStore->UnReference ();
}

void
LauncherController::OnExpoClicked ()
{
    PluginAdapter::Default ()->InitiateExpo ();
}

void
LauncherController::InsertExpoAction ()
{
    SimpleLauncherIcon *expoIcon;
    expoIcon = new SimpleLauncherIcon (m_Launcher);
    
    expoIcon->SetTooltipText ("Workspace Switcher");
    expoIcon->SetIconName ("workspace-switcher");
    expoIcon->SetVisible (true);
    expoIcon->SetRunning (false);
    
    expoIcon->MouseClick.connect (sigc::mem_fun (this, &LauncherController::OnExpoClicked));
    
    RegisterIcon (expoIcon);
}

void
LauncherController::RegisterIcon (LauncherIcon *icon)
{
    _model->AddIcon (icon);
}

/* static private */
bool 
LauncherController::BamfTimerCallback (void *data)
{
    LauncherController *self = (LauncherController*) data;
  
    self->SetupBamf ();
    
    return false;
}

/* static private */
void
LauncherController::OnViewOpened (BamfMatcher *matcher, BamfView *view, gpointer data)
{
    LauncherController *self = (LauncherController *) data;
    BamfApplication *app;
    
    if (!BAMF_IS_APPLICATION (view))
      return;
    
    app = BAMF_APPLICATION (view);
    
    BamfLauncherIcon *icon = new BamfLauncherIcon (self->m_Launcher, app, self->m_Screen);
    self->RegisterIcon (icon);
}

void
LauncherController::CreateFavorite (const char *file_path)
{
    BamfApplication *app;
    BamfLauncherIcon *icon;

    app = bamf_matcher_get_application_for_desktop_file (m_Matcher, file_path, true);
    
    if (g_object_get_qdata (G_OBJECT (app), g_quark_from_static_string ("unity-seen")))
    {
        bamf_view_set_sticky (BAMF_VIEW (app), true);
        return;
    }
    
    g_object_set_qdata (G_OBJECT (app), g_quark_from_static_string ("unity-seen"), GINT_TO_POINTER (1));
    
    bamf_view_set_sticky (BAMF_VIEW (app), true);
    icon = new BamfLauncherIcon (m_Launcher, app, m_Screen);
    RegisterIcon (icon);
}

/* private */
void
LauncherController::SetupBamf ()
{
    GList *apps, *l;
    GSList *favs, *f;
    BamfApplication *app;
    BamfLauncherIcon *icon;
    
    m_Matcher = bamf_matcher_get_default ();
    
    favs = FavoriteStore::GetDefault ()->GetFavorites ();
    
    for (f = favs; f; f = f->next)
      CreateFavorite ((const char *) f->data);
    
    apps = bamf_matcher_get_applications (m_Matcher);
    g_signal_connect (m_Matcher, "view-opened", (GCallback) &LauncherController::OnViewOpened, this);
    
    for (l = apps; l; l = l->next)
    {
        app = BAMF_APPLICATION (l->data);
        
        if (g_object_get_qdata (G_OBJECT (app), g_quark_from_static_string ("unity-seen")))
          continue;
        g_object_set_qdata (G_OBJECT (app), g_quark_from_static_string ("unity-seen"), GINT_TO_POINTER (1));
        
        icon = new BamfLauncherIcon (m_Launcher, app, m_Screen);
        RegisterIcon (icon);
    }
}

