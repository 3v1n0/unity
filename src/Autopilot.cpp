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
  std::cout << "Running unity tests" << std::endl;
  if (!_tests)
  {
    _tests = new UnityTests ();
  }
  std::cout << "Created unity tests object" << std::endl;

  std::cout << "Creating mouse" << std::endl;
  if (!_mouse)
  {
    _mouse = new Autopilot::Mouse ();
  }
  std::cout << "Created mouse" << std::endl;
  
  sleep(1);

  _tests->ShowTooltip ();
  _tests->ShowQuicklist ();
  /*_tests->DragLauncher ();
  _tests->DragLauncherIconAlongEdgeAndDrop ();
  _tests->DragLauncherIconOutAndDrop ();
  _tests->DragLauncherIconOutAndMove ();*/


  delete _mouse;
  delete _tests;
}

Autopilot::UnityTests::UnityTests ()
{
  std::cout << "Setting up initial nux::Points" << std::endl;
  _initial.Set (800, 500);
  _launcher.Set (32, 57);
  std::cout << "Created class nux::Points" << std::endl;
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
  std::cout << "Showing tooltip" << std::endl;
  _mouse->Move (&_launcher);
  std::cout << "Test finished" << std::endl;
}

void
Autopilot::UnityTests::TestSetup ()
{
  std::cout << "Setting up test" << std::endl;
  _mouse->SetPosition (&_initial);
  usleep (125000);
  std::cout << "Test initialized" << std::endl;
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
  //  XSync (_display, false);
}

void
Autopilot::Mouse::Release (Button button)
{
  XTestFakeButtonEvent (_display, (int) button, false, time (NULL));
  //  XSync (_display, false);
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
  /*  Window root = DefaultRootWindow (_display);
      g_debug ("XWarping to (%d, %d)\n", point->x, point->y);
      XWarpPointer (_display, 0, root, 0, 0, 0, 0, (int) point->x, (int) point->y);
      XFlush (_display);*/
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
