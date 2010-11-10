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
    BamfMatcher* m_Matcher;
    CompAction* m_ExpoAction;
    CompScreen* m_Screen;
    Launcher* m_Launcher;
    LauncherModel* _model;
    nux::BaseWindow* m_Window;
    FavoriteStore* m_FavoriteStore;

    void InsertExpoAction ();

    void RegisterIcon (LauncherIcon *icon);
    
    void CreateFavorite (const char *file_path);

    void SetupBamf ();

    void OnExpoClicked (int button);
    
    /* statics */
    
    static bool BamfTimerCallback (void *data);

    static void OnViewOpened (BamfMatcher *matcher, BamfView *view, gpointer data);
};

#endif // LAUNCHERCONTROLLER_H
