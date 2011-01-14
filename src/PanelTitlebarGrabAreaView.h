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
 * Authored by: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#ifndef PANEL_TITLEBAR_GRAB_AREA_H
#define PANEL_TITLEBAR_GRAB_AREA_H_H

#include <Nux/View.h>

#include "Introspectable.h"

class PanelTitlebarGrabArea : public nux::InputArea, public Introspectable
{
  // This acts a bit like a titlebar, it can be grabbed (such that we can pull
  // the window down)

public:
  PanelTitlebarGrabArea ();
  ~PanelTitlebarGrabArea ();

  sigc::signal <void, int, int> mouse_down;

protected:
  const gchar * GetName ();
  const gchar * GetChildsName ();
  void          AddProperties (GVariantBuilder *builder);

  void RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags);
};

#endif
