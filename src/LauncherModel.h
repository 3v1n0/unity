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

#ifndef LAUNCHERMODEL_H
#define LAUNCHERMODEL_H

#include "LauncherIcon.h"

#include <sigc++/sigc++.h>

class LauncherIcon;

class LauncherModel : public sigc::trackable
{

public:
    typedef std::list<LauncherIcon*> Base;
    typedef Base::iterator iterator; 
    typedef Base::reverse_iterator reverse_iterator; 
    typedef bool (*SortFunc) (LauncherIcon *first, LauncherIcon *second);
    
    LauncherModel();
    ~LauncherModel();

    void AddIcon (LauncherIcon *icon);
    void RemoveIcon (LauncherIcon *icon);
    void Sort (SortFunc func);
    int  Size ();

    void OnIconRemove (LauncherIcon *icon);
    
    bool IconHasSister (LauncherIcon *icon);
    
    iterator begin ();
    iterator end ();
    reverse_iterator rbegin ();
    reverse_iterator rend ();
    
    iterator main_begin ();
    iterator main_end ();
    reverse_iterator main_rbegin ();
    reverse_iterator main_rend ();
    
    iterator shelf_begin ();
    iterator shelf_end ();
    reverse_iterator shelf_rbegin ();
    reverse_iterator shelf_rend ();
    
    sigc::signal<void, LauncherIcon *> icon_added;
    sigc::signal<void, LauncherIcon *> icon_removed;
    sigc::signal<void> order_changed;
    
private:
    Base _inner;
    Base _inner_shelf;
    Base _inner_main;
    
    void Populate ();
    
    bool IconShouldShelf (LauncherIcon *icon);
    
    static gboolean RemoveCallback (gpointer data);
};

#endif // LAUNCHERMODEL_H
