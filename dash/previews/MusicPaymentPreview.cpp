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
 * Authored by: Manuel de la Pena <manuel.delapena@canonical.com>
 *
 */

#include "unity-shared/IntrospectableWrappers.h"
#include "unity-shared/PreviewStyle.h"
#include "unity-shared/CoverArt.h"
#include "unity-shared/StaticCairoText.h"
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
nux::logging::Logger logger("unity.dash.previews.payment.preview.music");

const RawPixel DATA_MAX_HEIGHT = 76_em;
const RawPixel TITLE_CHILDREN_SPACE = 10_em;
const RawPixel PRIZE_CHILDREN_SPACE = 5_em;
const RawPixel TITLE_MAX_WIDTH = 480_em;
const RawPixel INTRO_MIN_HEIGHT = 50_em;
const RawPixel FORM_MIN_HEIGHT = 107_em;
const RawPixel FORM_PADDING = 20_em;
const RawPixel LABELS_CHILDREN_SPACE = 18_em;
const RawPixel PASSWORD_MIN_HEIGHT = 40_em;
const RawPixel PASSWORD_MIN_WIDTH = 240_em;
const RawPixel ACTIONS_CHILDREN_SPACE_MAX = 16_em;
const RawPixel ACTIONS_CHILDREN_SPACE_MIN = 8_em;
const RawPixel BUTTONS_SPACE = 20_em;
const RawPixel HEADER_CHILDREN_SPACE = 10_em;
const RawPixel HEADER_MAX_SIZE = 76_em;
const RawPixel BODY_CHILDREN_SPACE = 20_em;
}

// static string definitions
const std::string MusicPaymentPreview::DATA_INFOHINT_ID = "album_purchase_preview";
const std::string MusicPaymentPreview::DATA_PASSWORD_KEY = "password";
const std::string MusicPaymentPreview::CHANGE_PAYMENT_ACTION = "change_payment_method";
const std::string MusicPaymentPreview::FORGOT_PASSWORD_ACTION = "forgot_password";
const std::string MusicPaymentPreview::CANCEL_PURCHASE_ACTION = "cancel_purchase";
const std::string MusicPaymentPreview::PURCHASE_ALBUM_ACTION = "purchase_album";

NUX_IMPLEMENT_OBJECT_TYPE(MusicPaymentPreview)

MusicPaymentPreview::MusicPaymentPreview(dash::Preview::Ptr preview_model)
: PaymentPreview(preview_model)
{
  SetupViews();
  PaymentPreview::SetupBackground();
  UpdateScale(scale);
}

std::string MusicPaymentPreview::GetName() const
{
  return "MusicPaymentPreview";
}

void MusicPaymentPreview::AddProperties(debug::IntrospectionData& introspection)
{
  PaymentPreview::AddProperties(introspection);
}

void MusicPaymentPreview::OnActionActivated(ActionButton* button, std::string const& id)
{
  // Check the action id and send the password only when we
  // purchasing a song
  if(id == MusicPaymentPreview::PURCHASE_ALBUM_ACTION && preview_model_
          && password_entry_)
  {
    // HACK: We need to think a better way to do this
    auto const& password = password_entry_->text_entry()->GetText();
    glib::HintsMap hints {
      {MusicPaymentPreview::DATA_PASSWORD_KEY, glib::Variant(password)}
    };
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
      if(MusicPaymentPreview::CHANGE_PAYMENT_ACTION == action_id
              || MusicPaymentPreview::FORGOT_PASSWORD_ACTION == action_id)
      {
        nux::ObjectPtr<ActionLink> link = this->CreateLink(action);
        link->scale = scale();
        link->activate.connect(sigc::mem_fun(this,
                    &MusicPaymentPreview::OnActionLinkActivated));

        buttons_map_.insert(std::make_pair(action->id, link));
      }
      else
      {
        nux::ObjectPtr<ActionButton> button = this->CreateButton(action);
        button->activate.connect(sigc::mem_fun(this,
                    &MusicPaymentPreview::OnActionActivated));
        button->scale = scale();

        buttons_map_.insert(std::make_pair(action->id, button));
      }
      LOG_DEBUG(logger) << "added button for action with id '" << action->id << "'";
  }
}

nux::Layout* MusicPaymentPreview::GetTitle()
{
  previews::Style& style = dash::previews::Style::Instance();
  nux::VLayout* title_data_layout = new nux::VLayout();
  title_data_layout->SetMaximumHeight(DATA_MAX_HEIGHT.CP(scale));
  title_data_layout->SetSpaceBetweenChildren(TITLE_CHILDREN_SPACE.CP(scale));

  title_ = new StaticCairoText(
          preview_model_->title.Get(), true,
          NUX_TRACKER_LOCATION);

  title_->SetFont(style.payment_title_font());
  title_->SetLines(-1);
  title_->SetScale(scale);
  title_->SetFont(style.title_font());
  title_->SetMaximumWidth(TITLE_MAX_WIDTH.CP(scale));
  title_->SetTextEllipsize(StaticCairoText::EllipsizeState::NUX_ELLIPSIZE_END);
  title_data_layout->AddView(title_.GetPointer(), 1);

  subtitle_ = new StaticCairoText(
          preview_model_->subtitle.Get(), true,
          NUX_TRACKER_LOCATION);
  subtitle_->SetLines(-1);
  subtitle_->SetScale(scale);
  subtitle_->SetFont(style.payment_subtitle_font());
  title_data_layout->AddView(subtitle_.GetPointer(), 1);
  title_data_layout->AddSpace(1, 1);
  return title_data_layout;
}

nux::Layout* MusicPaymentPreview::GetPrice()
{
  previews::Style& style = dash::previews::Style::Instance();
  nux::VLayout *prize_data_layout = new nux::VLayout();
  prize_data_layout->SetMaximumHeight(DATA_MAX_HEIGHT.CP(scale));
  prize_data_layout->SetSpaceBetweenChildren(PRIZE_CHILDREN_SPACE.CP(scale));
  prize_data_layout->SetPadding(0, TITLE_CHILDREN_SPACE.CP(scale), 0, 0);

  purchase_prize_ = new StaticCairoText(
          payment_preview_model_->purchase_prize.Get(), true,
          NUX_TRACKER_LOCATION);
  purchase_prize_->SetLines(-1);
  purchase_prize_->SetScale(scale);
  purchase_prize_->SetFont(style.payment_prize_title_font());
  prize_data_layout->AddView(purchase_prize_.GetPointer(), 1,
          nux::MINOR_POSITION_END);

  purchase_hint_ = new StaticCairoText(
          _("Ubuntu One best offer"),
          true, NUX_TRACKER_LOCATION);
  purchase_hint_->SetLines(-1);
  purchase_hint_->SetScale(scale);
  purchase_hint_->SetFont(style.payment_prize_subtitle_font());
  prize_data_layout->AddView(purchase_hint_.GetPointer(), 1,
          nux::MINOR_POSITION_END);

  purchase_type_ = new StaticCairoText(
          payment_preview_model_->purchase_type.Get(), true,
          NUX_TRACKER_LOCATION);
  purchase_type_->SetLines(-1);
  purchase_type_->SetScale(scale);
  purchase_type_->SetFont(style.payment_prize_subtitle_font());
  prize_data_layout->AddView(purchase_type_.GetPointer(), 1,
          nux::MINOR_POSITION_END);
  return prize_data_layout;
}

nux::Layout* MusicPaymentPreview::GetBody()
{
  previews::Style& style = dash::previews::Style::Instance();
  nux::VLayout *body_layout = new  nux::VLayout();
  body_layout->SetSpaceBetweenChildren(BODY_CHILDREN_SPACE.CP(scale));

  intro_ = new StaticCairoText(
          payment_preview_model_->header.Get(), true,
          NUX_TRACKER_LOCATION);
  intro_->SetFont(style.payment_intro_font());
  intro_->SetScale(scale);
  intro_->SetLineSpacing(TITLE_CHILDREN_SPACE.CP(scale));
  intro_->SetLines(-style.GetDescriptionLineCount());
  intro_->SetMinimumHeight(INTRO_MIN_HEIGHT.CP(scale));

  form_layout_ = new nux::HLayout();
  form_layout_->SetSpaceBetweenChildren(TITLE_CHILDREN_SPACE.CP(scale));
  form_layout_->SetMinimumHeight(FORM_MIN_HEIGHT.CP(scale));
  form_layout_->SetLeftAndRightPadding(FORM_PADDING.CP(scale));
  form_layout_->SetTopAndBottomPadding(TITLE_CHILDREN_SPACE.CP(scale));

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
  if (error_message_.empty())
  {
    labels_layout->SetSpaceBetweenChildren(LABELS_CHILDREN_SPACE.CP(scale));
  }
  else
  {
    labels_layout->SetSpaceBetweenChildren(TITLE_CHILDREN_SPACE.CP(scale));
  }

  email_label_ = new StaticCairoText(
          _("Ubuntu One email:"), true,
          NUX_TRACKER_LOCATION);
  email_label_->SetLines(-1);
  email_label_->SetScale(scale);
  email_label_->SetFont(style.payment_form_labels_font());
  labels_layout->AddView(email_label_.GetPointer(), 0, nux::MINOR_POSITION_END);

  payment_label_ = new StaticCairoText(
          _("Payment method:"), true,
          NUX_TRACKER_LOCATION);
  payment_label_->SetLines(-1);
  payment_label_->SetScale(scale);
  payment_label_->SetFont(style.payment_form_labels_font());
  labels_layout->AddView(payment_label_.GetPointer(), 0, nux::MINOR_POSITION_END);

  password_label_ = new StaticCairoText(
          _("Ubuntu One password:"), true,
          NUX_TRACKER_LOCATION);
  password_label_->SetLines(-1);
  password_label_->SetScale(scale);
  password_label_->SetFont(style.payment_form_labels_font());
  password_label_->SetMinimumHeight(PASSWORD_MIN_HEIGHT.CP(scale));
  labels_layout->AddView(password_label_.GetPointer(), 0, nux::MINOR_POSITION_END);

  return labels_layout;
}

nux::Layout* MusicPaymentPreview::GetFormFields()
{
  previews::Style& style = dash::previews::Style::Instance();
  nux::VLayout *fields_layout = new nux::VLayout();
  if (error_message_.empty())
  {
    fields_layout->SetSpaceBetweenChildren(LABELS_CHILDREN_SPACE.CP(scale));
  }
  else
  {
    fields_layout->SetSpaceBetweenChildren(TITLE_CHILDREN_SPACE.CP(scale));
  }

  email_ = new StaticCairoText(
          payment_preview_model_->email.Get(), true,
          NUX_TRACKER_LOCATION);
  email_->SetLines(-1);
  email_->SetScale(scale);
  email_->SetFont(style.payment_form_data_font());
  fields_layout->AddView(email_.GetPointer(), 1,
                  nux::MINOR_POSITION_START);

  payment_ = new StaticCairoText(
          payment_preview_model_->payment_method.Get(), true,
          NUX_TRACKER_LOCATION);
  payment_->SetLines(-1);
  payment_->SetScale(scale);
  payment_->SetFont(style.payment_form_data_font());
  fields_layout->AddView(payment_.GetPointer(), 1,
                  nux::MINOR_POSITION_START);

  password_entry_ = new TextInput();
  password_entry_->SetMinimumHeight(PASSWORD_MIN_HEIGHT.CP(scale));
  password_entry_->SetMinimumWidth(PASSWORD_MIN_WIDTH.CP(scale));
  password_entry_->input_hint = _("Password");

  fields_layout->AddView(password_entry_.GetPointer(),
          1, nux::MINOR_POSITION_START);

  password_entry_->text_entry()->SetPasswordMode(true);
  const char password_char = '*';
  password_entry_->text_entry()->SetPasswordChar(&password_char);

  if (!error_message_.empty())
  {
    StaticCairoText* error = new StaticCairoText(
            _("Wrong password"), true, NUX_TRACKER_LOCATION);
    error->SetLines(-1);
    error->SetScale(scale);
    error->SetFont(style.payment_form_data_font());
    // ensure it is an error using red
    error->SetTextColor(style.payment_error_color());
    fields_layout->AddView(error, 0, nux::MINOR_POSITION_START);
  }

  return fields_layout;
}

nux::Layout* MusicPaymentPreview::GetFormActions()
{
  previews::Style& style = dash::previews::Style::Instance();
  nux::VLayout *actions_layout = new nux::VLayout();
  if (error_message_.empty())
  {
    actions_layout->SetSpaceBetweenChildren(ACTIONS_CHILDREN_SPACE_MAX.CP(scale));
  }
  else
  {
    actions_layout->SetSpaceBetweenChildren(ACTIONS_CHILDREN_SPACE_MIN.CP(scale));
  }

  nux::ObjectPtr<StaticCairoText> empty_;
  empty_ = new StaticCairoText(
          "", true,
          NUX_TRACKER_LOCATION);
  empty_->SetLines(-1);
  empty_->SetScale(scale);
  empty_->SetFont(style.payment_form_labels_font());
  actions_layout->AddView(empty_.GetPointer(), 1,
                  nux::MINOR_POSITION_START);

  if(buttons_map_[MusicPaymentPreview::CHANGE_PAYMENT_ACTION].GetPointer())
    actions_layout->AddView(
            buttons_map_[MusicPaymentPreview::CHANGE_PAYMENT_ACTION].GetPointer(),
            1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL,
            100.0f, nux::NUX_LAYOUT_END);
  if(buttons_map_[MusicPaymentPreview::FORGOT_PASSWORD_ACTION].GetPointer())
    actions_layout->AddView(
            buttons_map_[MusicPaymentPreview::FORGOT_PASSWORD_ACTION].GetPointer(),
            1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL,
            100.0f, nux::NUX_LAYOUT_END);

  return actions_layout;
}

nux::Layout* MusicPaymentPreview::GetFooter()
{
  previews::Style& style = dash::previews::Style::Instance();
  nux::HLayout* actions_buffer_h = new nux::HLayout();
  actions_buffer_h->AddSpace(0, 1);

  nux::HLayout* buttons_data_layout = new nux::HLayout();
  buttons_data_layout->SetSpaceBetweenChildren(style.GetSpaceBetweenActions().CP(scale));

  lock_texture_ = new IconTexture(style.GetLockIcon(), style.GetPaymentLockWidth().CP(scale),
          style.GetPaymentLockHeight().CP(scale));
  buttons_data_layout->AddView(lock_texture_.GetPointer(), 0, nux::MINOR_POSITION_CENTER,
          nux::MINOR_SIZE_FULL, 100.0f, nux::NUX_LAYOUT_BEGIN);

  buttons_data_layout->AddSpace(BUTTONS_SPACE.CP(scale), 1);
  if(buttons_map_[MusicPaymentPreview::CANCEL_PURCHASE_ACTION].GetPointer())
    buttons_data_layout->AddView(buttons_map_[MusicPaymentPreview::CANCEL_PURCHASE_ACTION].GetPointer(),
            1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 100.0f,
            nux::NUX_LAYOUT_END);
  if(buttons_map_[MusicPaymentPreview::PURCHASE_ALBUM_ACTION].GetPointer())
    buttons_data_layout->AddView(buttons_map_[MusicPaymentPreview::PURCHASE_ALBUM_ACTION].GetPointer(),
            1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL, 100.0f,
            nux::NUX_LAYOUT_END);

  return buttons_data_layout;
}

const std::string MusicPaymentPreview::GetErrorMessage(GVariant *dict)
{

  glib::Variant data(g_variant_lookup_value(dict, "error_message",
    G_VARIANT_TYPE_ANY));

  if (!data)
    return "";

  return data.GetString();
}

void MusicPaymentPreview::PreLayoutManagement()
{
  nux::Geometry const& geo = GetGeometry();
  GetLayout()->SetGeometry(geo);

  previews::Style& style = dash::previews::Style::Instance();

  int content_width = geo.width - style.GetPanelSplitWidth().CP(scale) - style.GetDetailsLeftMargin().CP(scale) - style.GetDetailsRightMargin().CP(scale);
  int width = std::max<int>(0, content_width);

  if(full_data_layout_) { full_data_layout_->SetMaximumWidth(width); }
  if(header_layout_) { header_layout_->SetMaximumWidth(width); }
  if(intro_) { intro_->SetMaximumWidth(width); }
  if(form_layout_) { form_layout_->SetMaximumWidth(width); }
  if(footer_layout_) { footer_layout_->SetMaximumWidth(width); }

  // set the tab ordering
  SetFirstInTabOrder(password_entry_->text_entry());
  SetLastInTabOrder(buttons_map_[MusicPaymentPreview::CANCEL_PURCHASE_ACTION].GetPointer());
  SetLastInTabOrder(buttons_map_[MusicPaymentPreview::PURCHASE_ALBUM_ACTION].GetPointer());
  SetLastInTabOrder(buttons_map_[MusicPaymentPreview::CHANGE_PAYMENT_ACTION].GetPointer());
  SetLastInTabOrder(buttons_map_[MusicPaymentPreview::FORGOT_PASSWORD_ACTION].GetPointer());

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

  dash::Preview::InfoHintPtrList hints = preview_model_->GetInfoHints();
  GVariant *preview_data = NULL;
  for (dash::Preview::InfoHintPtr info_hint : hints)
  {
    if (info_hint->id == MusicPaymentPreview::DATA_INFOHINT_ID)
    {
      preview_data = info_hint->value;
      if (preview_data != NULL)
      {
        error_message_ = GetErrorMessage(preview_data);
      }
      break;
    }
  }

  // load the buttons so that they can be accessed in order
  LoadActions();
  PaymentPreview::SetupViews();
}

void MusicPaymentPreview::UpdateScale(double scale)
{
  PaymentPreview::UpdateScale(scale);

  if (intro_)
    intro_->SetScale(scale);
  if (email_label_)
    email_label_->SetScale(scale);
  if (payment_label_)
    payment_label_->SetScale(scale);
  if (payment_)
    payment_->SetScale(scale);
  if (password_label_)
    password_label_->SetScale(scale);
  if (purchase_hint_)
    purchase_hint_->SetScale(scale);
  if (purchase_prize_)
    purchase_prize_->SetScale(scale);
  if (purchase_type_)
    purchase_type_->SetScale(scale);
  if (change_payment_)
    change_payment_->SetScale(scale);
  if (error_label_)
    error_label_->SetScale(scale);

  previews::Style& style = dash::previews::Style::Instance();
  if (lock_texture_)
    lock_texture_->SetSize(std::max(style.GetPaymentLockWidth().CP(scale), style.GetPaymentLockHeight().CP(scale)));

  if (password_entry_)
  {
    password_entry_->SetMinimumHeight(PASSWORD_MIN_HEIGHT.CP(scale));
    password_entry_->SetMinimumWidth(PASSWORD_MIN_WIDTH.CP(scale));
  }

  if (form_layout_)
  {
    form_layout_->SetSpaceBetweenChildren(TITLE_CHILDREN_SPACE.CP(scale));
    form_layout_->SetMinimumHeight(FORM_MIN_HEIGHT.CP(scale));
    form_layout_->SetLeftAndRightPadding(FORM_PADDING.CP(scale));
    form_layout_->SetTopAndBottomPadding(TITLE_CHILDREN_SPACE.CP(scale));
  }
}

}
}
}
