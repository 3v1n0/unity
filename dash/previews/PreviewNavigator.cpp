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
DECLARE_LOGGER(logger, "unity.dash.preview.navigator");

NUX_IMPLEMENT_OBJECT_TYPE(PreviewNavigator);

PreviewNavigator::PreviewNavigator(Orientation direction, NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , direction_(direction)
  , texture_(nullptr)
  , visual_state_(VisualState::NORMAL)
{
  SetupViews();
  UpdateTexture();
}

void PreviewNavigator::SetEnabled(bool enabled)
{
  if (enabled != texture_->IsVisible())
  {
    texture_->SetVisible(enabled);
    QueueRelayout();
  }
}

std::string PreviewNavigator::GetName() const
{
  return "PreviewNavigator";
}

void PreviewNavigator::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
    .add("button-x", texture_->GetAbsoluteX())
    .add("button-y", texture_->GetAbsoluteY())
    .add("button-width", texture_->GetGeometry().width)
    .add("button-height", texture_->GetGeometry().height)
    .add("button-geo", texture_->GetAbsoluteGeometry())
    .add("direction", static_cast<int>(direction_));
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
  previews::Style& style = dash::previews::Style::Instance();
  
  if (direction_ == Orientation::LEFT || direction_ == Orientation::RIGHT)
  {
    nux::VLayout* vlayout = new nux::VLayout();
    nux::HLayout* hlayout = new nux::HLayout();
    vlayout->SetSpaceBetweenChildren(0);
    hlayout->SetSpaceBetweenChildren(0);
    layout_ = hlayout;

    if (direction_ == Orientation::LEFT)
      texture_ = new IconTexture(Style::Instance().GetNavLeftIcon(), style.GetNavigatorIconSize(), style.GetNavigatorIconSize());
    else 
      texture_ = new IconTexture(Style::Instance().GetNavRightIcon(), style.GetNavigatorIconSize(), style.GetNavigatorIconSize());
    texture_->SetDrawMode(IconTexture::DrawMode::STRETCH_WITH_ASPECT);

    vlayout->AddSpace(0,1);
    vlayout->AddLayout(hlayout);
    vlayout->AddSpace(0,1);
    SetLayout(vlayout);
  }
  else if (direction_ == Orientation::UP || direction_ == Orientation::DOWN)
  {
  // No support for this Yet
    g_assert(false);
    nux::HLayout* hlayout = new nux::HLayout();
    nux::VLayout* vlayout = new nux::VLayout();
    hlayout->SetSpaceBetweenChildren(0);
    vlayout->SetSpaceBetweenChildren(0);
    layout_ = vlayout;

    hlayout->AddSpace(0,1);
    hlayout->AddLayout(vlayout);
    hlayout->AddSpace(0,1);
    SetLayout(hlayout);
  }

  layout_->AddSpace(0, 1);

  if (texture_)
  {
    AddChild(texture_);
    layout_->AddView(texture_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);    

    texture_->mouse_click.connect([&](int, int, unsigned long, unsigned long) { activated.emit(); });
    texture_->mouse_enter.connect(sigc::mem_fun(this, &PreviewNavigator::TexRecvMouseEnter));
    texture_->mouse_leave.connect(sigc::mem_fun(this, &PreviewNavigator::TexRecvMouseLeave));
  }

  layout_->AddSpace(0, 1);
}

void PreviewNavigator::TexRecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  visual_state_ = VisualState::ACTIVE;
  UpdateTexture();
  QueueDraw();
}

void PreviewNavigator::TexRecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  visual_state_ = VisualState::NORMAL;
  UpdateTexture();
  QueueDraw();
}

void PreviewNavigator::UpdateTexture()
{
  if (!texture_)
    return;

  switch (visual_state_)
  {
    case VisualState::ACTIVE:
      texture_->SetOpacity(1.0);
      break;
    case VisualState::NORMAL:
    default:
      texture_->SetOpacity(0.2);
      break;
  }
}


} // namespace previews
} // namespace dash
} // namespace unity
