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
#include <Nux/ProgramFramework/TestView.h>

#include "PreviewNavigator.h"

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
nux::logging::Logger logger("unity.dash.preview");
}

class TestView2 : public nux::View
{
public:
  TestView2():View(NUX_TRACKER_LOCATION) {}


  void Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
  {
    // just for debugging, draw a vertical gradient
    gPainter.Paint2DQuadVGradient(gfx_engine, GetGeometry(), 
                                  nux::Color(0x96, 0x11, 0xDA), nux::Color(0x54, 0xD9, 0x11));
  }

  void DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
  {
  }
};

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

void Preview::DisableNavButton(NavButton button)
{
}

void Preview::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  // just for debugging, draw a vertical gradient
  // gPainter.Paint2DQuadColor(gfx_engine, GetGeometry(), 
  //                               nux::Color(0, 0, 0));
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
  previews::Style& stlye = previews::Style::Instance();

  layout_ = new nux::HLayout();
  SetLayout(layout_);

  nav_left_ = new PreviewNavigator(Orientation::LEFT, NUX_TRACKER_LOCATION);
  nav_left_->SetMinimumWidth(stlye.NavigatorMinimumWidth());
  nav_left_->SetMaximumWidth(stlye.NavigatorMaximumWidth());
  nav_left_->activated.connect([&]() { navigate_left.emit(); });
  layout_->AddView(nav_left_, 0);

  content_ = new TestView2();
  layout_->AddView(content_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  nav_right_ = new PreviewNavigator(Orientation::RIGHT, NUX_TRACKER_LOCATION);
  nav_right_->SetMinimumWidth(stlye.NavigatorMinimumWidth());
  nav_right_->SetMaximumWidth(stlye.NavigatorMaximumWidth());
  nav_right_->activated.connect([&]() { navigate_right.emit(); });
  layout_->AddView(nav_right_, 0);
}

}
}
}
