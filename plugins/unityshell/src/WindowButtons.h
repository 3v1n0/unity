// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef WINDOW_BUTTONS_H
#define WINDOW_BUTTONS_H

#include <Nux/HLayout.h>
#include <Nux/View.h>

#include "Introspectable.h"

class WindowButtons : public nux::HLayout, public unity::Introspectable
{
  // These are the [close][minimize][restore] buttons on the panel when there
  // is a maximized window

public:
  WindowButtons();
  ~WindowButtons();

  sigc::signal<void> close_clicked;
  sigc::signal<void> minimize_clicked;
  sigc::signal<void> restore_clicked;
  sigc::signal<void> redraw_signal;
  sigc::signal<void, int, int, int, int, unsigned long, unsigned long> mouse_moved;

  virtual nux::Area* FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type);

protected:
  const gchar* GetName();
  const gchar* GetChildsName();
  void          AddProperties(GVariantBuilder* builder);

private:
  void OnCloseClicked(nux::View *view);
  void OnMinimizeClicked(nux::View *view);
  void OnRestoreClicked(nux::View *view);

private:
  nux::HLayout* _layout;
};

#endif
