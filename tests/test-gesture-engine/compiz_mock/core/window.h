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

#ifndef COMPIZ_WINDOW_MOCK_H
#define COMPIZ_WINDOW_MOCK_H

/* The real CompWindow */
#include <core/window.h>

class CompWindowMock {
public:
  CompWindowMock() : _moved(false), _maximize_count(0), _maximize_state(0) {}

  int x() const {return _geometry.x();}
  int y() const {return _geometry.y();}
  int width() const {return _geometry.width() + (_geometry.border()*2);}
  int height() const {return _geometry.height() + (_geometry.border()*2);}

  void move(int dx, int dy, bool immediate = true) {
    _moved = true;
    _movement_x = dx;
    _movement_y = dy;
  }

  unsigned int actions () {return _actions;}

  void maximize(int state) {++_maximize_count; _maximize_state = state;}

  /* OBS: I wonder why it returns a reference */
  unsigned int &state() {return _state;}

  void grabNotify(int x, int y, unsigned int state, unsigned int mask) {}
  void ungrabNotify() {}

  void syncPosition() {}

  compiz::window::Geometry &serverGeometry() {return _serverGeometry;}

  unsigned int _actions;
  unsigned int _state;
  compiz::window::Geometry _serverGeometry;
  compiz::window::Geometry _geometry;

  bool _moved;
  int _movement_x;
  int _movement_y;

  int _maximize_count;
  int _maximize_state;
};

#endif
