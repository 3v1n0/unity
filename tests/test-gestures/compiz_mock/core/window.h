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

class CompWindowMock
{
public:
  CompWindowMock() : moved_(false), maximize_count_(0), maximize_state_(0),
                     minimized_(false) {}

  int x() const {return geometry_.x();}
  int y() const {return geometry_.y();}
  int width() const {return geometry_.width() + (geometry_.border()*2);}
  int height() const {return geometry_.height() + (geometry_.border()*2);}
  int id() { return id_; }

  void move(int dx, int dy, bool immediate = true)
  {
    moved_ = true;
    movement_x_ = dx;
    movement_y_ = dy;
  }

  unsigned int actions () {return actions_;}

  bool minimized() { return minimized_; }

  void maximize(int state) {++maximize_count_; maximize_state_ = state;}

  /* OBS: I wonder why it returns a reference */
  unsigned int &state() {return state_;}

  void grabNotify(int x, int y, unsigned int state, unsigned int mask) {}
  void ungrabNotify() {}

  compiz::window::Geometry &serverGeometry() {return server_geometry_;}

  unsigned int actions_;
  unsigned int state_;
  compiz::window::Geometry server_geometry_;
  compiz::window::Geometry geometry_;

  bool moved_;
  int movement_x_;
  int movement_y_;

  int maximize_count_;
  int maximize_state_;

  int id_;

  bool minimized_;
};

#endif
