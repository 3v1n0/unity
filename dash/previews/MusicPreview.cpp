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
#include <UnityCore/MusicPreview.h>
#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <Nux/AbstractButton.h>

#include "MusicPreview.h"
#include "ActionButton.h"
#include "Tracks.h"
#include "PreviewInfoHintWidget.h"
#include "PreviewPlayer.h"

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
  const RawPixel CHILDREN_SPACE = 16_em;
  const RawPixel ICON_LEFT_RIGHT_PADDING = 10_em;
  const RawPixel WARNING_MIN_HEIGHT = 50_em;
  const RawPixel WARNING_MAX_WIDTH = 300_em;
}

DECLARE_LOGGER(logger, "unity.dash.preview.music");

NUX_IMPLEMENT_OBJECT_TYPE(MusicPreview);

MusicPreview::MusicPreview(dash::Preview::Ptr preview_model)
: Preview(preview_model)
, actions_layout_(nullptr)
, image_data_layout_(nullptr)
, icon_layout_(nullptr)
{
  SetupViews();
  UpdateScale(scale);
}

MusicPreview::~MusicPreview()
{
}

void MusicPreview::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  gfx_engine.PushClippingRectangle(base);
  nux::GetPainter().PaintBackground(gfx_engine, base);

  gfx_engine.PopClippingRectangle();
}

void MusicPreview::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

  unsigned int alpha, src, dest = 0;
  gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
  gfx_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);

  gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);

  gfx_engine.PopClippingRectangle();
}

std::string MusicPreview::GetName() const
{
  return "MusicPreview";
}

void MusicPreview::AddProperties(debug::IntrospectionData& introspection)
{
  Preview::AddProperties(introspection);
}

bool MusicPreview::HasUbuntuOneCredentials()
{
  dash::Preview::InfoHintPtrList hints = preview_model_->GetInfoHints();
  GVariant *preview_data = NULL;
  for (dash::Preview::InfoHintPtr const& info_hint : hints)
  {
    if (info_hint->id == "music_preview")
    {
      preview_data = info_hint->value;
      if (preview_data != NULL)
      {
        glib::Variant data(g_variant_lookup_value(preview_data,
	  "no_credentials_label", G_VARIANT_TYPE_STRING));

        if (!data)
          no_credentials_message_ = "";
        else
          no_credentials_message_ = data.GetString();
      }
      break;
    }
  }
  return no_credentials_message_.empty(); 
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

  auto on_mouse_down = [this](int x, int y, unsigned long button_flags, unsigned long key_flags) { this->preview_container_->OnMouseDown(x, y, button_flags, key_flags); };

  image_data_layout_ = new nux::HLayout();
  image_data_layout_->SetSpaceBetweenChildren(style.GetPanelSplitWidth().CP(scale));

  /////////////////////
  // Image
  image_ = new CoverArt();
  image_->scale = scale();
  AddChild(image_.GetPointer());
  UpdateCoverArtImage(image_.GetPointer());
  /////////////////////

    /////////////////////
    // App Data Panel
    full_data_layout_ = new nux::VLayout();
    full_data_layout_->SetPadding(style.GetDetailsTopMargin().CP(scale), 0, style.GetDetailsBottomMargin().CP(scale), style.GetDetailsLeftMargin().CP(scale));
    full_data_layout_->SetSpaceBetweenChildren(CHILDREN_SPACE.CP(scale));

      /////////////////////
      // Music Info
      album_data_layout_ = new nux::VLayout();
      album_data_layout_->SetSpaceBetweenChildren(style.GetSpaceBetweenTitleAndSubtitle().CP(scale));

      title_ = new StaticCairoText(preview_model_->title, true, NUX_TRACKER_LOCATION);
      AddChild(title_.GetPointer());
      title_->SetFont(style.title_font().c_str());
      title_->SetLines(-1);
      title_->SetScale(scale);
      title_->mouse_click.connect(on_mouse_down);
      album_data_layout_->AddView(title_.GetPointer(), 1);

      if (!preview_model_->subtitle.Get().empty())
      {
        subtitle_ = new StaticCairoText(preview_model_->subtitle, true, NUX_TRACKER_LOCATION);
        AddChild(subtitle_.GetPointer());
        subtitle_->SetFont(style.subtitle_size_font().c_str());
        subtitle_->SetLines(-1);
        subtitle_->SetScale(scale);
        subtitle_->mouse_click.connect(on_mouse_down);
        album_data_layout_->AddView(subtitle_.GetPointer(), 1);
      }

      /////////////////////

      /////////////////////
      // Music Tracks
      dash::Tracks::Ptr tracks_model = music_preview_model->GetTracksModel();
      if (tracks_model)
      {
        tracks_ = new previews::Tracks(tracks_model, NUX_TRACKER_LOCATION);
        tracks_->scale = scale();
        AddChild(tracks_.GetPointer());
        tracks_->mouse_click.connect(on_mouse_down);
      }
      /////////////////////

      nux::HLayout* hint_actions_layout = new nux::HLayout();

      /////////////////////
      // Hints && Actions
      nux::VLayout* hints_layout = NULL;
      actions_layout_ = NULL;
      bool has_u1_creds = HasUbuntuOneCredentials();

      if (has_u1_creds)
      {
        if (!preview_model_->GetInfoHints().empty())
        {
          hints_layout = new nux::VLayout();
          hints_layout->SetSpaceBetweenChildren(0);
          hints_layout->AddSpace(0, 1);
          preview_info_hints_ = new PreviewInfoHintWidget(preview_model_, style.GetInfoHintIconSizeWidth().CP(scale));
          preview_info_hints_->scale = scale();
          AddChild(preview_info_hints_.GetPointer());
          preview_info_hints_->request_close().connect([this]() { preview_container_->request_close.emit(); });
          hints_layout->AddView(preview_info_hints_.GetPointer(), 0);

          // If there are actions, we use a vertical layout
          action_buttons_.clear();
          actions_layout_ = BuildVerticalActionsLayout(preview_model_->GetActions(), action_buttons_);
          actions_layout_->SetLeftAndRightPadding(0, style.GetDetailsRightMargin().CP(scale));
        }
        else // otherwise we add a grid layout.
        {
          action_buttons_.clear();
          actions_layout_ = BuildGridActionsLayout(preview_model_->GetActions(), action_buttons_);
          if (action_buttons_.size() < 2)
            hint_actions_layout->AddSpace(0, 1);
          actions_layout_->SetLeftAndRightPadding(0, style.GetDetailsRightMargin().CP(scale));
        }
      }
      else
      {
        // let the user know he needs to connect
        previews::Style& style = dash::previews::Style::Instance();
        nux::HLayout* actions_layout = new nux::HLayout();
        icon_layout_ = new nux::VLayout();
        icon_layout_->SetLeftAndRightPadding(ICON_LEFT_RIGHT_PADDING.CP(scale));

        warning_texture_ = new IconTexture(style.GetWarningIcon());
        icon_layout_->AddView(warning_texture_.GetPointer(), 0, nux::MINOR_POSITION_START,
          nux::MINOR_SIZE_FULL, 100.0f, nux::NUX_LAYOUT_BEGIN);
        actions_layout->AddLayout(icon_layout_, 0, nux::MINOR_POSITION_CENTER);

        warning_msg_ = new StaticCairoText(
                     no_credentials_message_, true,
                     NUX_TRACKER_LOCATION);
        AddChild(warning_msg_.GetPointer());
        warning_msg_->SetFont(style.u1_warning_font().c_str());
        warning_msg_->SetLines(-2);
        warning_msg_->SetScale(scale);
        warning_msg_->SetMinimumHeight(WARNING_MIN_HEIGHT.CP(scale));
        warning_msg_->SetMaximumWidth(WARNING_MAX_WIDTH.CP(scale));

        actions_layout->AddView(warning_msg_.GetPointer(), 0, nux::MINOR_POSITION_CENTER);

      }

        /////////////////////

      if (hints_layout) hint_actions_layout->AddView(hints_layout, 1);
      hint_actions_layout->AddView(actions_layout_, 0);

    full_data_layout_->AddLayout(album_data_layout_, 0);
    if (tracks_)
    {
      full_data_layout_->AddView(tracks_.GetPointer(), 1);
    }
    full_data_layout_->AddLayout(hint_actions_layout, 0);
    /////////////////////

  image_data_layout_->AddView(image_.GetPointer(), 0);
  image_data_layout_->AddLayout(full_data_layout_, 1);

  mouse_click.connect(on_mouse_down);

  SetLayout(image_data_layout_);
}

void MusicPreview::PreLayoutManagement()
{
  nux::Geometry geo = GetGeometry();
  GetLayout()->SetGeometry(geo);

  previews::Style& style = dash::previews::Style::Instance();

  nux::Geometry geo_art(geo.x, geo.y, style.GetAppImageAspectRatio() * geo.height, geo.height);

  int content_width = geo.width - style.GetPanelSplitWidth().CP(scale) - style.GetDetailsLeftMargin().CP(scale) - style.GetDetailsRightMargin().CP(scale);
  if (content_width - geo_art.width < style.GetDetailsPanelMinimumWidth().CP(scale))
    geo_art.width = MAX(0, content_width - style.GetDetailsPanelMinimumWidth().CP(scale));
  image_->SetMinMaxSize(geo_art.width, geo_art.height);

  int details_width = MAX(0, content_width - geo_art.width);

  if (title_) { title_->SetMaximumWidth(details_width); }
  if (subtitle_) { subtitle_->SetMaximumWidth(details_width); }

  for (nux::AbstractButton* button : action_buttons_)
  {
    int action_width = CLAMP((details_width - style.GetSpaceBetweenActions().CP(scale)) / 2, 0, style.GetActionButtonMaximumWidth().CP(scale));
    // do not use SetMinMax because width has to be able to grow
    button->SetMinimumWidth(action_width);
    button->SetMinimumHeight(style.GetActionButtonHeight().CP(scale));
    button->SetMaximumHeight(style.GetActionButtonHeight().CP(scale));
  }

  Preview::PreLayoutManagement();
}

void MusicPreview::OnNavigateOut()
{
  PreviewPlayer player;
  player.Stop();
}

void MusicPreview::UpdateScale(double scale)
{
  Preview::UpdateScale(scale);

  if (tracks_)
    tracks_->scale = scale;

  if (preview_info_hints_)
    preview_info_hints_->scale = scale;

  if (icon_layout_)
    icon_layout_->SetLeftAndRightPadding(ICON_LEFT_RIGHT_PADDING.CP(scale));

  if (warning_msg_)
  {
    warning_msg_->SetScale(scale);
    warning_msg_->SetMinimumHeight(WARNING_MIN_HEIGHT.CP(scale));
    warning_msg_->SetMaximumWidth(WARNING_MAX_WIDTH.CP(scale));
  }

  previews::Style& style = dash::previews::Style::Instance();

  if (image_data_layout_)
    image_data_layout_->SetSpaceBetweenChildren(style.GetPanelSplitWidth().CP(scale));

  if (full_data_layout_)
  {
    full_data_layout_->SetPadding(style.GetDetailsTopMargin().CP(scale), 0, style.GetDetailsBottomMargin().CP(scale), style.GetDetailsLeftMargin().CP(scale));
    full_data_layout_->SetSpaceBetweenChildren(CHILDREN_SPACE.CP(scale));
  }

  if (album_data_layout_)
    album_data_layout_->SetSpaceBetweenChildren(style.GetSpaceBetweenTitleAndSubtitle().CP(scale));

  if (actions_layout_)
    actions_layout_->SetLeftAndRightPadding(0, style.GetDetailsRightMargin().CP(scale));

  if (icon_layout_)
    icon_layout_->SetLeftAndRightPadding(ICON_LEFT_RIGHT_PADDING.CP(scale));
}

} // namespace previews
} // namespace dash
} // namespace unity
