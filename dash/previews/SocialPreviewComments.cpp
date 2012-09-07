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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#include "unity-shared/DashStyle.h"
#include "unity-shared/PreviewStyle.h"
#include "unity-shared/PlacesVScrollBar.h"
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

namespace
{
nux::logging::Logger logger("unity.dash.previews.socialpreviewcomments");

const int layout_spacing = 12;
}

NUX_IMPLEMENT_OBJECT_TYPE(SocialPreviewComments);

SocialPreviewComments::SocialPreviewComments(dash::Preview::Ptr preview_model, NUX_FILE_LINE_DECL)
: ScrollView(NUX_FILE_LINE_PARAM)
, preview_model_(preview_model)
{
  SetupViews();
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
    int width = style.GetCommentNameMinimumWidth();
    if (comment.first)
    {
      width = comment.first->GetTextExtents().width;

      if (width < style.GetCommentNameMinimumWidth())
        width = style.GetCommentNameMinimumWidth();
      else if (width > style.GetCommentNameMaximumWidth())
        width = style.GetCommentNameMaximumWidth();
    }

    if (comment_width < width)
    {
      comment_width = width;
    }
  }

  int comment_value_width = geo.width;
  comment_value_width -= layout_spacing;
  comment_value_width -= comment_width;
  comment_value_width = MAX(0, comment_value_width);

  for (Comment const& comment : comments_)
  {
    if (comment.first)
    {
      comment.first->SetMinimumWidth(comment_width);
      comment.first->SetMaximumWidth(comment_width);
    }
    if (comment.second)
    {
      comment.second->SetMinimumWidth(comment_value_width);
      comment.second->SetMaximumWidth(comment_value_width);
    }
  }
  ScrollView::PreLayoutManagement();
}

void SocialPreviewComments::SetupViews()
{
  dash::SocialPreview* social_preview_model = dynamic_cast<dash::SocialPreview*>(preview_model_.get());


  RemoveLayout();
  comments_.clear();

  SetVScrollBar(new dash::PlacesVScrollBar(NUX_TRACKER_LOCATION));

  previews::Style& style = previews::Style::Instance();

  nux::VLayout* layout = new nux::VLayout();
  layout->SetSpaceBetweenChildren(6);

  printf ("HERE\n");
  for (dash::SocialPreview::CommentPtr comment : social_preview_model->GetComments())
  {
    printf ("COMMENT: %s\n", comment->content.c_str());

    nux::HLayout* hint_layout = new nux::HLayout();
    hint_layout->SetSpaceBetweenChildren(layout_spacing);

    StaticCairoTextPtr comment_name;
    if (!comment->display_name.empty())
    {
      std::string tmp_display_name = comment->display_name;
      tmp_display_name += ":";

      comment_name = new nux::StaticCairoText(tmp_display_name, true, NUX_TRACKER_LOCATION);
      comment_name->SetFont(style.info_hint_bold_font());
      comment_name->SetLines(-1);
      comment_name->SetTextAlignment(nux::StaticCairoText::NUX_ALIGN_RIGHT);
      hint_layout->AddView(comment_name.GetPointer(), 0, nux::MINOR_POSITION_TOP);
    }

    StaticCairoTextPtr comment_value(new nux::StaticCairoText(comment->content, true, NUX_TRACKER_LOCATION));
    comment_value->SetFont(style.info_hint_font());
    comment_value->SetLines(-7);
    hint_layout->AddView(comment_value.GetPointer(), 1, nux::MINOR_POSITION_TOP);

    Comment comment_views(comment_name, comment_value);
    comments_.push_back(comment_views);

    layout->AddLayout(hint_layout, 1);
  }

  SetLayout(layout);

}

std::string SocialPreviewComments::GetName() const
{
  return "SocialPreviewComments";
}

void SocialPreviewComments::AddProperties(GVariantBuilder* builder)
{
}

}
}
}
