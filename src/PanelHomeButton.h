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
#include <NuxGraphics/OpenGLEngine.h>

class PanelHomeButton : public nux::TextureArea
{
public:
  PanelHomeButton ();
  ~PanelHomeButton ();

private:
  void Refresh ();

private:
  nux::CairoGraphics _util_cg;
  GdkPixbuf *_pixbuf;
};

#endif // PANEL_HOME_BUTTON_H
