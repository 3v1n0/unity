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
#include "unity-shared/PlacesVScrollBar.h"
#include <UnityCore/ApplicationPreview.h>
#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <Nux/GridHLayout.h>
#include <Nux/Button.h>

#include "config.h"
#include <glib/gi18n-lib.h>
 
#include "ApplicationPreview.h"
#include "ActionButton.h"
#include "PreviewInfoHintWidget.h"
#include "PreviewRatingsWidget.h"

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
nux::logging::Logger logger("unity.dash.previews.applicationpreview");
}

class DetailsScrollView : public nux::ScrollView
{
public:
  DetailsScrollView(NUX_FILE_LINE_PROTO)
  : ScrollView(NUX_FILE_LINE_PARAM)
  {
    SetVScrollBar(new dash::PlacesVScrollBar(NUX_TRACKER_LOCATION));
  }

};

NUX_IMPLEMENT_OBJECT_TYPE(ApplicationPreview);

ApplicationPreview::ApplicationPreview(dash::Preview::Ptr preview_model)
: Preview(preview_model)
, full_data_layout_(nullptr)
{
  SetupBackground();
  SetupViews();
}

ApplicationPreview::~ApplicationPreview()
{
}

void ApplicationPreview::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
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

void ApplicationPreview::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
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

std::string ApplicationPreview::GetName() const
{
  return "ApplicationPreview";
}

void ApplicationPreview::AddProperties(GVariantBuilder* builder)
{
  Preview::AddProperties(builder);
}

void ApplicationPreview::SetupBackground()
{
  details_bg_layer_.reset(dash::previews::Style::Instance().GetBackgroundLayer());
}

void ApplicationPreview::SetupViews()
{
  dash::ApplicationPreview* app_preview_model = dynamic_cast<dash::ApplicationPreview*>(preview_model_.get());
  if (!app_preview_model)
  {
    LOG_ERROR(logger) << "Could not derive application preview model from given parameter.";
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
      // Main App Info
      nux::HLayout* main_app_info = new nux::HLayout();
      main_app_info->SetSpaceBetweenChildren(style.GetSpaceBetweenIconAndDetails());

        /////////////////////
        // Icon Layout
        nux::VLayout* icon_layout = new nux::VLayout();
        icon_layout->SetSpaceBetweenChildren(3);
        app_icon_ = new IconTexture(app_preview_model->app_icon.Get().RawPtr() ? g_icon_to_string(app_preview_model->app_icon.Get().RawPtr()) : "", 72);
        app_icon_->SetMinimumSize(style.GetAppIconAreaWidth(), style.GetAppIconAreaWidth());
        app_icon_->SetMaximumSize(style.GetAppIconAreaWidth(), style.GetAppIconAreaWidth());
        icon_layout->AddView(app_icon_.GetPointer(), 0);

        app_rating_ = new PreviewRatingsWidget();
        AddChild(app_rating_.GetPointer());
        app_rating_->SetMaximumHeight(style.GetRatingWidgetHeight());
        app_rating_->SetMinimumHeight(style.GetRatingWidgetHeight());
        app_rating_->SetRating(app_preview_model->rating);
        app_rating_->SetReviews(app_preview_model->num_ratings);
        icon_layout->AddView(app_rating_.GetPointer(), 0);

        /////////////////////

        /////////////////////
        // Data

        nux::VLayout* app_data_layout = new nux::VLayout();
        app_data_layout->SetSpaceBetweenChildren(16);

        title_subtitle_layout_ = new nux::VLayout();
        title_subtitle_layout_->SetSpaceBetweenChildren(style.GetSpaceBetweenTitleAndSubtitle());

        title_ = new nux::StaticCairoText(preview_model_->title, true, NUX_TRACKER_LOCATION);
        title_->SetLines(-1);
        title_->SetFont(style.title_font().c_str());
        title_subtitle_layout_->AddView(title_.GetPointer(), 1);

        if (!preview_model_->subtitle.Get().empty())
        {
          subtitle_ = new nux::StaticCairoText(preview_model_->subtitle, true, NUX_TRACKER_LOCATION);
          subtitle_->SetFont(style.subtitle_size_font().c_str());
          subtitle_->SetLines(-1);
          title_subtitle_layout_->AddView(subtitle_.GetPointer(), 1);
        }

        nux::VLayout* app_updated_copywrite_layout = new nux::VLayout();
        app_updated_copywrite_layout->SetSpaceBetweenChildren(8);

        if (!app_preview_model->license.Get().empty())
        {
          license_ = new nux::StaticCairoText(app_preview_model->license, true, NUX_TRACKER_LOCATION);
          license_->SetFont(style.app_license_font().c_str());
          license_->SetLines(-1);
          app_updated_copywrite_layout->AddView(license_.GetPointer(), 1);
        }

        if (!app_preview_model->last_update.Get().empty())
        {
          std::stringstream last_update;
          last_update << _("Last Updated") << " " << app_preview_model->last_update.Get();

          last_update_ = new nux::StaticCairoText(last_update.str(), true, NUX_TRACKER_LOCATION);
          last_update_->SetFont(style.app_last_update_font().c_str());
          app_updated_copywrite_layout->AddView(last_update_.GetPointer(), 1);
        }

        if (!app_preview_model->copyright.Get().empty())
        {
          copywrite_ = new nux::StaticCairoText(app_preview_model->copyright, true, NUX_TRACKER_LOCATION);
          copywrite_->SetFont(style.app_copywrite_font().c_str());
          copywrite_->SetLines(-1);
          app_updated_copywrite_layout->AddView(copywrite_.GetPointer(), 1);
        }

        app_data_layout->AddLayout(title_subtitle_layout_);
        app_data_layout->AddLayout(app_updated_copywrite_layout);

        // buffer space
        /////////////////////

      main_app_info->AddLayout(icon_layout, 0);
      main_app_info->AddLayout(app_data_layout, 1);
      /////////////////////

      /////////////////////
      // Description
      nux::ScrollView* app_info = new DetailsScrollView(NUX_TRACKER_LOCATION);
      app_info->EnableHorizontalScrollBar(false);

      nux::VLayout* app_info_layout = new nux::VLayout();
      app_info_layout->SetSpaceBetweenChildren(12);
      app_info->SetLayout(app_info_layout);

      if (!preview_model_->description.Get().empty())
      {
        description_ = new nux::StaticCairoText(preview_model_->description, false, NUX_TRACKER_LOCATION); // not escaped!
        description_->SetFont(style.description_font().c_str());
        description_->SetTextAlignment(nux::StaticCairoText::NUX_ALIGN_TOP);
        description_->SetLines(-style.GetDescriptionLineCount());
        description_->SetLineSpacing(style.GetDescriptionLineSpacing());
        app_info_layout->AddView(description_.GetPointer());
      }

      if (!preview_model_->GetInfoHints().empty())
      {
        preview_info_hints_ = new PreviewInfoHintWidget(preview_model_, style.GetInfoHintIconSizeWidth());
        AddChild(preview_info_hints_.GetPointer());
        app_info_layout->AddView(preview_info_hints_.GetPointer());
      }
      /////////////////////

      /////////////////////
      // Actions
      action_buttons_.clear();
      nux::Layout* actions_layout = BuildGridActionsLayout(preview_model_->GetActions(), action_buttons_);
      actions_layout->SetLeftAndRightPadding(0, style.GetDetailsRightMargin());
      ///////////////////

    full_data_layout_->AddLayout(main_app_info, 0);
    full_data_layout_->AddView(app_info, 1);
    full_data_layout_->AddLayout(actions_layout, 0);
    /////////////////////
  
  image_data_layout->AddView(image_.GetPointer(), 0);
  image_data_layout->AddLayout(full_data_layout_, 1);


  SetLayout(image_data_layout);
}

void ApplicationPreview::PreLayoutManagement()
{
  nux::Geometry geo = GetGeometry();

  previews::Style& style = dash::previews::Style::Instance();

  nux::Geometry geo_art(geo.x, geo.y, style.GetAppImageAspectRatio() * geo.height, geo.height);

  if (geo.width - geo_art.width - style.GetPanelSplitWidth() - style.GetDetailsLeftMargin() - style.GetDetailsRightMargin() < style.GetDetailsPanelMinimumWidth())
    geo_art.width = MAX(0, geo.width - style.GetPanelSplitWidth() - style.GetDetailsLeftMargin() - style.GetDetailsRightMargin() - style.GetDetailsPanelMinimumWidth());
  image_->SetMinMaxSize(geo_art.width, geo_art.height);

  int details_width = MAX(0, geo.width - geo_art.width - style.GetPanelSplitWidth() - style.GetDetailsLeftMargin() - style.GetDetailsRightMargin());
  int top_app_info_max_width = MAX(0, details_width - style.GetAppIconAreaWidth() - style.GetSpaceBetweenIconAndDetails());

  if (title_) { title_->SetMaximumWidth(top_app_info_max_width); }
  if (subtitle_) { subtitle_->SetMaximumWidth(top_app_info_max_width); }
  if (license_) { license_->SetMaximumWidth(top_app_info_max_width); }
  if (last_update_) { last_update_->SetMaximumWidth(top_app_info_max_width); }
  if (copywrite_) { copywrite_->SetMaximumWidth(top_app_info_max_width); }
  if (description_) { description_->SetMaximumWidth(details_width); }

  for (nux::AbstractButton* button : action_buttons_)
  {
    button->SetMinMaxSize(CLAMP((details_width - style.GetSpaceBetweenActions()) / 2, 0, style.GetActionButtonMaximumWidth()), style.GetActionButtonHeight());
  }

  Preview::PreLayoutManagement();
}

} // namespace previews
} // namespace dash
} // namepsace unity
