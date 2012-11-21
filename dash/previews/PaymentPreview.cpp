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
 * Authored by: Diego Sarmentero <diego.sarmentero@canonical.com>
 *              Manuel de la Pena <manuel.delapena@canonical.com>
 *
 */
#include <NuxCore/Logger.h>
#include "PaymentPreview.h"
#include "unity-shared/CoverArt.h"

namespace unity
{

namespace dash
{

namespace previews
{

namespace
{
nux::logging::Logger logger("unity.dash.previews.MusicPaymentPreview");

}

PaymentPreview::PaymentPreview(dash::Preview::Ptr preview_model)
: Preview(preview_model)
, data_(nullptr)
, full_data_layout_(nullptr)
{
}

PaymentPreview::~PaymentPreview()
{
}


std::string PaymentPreview::GetName() const
{
  return "";
}


nux::Layout* PaymentPreview::GetHeader()
{
  nux::HLayout* header_data_layout = new nux::HLayout();
  header_data_layout->SetSpaceBetweenChildren(10);
  header_data_layout->SetMaximumHeight(76);
  header_data_layout->SetMinimumHeight(76);

  image_ = new CoverArt();
  image_->SetMinMaxSize(64, 64);
  AddChild(image_.GetPointer());
  UpdateCoverArtImage(image_.GetPointer());

  header_data_layout->AddView(image_.GetPointer(), 0);
  header_data_layout->AddLayout(GetTitle(), 0);
  header_data_layout->AddSpace(20, 1);
  header_data_layout->AddLayout(GetPrize(), 0);
  return header_data_layout;
}


void PaymentPreview::AddProperties(GVariantBuilder* builder)
{
  Preview::AddProperties(builder);
}


nux::ObjectPtr<ActionLink> PaymentPreview::CreateLink(dash::Preview::ActionPtr action)
{
  previews::Style& style = dash::previews::Style::Instance();

  nux::ObjectPtr<ActionLink> link;
  link = new ActionLink(action->id,
         action->display_name, NUX_TRACKER_LOCATION);
  link->SetFont(style.payment_form_labels_font().c_str());
  link->SetMinimumWidth(178);
  link->SetMinimumHeight(34);
  return link;
}


nux::ObjectPtr<ActionButton> PaymentPreview::CreateButton(dash::Preview::ActionPtr action)
{
  previews::Style& style = dash::previews::Style::Instance();

  nux::ObjectPtr<ActionButton> button;
  button = new ActionButton(action->id,
           action->display_name, action->icon_hint,
           NUX_TRACKER_LOCATION);
  button->SetFont(style.action_font());
  button->SetExtraHint(action->extra_text, style.action_extra_font());
  button->SetMinimumWidth(178);
  button->SetMinimumHeight(34);
  return button;
}


void PaymentPreview::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
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

void PaymentPreview::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
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

void PaymentPreview::SetupBackground()
{
  details_bg_layer_.reset(dash::previews::Style::Instance().GetBackgroundLayer());
}

void PaymentPreview::SetupViews()
{
  if (!preview_model_)
  {
    LOG_ERROR(logger) << "Could not derive preview model from given parameter.";
    return;
  }

  // HACK: All the information required by the preview is stored in an infor
  // hint, lets loop through them and store them
  dash::Preview::InfoHintPtrList hints = preview_model_->GetInfoHints();
  dash::Preview::InfoHintPtr data_info_hint_ = NULL;
  if (!hints.empty())
  {
    /*for (dash::Preview::InfoHintPtr info_hint : hints)
    {
       if (info_hint->id == DATA_INFOHINT_ID){
         this->data_ = info_hint->value;
       }
    }*/
    if (this->data_ == NULL)
    {
      LOG_ERROR(logger) << "The required data for the preview is missing.";
      return;
    }
  }
  else
  {
    LOG_ERROR(logger) << "The required data for the preview is missing.";
    return;
  }

  previews::Style& style = dash::previews::Style::Instance();

  // load the buttons so that they can be accessed in order
  //LoadActions();

  full_data_layout_ = new nux::VLayout();
  full_data_layout_->SetSpaceBetweenChildren(5);
  full_data_layout_->SetLeftAndRightPadding(10);
  full_data_layout_->SetTopAndBottomPadding(10);

  header_layout_ = GetHeader();

  full_data_layout_->AddLayout(header_layout_, 1);
  full_data_layout_->AddSpace(style.GetPaymentHeaderSpace(), 0);

  /*header_ = new nux::StaticCairoText(
          GetDataForKey(this->data_, DATA_HEADER_KEY), true,
          NUX_TRACKER_LOCATION);
  //header_->SetMaximumWidth(style.GetPaymentHeaderWidth());
  header_->SetFont(style.payment_intro_font().c_str());
  header_->SetLineSpacing(10);
  header_->SetLines(-style.GetDescriptionLineCount());
  header_->SetMinimumHeight(50);*/

  full_data_layout_->AddView(header_.GetPointer(), 1);
  full_data_layout_->AddSpace(style.GetPaymentFormSpace(), 1);

  /*body_layout_ = GetBody();
  full_data_layout_->AddLayout(body_layout_, 1);
  full_data_layout_->AddSpace(style.GetPaymentFooterSpace(), 1);

  footer_layout_ = GetFooter();
  full_data_layout_->AddLayout(footer_layout_, 0);*/

  SetLayout(full_data_layout_);
}

}

}

}
