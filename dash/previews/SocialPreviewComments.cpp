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
#include <NuxCore/Logger.h>
#include <Nux/Layout.h>

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
}

NUX_IMPLEMENT_OBJECT_TYPE(SocialPreviewComments);

SocialPreviewComments::SocialPreviewComments(std::string const& text, NUX_FILE_LINE_DECL)
: View(NUX_FILE_LINE_PARAM)
{
  SetupViews();
  text_->SetText(text);
}

SocialPreviewComments::~SocialPreviewComments()
{
}

void SocialPreviewComments::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
}

void SocialPreviewComments::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
}

void SocialPreviewComments::PreLayoutManagement()
{
}

void SocialPreviewComments::SetupViews()
{
  dash::previews::Style const& style = dash::previews::Style::Instance();

  text_ = new nux::StaticCairoText("", false, NUX_TRACKER_LOCATION);
  text_->SetLines(-7);
  text_->SetFont(style.content_font());
  text_->SetLineSpacing(5);
  text_->SetTextEllipsize(nux::StaticCairoText::NUX_ELLIPSIZE_MIDDLE);

  nux::Layout* layout = new nux::Layout();
  layout->AddView(text_.GetPointer(), 1);
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
