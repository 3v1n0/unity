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
 * Authored by: Alex Launi <alex.launi@canonical.com>
 */


#include <time.h>
#include <unistd.h>

#include <glib.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

#include <core/screen.h>

#include "Autopilot.h"

Autopilot::Mouse *Autopilot::UnityTests::_mouse = 0;
Autopilot::UnityTests *Autopilot::UnityTests::_tests = 0;

/* static */
void
Autopilot::UnityTests::Run ()
{
  g_debug ("Running unity tests");
  if (!_tests)
  {
    _tests = new UnityTests ();
  }
  g_debug ("Created unity tests object");

  
  sleep(1);

  /*_tests->ShowTooltip ();
  _tests->ShowQuicklist ();
  _tests->DragLauncher ();
  _tests->DragLauncherIconAlongEdgeAndDrop ();
  _tests->DragLauncherIconOutAndDrop ();
  _tests->DragLauncherIconOutAndMove ();*/

}

Autopilot::UnityTests::UnityTests ()
{
  g_debug ("Setting up initial nux::Points");
  _initial.Set (800, 500);
  _launcher.Set (32, 57);
  g_debug ("Created class nux::Points");
}

void
Autopilot::UnityTests::DragLauncher ()
{
  int new_x = _launcher.x, new_y = _launcher.y + 300;
  nux::Point newp (new_x, new_y);

  TestSetup ();
  _mouse->Move (&_launcher);
  usleep (125000);
  _mouse->Press (Mouse::Left);
  _mouse->Move (&newp);
  usleep (250000);
  _mouse->Release (Mouse::Left);
}

void
Autopilot::UnityTests::DragLauncherIconAlongEdgeAndDrop ()
{
  nux::Point a (_launcher.x + 25, _launcher.y);
  nux::Point b (_launcher.x + 25, _launcher.y + 500);

  TestSetup ();
  _mouse->Move (&_launcher);
  usleep (125000);
  _mouse->Press (Mouse::Left);
  _mouse->Move (&a);
  _mouse->Move (&b);
  usleep (250000);
  _mouse->Release (Mouse::Left);
}

void
Autopilot::UnityTests::DragLauncherIconOutAndDrop ()
{
  nux::Point a (_launcher.x + 300, _launcher.y);

  TestSetup ();
  _mouse->SetPosition (&_launcher);
  usleep (125000);
  _mouse->Press (Mouse::Left);
  _mouse->Move (&a);
  usleep (250000);
  _mouse->Release (Mouse::Left);
}
 
void
Autopilot::UnityTests::DragLauncherIconOutAndMove ()
{
  nux::Point a (_launcher.x + 300, _launcher.y);
  nux::Point b (_launcher.x + 300, _launcher.y + 300);

  TestSetup ();
  _mouse->Move (&_launcher);
  usleep (125000);
  _mouse->Press (Mouse::Left);
  _mouse->Move (&a);
  _mouse->Move (&b);
  _mouse->Release (Mouse::Left);
}

void
Autopilot::UnityTests::ShowQuicklist ()
{
  TestSetup ();
  _mouse->Move (&_launcher);
  usleep (125000);
  _mouse->Click (Mouse::Right);
  sleep (2);
  _mouse->Click (Mouse::Right);
}

void
Autopilot::UnityTests::ShowTooltip ()
{
  TestSetup ();
  _mouse->Move (&_launcher);
  g_debug ("Showing tooltip");
  _mouse->Move (&_launcher);
  g_debug ("Test finished");
}

void
Autopilot::UnityTests::TestSetup ()
{
  g_debug ("Setting up test");
  _mouse->SetPosition (&_initial);
  usleep (125000);
  g_debug ("Test initialized");
}

void
Autopilot::Mouse::Press (Button button)
{
  XTestFakeButtonEvent (screen->dpy (), (int) button, true, time (NULL));
  //  XSync (_display, false);
}

void
Autopilot::Mouse::Release (Button button)
{
  XTestFakeButtonEvent (screen->dpy (), (int) button, false, time (NULL));
  //  XSync (screen->dpy (), false);
}

void
Autopilot::Mouse::Click (Button button)
{
  Press (button);
  /* sleep for 1/4 of a second */
  usleep (250000);
  Release (button);
}

void
Autopilot::Mouse::Move (nux::Point *destination)
{
  // FIXME: animate!

  SetPosition (destination);
}

void
Autopilot::Mouse::SetPosition (nux::Point *point)
{
  /*  Window root = DefaultRootWindow (screen->dpy ());
      g_debug ("XWarping to (%d, %d)\n", point->x, point->y);
      XWarpPointer (screen->dpy (), 0, root, 0, 0, 0, 0, (int) point->x, (int) point->y);
      XFlush (screen->dpy ());*/
  nux::Point* curr = GetPosition ();
  g_debug ("comp->warp (%d, %d)", point->x - curr->x, point->y - curr->y);
  screen->warpPointer (point->x - curr->x, point->y - curr->y);
}

nux::Point*
Autopilot::Mouse::GetPosition ()
{
  uint mask;
  int x, y, winx, winy;
  Window w, root_return, child_return;
  
  w = DefaultRootWindow (screen->dpy ());
  XQueryPointer (screen->dpy (), 
		 w,
		 &root_return,
		 &child_return,
		 &x,
		 &y,
		 &winx,
		 &winy,
		 &mask);
  return new nux::Point (x, y);
}
