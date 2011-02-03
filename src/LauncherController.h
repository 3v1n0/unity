// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#include "DeviceLauncherSection.h"
#include "FavoriteStore.h"
#include "PlaceLauncherSection.h"

#include "LauncherEntryRemote.h"
#include "LauncherEntryRemoteModel.h"

#include <libbamf/libbamf.h>
#include <sigc++/sigc++.h>


class Launcher;

class LauncherController : public sigc::trackable
{

public:
    LauncherController(Launcher* launcher, CompScreen *screen, nux::BaseWindow* window);
    ~LauncherController();

private:
    BamfMatcher*           _matcher;
    CompAction*            _expo_action;
    CompScreen*            _screen;
    Launcher*              _launcher;
    LauncherModel*         _model;
    nux::BaseWindow*       _window;
    FavoriteStore*         _favorite_store;
    int                    _sort_priority;
    PlaceLauncherSection*  _place_section;
    DeviceLauncherSection* _device_section;
    LauncherEntryRemoteModel* _remote_model;

    void SortAndSave ();

    void OnLauncherRequestReorderSmart (LauncherIcon *icon, LauncherIcon *other, bool save);
    void OnLauncherRequestReorderBefore (LauncherIcon *icon, LauncherIcon *before, bool save);
    void OnIconAdded (LauncherIcon *icon);

    void OnLauncerEntryRemoteAdded   (LauncherEntryRemote *entry);
    void OnLauncerEntryRemoteRemoved (LauncherEntryRemote *entry);

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
