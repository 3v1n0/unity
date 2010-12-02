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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 */

#include "EventFaker.h"

#include "NuxGraphics/GLWindowManager.h"
#include <X11/Xlib.h>

void
EventFaker::EventFaker ()
{
}

void
EventFaker::~EventFaker ()
{
}

void
EventFaker::SendClick (nux::View*         view,
                       nux::WindowThread* thread)
{
  nux::Point point;
  XEvent*    event   = NULL;
  Display*   display = NULL;

  // sanity check
  if (!view || !thread)
    return;

  display = nux::GetThreadGLWindow()->GetX11Display ();
  XUngrabPointer (display, CurrentTime);
  XFlush (display);

  // get a point inside the view
  point = view->GetCenter ();

  // assemble a button-click event
  XButtonEvent buttonEvent = {
    ButtonRelease,
    0,
    False,
    display,
    0,
    0,
    0,
    CurrentTime,
    point.x, point.y,
    point.x, point.y,
    0,
    Button1,
    True
  };

  // send that button-click to the thread
  event = (XEvent*) &buttonEvent;
  thread->ProcessForeignEvent (event, NULL);
}
