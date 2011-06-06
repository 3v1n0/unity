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
 * Authored by: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include <core/core.h>
#include <core/pluginclasshandler.h>

#include <core/atoms.h>
#include <X11/Xatom.h>

#include "networkareamonitors_options.h"

class UnityNETWorkareaMonitorsScreen :
    public PluginClassHandler <UnityNETWorkareaMonitorsScreen, CompScreen>,
    public ScreenInterface,
    public NetworkareamonitorsOptions
{
    public:
        UnityNETWorkareaMonitorsScreen (CompScreen *);

        void
        outputChangeNotify ();

        void
        setProperty ();

        void
        handleEvent (XEvent *event);

    private:

        Atom mUnityNETWorkareaMonitorsAtom;
};

class UnityNETWorkareaMonitorsWindow :
    public PluginClassHandler <UnityNETWorkareaMonitorsWindow, CompWindow>,
    public WindowInterface
{
    public:

        UnityNETWorkareaMonitorsWindow (CompWindow *);

        CompWindow *window;

        void
        moveNotify (int dx, int dy, bool immediate);

        void
        resizeNotify (int dx, int dy, unsigned int dwidth, unsigned int dheight);
};

class UnityNETWorkareaMonitorsPluginVTable :
    public CompPlugin::VTableForScreenAndWindow <UnityNETWorkareaMonitorsScreen, UnityNETWorkareaMonitorsWindow>
{
    public:
        bool init ();
};
