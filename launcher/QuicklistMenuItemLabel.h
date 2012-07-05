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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 * Authored by: Jay Taoko <jay.taoko@canonical.com>
 */

#ifndef QUICKLISTMENUITEMLABEL_H
#define QUICKLISTMENUITEMLABEL_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <NuxGraphics/CairoGraphics.h>

#include "QuicklistMenuItem.h"

#include <X11/Xlib.h>

namespace unity
{

class QuicklistMenuItemLabel : public QuicklistMenuItem
{
public:
  QuicklistMenuItemLabel(DbusmenuMenuitem* item,
                         NUX_FILE_LINE_PROTO);

  QuicklistMenuItemLabel(DbusmenuMenuitem* item,
                         bool              debug,
                         NUX_FILE_LINE_PROTO);

  ~QuicklistMenuItemLabel();

protected:

  void PreLayoutManagement();

  long PostLayoutManagement(long layoutResult);

  void Draw(nux::GraphicsEngine& gfxContext, bool forceDraw);

  void DrawContent(nux::GraphicsEngine& gfxContext, bool forceDraw);

  void PostDraw(nux::GraphicsEngine& gfxContext, bool forceDraw);

  virtual const gchar* GetDefaultText();

  virtual void UpdateTexture();
  virtual int CairoSurfaceWidth();
};

} // NAMESPACE

#endif // QUICKLISTMENUITEMLABEL_H
