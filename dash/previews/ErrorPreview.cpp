// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012-2013 Canonical Ltd.
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

#include "TabIterator.h"
#include "unity-shared/IntrospectableWrappers.h"
#include "unity-shared/PreviewStyle.h"
#include "unity-shared/CoverArt.h"
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/PlacesVScrollBar.h"
#include <NuxCore/Logger.h>
#include <Nux/VLayout.h>
#include <Nux/HLayout.h>
#include <Nux/GridHLayout.h>
#include <Nux/AbstractButton.h>

#include "ErrorPreview.h"
#include "PreviewInfoHintWidget.h"

#include "stdio.h"
#include "config.h"

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
nux::logging::Logger logger("unity.dash.previews.ErrorPreview");

}

const std::string ErrorPreview::CANCEL_ACTION = "cancel";
const std::string ErrorPreview::GO_TO_U1_ACTION = "open_u1_link";

class DetailsScrollView : public nux::ScrollView
{
public:
  DetailsScrollView(NUX_FILE_LINE_PROTO)
  : ScrollView(NUX_FILE_LINE_PARAM)
  {
    SetVScrollBar(new dash::PlacesVScrollBar(NUX_TRACKER_LOCATION));
  }

};

NUX_IMPLEMENT_OBJECT_TYPE(ErrorPreview)

ErrorPreview::ErrorPreview(dash::Preview::Ptr preview_model)
: PaymentPreview(preview_model)
{
  PaymentPreview::SetupBackground();
  SetupViews();
}

ErrorPreview::~ErrorPreview()
{
}

nux::Area* ErrorPreview::FindKeyFocusArea(unsigned int key_symbol,
                                    unsigned long x11_key_code,
                                    unsigned long special_keys_state)
{
  return Preview::FindKeyFocusArea(key_symbol, x11_key_code, special_keys_state); 
}

std::string ErrorPreview::GetName() const
{
  return "ErrorPreview";
}

void ErrorPreview::OnActionActivated(ActionButton* button, std::string const& id)
{
  Preview::OnActionActivated(button, id);
}

void ErrorPreview::OnActionLinkActivated(ActionLink *link, std::string const& id)
{
  if (preview_model_)
    preview_model_->PerformAction(id);
}

void ErrorPreview::LoadActions()
{
  // Loop over the buttons and add them to the correct var
  // this is not efficient but is the only way we have atm
  for (dash::Preview::ActionPtr action : preview_model_->GetActions())
  {
    nux::ObjectPtr<ActionButton> button = this->CreateButton(action);
    button->activate.connect(sigc::mem_fun(this, &ErrorPreview::OnActionActivated));
    buttons_map_.insert(std::make_pair(action->id, button));
  }
}

nux::Layout* ErrorPreview::GetTitle()
{
  previews::Style& style = dash::previews::Style::Instance();
  nux::VLayout* title_data_layout = new nux::VLayout();
  title_data_layout->SetMaximumHeight(76);
  title_data_layout->SetSpaceBetweenChildren(10);

  title_ = new StaticCairoText(
          preview_model_->title.Get(), true,
          NUX_TRACKER_LOCATION);

  title_->SetFont(style.payment_title_font());
  title_->SetLines(-1);
  title_->SetFont(style.title_font());
  title_->SetMaximumWidth(480);
  title_->SetTextEllipsize(StaticCairoText::EllipsizeState::NUX_ELLIPSIZE_END);
  title_data_layout->AddView(title_.GetPointer(), 1);

  subtitle_ = new StaticCairoText(
          preview_model_->subtitle.Get(), true,
          NUX_TRACKER_LOCATION);
  subtitle_->SetLines(-1);
  subtitle_->SetFont(style.payment_subtitle_font());
  title_data_layout->AddView(subtitle_.GetPointer(), 1);
  title_data_layout->AddSpace(1, 1);
  return title_data_layout;
}

nux::Layout* ErrorPreview::GetPrice()
{
  previews::Style& style = dash::previews::Style::Instance();
  nux::VLayout *prize_data_layout = new nux::VLayout();
  prize_data_layout->SetMaximumHeight(76);
  prize_data_layout->SetSpaceBetweenChildren(5);
  prize_data_layout->SetPadding(0, 10, 0, 0);

  purchase_prize_ = new StaticCairoText(
          error_preview_model_->purchase_prize.Get(), true,
          NUX_TRACKER_LOCATION);
  purchase_prize_->SetLines(-1);
  purchase_prize_->SetFont(style.payment_prize_title_font());
  prize_data_layout->AddView(purchase_prize_.GetPointer(), 1,
          nux::MINOR_POSITION_END);

  purchase_hint_ = new StaticCairoText(
          _("Ubuntu One best offer"),
          true, NUX_TRACKER_LOCATION);
  purchase_hint_->SetLines(-1);
  purchase_hint_->SetFont(style.payment_prize_subtitle_font());
  prize_data_layout->AddView(purchase_hint_.GetPointer(), 1,
          nux::MINOR_POSITION_END);

  purchase_type_ = new StaticCairoText(
          error_preview_model_->purchase_type.Get(), true,
          NUX_TRACKER_LOCATION);
  purchase_type_->SetLines(-1);
  purchase_type_->SetFont(style.payment_prize_subtitle_font());
  prize_data_layout->AddView(purchase_type_.GetPointer(), 1,
          nux::MINOR_POSITION_END);
  return prize_data_layout;
}

nux::Layout* ErrorPreview::GetBody()
{
  previews::Style& style = dash::previews::Style::Instance();
  nux::HLayout *body_layout = new nux::HLayout();
  nux::HLayout *intro_layout = new nux::HLayout();
  nux::VLayout *icon_layout = new nux::VLayout();

  icon_layout->SetPadding(78, 10, 90, 43);
  intro_layout->SetPadding(75, 20, 0, 0);
  intro_layout->SetSpaceBetweenChildren(5);

  intro_ = new StaticCairoText(
              error_preview_model_->header.Get(), true,
          NUX_TRACKER_LOCATION);
  intro_->SetFont(style.payment_intro_font().c_str());
  intro_->SetLines(-3);
  intro_->SetLineSpacing(10);
  intro_->SetTextEllipsize(StaticCairoText::EllipsizeState::NUX_ELLIPSIZE_END);

  intro_layout->AddView(intro_.GetPointer());//, 0, nux::MINOR_POSITION_CENTER);

  warning_texture_ = new IconTexture(style.GetWarningIcon());
  icon_layout->AddView(warning_texture_.GetPointer(),
    0, nux::MINOR_POSITION_END);

  body_layout->AddLayout(icon_layout, 0);
  body_layout->AddLayout(intro_layout, 0);
  body_layout->AddSpace(1, 1);

  return body_layout;
}

nux::Layout* ErrorPreview::GetFooter()
{
  previews::Style& style = dash::previews::Style::Instance();
  nux::HLayout* actions_buffer_h = new nux::HLayout();
  actions_buffer_h->AddSpace(0, 1);

  nux::HLayout* buttons_data_layout = new TabIteratorHLayout(tab_iterator_);
  buttons_data_layout->SetSpaceBetweenChildren(style.GetSpaceBetweenActions());

  buttons_data_layout->AddSpace(20, 1);
  if(buttons_map_[ErrorPreview::CANCEL_ACTION].GetPointer()){
    buttons_data_layout->AddView(buttons_map_[ErrorPreview::CANCEL_ACTION].GetPointer(),
           1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 100.0f,
            nux::NUX_LAYOUT_END);
    AddChild((ActionButton*)buttons_map_[ErrorPreview::CANCEL_ACTION].GetPointer());
  }
  if(buttons_map_[ErrorPreview::GO_TO_U1_ACTION].GetPointer()){
    buttons_data_layout->AddView(buttons_map_[ErrorPreview::GO_TO_U1_ACTION].GetPointer(),
            1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 100.0f,
            nux::NUX_LAYOUT_END);
    AddChild((ActionButton*)buttons_map_[ErrorPreview::GO_TO_U1_ACTION].GetPointer());
  }
  return buttons_data_layout;
}

void ErrorPreview::PreLayoutManagement()
{
  nux::Geometry geo = GetGeometry();
  GetLayout()->SetGeometry(geo);

  previews::Style& style = dash::previews::Style::Instance();

  int width = MAX(0, geo.width - style.GetPanelSplitWidth() - style.GetDetailsLeftMargin() - style.GetDetailsRightMargin());

  if(full_data_layout_) { full_data_layout_->SetMaximumWidth(width); }
  if(header_layout_) { header_layout_->SetMaximumWidth(width); }
  if(intro_) { intro_->SetMaximumWidth(width - 110); }
  if(footer_layout_) { footer_layout_->SetMaximumWidth(width); }

  Preview::PreLayoutManagement();
}

void ErrorPreview::SetupViews()
{
  error_preview_model_ = dynamic_cast<dash::PaymentPreview*>(preview_model_.get());
  if (!error_preview_model_)
  {
    LOG_ERROR(logger) << "Could not derive preview model from given parameter.";
    return;
  }

  // load the buttons so that they can be accessed in order
  LoadActions();

  PaymentPreview::SetupViews();
}


}
}
}
