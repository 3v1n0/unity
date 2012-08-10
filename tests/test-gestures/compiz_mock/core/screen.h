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
  CompScreenMock() : grab_count_(0), next_grab_handle_(1) {}

  typedef int GrabHandle;

  int width() const {return width_;}
  int height() const {return height_;}

  Display *dpy() {return dpy_;}

  const CompWindowMockVector & clientList(bool stackingOrder = true) {
    if (stackingOrder)
      return client_list_stacking_;
    else
      return client_list_;
  }

  Window root() {return root_;}

  GrabHandle pushGrab(Cursor cursor, const char *name) {
    grab_count_++;
    return next_grab_handle_++;
  }
  void removeGrab(GrabHandle handle, CompPoint *restorePointer) {
    grab_count_--;
  }

  Cursor invisibleCursor() {return 1;}

  int width_;
  int height_;
  Display *dpy_;
  CompWindowMockVector client_list_;
  CompWindowMockVector client_list_stacking_;
  Window root_;
  int grab_count_;
  int next_grab_handle_;
};

extern CompScreenMock *screen_mock;
extern int pointerX_mock;
extern int pointerY_mock;

#endif

