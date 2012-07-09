// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *              Sam Spilsbury <sam.spilsbury@canonical.com>
 *              Didier Roche <didier.roche@canonical.com>
 *              Marco Trevisan (Treviño) <mail@3v1n0.net>
 */

#ifndef PANEL_TITLEBAR_GRAB_AREA_H
#define PANEL_TITLEBAR_GRAB_AREA_H_H

#include <Nux/View.h>
#include <UnityCore/GLibSource.h>

#include "unity-shared/Introspectable.h"

namespace unity
{

class PanelTitlebarGrabArea : public nux::InputArea, public unity::debug::Introspectable
{
  /* This acts a bit like a decorator, it can be clicked or grabbed (such that
   * we can pull the window down) */

public:
  PanelTitlebarGrabArea();
  ~PanelTitlebarGrabArea();

  void SetGrabbed(bool enabled);
  bool IsGrabbed();

  sigc::signal<void, int, int> lower_request;
  sigc::signal<void, int, int> activate_request;
  sigc::signal<void, int, int> restore_request;
  sigc::signal<void, int, int> grab_started;
  sigc::signal<void, int, int> grab_move;
  sigc::signal<void, int, int> grab_end;

protected:
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  void OnMouseDown(int x, int y, unsigned long button_flags, unsigned long);
  void OnMouseUp(int x, int y, unsigned long button_flags, unsigned long);
  void OnGrabMove(int x, int y, int, int, unsigned long button_flags, unsigned long);

  Cursor grab_cursor_;
  bool grab_started_;
  nux::Point mouse_down_point_;
  unsigned int mouse_down_button_;
  glib::Source::UniquePtr mouse_down_timer_;
};

} // NAMESPACE

#endif
