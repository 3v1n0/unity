// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012 Canonical Ltd.
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */


#include "PreviewNavigator.h"
#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <unity-shared/IconTexture.h>
#include <unity-shared/PreviewStyle.h>

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
nux::logging::Logger logger("unity.dash.previews.previewnavigator");
}

NUX_IMPLEMENT_OBJECT_TYPE(PreviewNavigator);

PreviewNavigator::PreviewNavigator(Orientation orientation, NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , orientation_(orientation)
  , texture_(nullptr)
{
  SetupViews();
}

std::string PreviewNavigator::GetName() const
{
  return "PreviewNavigator";
}

void PreviewNavigator::SetEnabled(bool enabled)
{
  texture_->SetEnableView(enabled);
  texture_->SetVisible(enabled);
}

void PreviewNavigator::AddProperties(GVariantBuilder* builder)
{

}

void PreviewNavigator::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
}

void PreviewNavigator::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

    unsigned int alpha, src, dest = 0;
    gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
    gfx_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);

  gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);

  gfx_engine.PopClippingRectangle();
}

void PreviewNavigator::SetupViews()
{
  if (orientation_ == Orientation::LEFT || orientation_ == Orientation::RIGHT)
  {
    nux::VLayout* vlayout = new nux::VLayout();
    vlayout->SetSpaceBetweenChildren(0);
    layout_ = vlayout;

    if (orientation_ == Orientation::LEFT)
      texture_ = new IconTexture(Style::Instance().GetNavLeftIcon(), 32, 32);
    else 
      texture_ = new IconTexture(Style::Instance().GetNavRightIcon(), 32, 32);
  }
  else if (orientation_ == Orientation::UP || orientation_ == Orientation::DOWN)
  {
    nux::HLayout* hlayout = new nux::HLayout();
    hlayout->SetSpaceBetweenChildren(0);
    layout_ = hlayout;
  }
  SetLayout(layout_);

  layout_->AddSpace(0, 1);

  if (texture_)
  {
    texture_->mouse_click.connect([&](int, int, unsigned long, unsigned long) { activated.emit(); });
    texture_->SetMinimumHeight(32);
    texture_->SetMaximumHeight(32);
    texture_->SetMinimumWidth(32);
    texture_->SetMaximumWidth(32);
    layout_->AddView(texture_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);    
  }

  layout_->AddSpace(0, 1);
}


} // namespace previews
} // namespace dash
} // namespace unity