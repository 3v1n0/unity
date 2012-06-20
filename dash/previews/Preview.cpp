// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */

#include "Preview.h"
#include "unity-shared/IntrospectableWrappers.h"
#include <NuxCore/Logger.h>
#include <Nux/Layout.h>
namespace unity
{
namespace dash
{
namespace
{
nux::logging::Logger logger("unity.dash.preview");
}

NUX_IMPLEMENT_OBJECT_TYPE(Preview);

Preview::Preview()
  : View(NUX_TRACKER_LOCATION)
{
}

Preview::~Preview()
{
}

void Preview::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  // just for debugging, draw a vertical gradient
  gPainter.Paint2DQuadVGradient(gfx_engine, GetGeometry(), 
                                nux::Color(0x96, 0x11, 0xDA), nux::Color(0x54, 0xD9, 0x11));
}

long Preview::ComputeContentSize()
{
  return View::ComputeContentSize();
}


void Preview::DrawContent(nux::GraphicsEngine& GfxContent, bool force_draw)
{
  nux::Geometry base = GetGeometry();
  GfxContent.PushClippingRectangle(base);

  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(GfxContent, force_draw);

  GfxContent.PopClippingRectangle();
}

std::string Preview::GetName() const
{
  return "Preview";
}

void Preview::AddProperties(GVariantBuilder* builder)
{
}

}
}
