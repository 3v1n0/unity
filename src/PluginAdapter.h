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

#ifndef PLUGINADAPTER_H
#define PLUGINADAPTER_H

/* Compiz */
#include <core/core.h>

#include <sigc++/sigc++.h>

#include "WindowManager.h"

class MultiActionList
{
public:

    MultiActionList (int n) :
        m_ActionList (n),
        _primary_action (NULL) {};

    void InitiateAll (CompOption::Vector &extraArgs);
    void TerminateAll (CompOption::Vector &extraArgs);

    void AddNewAction (CompAction *, bool primary);
    void RemoveAction (CompAction *);
private:

    std::list <CompAction *> m_ActionList;
    CompAction *             _primary_action;
};
    

class PluginAdapter : public sigc::trackable, public WindowManager
{
public:
    static PluginAdapter * Default ();

    static void Initialize (CompScreen *screen);

    ~PluginAdapter();
    
    std::string * MatchStringForXids (std::list<Window> *windows);
    
    void SetScaleAction (MultiActionList &scale);    
    void SetExpoAction (MultiActionList &expo);

    void OnScreenGrabbed ();
    void OnScreenUngrabbed ();

    void InitiateScale (std::string *match);
    void TerminateScale ();
    bool IsScaleActive ();
    
    void InitiateExpo ();

    void Notify (CompWindow *window, CompWindowNotify notify);
    void NotifyMoved (CompWindow *window, int x, int y);
    void NotifyResized (CompWindow *window, int x, int y, int w, int h);
    void NotifyStateChange (CompWindow *window, unsigned int state, unsigned int last_state);
    
    // WindowManager implementation
    bool IsWindowMaximized (guint xid);
    bool IsWindowDecorated (guint xid);
    bool IsWindowFocussed (guint xid);
    void Restore (guint32 xid);
    void Minimize (guint32 xid);
    void Close (guint32 xid);

    void MaximizeIfBigEnough (CompWindow *window);
    
protected:
    PluginAdapter(CompScreen *screen);

private:
    CompScreen *m_Screen;
    MultiActionList m_ExpoActionList;
    MultiActionList m_ScaleActionList;
    std::list <guint32> m_SpreadedWindows;
    CompWindow *m_focussed;
    
    static PluginAdapter *_default;
};

#endif
