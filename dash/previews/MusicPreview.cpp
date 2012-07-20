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
#include <Nux/Button.h>
#include <PreviewFactory.h>
 
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

namespace
{
nux::logging::Logger logger("unity.dash.previews.moviepreview");

}

NUX_IMPLEMENT_OBJECT_TYPE(MusicPreview);

MusicPreview::MusicPreview(dash::Preview::Ptr preview_model)
: Preview(preview_model)
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

  gfx_engine.PushClippingRectangle(base);
  nux::GetPainter().PaintBackground(gfx_engine, base);

  if (full_data_layout_)
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

  if (!IsFullRedraw())
    nux::GetPainter().PushLayer(gfx_engine, details_bg_layer_->GetGeometry(), details_bg_layer_.get());

  unsigned int alpha, src, dest = 0;
  gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
  gfx_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);

  gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);

  if (!IsFullRedraw())
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
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  details_bg_layer_.reset(new nux::ColorLayer(nux::Color(0.03f, 0.03f, 0.03f, 0.0f), true, rop));
}

void MusicPreview::SetupViews()
{
  dash::MusicPreview* music_preview_model = dynamic_cast<dash::MusicPreview*>(preview_model_.get());
  if (!music_preview_model)
    return;

  previews::Style& style = dash::previews::Style::Instance();

  int details_width = style.GetPreviewWidth() - style.GetPreviewHeight() - style.GetPanelSplitWidth();

  nux::HLayout* image_data_layout = new nux::HLayout();
  image_data_layout->SetSpaceBetweenChildren(style.GetPanelSplitWidth());

  CoverArt* album_cover = new CoverArt();
  album_cover->SetImage(preview_model_->image.Get().RawPtr() ? g_icon_to_string(preview_model_->image.Get().RawPtr()) : "");
  album_cover->SetFont(style.no_preview_image_font());
  album_cover->SetMinMaxSize(style.GetPreviewHeight(), style.GetPreviewHeight());

    /////////////////////
    // App Data Panel
    full_data_layout_ = new nux::VLayout();
    full_data_layout_->SetPadding(style.GetDetailsTopMargin(), 0, style.GetDetailsBottomMargin(), style.GetDetailsLeftMargin());
    full_data_layout_->SetSpaceBetweenChildren(16);

      /////////////////////
      // Music Info
      int top_app_info_max_width = details_width - style.GetDetailsLeftMargin() - style.GetDetailsRightMargin() - style.GetSpaceBetweenIconAndDetails();;

      nux::VLayout* album_data_layout = new nux::VLayout();
      album_data_layout->SetSpaceBetweenChildren(style.GetSpaceBetweenTitleAndSubtitle());

      title_ = new nux::StaticCairoText(preview_model_->title);
      title_->SetFont(style.title_font().c_str());
      title_->SetMaximumWidth(top_app_info_max_width);

      subtitle_ = new nux::StaticCairoText(preview_model_->subtitle);
      subtitle_->SetFont(style.subtitle_size_font().c_str());
      subtitle_->SetMaximumWidth(top_app_info_max_width);

      album_data_layout->AddView(title_, 1);
      album_data_layout->AddView(subtitle_, 1);

      /////////////////////


      /////////////////////
      // Music Tracks
      dash::Tracks::Ptr tracks_model = music_preview_model->GetTracksModel();
      if (tracks_model)
      {
        previews::Tracks* tracks = new previews::Tracks(tracks_model, NUX_TRACKER_LOCATION);
        tracks->play.connect(sigc::mem_fun(this, &MusicPreview::OnPlayTrack));
        tracks->pause.connect(sigc::mem_fun(this, &MusicPreview::OnPauseTrack));
        tracks_ = tracks;
      }
      /////////////////////

      nux::HLayout* hint_actions_layout = new nux::HLayout();

       /////////////////////
        // Hints
        nux::VLayout* hints_layout = new nux::VLayout();
        hints_layout->SetSpaceBetweenChildren(0);
        hints_layout->AddSpace(0, 1);
        PreviewInfoHintWidget* preview_info_hints = new PreviewInfoHintWidget(preview_model_, 24);
        hints_layout->AddView(preview_info_hints, 0);

        /////////////////////
        // Actions
        nux::VLayout* actions_container_layout = new nux::VLayout();
        actions_container_layout->SetLeftAndRightPadding(0, style.GetDetailsRightMargin());
        actions_container_layout->SetSpaceBetweenChildren(0);
        actions_container_layout->AddSpace(0, 1);
        
        nux::VLayout* actions_layout = new nux::VLayout();
        actions_layout->SetSpaceBetweenChildren(style.GetSpaceBetweenActions());
        actions_container_layout->AddLayout(actions_layout, 0);

        for (dash::Preview::ActionPtr action : preview_model_->GetActions())
        {
          ActionButton* button = new ActionButton(action->display_name, action->icon_hint, NUX_TRACKER_LOCATION);
          button->click.connect(sigc::bind(sigc::mem_fun(this, &MusicPreview::OnActionActivated), action->id));

          actions_layout->AddView(button, 0);
        }
        /////////////////////

      hint_actions_layout->AddView(hints_layout, 1);
      hint_actions_layout->AddView(actions_container_layout, 0);

    full_data_layout_->AddLayout(album_data_layout, 0);
    if (tracks_) { full_data_layout_->AddView(tracks_, 1); }
    full_data_layout_->AddLayout(hint_actions_layout, 0);
    /////////////////////
  
  image_data_layout->AddView(album_cover, 0);
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


}
}
}
