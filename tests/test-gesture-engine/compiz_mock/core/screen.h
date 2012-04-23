/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Daniel d'Andrada <daniel.dandrada@canonical.com>
 *
 */

#ifndef COMPIZ_SCREEN_MOCK_H
#define COMPIZ_SCREEN_MOCK_H

#include <X11/Xlib.h>
#include <vector>

// The real CompScreen
#include <core/screen.h>

typedef std::vector<CompWindowMock*> CompWindowMockVector;

class CompScreenMock {
public:
  CompScreenMock() : _grab_count(0) {}

  typedef int GrabHandle;

  int width() const {return _width;}
  int height() const {return _height;}

  Display *dpy() {return _dpy;}

  const CompWindowMockVector & clientList(bool stackingOrder = true) {
    if (stackingOrder)
      return _client_list_stacking;
    else
      return _client_list;
  }

  Window root() {return _root;}

  GrabHandle pushGrab(Cursor cursor, const char *name) {
    _grab_count++;
    return 0;
  }
  void removeGrab(GrabHandle handle, CompPoint *restorePointer) {
    _grab_count--;
  }

  Cursor invisibleCursor() {return 1;}

  int _width;
  int _height;
  Display *_dpy;
  CompWindowMockVector _client_list;
  CompWindowMockVector _client_list_stacking;
  Window _root;
  int _grab_count;
};

extern CompScreenMock *screen_mock;
extern int pointerX_mock;
extern int pointerY_mock;

#endif

