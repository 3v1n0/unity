// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 */

#ifndef PLACES_VSCROLLBAR_H
#define PLACES_VSCROLLBAR_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <Nux/ScrollView.h>
#include <Nux/BaseWindow.h>
#include <Nux/VScrollBar.h>
#include <NuxGraphics/CairoGraphics.h>

namespace unity
{
namespace dash
{

class PlacesVScrollBar : public nux::VScrollBar
{
public:
  PlacesVScrollBar(NUX_FILE_LINE_PROTO);
  virtual ~PlacesVScrollBar();

protected:
  virtual void PreLayoutManagement();
  virtual long PostLayoutManagement(long LayoutResult);

  void Draw(nux::GraphicsEngine& gfxContext,
            bool                 forceDraw);
  void DrawContent(nux::GraphicsEngine& gfxContext,
            bool                 forceDraw);

private:
  void UpdateTexture();
  void DrawScrollbar(nux::GraphicsEngine& graphics_engine);

private:
  nux::BaseTexture* _slider_texture;
};

} // namespace dash
} // namespace unity

#endif // PLACES_VSCROLLBAR_H
