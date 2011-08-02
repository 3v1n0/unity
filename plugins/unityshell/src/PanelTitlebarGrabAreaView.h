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
 * Authored by: Didier Roche <didier.roche@canonical.com>
 */

#ifndef PANEL_TITLEBAR_GRAB_AREA_H
#define PANEL_TITLEBAR_GRAB_AREA_H_H


// TODO: remove once double click will work in nux
#include <time.h>

#include <Nux/View.h>

#include "Introspectable.h"

class PanelTitlebarGrabArea : public nux::InputArea, public unity::Introspectable
{
  // This acts a bit like a titlebar, it can be grabbed (such that we can pull
  // the window down)

public:
  PanelTitlebarGrabArea();
  ~PanelTitlebarGrabArea();

  sigc::signal <void, int, int> mouse_down;
  sigc::signal <void> mouse_middleclick;
  sigc::signal <void> mouse_doubleclick;
  sigc::signal <void> mouse_doubleleftclick;

protected:
  void RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseDoubleClick(int x, int y, unsigned long button_flags, unsigned long key_flags);
  // TODO: can be safely removed once OnMouseDoubleClick is fixed in nux
  void RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags);
  struct timespec time_diff(struct timespec start, struct timespec end);

  struct timespec _last_click_time;

  const gchar* GetName();
  const gchar* GetChildsName();
  void          AddProperties(GVariantBuilder* builder);
};

#endif
