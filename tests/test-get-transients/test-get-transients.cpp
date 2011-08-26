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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <unistd.h> 
#include <sstream>
#include <cstring>

#include <x11-window-read-transients.h>

void usage ()
{
  std::cout << "test-get-transients [WINDOW]" << std::endl;
}

int main (int argc, char **argv)
{
  Display                    *dpy;
  Window                     xid = 0;
  X11WindowReadTransients    *window = NULL;
  X11WindowReadTransients    *transient = NULL;
  X11WindowReadTransients    *hasClientLeader = NULL;
  std::vector<unsigned int>  expectedTransients;

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

  if (argc > 1)
    std::stringstream (argv[1]) >> std::hex >> xid;

  window = new X11WindowReadTransients (dpy, xid);

  if (!xid)
  {
    transient = new X11WindowReadTransients (dpy, 0);
    hasClientLeader = new X11WindowReadTransients (dpy, 0);

    transient->makeTransientFor (window);
    window->setClientLeader (window);
    hasClientLeader->setClientLeader (window);
  }

  /* This assumes that compiz/X is going to process
   * our requests in 3 seconds. That's plenty of time */

  sleep (3);
  window->printTransients ();

  if (transient && hasClientLeader)
  {
    expectedTransients.push_back (transient->id ());
    expectedTransients.push_back (hasClientLeader->id ());

    if (expectedTransients != window->transients ())
    {
      delete transient;
      delete hasClientLeader;
      delete window;
      XCloseDisplay (dpy);

      std::cout << "FAIL: returned transients did not match expected transients" << std::endl;
      return 1;
    }
    else
      std::cout << "PASS: returned transients did match expected transients" << std::endl;
  }

  if (transient)
    delete transient;

  delete window;

  XCloseDisplay (dpy);

  return 0;
}
