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

    void InitiateAll (CompOption::Vector &extraArgs, int state);
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
    
    void SetShowHandlesAction (CompAction *action) { _grab_show_action = action; }
    void SetHideHandlesAction (CompAction *action) { _grab_hide_action = action; }
    void SetToggleHandlesAction (CompAction *action) { _grab_toggle_action = action; }

    void OnScreenGrabbed ();
    void OnScreenUngrabbed ();

    void InitiateScale (std::string *match, int state = 0);
    void TerminateScale ();
    bool IsScaleActive ();
    
    void InitiateExpo ();
    bool IsExpoActive ();
    
    void ShowGrabHandles (CompWindow *window);
    void HideGrabHandles (CompWindow *window);
    void ToggleGrabHandles (CompWindow *window);

    void Notify (CompWindow *window, CompWindowNotify notify);
    void NotifyMoved (CompWindow *window, int x, int y);
    void NotifyResized (CompWindow *window, int x, int y, int w, int h);
    void NotifyStateChange (CompWindow *window, unsigned int state, unsigned int last_state);
    
    // WindowManager implementation
    bool IsWindowMaximized (guint xid);
    bool IsWindowDecorated (guint xid);
    bool IsWindowOnCurrentDesktop (guint xid);
    bool IsWindowObscured (guint xid);
    void Restore (guint32 xid);
    void Minimize (guint32 xid);
    void Close (guint32 xid);
    void Activate (guint32 xid);
    void Raise (guint32 xid);
    void Lower (guint32 xid);
    
    bool IsScreenGrabbed ();

    void MaximizeIfBigEnough (CompWindow *window);

    nux::Geometry GetWindowGeometry (guint32 xid);
    
protected:
    PluginAdapter(CompScreen *screen);

private:
    CompScreen *m_Screen;
    MultiActionList m_ExpoActionList;
    MultiActionList m_ScaleActionList;
    std::list <guint32> m_SpreadedWindows;
    
    bool _spread_state;
    bool _expo_state;
    
    CompAction *_grab_show_action;
    CompAction *_grab_hide_action;
    CompAction *_grab_toggle_action;
    
    static PluginAdapter *_default;
};

#endif
