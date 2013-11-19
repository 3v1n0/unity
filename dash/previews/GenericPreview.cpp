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
#include "unity-shared/PlacesOverlayVScrollBar.h"
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
DECLARE_LOGGER(logger, "unity.dash.preview.generic");

class DetailsScrollView : public nux::ScrollView
{
public:
  DetailsScrollView(NUX_FILE_LINE_PROTO)
  : ScrollView(NUX_FILE_LINE_PARAM)
  {
    SetVScrollBar(new dash::PlacesOverlayVScrollBar(NUX_TRACKER_LOCATION));
  }

};

NUX_IMPLEMENT_OBJECT_TYPE(GenericPreview);

GenericPreview::GenericPreview(dash::Preview::Ptr preview_model)
: Preview(preview_model)
{
  SetupViews();
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

void GenericPreview::AddProperties(GVariantBuilder* builder)
{
  Preview::AddProperties(builder);
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

  nux::HLayout* image_data_layout = new nux::HLayout();
  image_data_layout->SetSpaceBetweenChildren(style.GetPanelSplitWidth());

  /////////////////////
  // Image
  image_ = new CoverArt();
  AddChild(image_.GetPointer());
  UpdateCoverArtImage(image_.GetPointer());
  /////////////////////

    /////////////////////
    // Data Panel
    full_data_layout_ = new nux::VLayout();
    full_data_layout_->SetPadding(style.GetDetailsTopMargin(), 0, style.GetDetailsBottomMargin(), style.GetDetailsLeftMargin());
    full_data_layout_->SetSpaceBetweenChildren(16);

      /////////////////////
      // Data

      nux::VLayout* preview_data_layout = new nux::VLayout();
      preview_data_layout->SetSpaceBetweenChildren(style.GetSpaceBetweenTitleAndSubtitle());

      title_ = new StaticCairoText(preview_model_->title, true, NUX_TRACKER_LOCATION);
      AddChild(title_.GetPointer());
      title_->SetLines(-1);
      title_->SetFont(style.title_font().c_str());
      title_->mouse_click.connect(on_mouse_down);
      preview_data_layout->AddView(title_.GetPointer(), 1);

      if (!preview_model_->subtitle.Get().empty())
      {
        subtitle_ = new StaticCairoText(preview_model_->subtitle, true, NUX_TRACKER_LOCATION);
        AddChild(subtitle_.GetPointer());
        subtitle_->SetLines(-1);
        subtitle_->SetFont(style.subtitle_size_font().c_str());
        subtitle_->mouse_click.connect(on_mouse_down);
        preview_data_layout->AddView(subtitle_.GetPointer(), 1);
      }
      /////////////////////

      /////////////////////
      // Description
      nux::ScrollView* preview_info = new DetailsScrollView(NUX_TRACKER_LOCATION);
      preview_info->EnableHorizontalScrollBar(false);
      preview_info->mouse_click.connect(on_mouse_down);

      nux::VLayout* preview_info_layout = new nux::VLayout();
      preview_info_layout->SetSpaceBetweenChildren(12);
      preview_info->SetLayout(preview_info_layout);

      if (!preview_model_->description.Get().empty())
      {
        description_ = new StaticCairoText(preview_model_->description, false, NUX_TRACKER_LOCATION); // not escaped!
        AddChild(description_.GetPointer());
        description_->SetFont(style.description_font().c_str());
        description_->SetTextAlignment(StaticCairoText::NUX_ALIGN_TOP);
        description_->SetLines(-style.GetDescriptionLineCount());
        description_->SetLineSpacing(style.GetDescriptionLineSpacing());
        description_->mouse_click.connect(on_mouse_down);
        preview_info_layout->AddView(description_.GetPointer());
      }

      if (!preview_model_->GetInfoHints().empty())
      {
        preview_info_hints_ = new PreviewInfoHintWidget(preview_model_, style.GetInfoHintIconSizeWidth());
        AddChild(preview_info_hints_.GetPointer());
        preview_info_hints_->request_close().connect([this]() { preview_container_->request_close.emit(); });
        preview_info_layout->AddView(preview_info_hints_.GetPointer());
      }
      /////////////////////

      /////////////////////
      // Actions
      action_buttons_.clear();
      nux::Layout* actions_layout = BuildGridActionsLayout(preview_model_->GetActions(), action_buttons_);
      actions_layout->SetLeftAndRightPadding(0, style.GetDetailsRightMargin());
      ///////////////////

    full_data_layout_->AddLayout(preview_data_layout, 0);
    full_data_layout_->AddView(preview_info, 1);
    full_data_layout_->AddView(actions_layout, 0);
    /////////////////////
  
  image_data_layout->AddView(image_.GetPointer(), 0);

  image_data_layout->AddLayout(full_data_layout_, 1);

  mouse_click.connect(on_mouse_down);

  SetLayout(image_data_layout);
}

void GenericPreview::PreLayoutManagement()
{
  nux::Geometry geo = GetGeometry();

  previews::Style& style = dash::previews::Style::Instance();

  nux::Geometry geo_art(geo.x, geo.y, style.GetAppImageAspectRatio() * geo.height, geo.height);

  if (geo.width - geo_art.width - style.GetPanelSplitWidth() - style.GetDetailsLeftMargin() - style.GetDetailsRightMargin() < style.GetDetailsPanelMinimumWidth())
    geo_art.width = MAX(0, geo.width - style.GetPanelSplitWidth() - style.GetDetailsLeftMargin() - style.GetDetailsRightMargin() - style.GetDetailsPanelMinimumWidth());
  image_->SetMinMaxSize(geo_art.width, geo_art.height);

  int details_width = MAX(0, geo.width - geo_art.width - style.GetPanelSplitWidth() - style.GetDetailsLeftMargin() - style.GetDetailsRightMargin());

  if (title_) { title_->SetMaximumWidth(details_width); }
  if (subtitle_) { subtitle_->SetMaximumWidth(details_width); }
  if (description_) { description_->SetMaximumWidth(details_width); }

  for (nux::AbstractButton* button : action_buttons_)
  {
    button->SetMinMaxSize(CLAMP((details_width - style.GetSpaceBetweenActions()) / 2, 0, style.GetActionButtonMaximumWidth()), style.GetActionButtonHeight());
  }

  Preview::PreLayoutManagement();
}


}
}
}
