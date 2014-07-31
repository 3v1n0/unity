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
  const RawPixel ICON_SPACE_CHILDREN = 3_em;
  const RawPixel DATA_SPACE_CHILDREN = 16_em;
  const RawPixel INFO_SPACE_CHILDREN = 12_em;
  const RawPixel COPYRIGHT_SPACE_CHILDREN = 8_em;
  const RawPixel ICON_SIZE = 72_em;
}

DECLARE_LOGGER(logger, "unity.dash.preview.application");

NUX_IMPLEMENT_OBJECT_TYPE(ApplicationPreview);

ApplicationPreview::ApplicationPreview(dash::Preview::Ptr preview_model)
: Preview(preview_model)
, title_subtitle_layout_(nullptr)
, image_data_layout_(nullptr)
, main_app_info_(nullptr)
, icon_layout_(nullptr)
, app_data_layout_(nullptr)
, app_updated_copywrite_layout_(nullptr)
, app_info_layout_(nullptr)
, app_info_scroll_(nullptr)
, actions_layout_(nullptr)
{
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

  gfx_engine.PopClippingRectangle();
}

void ApplicationPreview::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
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

std::string ApplicationPreview::GetName() const
{
  return "ApplicationPreview";
}

void ApplicationPreview::AddProperties(debug::IntrospectionData& introspection)
{
  Preview::AddProperties(introspection);
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
    full_data_layout_->SetSpaceBetweenChildren(DATA_SPACE_CHILDREN.CP(scale));

      /////////////////////
      // Main App Info
      main_app_info_ = new nux::HLayout();
      main_app_info_->SetSpaceBetweenChildren(style.GetSpaceBetweenIconAndDetails().CP(scale));

        /////////////////////
        // Icon Layout
        icon_layout_ = new nux::VLayout();
        icon_layout_->SetSpaceBetweenChildren(ICON_SPACE_CHILDREN.CP(scale));
        app_icon_ = new IconTexture(app_preview_model->app_icon.Get().RawPtr() ? g_icon_to_string(app_preview_model->app_icon.Get().RawPtr()) : "", ICON_SIZE.CP(scale));
        AddChild(app_icon_.GetPointer());
        app_icon_->SetMinimumSize(style.GetAppIconAreaWidth().CP(scale), style.GetAppIconAreaWidth().CP(scale));
        app_icon_->SetMaximumSize(style.GetAppIconAreaWidth().CP(scale), style.GetAppIconAreaWidth().CP(scale));
        app_icon_->mouse_click.connect(on_mouse_down);
        icon_layout_->AddView(app_icon_.GetPointer(), 0);

        if (app_preview_model->rating >= 0)
        {
          app_rating_ = new PreviewRatingsWidget();
          AddChild(app_rating_.GetPointer());
          app_rating_->scale = scale();
          app_rating_->SetMaximumHeight(style.GetRatingWidgetHeight().CP(scale));
          app_rating_->SetMinimumHeight(style.GetRatingWidgetHeight().CP(scale));
          app_rating_->SetRating(app_preview_model->rating);
          app_rating_->SetReviews(app_preview_model->num_ratings);
          app_rating_->request_close().connect([this]() { preview_container_->request_close.emit(); });
          icon_layout_->AddView(app_rating_.GetPointer(), 0);
        }

        /////////////////////

        /////////////////////
        // Data

        app_data_layout_ = new nux::VLayout();
        app_data_layout_->SetSpaceBetweenChildren(DATA_SPACE_CHILDREN.CP(scale));

        title_subtitle_layout_ = new nux::VLayout();
        title_subtitle_layout_->SetSpaceBetweenChildren(style.GetSpaceBetweenTitleAndSubtitle().CP(scale));

        title_ = new StaticCairoText(preview_model_->title, true, NUX_TRACKER_LOCATION);
        AddChild(title_.GetPointer());
        title_->SetLines(-1);
        title_->SetScale(scale);
        title_->SetFont(style.title_font().c_str());
        title_->mouse_click.connect(on_mouse_down);
        title_subtitle_layout_->AddView(title_.GetPointer(), 1);

        if (!preview_model_->subtitle.Get().empty())
        {
          subtitle_ = new StaticCairoText(preview_model_->subtitle, true, NUX_TRACKER_LOCATION);
          AddChild(subtitle_.GetPointer());
          subtitle_->SetFont(style.subtitle_size_font().c_str());
          subtitle_->SetLines(-1);
          subtitle_->SetScale(scale);
          subtitle_->mouse_click.connect(on_mouse_down);
          title_subtitle_layout_->AddView(subtitle_.GetPointer(), 1);
        }

        app_updated_copywrite_layout_ = new nux::VLayout();
        app_updated_copywrite_layout_->SetSpaceBetweenChildren(COPYRIGHT_SPACE_CHILDREN.CP(scale));

        if (!app_preview_model->license.Get().empty())
        {
          license_ = new StaticCairoText(app_preview_model->license, true, NUX_TRACKER_LOCATION);
          AddChild(license_.GetPointer());
          license_->SetFont(style.app_license_font().c_str());
          license_->SetLines(-1);
          license_->SetScale(scale);
          license_->mouse_click.connect(on_mouse_down);
          app_updated_copywrite_layout_->AddView(license_.GetPointer(), 1);
        }

        if (!app_preview_model->last_update.Get().empty())
        {
          std::stringstream last_update;
          last_update << _("Last Updated") << " " << app_preview_model->last_update.Get();

          last_update_ = new StaticCairoText(last_update.str(), true, NUX_TRACKER_LOCATION);
          AddChild(last_update_.GetPointer());
          last_update_->SetFont(style.app_last_update_font().c_str());
          last_update_->SetScale(scale);
          last_update_->mouse_click.connect(on_mouse_down);
          app_updated_copywrite_layout_->AddView(last_update_.GetPointer(), 1);
        }

        if (!app_preview_model->copyright.Get().empty())
        {
          copywrite_ = new StaticCairoText(app_preview_model->copyright, true, NUX_TRACKER_LOCATION);
          AddChild(copywrite_.GetPointer());
          copywrite_->SetFont(style.app_copywrite_font().c_str());
          copywrite_->SetLines(-1);
          copywrite_->SetScale(scale);
          copywrite_->mouse_click.connect(on_mouse_down);
          app_updated_copywrite_layout_->AddView(copywrite_.GetPointer(), 1);
        }

        app_data_layout_->AddLayout(title_subtitle_layout_);
        app_data_layout_->AddLayout(app_updated_copywrite_layout_);

        // buffer space
        /////////////////////

      main_app_info_->AddLayout(icon_layout_, 0);
      main_app_info_->AddLayout(app_data_layout_, 1);
      /////////////////////

      /////////////////////
      // Description
      auto* app_info = new ScrollView(NUX_TRACKER_LOCATION);
      app_info_scroll_ = app_info; 
      app_info->scale = scale();
      app_info->EnableHorizontalScrollBar(false);
      app_info->mouse_click.connect(on_mouse_down);

      app_info_layout_ = new nux::VLayout();
      app_info_layout_->SetSpaceBetweenChildren(INFO_SPACE_CHILDREN.CP(scale));
      app_info->SetLayout(app_info_layout_);

      if (!preview_model_->description.Get().empty())
      {
        description_ = new StaticCairoText(preview_model_->description, false, NUX_TRACKER_LOCATION); // not escaped!
        AddChild(description_.GetPointer());
        description_->SetFont(style.description_font().c_str());
        description_->SetTextAlignment(StaticCairoText::NUX_ALIGN_TOP);
        description_->SetLines(-style.GetDescriptionLineCount());
        description_->SetLineSpacing(style.GetDescriptionLineSpacing());
        description_->mouse_click.connect(on_mouse_down);
        app_info_layout_->AddView(description_.GetPointer());
      }

      if (!preview_model_->GetInfoHints().empty())
      {
        preview_info_hints_ = new PreviewInfoHintWidget(preview_model_, style.GetInfoHintIconSizeWidth().CP(scale));
        AddChild(preview_info_hints_.GetPointer());
        preview_info_hints_->request_close().connect([this]() { preview_container_->request_close.emit(); });
        app_info_layout_->AddView(preview_info_hints_.GetPointer());
      }
      /////////////////////

      /////////////////////
      // Actions
      action_buttons_.clear();
      actions_layout_ = BuildGridActionsLayout(preview_model_->GetActions(), action_buttons_);
      actions_layout_->SetLeftAndRightPadding(0, style.GetDetailsRightMargin().CP(scale));
      ///////////////////

    full_data_layout_->AddLayout(main_app_info_, 0);
    full_data_layout_->AddView(app_info, 1);
    full_data_layout_->AddLayout(actions_layout_, 0);
    /////////////////////

  image_data_layout_->AddView(image_.GetPointer(), 0);
  image_data_layout_->AddLayout(full_data_layout_, 1);

  mouse_click.connect(on_mouse_down);

  SetLayout(image_data_layout_);
}

void ApplicationPreview::PreLayoutManagement()
{
  nux::Geometry geo = GetGeometry();

  previews::Style& style = dash::previews::Style::Instance();

  nux::Geometry geo_art(geo.x, geo.y, style.GetAppImageAspectRatio() * geo.height, geo.height);

  int content_width = geo.width - style.GetPanelSplitWidth().CP(scale)
                                - style.GetDetailsLeftMargin().CP(scale)
                                - style.GetDetailsRightMargin().CP(scale);
  if (content_width - geo_art.width < style.GetDetailsPanelMinimumWidth().CP(scale))
    geo_art.width = std::max(0, content_width - style.GetDetailsPanelMinimumWidth().CP(scale));

  image_->SetMinMaxSize(geo_art.width, geo_art.height);

  int details_width = std::max(0, content_width - geo_art.width);
  int top_app_info_max_width = std::max(0, details_width - style.GetAppIconAreaWidth().CP(scale) - style.GetSpaceBetweenIconAndDetails().CP(scale));

  if (title_) { title_->SetMaximumWidth(top_app_info_max_width); }
  if (subtitle_) { subtitle_->SetMaximumWidth(top_app_info_max_width); }
  if (license_) { license_->SetMaximumWidth(top_app_info_max_width); }
  if (last_update_) { last_update_->SetMaximumWidth(top_app_info_max_width); }
  if (copywrite_) { copywrite_->SetMaximumWidth(top_app_info_max_width); }
  if (description_) { description_->SetMaximumWidth(details_width); }

  int button_w = CLAMP((details_width - style.GetSpaceBetweenActions().CP(scale)) / 2, 0, style.GetActionButtonMaximumWidth().CP(scale));
  int button_h = style.GetActionButtonHeight().CP(scale);

  for (nux::AbstractButton* button : action_buttons_)
    button->SetMinMaxSize(button_w, button_h);

  Preview::PreLayoutManagement();
}

void ApplicationPreview::UpdateScale(double scale)
{
  Preview::UpdateScale(scale);

  previews::Style& style = dash::previews::Style::Instance();

  if (app_icon_)
  {
    app_icon_->SetSize(ICON_SIZE.CP(scale));
    app_icon_->SetMinimumSize(style.GetAppIconAreaWidth().CP(scale), style.GetAppIconAreaWidth().CP(scale));
    app_icon_->SetMaximumSize(style.GetAppIconAreaWidth().CP(scale), style.GetAppIconAreaWidth().CP(scale));
    app_icon_->ReLoadIcon();
  }

  if (app_info_scroll_)
    app_info_scroll_->scale = scale;

  if (license_)
    license_->SetScale(scale);

  if (last_update_)
    last_update_->SetScale(scale);

  if (copywrite_)
    copywrite_->SetScale(scale);

  if (app_rating_)
  {
    app_rating_->SetMaximumHeight(style.GetRatingWidgetHeight().CP(scale));
    app_rating_->SetMinimumHeight(style.GetRatingWidgetHeight().CP(scale));
    app_rating_->scale = scale;
  }

  if (image_data_layout_)
    image_data_layout_->SetSpaceBetweenChildren(style.GetPanelSplitWidth().CP(scale));

  if (full_data_layout_)
  {
    full_data_layout_->SetPadding(style.GetDetailsTopMargin().CP(scale), 0, style.GetDetailsBottomMargin().CP(scale), style.GetDetailsLeftMargin().CP(scale));
    full_data_layout_->SetSpaceBetweenChildren(DATA_SPACE_CHILDREN.CP(scale));
  }

  if (main_app_info_)
    main_app_info_->SetSpaceBetweenChildren(style.GetSpaceBetweenIconAndDetails().CP(scale));

  if (icon_layout_)
    icon_layout_->SetSpaceBetweenChildren(ICON_SPACE_CHILDREN.CP(scale));

  if (app_data_layout_)
    app_data_layout_->SetSpaceBetweenChildren(DATA_SPACE_CHILDREN.CP(scale));

  if (title_subtitle_layout_)
    title_subtitle_layout_->SetSpaceBetweenChildren(style.GetSpaceBetweenTitleAndSubtitle().CP(scale));

  if (app_info_layout_)
    app_info_layout_->SetSpaceBetweenChildren(INFO_SPACE_CHILDREN.CP(scale));

  if (actions_layout_)
    actions_layout_->SetLeftAndRightPadding(0, style.GetDetailsRightMargin().CP(scale));

  if (app_updated_copywrite_layout_)
    app_updated_copywrite_layout_->SetSpaceBetweenChildren(COPYRIGHT_SPACE_CHILDREN.CP(scale));
}

} // namespace previews
} // namespace dash
} // namepsace unity
