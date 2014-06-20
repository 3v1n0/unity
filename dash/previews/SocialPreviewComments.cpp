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
 * Authored by: Ken VanDine <ken.vandine@canonical.com>
 *
 */

#include "unity-shared/DashStyle.h"
#include "unity-shared/PreviewStyle.h"
#include <UnityCore/SocialPreview.h>
#include <NuxCore/Logger.h>
#include <Nux/Layout.h>
#include <Nux/VLayout.h>
#include <Nux/HLayout.h>

#include "SocialPreviewComments.h"

namespace unity
{
namespace dash
{
namespace previews
{
DECLARE_LOGGER(logger, "unity.dash.preview.social.comments");

namespace
{
const RawPixel LAYOUT_SPACING = 12_em;
const RawPixel CHILDREN_SPACE = 6_em;
}

NUX_IMPLEMENT_OBJECT_TYPE(SocialPreviewComments);

SocialPreviewComments::SocialPreviewComments(dash::Preview::Ptr preview_model, NUX_FILE_LINE_DECL)
: View(NUX_FILE_LINE_PARAM)
, scale(1.0)
, preview_model_(preview_model)
, layout_(nullptr)
, name_layout_(nullptr)
, comment_name_(nullptr)
, comment_time_(nullptr)
, comment_value_(nullptr)
{
  SetupViews();
  UpdateScale(scale);
  scale.changed.connect(sigc::mem_fun(this, &SocialPreviewComments::UpdateScale));
}

SocialPreviewComments::~SocialPreviewComments()
{
}

void SocialPreviewComments::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
}

void SocialPreviewComments::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

  if (GetCompositionLayout())
  {
    unsigned int alpha, src, dest = 0;
    gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
    gfx_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);

    gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);
  }

  gfx_engine.PopClippingRectangle();
}

void SocialPreviewComments::PreLayoutManagement()
{
  previews::Style& style = previews::Style::Instance();
  nux::Geometry const& geo = GetGeometry();

  int comment_width = 0;
  for (Comment const& comment : comments_)
  {
    int width = style.GetDetailsPanelMinimumWidth().CP(scale);
    if (comment.first)
    {
      width = comment.first->GetTextExtents().width;

      if (width < style.GetDetailsPanelMinimumWidth().CP(scale))
        width = style.GetDetailsPanelMinimumWidth().CP(scale);
    }

    if (comment_width < width)
    {
      comment_width = width;
    }
  }

  int comment_value_width = MAX(0, geo.width - style.GetDetailsLeftMargin().CP(scale) - style.GetDetailsRightMargin().CP(scale));

  for (Comment const& comment : comments_)
  {
    if (comment.first)
    {
      comment.first->SetMaximumWidth(comment_value_width);
    }
    if (comment.second)
    {
      comment.second->SetMaximumWidth(comment_value_width);
    }
  }
  View::PreLayoutManagement();
}

void SocialPreviewComments::SetupViews()
{
  dash::SocialPreview* social_preview_model = dynamic_cast<dash::SocialPreview*>(preview_model_.get());

  RemoveLayout();
  comments_.clear();

  previews::Style& style = previews::Style::Instance();

  auto on_mouse_down = [this](int x, int y, unsigned long button_flags, unsigned long key_flags) { this->preview_container_.OnMouseDown(x, y, button_flags, key_flags); };

  layout_ = new nux::VLayout();
  layout_->SetSpaceBetweenChildren(CHILDREN_SPACE.CP(scale));

  for (dash::SocialPreview::CommentPtr comment : social_preview_model->GetComments())
  {
    name_layout_ = new nux::HLayout();
    name_layout_->SetSpaceBetweenChildren(LAYOUT_SPACING.CP(scale));

    if (!comment->display_name.empty())
    {
      comment_name_ = new StaticCairoText(comment->display_name, true, NUX_TRACKER_LOCATION);
      comment_name_->SetFont(style.info_hint_bold_font());
      comment_name_->SetLines(-1);
      comment_name_->SetScale(scale);
      comment_name_->SetTextAlignment(StaticCairoText::NUX_ALIGN_LEFT);
      comment_name_->mouse_click.connect(on_mouse_down);
      name_layout_->AddView(comment_name_.GetPointer(), 0, nux::MINOR_POSITION_START);
    }

    if (!comment->time.empty())
    {
      comment_time_ = new StaticCairoText(comment->time, true, NUX_TRACKER_LOCATION);
      comment_time_->SetFont(style.info_hint_font());
      comment_time_->SetLines(-1);
      comment_time_->SetScale(scale);
      comment_time_->SetTextAlignment(StaticCairoText::NUX_ALIGN_RIGHT);
      comment_time_->mouse_click.connect(on_mouse_down);
      name_layout_->AddView(comment_time_.GetPointer(), 0, nux::MINOR_POSITION_START);
    }


    nux::HLayout* comment_layout = new nux::HLayout();
    comment_layout->SetSpaceBetweenChildren(LAYOUT_SPACING);

    comment_value_ = new StaticCairoText(comment->content, false, NUX_TRACKER_LOCATION);

    comment_value_->SetFont(style.info_hint_font());
    comment_value_->SetLines(-7);
    comment_value_->SetScale(scale);
    comment_value_->SetTextAlignment(StaticCairoText::NUX_ALIGN_LEFT);
    comment_value_->mouse_click.connect(on_mouse_down);
    comment_layout->AddView(comment_value_.GetPointer(), 1, nux::MINOR_POSITION_START);

    Comment comment_views(comment_name_, comment_value_);
    comments_.push_back(comment_views);

    layout_->AddLayout(name_layout_, 0);
    layout_->AddLayout(comment_layout, 1);
  }
  mouse_click.connect(on_mouse_down);

  SetLayout(layout_);
}

std::string SocialPreviewComments::GetName() const
{
  return "SocialPreviewComments";
}

void SocialPreviewComments::AddProperties(debug::IntrospectionData& introspection)
{
  introspection.add(GetAbsoluteGeometry());
}

void SocialPreviewComments::UpdateScale(double scale)
{
  if (comment_name_)
    comment_name_->SetScale(scale);

  if (comment_time_)
    comment_time_->SetScale(scale);

  if (comment_value_)
    comment_value_->SetScale(scale);

  if (layout_)
    layout_->SetSpaceBetweenChildren(CHILDREN_SPACE.CP(scale));

  if (name_layout_)
    name_layout_->SetSpaceBetweenChildren(LAYOUT_SPACING.CP(scale));
}

}
}
}
