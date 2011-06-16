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

EventFaker::EventFaker (nux::WindowThread* thread)
{
  _thread = thread;
}

EventFaker::~EventFaker ()
{
}

void
EventFaker::SendClick (nux::View* view)
{
  Display* display = NULL;
  int      x       = 0;
  int      y       = 0;

  // sanity check
  if (!view)
    return;

  display = nux::GetGraphicsDisplay ()->GetX11Display ();

  // get a point inside the view
  x = view->GetBaseX () + 1;
  y = view->GetBaseY () + 1;

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
    x, y,
    x, y,
    0,
    Button1,
    True
  };

  // send that button-click to the "thread"
  doEvent (view, (XEvent*) &buttonEvent);
}

void
EventFaker::doEvent (nux::View* view,
                     XEvent*    event)
{
  Display* display = NULL;

  // sanity check
  if (!view || !event)
    return;

  display = nux::GetGraphicsDisplay ()->GetX11Display ();
  XUngrabPointer (display, CurrentTime);
  XFlush (display);

  _thread->ProcessForeignEvent (event, NULL);

  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, false);
}
