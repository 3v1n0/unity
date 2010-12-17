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

class PluginAdapter : public sigc::trackable
{
public:
    static PluginAdapter * Default ();

    static void Initialize (CompScreen *screen);

    ~PluginAdapter();
    
    std::string * MatchStringForXids (std::list<Window> *windows);
    
    void SetScaleAction (CompAction *scale);
    
    void SetExpoAction (CompAction *expo);
    
    void InitiateScale (std::string *match);

    bool IsScaleActive ();

    void TerminateScale ();
    
    void InitiateExpo ();
    
    void Notify (CompWindow *window, CompWindowNotify notify);
    void NotifyMoved (CompWindow *window, int x, int y);
    void NotifyResized (CompWindow *window, int x, int y, int w, int h);
    void NotifyStateChange (CompWindow *window, unsigned int state, unsigned int last_state);
    
    sigc::signal<void, CompWindow *> window_maximized;
    sigc::signal<void, CompWindow *> window_restored;
    sigc::signal<void, CompWindow *> window_minimized;
    sigc::signal<void, CompWindow *> window_unminimized;
    sigc::signal<void, CompWindow *> window_shaded;
    sigc::signal<void, CompWindow *> window_unshaded;
    sigc::signal<void, CompWindow *> window_mapped;
    sigc::signal<void, CompWindow *> window_unmapped;
    sigc::signal<void, CompWindow *> window_shown;
    sigc::signal<void, CompWindow *> window_hidden;
    sigc::signal<void, CompWindow *> window_resized;
    sigc::signal<void, CompWindow *> window_moved;
    
protected:
    PluginAdapter(CompScreen *screen);

private:
    CompScreen *m_Screen;
    CompAction *m_ExpoAction;
    CompAction *m_ScaleAction;
    
    static PluginAdapter *_default;
};

#endif
