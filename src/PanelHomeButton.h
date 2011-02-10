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

#ifndef PANEL_HOME_BUTTON_H
#define PANEL_HOME_BUTTON_H

#include <Nux/TextureArea.h>
#include <Nux/View.h>
#include <NuxImage/CairoGraphics.h>
#include <NuxGraphics/GraphicsEngine.h>

#include "Introspectable.h"

class PanelHomeButton : public nux::TextureArea, public Introspectable
{
public:
  PanelHomeButton ();
  ~PanelHomeButton ();

  void RecvMouseClick (int x, int y, unsigned long button_flags, unsigned long key_flags);
  
  void RecvMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags);

  void RecvMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags);

  void RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);

protected:
  const gchar* GetName ();
  void AddProperties (GVariantBuilder *builder);

private:
  void Refresh ();

private:
  nux::CairoGraphics _util_cg;
  GdkPixbuf *_pixbuf;
};

#endif // PANEL_HOME_BUTTON_H
