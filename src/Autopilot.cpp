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

#include "Autopilot.h"

Autopilot::UnityTests::UnityTests()
{
  _initial.Set (800, 500);
  _launcher.Set (32, 57);
}

void
Autopilot::UnityTests::Run ()
{
  DragLauncher ();
  DragLauncherIconAlongEdgeAndDrop ();
  DragLauncherIconOutAndDrop ();
  DragLauncherIconOutAndMove ();
  ShowQuicklist ();
  ShowTooltip ();
}

void
Autopilot::UnityTests::DragLauncher ()
{
  int new_x = _launcher.x, new_y = _launcher.y + 300;
  nux::Point newp (new_x, new_y);

  TestSetup ();
  _mouse.Press (Mouse::Left);
  _mouse.Move (&newp);
  usleep (250000);
  _mouse.Release (Mouse::Left);
}

void
Autopilot::UnityTests::DragLauncherIconAlongEdgeAndDrop ()
{
  nux::Point a (_launcher.x + 25, _launcher.y);
  nux::Point b (_launcher.x + 25, _launcher.y + 500);

  TestSetup ();
  _mouse.Press (Mouse::Left);
  _mouse.Move (&a);
  _mouse.Move (&b);
  usleep (250000);
  _mouse.Release (Mouse::Left);
}

void
Autopilot::UnityTests::DragLauncherIconOutAndDrop ()
{
  nux::Point a (_launcher.x + 300, _launcher.y);

  TestSetup ();
  _mouse.Press (Mouse::Left);
  _mouse.Move (&a);
  usleep (250000);
  _mouse.Release (Mouse::Left);
}
 
void
Autopilot::UnityTests::DragLauncherIconOutAndMove ()
{
  nux::Point a (_launcher.x + 300, _launcher.y);
  nux::Point b (_launcher.x + 300, _launcher.y + 300);

  TestSetup ();
  _mouse.Press (Mouse::Left);
  _mouse.Move (&a);
  _mouse.Move (&b);
  _mouse.Release (Mouse::Left);
}

void
Autopilot::UnityTests::ShowQuicklist ()
{
  TestSetup ();
  _mouse.Click (Mouse::Right);
  sleep (2);
  _mouse.Click (Mouse::Right);
}

void
Autopilot::UnityTests::ShowTooltip ()
{
  
  TestSetup ();
  _mouse.Move (&_launcher);
}

void
Autopilot::UnityTests::TestSetup ()
{
  _mouse.SetPosition (&_initial);
  usleep (125000);
  _mouse.Move (&_launcher);
  usleep (250000);
}


Autopilot::Mouse::Mouse ()
{
  int event_base, error_base, major_version, minor_version;
  _display = XOpenDisplay (NULL);
  if (_display == NULL || !XTestQueryExtension (_display, 
						&event_base,
						&error_base,
						&major_version,
						&minor_version))
  {
    throw 1;
  }
}

Autopilot::Mouse::~Mouse ()
{
  XCloseDisplay (_display);
}

void
Autopilot::Mouse::Press (Button button)
{
  XTestFakeButtonEvent (_display, (int) button, true, time (NULL));
  XSync (_display, false);
}

void
Autopilot::Mouse::Release (Button button)
{
  XTestFakeButtonEvent (_display, (int) button, false, time (NULL));
  XSync (_display, false);
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
  int xscale, yscale;
  nux::Point *yint;
  float dx, dy, slope;
  nux::Point *curr = GetPosition ();

  dy = (float) (destination->y - curr->y);
  dx = (float) (destination->x - curr->x);
  slope = dy/dx;
  yint = new nux::Point (0, (int) (curr->y - (slope * curr->x)));

  xscale = curr->x < destination->x ? 1 : -1;
  yscale = curr->y < destination->y ? 1 : -1;

  while (curr->x != destination->x)
  {
    curr->x += xscale;
    curr->y = (int) (slope * curr->x + yint->y);

    XTestFakeMotionEvent (_display, -1, curr->x, curr->y, time (NULL));
    XSync (_display, false);
    /* sleep for 1/100th of a second */
    usleep (10000);
  }

  while (curr->y != destination->y)
  {
    curr->y += yscale;
    XTestFakeMotionEvent (_display, -1, curr->x, curr->y, time (NULL));
    XSync (_display, false);
    usleep (10000);
  }
}

void
Autopilot::Mouse::SetPosition (nux::Point *point)
{
  XTestFakeMotionEvent (_display, -1, point->x, point->y, time (NULL));
  XSync (_display, false);
}

nux::Point*
Autopilot::Mouse::GetPosition ()
{
  uint mask;
  int x, y, winx, winy;
  Window w, root_return, child_return;
  
  w = DefaultRootWindow (_display);
  XQueryPointer (_display, 
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
