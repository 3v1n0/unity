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
    BamfLauncherIcon(Launcher* IconManager, BamfApplication *app, CompScreen *screen);
    ~BamfLauncherIcon();
    
    
protected:
    void OnMouseClick (int button);
    std::list<DbusmenuMenuitem *> GetMenus ();
    
    void UpdateIconGeometries (nux::Point3 center);
    void OnCenterStabilized (nux::Point3 center);
    
    bool IconOwnsWindow (Window w);
    
    void AddProperties (GVariantBuilder *builder);

private:
    BamfApplication *m_App;
    CompScreen *m_Screen;
    std::map<std::string, DbusmenuClient *> _menu_clients;
    std::map<std::string, DbusmenuMenuitem *> _menu_items;
    
    void EnsureWindowState ();
    
    void UpdateMenus ();
    
    void OpenInstance ();
    
    void EnsureMenuItemsReady ();
    
    static void OnClosed (BamfView *view, gpointer data);
    static void OnUserVisibleChanged (BamfView *view, gboolean visible, gpointer data);
    static void OnActiveChanged (BamfView *view, gboolean active, gpointer data);
    static void OnRunningChanged (BamfView *view, gboolean running, gpointer data);
    static void OnUrgentChanged (BamfView *view, gboolean urgent, gpointer data);
    static void OnChildAdded (BamfView *view, BamfView *child, gpointer data);
    static void OnChildRemoved (BamfView *view, BamfView *child, gpointer data);
    
    static void OnLaunch (DbusmenuMenuitem *item, int time, BamfLauncherIcon *self);
    static void OnQuit (DbusmenuMenuitem *item, int time, BamfLauncherIcon *self);
    static void OnTogglePin (DbusmenuMenuitem *item, int time, BamfLauncherIcon *self);
};

#endif // BAMFLAUNCHERICON_H

