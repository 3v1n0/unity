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
    LauncherController(Launcher* launcher, CompScreen *screen, nux::BaseWindow* window);
    ~LauncherController();

    void PresentIconOwningWindow (Window window);
    
private:
    BamfMatcher*     _matcher;
    CompAction*      _expo_action;
    CompScreen*      _screen;
    Launcher*        _launcher;
    LauncherModel*   _model;
    nux::BaseWindow* _window;
    FavoriteStore*   _favorite_store;
    int              _sort_priority;

    void OnLauncherRequestReorder (LauncherIcon *icon, LauncherIcon *after);

    void InsertExpoAction ();
    
    void InsertTrash ();

    void RegisterIcon (LauncherIcon *icon);
    
    LauncherIcon * CreateFavorite (const char *file_path);

    void SetupBamf ();

    void OnExpoClicked (int button);
    
    /* statics */
    
    static bool CompareIcons (LauncherIcon *first, LauncherIcon *second);
    
    static bool BamfTimerCallback (void *data);

    static void OnViewOpened (BamfMatcher *matcher, BamfView *view, gpointer data);
};

#endif // LAUNCHERCONTROLLER_H
