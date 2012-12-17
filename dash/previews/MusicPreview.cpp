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

#include "unity-shared/IntrospectableWrappers.h"
#include "unity-shared/PreviewStyle.h"
#include "unity-shared/CoverArt.h"
#include "unity-shared/IconTexture.h"
#include "unity-shared/StaticCairoText.h"
#include <UnityCore/MusicPreview.h>
#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <Nux/AbstractButton.h>
 
#include "MusicPreview.h"
#include "ActionButton.h"
#include "PreviewInfoHintWidget.h"
#include "Tracks.h"

namespace unity
{
namespace dash
{
namespace previews
{
DECLARE_LOGGER(logger, "unity.dash.preview.music");

NUX_IMPLEMENT_OBJECT_TYPE(MusicPreview);

MusicPreview::MusicPreview(dash::Preview::Ptr preview_model)
: Preview(preview_model)
, image_(nullptr)
, title_(nullptr)
, subtitle_(nullptr)
{
  SetupBackground();
  SetupViews();
}

MusicPreview::~MusicPreview()
{
}

void MusicPreview::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  bool enable_bg_shadows = dash::previews::Style::Instance().GetShadowBackgroundEnabled();

  gfx_engine.PushClippingRectangle(base);
  nux::GetPainter().PaintBackground(gfx_engine, base);

  if (enable_bg_shadows && full_data_layout_)
  {
    unsigned int alpha, src, dest = 0;
    gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
    gfx_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    details_bg_layer_->SetGeometry(full_data_layout_->GetGeometry());
    nux::GetPainter().RenderSinglePaintLayer(gfx_engine, full_data_layout_->GetGeometry(), details_bg_layer_.get());

    gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);
  }

  gfx_engine.PopClippingRectangle();
}

void MusicPreview::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

  bool enable_bg_shadows = dash::previews::Style::Instance().GetShadowBackgroundEnabled();

  if (enable_bg_shadows && !IsFullRedraw())
    nux::GetPainter().PushLayer(gfx_engine, details_bg_layer_->GetGeometry(), details_bg_layer_.get());

  unsigned int alpha, src, dest = 0;
  gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
  gfx_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);

  gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);

  if (enable_bg_shadows && !IsFullRedraw())
    nux::GetPainter().PopBackground();

  gfx_engine.PopClippingRectangle();
}

std::string MusicPreview::GetName() const
{
  return "MusicPreview";
}

void MusicPreview::AddProperties(GVariantBuilder* builder)
{
  Preview::AddProperties(builder);
}

void MusicPreview::SetupBackground()
{
  details_bg_layer_.reset(dash::previews::Style::Instance().GetBackgroundLayer());
}

void MusicPreview::SetupViews()
{
  dash::MusicPreview* music_preview_model = dynamic_cast<dash::MusicPreview*>(preview_model_.get());
  if (!music_preview_model)
  {
    LOG_ERROR(logger) << "Could not derive music preview model from given parameter.";
    return;
  }
  previews::Style& style = dash::previews::Style::Instance();

  nux::HLayout* image_data_layout = new nux::HLayout();
  image_data_layout->SetSpaceBetweenChildren(style.GetPanelSplitWidth());

  /////////////////////
  // Image
  image_ = new CoverArt();
  
  AddChild(image_.GetPointer());
  UpdateCoverArtImage(image_.GetPointer());
  /////////////////////

    /////////////////////
    // App Data Panel
    full_data_layout_ = new nux::VLayout();
    full_data_layout_->SetPadding(style.GetDetailsTopMargin(), 0, style.GetDetailsBottomMargin(), style.GetDetailsLeftMargin());
    full_data_layout_->SetSpaceBetweenChildren(16);

      /////////////////////
      // Music Info
      nux::VLayout* album_data_layout = new nux::VLayout();
      album_data_layout->SetSpaceBetweenChildren(style.GetSpaceBetweenTitleAndSubtitle());

      title_ = new StaticCairoText(preview_model_->title, true, NUX_TRACKER_LOCATION);
      title_->SetFont(style.title_font().c_str());
      title_->SetLines(-1);
      album_data_layout->AddView(title_.GetPointer(), 1);

      if (!preview_model_->subtitle.Get().empty())
      {
        subtitle_ = new StaticCairoText(preview_model_->subtitle, true, NUX_TRACKER_LOCATION);
        subtitle_->SetFont(style.subtitle_size_font().c_str());
        subtitle_->SetLines(-1);
        album_data_layout->AddView(subtitle_.GetPointer(), 1);
      }

      /////////////////////

      /////////////////////
      // Music Tracks
      dash::Tracks::Ptr tracks_model = music_preview_model->GetTracksModel();
      if (tracks_model)
      {
        tracks_ = new previews::Tracks(tracks_model, NUX_TRACKER_LOCATION);
        AddChild(tracks_.GetPointer());
        tracks_->play.connect(sigc::mem_fun(this, &MusicPreview::OnPlayTrack));
        tracks_->pause.connect(sigc::mem_fun(this, &MusicPreview::OnPauseTrack));
      }
      /////////////////////

      nux::HLayout* hint_actions_layout = new nux::HLayout();

      /////////////////////
      // Hints && Actions
      nux::VLayout* hints_layout = NULL;
      nux::Layout* actions_layout = NULL;
      if (!preview_model_->GetInfoHints().empty())
      {
        hints_layout = new nux::VLayout();
        hints_layout->SetSpaceBetweenChildren(0);
        hints_layout->AddSpace(0, 1);
        preview_info_hints_ = new PreviewInfoHintWidget(preview_model_, style.GetInfoHintIconSizeWidth());
        AddChild(preview_info_hints_.GetPointer());
        hints_layout->AddView(preview_info_hints_.GetPointer(), 0);

        // If there are actions, we use a vertical layout
        action_buttons_.clear();
        actions_layout = BuildVerticalActionsLayout(preview_model_->GetActions(), action_buttons_);
        actions_layout->SetLeftAndRightPadding(0, style.GetDetailsRightMargin());
      }
      else // otherwise we add a grid layout.
      {
        action_buttons_.clear();
        actions_layout = BuildGridActionsLayout(preview_model_->GetActions(), action_buttons_);
        if (action_buttons_.size() < 2)
          hint_actions_layout->AddSpace(0, 1);
        actions_layout->SetLeftAndRightPadding(0, style.GetDetailsRightMargin());
      }
        /////////////////////

      if (hints_layout) hint_actions_layout->AddView(hints_layout, 1);
      hint_actions_layout->AddView(actions_layout, 0);

    full_data_layout_->AddLayout(album_data_layout, 0);
    if (tracks_)
    {
      full_data_layout_->AddView(tracks_.GetPointer(), 1);
    }
    full_data_layout_->AddLayout(hint_actions_layout, 0);
    /////////////////////
  
  image_data_layout->AddView(image_.GetPointer(), 0);
  image_data_layout->AddLayout(full_data_layout_, 1);

  SetLayout(image_data_layout);
}

void MusicPreview::OnPlayTrack(std::string const& uri)
{ 
  dash::MusicPreview* music_preview_model = dynamic_cast<dash::MusicPreview*>(preview_model_.get());
  if (!music_preview_model)
  {
    LOG_ERROR(logger) << "Play failed. No preview found";
    return;
  }

  music_preview_model->PlayUri(uri);
}

void MusicPreview::OnPauseTrack(std::string const& uri)
{
  dash::MusicPreview* music_preview_model = dynamic_cast<dash::MusicPreview*>(preview_model_.get());
  if (!music_preview_model)
  {
    LOG_ERROR(logger) << "Pause failed. No preview found";
    return;
  }

  music_preview_model->PauseUri(uri);
}

void MusicPreview::PreLayoutManagement()
{
  nux::Geometry geo = GetGeometry();
  GetLayout()->SetGeometry(geo);

  previews::Style& style = dash::previews::Style::Instance();

  nux::Geometry geo_art(geo.x, geo.y, style.GetAppImageAspectRatio() * geo.height, geo.height);

  if (geo.width - geo_art.width - style.GetPanelSplitWidth() - style.GetDetailsLeftMargin() - style.GetDetailsRightMargin() < style.GetDetailsPanelMinimumWidth())
    geo_art.width = MAX(0, geo.width - style.GetPanelSplitWidth() - style.GetDetailsLeftMargin() - style.GetDetailsRightMargin() - style.GetDetailsPanelMinimumWidth());
  image_->SetMinMaxSize(geo_art.width, geo_art.height);

  int details_width = MAX(0, geo.width - geo_art.width - style.GetPanelSplitWidth() - style.GetDetailsLeftMargin() - style.GetDetailsRightMargin());

  if (title_) { title_->SetMaximumWidth(details_width); }
  if (subtitle_) { subtitle_->SetMaximumWidth(details_width); }

  for (nux::AbstractButton* button : action_buttons_)
  {
    button->SetMinMaxSize(CLAMP((details_width - style.GetSpaceBetweenActions()) / 2, 0, style.GetActionButtonMaximumWidth()), style.GetActionButtonHeight());
  }

  Preview::PreLayoutManagement();
}

}
}
}
