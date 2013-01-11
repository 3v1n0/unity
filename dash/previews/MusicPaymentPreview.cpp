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
 * Authored by: Manuel de la Pena <manuel.delapena@canonical.com>
 *
 */

#include "unity-shared/IntrospectableWrappers.h"
#include "unity-shared/PreviewStyle.h"
#include "unity-shared/CoverArt.h"
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/PlacesVScrollBar.h"
#include "config.h"

#include <glib/gi18n-lib.h>
#include <NuxCore/Logger.h>
#include <Nux/VLayout.h>
#include <Nux/GridHLayout.h>
#include <Nux/AbstractButton.h>

#include "MusicPaymentPreview.h"
#include "PreviewInfoHintWidget.h"

#include "stdio.h"

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

class DetailsScrollView : public nux::ScrollView
{
public:
  DetailsScrollView(NUX_FILE_LINE_PROTO)
  : ScrollView(NUX_FILE_LINE_PARAM)
  {
    SetVScrollBar(new dash::PlacesVScrollBar(NUX_TRACKER_LOCATION));
  }

};

NUX_IMPLEMENT_OBJECT_TYPE(MusicPaymentPreview)

MusicPaymentPreview::MusicPaymentPreview(dash::Preview::Ptr preview_model)
: PaymentPreview(preview_model)
{
  PaymentPreview::SetupBackground();
  SetupViews();
}

MusicPaymentPreview::~MusicPaymentPreview()
{
}

std::string MusicPaymentPreview::GetName() const
{
  return "MusicPaymentPreview";
}

void MusicPaymentPreview::OnActionActivated(ActionButton* button, std::string const& id)
{
  // Check the action id and send the password only when we
  // purchasing a song
  if(id.compare(PURCHASE_ALBUM_ACTION) == 0 && preview_model_
          && password_entry_)
  {
    // HACK: We need to think a better way to do this
    GVariant *variant = g_variant_new_string(
        password_entry_->text_entry()->GetText().c_str());
    glib::Variant* password = new glib::Variant(variant);
    Lens::Hints hints;
    hints[DATA_PASSWORD_KEY] = *password;
    preview_model_->PerformAction(id, hints);
    // show the overlay
    ShowOverlay();
    return;
  }
  Preview::OnActionActivated(button, id);
}

void MusicPaymentPreview::OnActionLinkActivated(ActionLink *link, std::string const& id)
{
  if (preview_model_)
    preview_model_->PerformAction(id);
}

void MusicPaymentPreview::LoadActions()
{
  // Loop over the buttons and add them to the correct var
  // this is not efficient but is the only way we have atm
  for (dash::Preview::ActionPtr action : preview_model_->GetActions())
  {
      const char *action_id = action->id.c_str();
      if(strcmp(CHANGE_PAYMENT_ACTION, action_id) == 0
              || strcmp(FORGOT_PASSWORD_ACTION, action_id) == 0)
      {
        nux::ObjectPtr<ActionLink> link = this->CreateLink(action);
        link->activate.connect(sigc::mem_fun(this,
                    &MusicPaymentPreview::OnActionLinkActivated));

        std::pair<std::string, nux::ObjectPtr<nux::AbstractButton>> data (action->id, link);
        sorted_buttons_.insert(data);
      }
      else
      {
        nux::ObjectPtr<ActionButton> button = this->CreateButton(action);
        button->activate.connect(sigc::mem_fun(this,
                    &MusicPaymentPreview::OnActionActivated));

        std::pair<std::string, nux::ObjectPtr<nux::AbstractButton>> data (action->id, button);
        sorted_buttons_.insert(data);
      }
      LOG_DEBUG(logger) << "added button for action with id '" << action->id << "'";
  }
}

nux::Layout* MusicPaymentPreview::GetTitle()
{
  previews::Style& style = dash::previews::Style::Instance();
  nux::VLayout* title_data_layout = new nux::VLayout();
  title_data_layout->SetMaximumHeight(76);
  title_data_layout->SetSpaceBetweenChildren(10);

  title_ = new StaticCairoText(
          preview_model_->title.Get(), true,
          NUX_TRACKER_LOCATION);

  title_->SetFont(style.payment_title_font().c_str());
  title_->SetLines(-1);
  title_->SetFont(style.title_font().c_str());
  title_data_layout->AddView(title_.GetPointer(), 1);

  subtitle_ = new StaticCairoText(
          preview_model_->subtitle.Get(), true,
          NUX_TRACKER_LOCATION);
  subtitle_->SetLines(-1);
  subtitle_->SetFont(style.payment_subtitle_font().c_str());
  title_data_layout->AddView(subtitle_.GetPointer(), 1);
  title_data_layout->AddSpace(1, 1);
  return title_data_layout;
}

nux::Layout* MusicPaymentPreview::GetPrize()
{
  previews::Style& style = dash::previews::Style::Instance();
  nux::VLayout *prize_data_layout = new nux::VLayout();
  prize_data_layout->SetMaximumHeight(76);
  prize_data_layout->SetSpaceBetweenChildren(5);

  purchase_prize_ = new StaticCairoText(
          payment_preview_model_->purchase_prize.Get(), true,
          NUX_TRACKER_LOCATION);
  purchase_prize_->SetLines(-1);
  purchase_prize_->SetFont(style.payment_prize_title_font().c_str());
  prize_data_layout->AddView(purchase_prize_.GetPointer(), 1,
          nux::MINOR_POSITION_END);

  purchase_hint_ = new StaticCairoText(
          _("Ubuntu One best offer"),
          true, NUX_TRACKER_LOCATION);
  purchase_hint_->SetLines(-1);
  purchase_hint_->SetFont(style.payment_prize_subtitle_font().c_str());
  prize_data_layout->AddView(purchase_hint_.GetPointer(), 1,
          nux::MINOR_POSITION_END);

  purchase_type_ = new StaticCairoText(
          payment_preview_model_->purchase_type.Get(), true,
          NUX_TRACKER_LOCATION);
  purchase_type_->SetLines(-1);
  purchase_type_->SetFont(style.payment_prize_subtitle_font().c_str());
  prize_data_layout->AddView(purchase_type_.GetPointer(), 1,
          nux::MINOR_POSITION_END);
  return prize_data_layout;
}

nux::Layout* MusicPaymentPreview::GetBody()
{
  previews::Style& style = dash::previews::Style::Instance();

  nux::VLayout *body_layout = new  nux::VLayout();
  body_layout->SetSpaceBetweenChildren(20);

  intro_ = new StaticCairoText(
          payment_preview_model_->header.Get(), true,
          NUX_TRACKER_LOCATION);
  intro_->SetFont(style.payment_intro_font().c_str());
  intro_->SetLineSpacing(10);
  intro_->SetLines(-style.GetDescriptionLineCount());
  intro_->SetMinimumHeight(50);

  form_layout_ = new nux::HLayout();
  form_layout_->SetSpaceBetweenChildren(10);
  form_layout_->SetMinimumHeight(107);
  form_layout_->SetLeftAndRightPadding(20);
  form_layout_->SetTopAndBottomPadding(10);

  form_layout_->AddLayout(GetFormLabels(), 1, nux::MINOR_POSITION_END);
  form_layout_->AddLayout(GetFormFields(), 1, nux::MINOR_POSITION_END);
  form_layout_->AddLayout(GetFormActions(), 1, nux::MINOR_POSITION_END);

  body_layout->AddView(intro_.GetPointer(), 1);
  body_layout->AddLayout(form_layout_.GetPointer(), 1);

  return body_layout;
}

nux::Layout* MusicPaymentPreview::GetFormLabels()
{
  previews::Style& style = dash::previews::Style::Instance();
  nux::VLayout *labels_layout = new nux::VLayout();
  labels_layout->SetSpaceBetweenChildren(18);
  email_label_ = new StaticCairoText(
          _("Ubuntu One email:"), true,
          NUX_TRACKER_LOCATION);
  email_label_->SetLines(-1);
  email_label_->SetFont(style.payment_form_labels_font().c_str());
  labels_layout->AddView(email_label_.GetPointer(), 1, nux::MINOR_POSITION_END);

  payment_label_ = new StaticCairoText(
          _("Payment method:"), true,
          NUX_TRACKER_LOCATION);
  payment_label_->SetLines(-1);
  payment_label_->SetFont(style.payment_form_labels_font().c_str());
  labels_layout->AddView(payment_label_.GetPointer(), 1, nux::MINOR_POSITION_END);

  password_label_ = new StaticCairoText(
          _("Ubuntu One Password:"), true,
          NUX_TRACKER_LOCATION);
  password_label_->SetLines(-1);
  password_label_->SetFont(style.payment_form_labels_font().c_str());
  password_label_->SetMinimumHeight(40);
  labels_layout->AddView(password_label_.GetPointer(), 1, nux::MINOR_POSITION_END);

  return labels_layout;
}

nux::Layout* MusicPaymentPreview::GetFormFields()
{
  previews::Style& style = dash::previews::Style::Instance();
  nux::VLayout *fields_layout = new nux::VLayout();
  fields_layout->SetSpaceBetweenChildren(18);
  email_ = new StaticCairoText(
          payment_preview_model_->email.Get(), true,
          NUX_TRACKER_LOCATION);
  email_->SetLines(-1);
  email_->SetFont(style.payment_form_data_font().c_str());
  fields_layout->AddView(email_.GetPointer(), 1,
                  nux::MINOR_POSITION_START);

  payment_ = new StaticCairoText(
          payment_preview_model_->payment_method.Get(), true,
          NUX_TRACKER_LOCATION);
  payment_->SetLines(-1);
  payment_->SetFont(style.payment_form_data_font().c_str());
  fields_layout->AddView(payment_.GetPointer(), 1,
                  nux::MINOR_POSITION_START);

  password_entry_ = new TextInput();
  password_entry_->SetMinimumHeight(40);
  password_entry_->SetMinimumWidth(240);
  password_entry_->input_hint = _("Password");

  fields_layout->AddView(password_entry_.GetPointer(),
          1, nux::MINOR_POSITION_START);

  password_entry_->text_entry()->SetPasswordMode(true);
  const char password_char = '*';
  password_entry_->text_entry()->SetPasswordChar(&password_char);

  return fields_layout;
}

nux::Layout* MusicPaymentPreview::GetFormActions()
{
  previews::Style& style = dash::previews::Style::Instance();
  nux::VLayout *actions_layout = new nux::VLayout();
  actions_layout->SetSpaceBetweenChildren(16);

  nux::ObjectPtr<StaticCairoText> empty_;
  empty_ = new StaticCairoText(
          "", true,
          NUX_TRACKER_LOCATION);
  empty_->SetLines(-1);
  empty_->SetFont(style.payment_form_labels_font().c_str());
  actions_layout->AddView(empty_.GetPointer(), 1,
                  nux::MINOR_POSITION_START);

  actions_layout->AddView(
          sorted_buttons_[CHANGE_PAYMENT_ACTION].GetPointer(),
          1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL,
      100.0f, nux::NUX_LAYOUT_END);
  actions_layout->AddView(
           sorted_buttons_[FORGOT_PASSWORD_ACTION].GetPointer(),
          1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL,
      100.0f, nux::NUX_LAYOUT_END);

  return actions_layout;
}

nux::Layout* MusicPaymentPreview::GetEmail()
{
  nux::HLayout *email_data_layout = new nux::HLayout();
  return email_data_layout;
}

nux::Layout* MusicPaymentPreview::GetPayment()
{
  nux::HLayout *payment_data_layout = new nux::HLayout();
  payment_data_layout->SetSpaceBetweenChildren(5);

  payment_data_layout->AddSpace(20, 0);


  payment_data_layout->AddSpace(20, 0);

  return payment_data_layout;
}

nux::Layout* MusicPaymentPreview::GetPassword()
{
  nux::HLayout *password_data_layout = new nux::HLayout();
  password_data_layout->SetSpaceBetweenChildren(5);

  password_data_layout->AddSpace(20, 0);


  password_data_layout->AddSpace(20, 0);
  return password_data_layout;
}

nux::Layout* MusicPaymentPreview::GetFooter()
{
  previews::Style& style = dash::previews::Style::Instance();
  nux::HLayout* actions_buffer_h = new nux::HLayout();
  actions_buffer_h->AddSpace(0, 1);

  nux::HLayout* buttons_data_layout = new nux::HLayout();
  buttons_data_layout->SetSpaceBetweenChildren(style.GetSpaceBetweenActions());

  lock_texture_ = new IconTexture(style.GetLockIcon(), style.GetPaymentLockWidth(),
          style.GetPaymentLockHeight());
  buttons_data_layout->AddView(lock_texture_, 0, nux::MINOR_POSITION_CENTER,
          nux::MINOR_SIZE_FULL, 100.0f, nux::NUX_LAYOUT_BEGIN);

  buttons_data_layout->AddSpace(20, 1);
  buttons_data_layout->AddView(sorted_buttons_[CANCEL_PURCHASE_ACTION].GetPointer(),
          1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 100.0f,
          nux::NUX_LAYOUT_END);
  buttons_data_layout->AddView(sorted_buttons_[PURCHASE_ALBUM_ACTION].GetPointer(),
          1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 100.0f,
          nux::NUX_LAYOUT_END);

  return buttons_data_layout;
}

void MusicPaymentPreview::PreLayoutManagement()
{
  nux::Geometry geo = GetGeometry();
  GetLayout()->SetGeometry(geo);

  previews::Style& style = dash::previews::Style::Instance();

  int width = MAX(0, geo.width - style.GetPanelSplitWidth() - style.GetDetailsLeftMargin() - style.GetDetailsRightMargin());

  if(full_data_layout_) { full_data_layout_->SetMaximumWidth(width); }
  if(header_layout_) { header_layout_->SetMaximumWidth(width); }
  if(intro_) { intro_->SetMaximumWidth(width); }
  if(form_layout_) { form_layout_->SetMaximumWidth(width); }
  if(footer_layout_) { footer_layout_->SetMaximumWidth(width); }

  // set the tab ordering
  SetFirstInTabOrder(password_entry_->text_entry());
  SetLastInTabOrder(sorted_buttons_[CANCEL_PURCHASE_ACTION].GetPointer());
  SetLastInTabOrder(sorted_buttons_[PURCHASE_ALBUM_ACTION].GetPointer());
  SetLastInTabOrder(sorted_buttons_[CHANGE_PAYMENT_ACTION].GetPointer());
  SetLastInTabOrder(sorted_buttons_[FORGOT_PASSWORD_ACTION].GetPointer());

  Preview::PreLayoutManagement();
}

void MusicPaymentPreview::SetupViews()
{
  payment_preview_model_ = dynamic_cast<dash::PaymentPreview*>(preview_model_.get());
  if (!payment_preview_model_)
  {
    LOG_ERROR(logger) << "Could not derive preview model from given parameter.";
    return;
  }

  // load the buttons so that they can be accessed in order
  LoadActions();

  // TODO: change api so that this makes more sense


  PaymentPreview::SetupViews();
}


}
}
}
