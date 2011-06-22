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

class WindowButtons : public nux::HLayout, public Introspectable
{
  // These are the [close][minimize][restore] buttons on the panel when there
  // is a maximized window

public:
  WindowButtons ();
  ~WindowButtons ();

  sigc::signal<void> close_clicked;
  sigc::signal<void> minimize_clicked;
  sigc::signal<void> restore_clicked;
  sigc::signal<void> redraw_signal;

protected:
  const gchar * GetName ();
  const gchar * GetChildsName ();
  void          AddProperties (GVariantBuilder *builder);


  // For testing the buttons
  void RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseClick (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseMove (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);

private:
  void OnCloseClicked ();
  void OnMinimizeClicked ();
  void OnRestoreClicked ();

private:
  nux::HLayout *_layout;
};

#endif
