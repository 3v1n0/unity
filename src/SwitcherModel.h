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

#ifndef SWITCHERMODEL_H
#define SWITCHERMODEL_H

#include "LauncherIcon.h"
#include "LauncherModel.h"

#include <sigc++/sigc++.h>

class SwitcherModel : public sigc::trackable
{

public:
    typedef std::vector<LauncherIcon*> Base;
    typedef Base::iterator iterator; 
    typedef Base::reverse_iterator reverse_iterator; 
    
    SwitcherModel(LauncherModel *model);
    virtual ~SwitcherModel();

    iterator begin ();
    iterator end ();
    
    int Size ();
    
    LauncherIcon *Selected ();
    int SelectedIndex ();
    
    void Next ();
    void Prev ();
    
    void Select (LauncherIcon *selection);
    void SelectIndex (int index);
    
private:
    Base             _inner;
    LauncherModel   *_model;
    int              _index;
    
    void SetInitialIndex ();
    static bool CompareSwitcherItem (LauncherIcon *first, LauncherIcon *second);
};

#endif // SWITCHERMODEL_H

