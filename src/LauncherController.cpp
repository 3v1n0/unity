
#include "FavoriteStore.h"
#include "LauncherController.h"
#include "LauncherIcon.h"
#include "Launcher.h"
#include "PluginAdapter.h"


#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>

LauncherController::LauncherController(Launcher* launcher, CompScreen *screen, nux::BaseWindow* window, NUX_FILE_LINE_DECL)
{
    _launcher = launcher;
    _window = window;
    _screen = screen;
    _model = new LauncherModel ();
    
    _launcher->SetModel (_model);
    _favorite_store = FavoriteStore::GetDefault ();
  
    g_timeout_add (5000, (GSourceFunc) &LauncherController::BamfTimerCallback, this);
    InsertExpoAction ();
}

LauncherController::~LauncherController()
{
  _favorite_store->UnReference ();
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
    expoIcon = new SimpleLauncherIcon (_launcher);
    
    expoIcon->SetTooltipText ("Workspace Switcher");
    expoIcon->SetIconName ("workspace-switcher");
    expoIcon->SetVisible (true);
    expoIcon->SetRunning (false);
    expoIcon->SetIconType (LAUNCHER_ICON_TYPE_END);
    
    expoIcon->MouseClick.connect (sigc::mem_fun (this, &LauncherController::OnExpoClicked));
    
    RegisterIcon (expoIcon);
}

bool
LauncherController::CompareIcons (LauncherIcon *first, LauncherIcon *second)
{
    if (first->Type () < second->Type ())
        return true;
    else if (first->Type () > second->Type ())
        return false;
    
    return first->SortPriority () < second->SortPriority ();
}

void
LauncherController::RegisterIcon (LauncherIcon *icon)
{
    _model->AddIcon (icon);
    _model->Sort (&LauncherController::CompareIcons);
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
    
    BamfLauncherIcon *icon = new BamfLauncherIcon (self->_launcher, app, self->_screen);
    icon->SetIconType (LAUNCHER_ICON_TYPE_APPLICATION);

    self->RegisterIcon (icon);
}

LauncherIcon *
LauncherController::CreateFavorite (const char *file_path)
{
    BamfApplication *app;
    BamfLauncherIcon *icon;

    app = bamf_matcher_get_application_for_desktop_file (_matcher, file_path, true);
    
    if (g_object_get_qdata (G_OBJECT (app), g_quark_from_static_string ("unity-seen")))
    {
        bamf_view_set_sticky (BAMF_VIEW (app), true);
        return 0;
    }
    
    g_object_set_qdata (G_OBJECT (app), g_quark_from_static_string ("unity-seen"), GINT_TO_POINTER (1));
    
    bamf_view_set_sticky (BAMF_VIEW (app), true);
    icon = new BamfLauncherIcon (_launcher, app, _screen);
    icon->SetIconType (LAUNCHER_ICON_TYPE_FAVORITE);
    
    return icon;
}

/* private */
void
LauncherController::SetupBamf ()
{
    GList *apps, *l;
    GSList *favs, *f;
    BamfApplication *app;
    BamfLauncherIcon *icon;
    int priority = 0;
    
    _matcher = bamf_matcher_get_default ();
    
    favs = FavoriteStore::GetDefault ()->GetFavorites ();
    
    for (f = favs; f; f = f->next)
    {
        LauncherIcon *fav = CreateFavorite ((const char *) f->data);
        
        if (fav)
        {
            fav->SetSortPriority (priority);
            RegisterIcon (fav);
            priority++;
        }
    }
    
    priority = 0;
    
    apps = bamf_matcher_get_applications (_matcher);
    g_signal_connect (_matcher, "view-opened", (GCallback) &LauncherController::OnViewOpened, this);
    
    for (l = apps; l; l = l->next)
    {
        app = BAMF_APPLICATION (l->data);
        
        if (g_object_get_qdata (G_OBJECT (app), g_quark_from_static_string ("unity-seen")))
          continue;
        g_object_set_qdata (G_OBJECT (app), g_quark_from_static_string ("unity-seen"), GINT_TO_POINTER (1));
        
        icon = new BamfLauncherIcon (_launcher, app, _screen);
        icon->SetSortPriority (priority);
        RegisterIcon (icon);
        
        priority++;
    }
}

