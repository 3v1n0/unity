// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2013 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Manuel de la Pena <manuel.delapena@canonical.com>
 */

#include <unity-protocol.h>

#include "PaymentPreview.h"

namespace unity
{
namespace dash
{

class PaymentPreview::Impl
{

public:
  Impl(PaymentPreview* owner, glib::Object<GObject> const& proto_obj);

  void SetupGetters();

  // getters for the data properties

  std::string get_header() const { return header_; };
  std::string get_email() const { return email_; };
  std::string get_payment_method() const { return payment_method_; };
  std::string get_purchase_prize() const { return purchase_prize_; };
  std::string get_purchase_type() const { return purchase_type_; };
  PreviewType get_preview_type() const { return preview_type_; };

  // getters for the lables

  // instance vars
  PaymentPreview* owner_;

  std::string header_;
  std::string email_;
  std::string payment_method_;
  std::string purchase_prize_;
  std::string purchase_type_;
  PaymentPreview::PreviewType preview_type_;

};

PaymentPreview::Impl::Impl(PaymentPreview* owner, glib::Object<GObject> const& proto_obj)
  : owner_(owner)
{
  const gchar* s;
  auto preview = glib::object_cast<UnityProtocolPaymentPreview>(proto_obj);

  s = unity_protocol_payment_preview_get_header(preview);
  if (s) header_ = s;
  s = unity_protocol_payment_preview_get_email(preview);
  if (s) email_ = s;
  s = unity_protocol_payment_preview_get_email(preview);
  if (s) email_ = s;
  s = unity_protocol_payment_preview_get_payment_method(preview);
  if (s) payment_method_ = s;
  s = unity_protocol_payment_preview_get_purchase_prize(preview);
  if (s) purchase_prize_ = s;
  s = unity_protocol_payment_preview_get_purchase_type(preview);
  if (s) purchase_type_ = s;
  UnityProtocolPreviewPaymentType t = unity_protocol_payment_preview_get_preview_type(preview);
  switch(t)
  {
    case UNITY_PROTOCOL_PREVIEW_PAYMENT_TYPE_APPLICATION:
      preview_type_ = PaymentPreview::APPLICATION;
      break;
    case UNITY_PROTOCOL_PREVIEW_PAYMENT_TYPE_MUSIC:
      preview_type_ = PaymentPreview::MUSIC;
      break;
    case UNITY_PROTOCOL_PREVIEW_PAYMENT_TYPE_ERROR:
      preview_type_ = PaymentPreview::ERROR;
      break;
  }
  SetupGetters();
}

void PaymentPreview::Impl::SetupGetters()
{
  owner_->header.SetGetterFunction(
            sigc::mem_fun(this, &PaymentPreview::Impl::get_header));
  owner_->email.SetGetterFunction(
            sigc::mem_fun(this, &PaymentPreview::Impl::get_email));
  owner_->payment_method.SetGetterFunction(
            sigc::mem_fun(this, &PaymentPreview::Impl::get_payment_method));
  owner_->purchase_prize.SetGetterFunction(
            sigc::mem_fun(this, &PaymentPreview::Impl::get_purchase_prize));
  owner_->purchase_type.SetGetterFunction(
            sigc::mem_fun(this, &PaymentPreview::Impl::get_purchase_type));
  owner_->preview_type.SetGetterFunction(
            sigc::mem_fun(this, &PaymentPreview::Impl::get_preview_type));
}

PaymentPreview::PaymentPreview(unity::glib::Object<GObject> const& proto_obj)
  : Preview(proto_obj)
  , pimpl(new Impl(this, proto_obj))
{
}

PaymentPreview::~PaymentPreview()
{
}


}
}
