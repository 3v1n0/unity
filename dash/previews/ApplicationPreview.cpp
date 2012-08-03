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

void ApplicationPreview::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
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
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  details_bg_layer_.reset(new nux::ColorLayer(nux::Color(0.03f, 0.03f, 0.03f, 0.0f), true, rop));
}

void ApplicationPreview::SetupViews()
{
  dash::ApplicationPreview* app_preview_model = dynamic_cast<dash::ApplicationPreview*>(preview_model_.get());
  if (!app_preview_model)
    return;

  previews::Style& style = dash::previews::Style::Instance();

  nux::HLayout* image_data_layout = new nux::HLayout();
  image_data_layout->SetSpaceBetweenChildren(style.GetPanelSplitWidth());

  image_ = new CoverArt();
  std::string image_hint = preview_model_->image.Get().RawPtr() ? g_icon_to_string(preview_model_->image.Get().RawPtr()) : "";
  if (image_hint != "")
    image_->SetImage(image_hint);
  else
    image_->GenerateImage(preview_model_->image_source_uri);
  image_->SetFont(style.no_preview_image_font());

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
        icon_layout->AddView(app_icon_, 0);

        app_rating_ = new PreviewRatingsWidget();
        app_rating_->SetMinimumHeight(36);
        app_rating_->SetRating(app_preview_model->rating);
        app_rating_->SetReviews(app_preview_model->num_ratings);
        icon_layout->AddView(app_rating_, 0);

        /////////////////////

        /////////////////////
        // Data

        nux::VLayout* app_data_layout = new nux::VLayout();
        app_data_layout->SetSpaceBetweenChildren(16);

        title_subtitle_layout_ = new nux::VLayout();
        title_subtitle_layout_->SetSpaceBetweenChildren(style.GetSpaceBetweenTitleAndSubtitle());

        title_ = new nux::StaticCairoText(app_preview_model->title);
        title_->SetLines(-1);
        title_->SetFont(style.title_font().c_str());

        subtitle_ = new nux::StaticCairoText(app_preview_model->subtitle);
        subtitle_->SetFont(style.subtitle_size_font().c_str());
        subtitle_->SetLines(-1);

        nux::VLayout* app_updated_copywrite_layout = new nux::VLayout();
        app_updated_copywrite_layout->SetSpaceBetweenChildren(8);

        license_ = new nux::StaticCairoText(app_preview_model->license);
        license_->SetFont(style.app_license_font().c_str());
        license_->SetLines(-1);

        last_update_ = new nux::StaticCairoText(_("Last Updated ") + app_preview_model->last_update.Get());
        last_update_->SetFont(style.app_last_update_font().c_str());

        copywrite_ = new nux::StaticCairoText(app_preview_model->copyright);
        copywrite_->SetFont(style.app_copywrite_font().c_str());
        copywrite_->SetLines(-1);

        title_subtitle_layout_->AddView(title_, 1);
        title_subtitle_layout_->AddView(subtitle_, 1);

        app_updated_copywrite_layout->AddView(license_, 1);
        app_updated_copywrite_layout->AddView(last_update_, 1);
        app_updated_copywrite_layout->AddView(copywrite_, 1);

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

      description_ = new nux::StaticCairoText("");
      description_->SetFont(style.description_font().c_str());
      description_->SetTextAlignment(nux::StaticCairoText::NUX_ALIGN_TOP);
      description_->SetLines(-20);
      description_->SetLineSpacing(2);
      description_->SetText(app_preview_model->description);
      app_info_layout->AddView(description_);
      if (preview_model_->GetInfoHints().size() > 0)
      {
        PreviewInfoHintWidget* preview_info_hints = new PreviewInfoHintWidget(preview_model_, 24);
        app_info_layout->AddView(preview_info_hints);
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
  
  image_data_layout->AddView(image_, 0);
  image_data_layout->AddLayout(full_data_layout_, 1);


  SetLayout(image_data_layout);
}

long ApplicationPreview::ComputeContentSize()
{
  nux::Geometry geo = GetGeometry();
  GetLayout()->SetGeometry(geo);

  previews::Style& style = dash::previews::Style::Instance();

  nux::Geometry geo_art(geo.x, geo.y, style.GetAppImageAspectRatio() * geo.height, geo.height);

  if (geo.width - geo_art.width - style.GetPanelSplitWidth() - style.GetDetailsLeftMargin() - style.GetDetailsRightMargin() < style.GetDetailsPanelMinimumWidth())
    geo_art.width = MAX(0, geo.width - style.GetPanelSplitWidth() - style.GetDetailsLeftMargin() - style.GetDetailsRightMargin() - style.GetDetailsPanelMinimumWidth());
  image_->SetGeometry(geo_art);

  full_data_layout_->SetGeometry(nux::Geometry(geo_art.x + geo_art.width + style.GetPanelSplitWidth(), geo.y,
                    geo.width - geo_art.width - style.GetPanelSplitWidth(), geo.height));

  int details_width = MAX(0, geo.width - geo_art.width - style.GetPanelSplitWidth() - style.GetDetailsLeftMargin() - style.GetDetailsRightMargin());
  int top_app_info_max_width = details_width - style.GetAppIconAreaWidth() - style.GetSpaceBetweenIconAndDetails();

  title_->SetMaximumWidth(top_app_info_max_width);
  subtitle_->SetMaximumWidth(top_app_info_max_width);
  license_->SetMaximumWidth(top_app_info_max_width);
  last_update_->SetMaximumWidth(top_app_info_max_width);
  copywrite_->SetMaximumWidth(top_app_info_max_width);
  description_->SetMaximumWidth(details_width);

  for (nux::AbstractButton* button : action_buttons_)
  {
    button->SetMinMaxSize(MIN((details_width - style.GetSpaceBetweenActions()) / 2, style.GetActionButtonMaximumWidth()), style.GetActionButtonHeight());
  }

  image_->ComputeContentSize();
  full_data_layout_->ComputeContentSize();

  return nux::eCompliantHeight | nux::eCompliantWidth;
}


} // namespace previews
} // namespace dash
} // namepsace unity
