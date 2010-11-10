#ifndef BAMFLAUNCHERICON_H
#define BAMFLAUNCHERICON_H

/* Compiz */
#include <core/core.h>

#include <Nux/BaseWindow.h>
#include <NuxCore/Math/MathInc.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libbamf/libbamf.h>
#include <sigc++/sigc++.h>

#include "SimpleLauncherIcon.h"

class Launcher;

class BamfLauncherIcon : public SimpleLauncherIcon
{
public:
    BamfLauncherIcon(Launcher* IconManager, BamfApplication *app, CompScreen *screen, NUX_FILE_LINE_PROTO);
    ~BamfLauncherIcon();
    
private:
    BamfApplication *m_App;
    CompScreen *m_Screen;
    
    void OnMouseClick ();
    void EnsureWindowState ();
    
    static void OnClosed (BamfView *view, gpointer data);
    static void OnUserVisibleChanged (BamfView *view, gboolean visible, gpointer data);
    static void OnActiveChanged (BamfView *view, gboolean active, gpointer data);
    static void OnRunningChanged (BamfView *view, gboolean running, gpointer data);
    static void OnUrgentChanged (BamfView *view, gboolean urgent, gpointer data);
    static void OnChildAdded (BamfView *view, BamfView *child, gpointer data);
    static void OnChildRemoved (BamfView *view, BamfView *child, gpointer data);
};

#endif // BAMFLAUNCHERICON_H

