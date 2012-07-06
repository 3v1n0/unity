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
#include <Nux/HLayout.h>

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
nux::logging::Logger logger("unity.dash.previews.preview");
}

NUX_IMPLEMENT_OBJECT_TYPE(Preview);

Preview::Preview(dash::Preview::Ptr preview_model)
  : View(NUX_TRACKER_LOCATION)
  , preview_model_(preview_model)
{
  SetupViews();
}

Preview::~Preview()
{
}

void Preview::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
   // gPainter.Paint2DQuadVGradient(gfx_engine, GetGeometry(), 
   //                             nux::Color(0x96, 0x11, 0xDA), nux::Color(0x54, 0xD9, 0x11));
}

void Preview::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);

  gfx_engine.PopClippingRectangle();
}

std::string Preview::GetName() const
{
  return "Preview";
}

void Preview::AddProperties(GVariantBuilder* builder)
{
}

void Preview::SetupViews()
{
}

}
}
}
