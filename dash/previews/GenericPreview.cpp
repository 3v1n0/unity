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
#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <Nux/GridHLayout.h>
#include <UnityCore/GenericPreview.h>
#include <Nux/AbstractButton.h>

#include "GenericPreview.h"
#include "PreviewInfoHintWidget.h"

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
  const RawPixel CHILDREN_SPACE = 12_em;
  const RawPixel FULL_CHILDREN_SPACE = 16_em;
}

DECLARE_LOGGER(logger, "unity.dash.preview.generic");

NUX_IMPLEMENT_OBJECT_TYPE(GenericPreview);

GenericPreview::GenericPreview(dash::Preview::Ptr preview_model)
: Preview(preview_model)
, image_data_layout_(nullptr)
, preview_info_layout_(nullptr)
, preview_info_scroll_(nullptr)
, preview_data_layout_(nullptr)
, actions_layout_(nullptr)
{
  SetupViews();
  UpdateScale(scale);
  scale.changed.connect(sigc::mem_fun(this, &GenericPreview::UpdateScale));
}

GenericPreview::~GenericPreview()
{
}

void GenericPreview::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  gfx_engine.PushClippingRectangle(base);
  nux::GetPainter().PaintBackground(gfx_engine, base);

  gfx_engine.PopClippingRectangle();
}

void GenericPreview::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
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

std::string GenericPreview::GetName() const
{
  return "GenericPreview";
}

void GenericPreview::AddProperties(debug::IntrospectionData& introspection)
{
  Preview::AddProperties(introspection);
}

void GenericPreview::SetupViews()
{
  if (!preview_model_)
  {
    LOG_ERROR(logger) << "Could not derive preview model from given parameter.";
    return;
  }
  previews::Style& style = dash::previews::Style::Instance();

  auto on_mouse_down = [this](int x, int y, unsigned long button_flags, unsigned long key_flags) { this->preview_container_->OnMouseDown(x, y, button_flags, key_flags); };

  image_data_layout_ = new nux::HLayout();
  image_data_layout_->SetSpaceBetweenChildren(style.GetPanelSplitWidth().CP(scale));

  /////////////////////
  // Image
  image_ = new CoverArt();
  AddChild(image_.GetPointer());
  UpdateCoverArtImage(image_.GetPointer());
  /////////////////////

    /////////////////////
    // Data Panel
    full_data_layout_ = new nux::VLayout();
    full_data_layout_->SetPadding(style.GetDetailsTopMargin().CP(scale), 0,
        style.GetDetailsBottomMargin().CP(scale), style.GetDetailsLeftMargin().CP(scale));
    full_data_layout_->SetSpaceBetweenChildren(FULL_CHILDREN_SPACE.CP(scale));

      /////////////////////
      // Data

      preview_data_layout_ = new nux::VLayout();
      preview_data_layout_->SetSpaceBetweenChildren(style.GetSpaceBetweenTitleAndSubtitle().CP(scale));

      title_ = new StaticCairoText(preview_model_->title, true, NUX_TRACKER_LOCATION);
      AddChild(title_.GetPointer());
      title_->SetLines(-1);
      title_->SetFont(style.title_font().c_str());
      title_->mouse_click.connect(on_mouse_down);
      preview_data_layout_->AddView(title_.GetPointer(), 1);

      if (!preview_model_->subtitle.Get().empty())
      {
        subtitle_ = new StaticCairoText(preview_model_->subtitle, true, NUX_TRACKER_LOCATION);
        AddChild(subtitle_.GetPointer());
        subtitle_->SetLines(-1);
        subtitle_->SetFont(style.subtitle_size_font().c_str());
        subtitle_->mouse_click.connect(on_mouse_down);
        preview_data_layout_->AddView(subtitle_.GetPointer(), 1);
      }
      /////////////////////

      /////////////////////
      // Description
      auto* preview_info = new ScrollView(NUX_TRACKER_LOCATION);
      preview_info_scroll_ = preview_info;
      preview_info->scale = scale();
      preview_info->EnableHorizontalScrollBar(false);
      preview_info->mouse_click.connect(on_mouse_down);

      preview_info_layout_ = new nux::VLayout();
      preview_info_layout_->SetSpaceBetweenChildren(CHILDREN_SPACE.CP(scale));
      preview_info->SetLayout(preview_info_layout_);

      if (!preview_model_->description.Get().empty())
      {
        description_ = new StaticCairoText(preview_model_->description, false, NUX_TRACKER_LOCATION); // not escaped!
        AddChild(description_.GetPointer());
        description_->SetFont(style.description_font().c_str());
        description_->SetTextAlignment(StaticCairoText::NUX_ALIGN_TOP);
        description_->SetLines(-style.GetDescriptionLineCount());
        description_->SetLineSpacing(style.GetDescriptionLineSpacing());
        description_->mouse_click.connect(on_mouse_down);
        preview_info_layout_->AddView(description_.GetPointer());
      }

      if (!preview_model_->GetInfoHints().empty())
      {
        preview_info_hints_ = new PreviewInfoHintWidget(preview_model_, style.GetInfoHintIconSizeWidth().CP(scale));
        AddChild(preview_info_hints_.GetPointer());
        preview_info_hints_->request_close().connect([this]() { preview_container_->request_close.emit(); });
        preview_info_layout_->AddView(preview_info_hints_.GetPointer());
      }
      /////////////////////

      /////////////////////
      // Actions
      action_buttons_.clear();
      actions_layout_ = BuildGridActionsLayout(preview_model_->GetActions(), action_buttons_);
      actions_layout_->SetLeftAndRightPadding(0, style.GetDetailsRightMargin().CP(scale));
      ///////////////////

    full_data_layout_->AddLayout(preview_data_layout_, 0);
    full_data_layout_->AddView(preview_info, 1);
    full_data_layout_->AddView(actions_layout_, 0);
    /////////////////////
  
  image_data_layout_->AddView(image_.GetPointer(), 0);

  image_data_layout_->AddLayout(full_data_layout_, 1);

  mouse_click.connect(on_mouse_down);

  SetLayout(image_data_layout_);
}

void GenericPreview::PreLayoutManagement()
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

  if (title_) { title_->SetMaximumWidth(details_width); }
  if (subtitle_) { subtitle_->SetMaximumWidth(details_width); }
  if (description_) { description_->SetMaximumWidth(details_width); }

  int button_w = CLAMP((details_width - style.GetSpaceBetweenActions().CP(scale)) / 2, 0, style.GetActionButtonMaximumWidth().CP(scale));
  int button_h = style.GetActionButtonHeight().CP(scale);

  for (nux::AbstractButton* button : action_buttons_)
    button->SetMinMaxSize(button_w, button_h);

  Preview::PreLayoutManagement();
}

void GenericPreview::UpdateScale(double scale)
{
  if (image_)
    image_->scale = scale;

  if (preview_info_scroll_)
    preview_info_scroll_->scale = scale;

  if (preview_info_hints_)
    preview_info_hints_->scale = scale;

  previews::Style& style = dash::previews::Style::Instance();

  if (full_data_layout_)
  {
    full_data_layout_->SetPadding(style.GetDetailsTopMargin().CP(scale), 0, style.GetDetailsBottomMargin().CP(scale), style.GetDetailsLeftMargin().CP(scale));
    full_data_layout_->SetSpaceBetweenChildren(FULL_CHILDREN_SPACE.CP(scale));
  }

  if (image_data_layout_)
    image_data_layout_->SetSpaceBetweenChildren(style.GetPanelSplitWidth().CP(scale));

  if (preview_info_layout_)
    preview_info_layout_->SetSpaceBetweenChildren(CHILDREN_SPACE.CP(scale));

  if (preview_data_layout_)
    preview_data_layout_->SetSpaceBetweenChildren(style.GetSpaceBetweenTitleAndSubtitle().CP(scale));

  if (actions_layout_)
    actions_layout_->SetLeftAndRightPadding(0, style.GetDetailsRightMargin().CP(scale));

  Preview::UpdateScale(scale);
}


}
}
}
