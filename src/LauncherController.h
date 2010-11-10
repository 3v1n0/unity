#ifndef LAUNCHERCONTROLLER_H
#define LAUNCHERCONTROLLER_H

/* Compiz */
#include <core/core.h>

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>

#include "BamfLauncherIcon.h"
#include "LauncherModel.h"

#include "FavoriteStore.h"

#include <libbamf/libbamf.h>
#include <sigc++/sigc++.h>


class Launcher;

class LauncherController : public sigc::trackable
{

public:
    LauncherController(Launcher* launcher, CompScreen *screen, nux::BaseWindow* window, NUX_FILE_LINE_PROTO);
    ~LauncherController();


private:
    BamfMatcher*     _matcher;
    CompAction*      _expo_action;
    CompScreen*      _screen;
    Launcher*        _launcher;
    LauncherModel*   _model;
    nux::BaseWindow* _window;
    FavoriteStore*   _favorite_store;

    void InsertExpoAction ();

    void RegisterIcon (LauncherIcon *icon);
    
    LauncherIcon * CreateFavorite (const char *file_path);

    void SetupBamf ();

    void OnExpoClicked ();
    
    /* statics */
    
    static bool CompareIcons (LauncherIcon *first, LauncherIcon *second);
    
    static bool BamfTimerCallback (void *data);

    static void OnViewOpened (BamfMatcher *matcher, BamfView *view, gpointer data);
};

#endif // LAUNCHERCONTROLLER_H
