/*
 * Copyright (C) 2011 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authored By:
 * Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include <inputremover.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <unistd.h> 
#include <sstream>

void usage ()
{
  std::cout << "test-input-remover [WINDOW] [TIME]" << std::endl;
}

bool
print_rects (Display *dpy, Window xid)
{
  XRectangle		       *rects;
  int			       count = 0, ordering;
  int			       x, y;
  unsigned int	       width, height, border, depth;
  Window		       root;

  XGetGeometry (dpy, xid, &root, &x, &y, &width, &height, &border, &depth);

  rects = XShapeGetRectangles (dpy, xid, ShapeInput,
                               &count, &ordering);

  if (count == 0)
    std::cout << "No Input Shape Set" << std::endl;

  /* check if the returned shape exactly matches the window shape -
     *      if that is true, the window currently has no set input shape */
  if ((count == 1) &&
      (rects[0].x == -((int) border)) &&
      (rects[0].y == -((int) border)) &&
      (rects[0].width == (width + border)) &&
      (rects[0].height == (height + border)))
  {
    std::cout << "No Input Shape Defined" << std::endl;
  }

  for (int i = 0; i < count; i++)
  {
    std::cout << "Rect - " << rects[i].x << ", " << rects[i].y << ", " << rects[i].width << ", " << rects[i].height << std::endl;
  }

  if (rects)
    XFree (rects);

  return count > 0;
}

int main (int argc, char **argv)
{
  Display                    *dpy;
  Window                     xid;
  int                        time = 0;
  compiz::WindowInputRemover *remover;
  bool		             shapeExt;
  int			     shapeEvent;
  int			     shapeError;
  XEvent                     event;

  if ((argc == 2 && std::string (argv[1]) == "--help") || argc > 3)
  {
    usage ();
    return 1;
  }

  dpy = XOpenDisplay (NULL);

  if (!dpy)
  {
    std::cerr << "Failed to open display ... setting test to passed" << std::endl;
    return 0;
  }

  shapeExt = XShapeQueryExtension (dpy, &shapeEvent, &shapeError);

  if (!shapeExt)
  {
    std::cerr << "No shape extension .. setting test to passed" << std::endl;
    XCloseDisplay (dpy);
    return 0;
  }

  if (argc > 1)
    std::stringstream (argv[1]) >> std::hex >> xid;
  else
  {
    XSetWindowAttributes attrib;

    attrib.backing_pixel = 0x0;

    xid = XCreateWindow (dpy, DefaultRootWindow (dpy), 0, 0, 100, 100, 0,
                         DefaultDepth (dpy, DefaultScreen (dpy)), InputOutput,
                         DefaultVisual (dpy, DefaultScreen (dpy)), CWBackingPixel, &attrib);

    XSelectInput (dpy, xid, ExposureMask | StructureNotifyMask);
    XMapRaised (dpy, xid);
    do
    {
      XNextEvent (dpy, &event);
      bool exposed = false;

      switch (event.type)
      {
        case Expose:
          if (event.xexpose.window == xid)
            exposed = true;
          break;
        default:
          break;
      }

      if (exposed)
        break;

    } while (XPending (dpy));
  }

  XShapeSelectInput (dpy, xid, ShapeNotifyMask);

  if (argc == 3)
    std::stringstream (argv[2]) >> std::dec >> time;

  remover = new compiz::WindowInputRemover (dpy, xid, xid);
  if (!remover)
    return 1;

  print_rects (dpy, xid);

  std::cout << "Saving input shape of 0x" << std::hex << xid << std::dec << std::endl;
  remover->save ();
  std::cout << "Removing input shape of 0x" << std::hex << xid << std::dec << std::endl;
  remover->remove ();

  /* We won't get a synethetic ShapeNotify event if this
   * is on a window that we don't own */

  if (argc == 1)
  {
    do
    {
      XNextEvent (dpy, &event);
      if (event.type == shapeEvent + ShapeNotify &&
  	event.xany.send_event)
      {
        std::cout << "received ShapeNotify on 0x" << std::hex << xid << std::endl;
  	break;
      }

    } while (XPending (dpy));
  }
  else
    XSync (dpy, false);

  std::cout << "Getting input rects for 0x" << std::hex << xid << std::dec << std::endl;
  if (print_rects (dpy, xid))
  {
    std::cout << "Error! Window still has rects after shape was removed!" << std::endl;
    delete remover;
    XCloseDisplay (dpy);
    return 1;
  }

  std::cout << "Waiting " << std::dec << time << " seconds" << std::endl;
  sleep (time);

  std::cout << "Restoring input shape of 0x" << std::hex << xid << std::dec << std::endl;
  remover->restore ();

  if (argc == 1)
  {
    do
    {
      XNextEvent (dpy, &event);
      if (event.type == shapeEvent + ShapeNotify &&
  	event.xany.send_event)
      {
          std::cout << "received ShapeNotify on 0x" << std::hex << xid << std::endl;
  	break;
      }

    } while (XPending (dpy));
  }
  else
    XSync (dpy, false);

  if (!print_rects (dpy, xid))
  {
    std::cout << "Error! Failed to restore input shape for 0x" << std::hex << xid << std::dec << std::endl;
    delete remover;
    XCloseDisplay (dpy);
    return 1;
  }

  delete remover;

  XCloseDisplay (dpy);

  return 0;
}
