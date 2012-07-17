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

#include "unity-shared/IntrospectableWrappers.h"
#include "unity-shared/PreviewStyle.h"
#include "unity-shared/CoverArt.h"
#include "unity-shared/StaticCairoText.h"
#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <Nux/Button.h>
#include <UnityCore/GenericPreview.h>
 
#include "GenericPreview.h"
#include "ActionButton.h"
#include "PreviewInfoHintWidget.h"

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
nux::logging::Logger logger("unity.dash.previews.genericpreview");

}

NUX_IMPLEMENT_OBJECT_TYPE(GenericPreview);

GenericPreview::GenericPreview(dash::Preview::Ptr preview_model)
: Preview(preview_model)
{
  SetupBackground();
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

void GenericPreview::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
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

std::string GenericPreview::GetName() const
{
  return "GenericPreview";
}

void GenericPreview::AddProperties(GVariantBuilder* builder)
{
  Preview::AddProperties(builder);
}

void GenericPreview::SetupBackground()
{
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  details_bg_layer_.reset(new nux::ColorLayer(nux::Color(0.03f, 0.03f, 0.03f, 0.0f), true, rop));
}

void GenericPreview::SetupViews()
{
  dash::GenericPreview* app_preview_model = dynamic_cast<dash::GenericPreview*>(preview_model_.get());
  if (!app_preview_model)
    return;

  previews::Style& style = dash::previews::Style::Instance();

  nux::HLayout* image_data_layout = new nux::HLayout();
  image_data_layout->SetSpaceBetweenChildren(12);

  CoverArt* app_image = new CoverArt();
  app_image->SetImage(preview_model_->image.Get().RawPtr() ? g_icon_to_string(preview_model_->image.Get().RawPtr()) : "");
  app_image->SetFont(style.no_preview_image_font());
  app_image->SetMinimumWidth(style.GetImageWidth());
  app_image->SetMaximumWidth(style.GetImageWidth());

    /////////////////////
    // Data Panel
    full_data_layout_ = new nux::VLayout();
    full_data_layout_->SetPadding(10);
    full_data_layout_->SetSpaceBetweenChildren(16);

      /////////////////////
      // Data
      nux::VLayout* app_data_layout = new nux::VLayout();
      app_data_layout->SetSpaceBetweenChildren(8);

      title_ = new nux::StaticCairoText(app_preview_model->title);
      title_->SetFont(style.title_font().c_str());

      subtitle_ = new nux::StaticCairoText(app_preview_model->subtitle);
      subtitle_->SetFont(style.subtitle_size_font().c_str());

      app_data_layout->AddView(title_, 1);
      app_data_layout->AddView(subtitle_, 1);
      /////////////////////

      /////////////////////
      // Description
      nux::ScrollView* app_info = new nux::ScrollView(NUX_TRACKER_LOCATION);
      app_info->EnableHorizontalScrollBar(false);

      nux::VLayout* app_info_layout = new nux::VLayout();
      app_info_layout->SetSpaceBetweenChildren(16);
      app_info->SetLayout(app_info_layout);

      app_description_ = new nux::StaticCairoText("");
      app_description_->SetFont(style.app_description_font().c_str());
      app_description_->SetTextAlignment(nux::StaticCairoText::NUX_ALIGN_TOP);
      app_description_->SetLines(-20);
      app_description_->SetLineSpacing(1);
      app_description_->SetMaximumWidth(400);
      app_description_->SetText(app_preview_model->description);

      PreviewInfoHintWidget* preview_info_hints = new PreviewInfoHintWidget(preview_model_, 24);

      app_info_layout->AddView(app_description_);
      app_info_layout->AddView(preview_info_hints);
      /////////////////////

      /////////////////////
      // Actions
      nux::HLayout* actions_layout = new nux::HLayout();
      actions_layout->SetSpaceBetweenChildren(12);
      actions_layout->AddSpace(0, 1);

      for (dash::Preview::ActionPtr action : preview_model_->GetActions())
      {
        ActionButton* button = new ActionButton(action->display_name, NUX_TRACKER_LOCATION);
        button->click.connect(sigc::bind(sigc::mem_fun(this, &GenericPreview::OnActionActivated), action->id));

        actions_layout->AddView(button, 0);
      }
      /////////////////////

    full_data_layout_->AddLayout(app_data_layout, 0);
    full_data_layout_->AddView(app_info, 1);
    full_data_layout_->AddLayout(actions_layout, 0);
    /////////////////////
  
  image_data_layout->AddView(app_image, 0);
  image_data_layout->AddLayout(full_data_layout_, 1);

  SetLayout(image_data_layout);
}



}
}
}
