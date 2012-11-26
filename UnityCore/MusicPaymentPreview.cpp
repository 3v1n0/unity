// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2012 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *              Michal Hruby <michal.hruby@canonical.com>
 */

#include <unity-protocol.h>

#include "MusicPaymentPreview.h"

namespace unity
{
namespace dash
{

class MusicPaymentPreview::Impl
{

public:
  Impl(MusicPaymentPreview* owner, glib::Object<GObject> const& proto_obj);

  void SetupGetters();

  // getters for the data properties

  std::string get_title() const { return title_; };
  std::string get_subtitle() const { return subtitle_; };
  std::string get_header() const { return header_; };
  std::string get_email() const { return email_; };
  std::string get_payment_method() const { return payment_method_; };
  std::string get_purchase_prize() const { return purchase_prize_; };
  std::string get_purchase_type() const { return purchase_type_; };

  // getters for the lables

  // instance vars
  MusicPaymentPreview* owner_;

  std::string title_;
  std::string subtitle_;
  std::string header_;
  std::string email_;
  std::string payment_method_;
  std::string purchase_prize_;
  std::string purchase_type_;

};

MusicPaymentPreview::Impl::Impl(MusicPaymentPreview* owner, glib::Object<GObject> const& proto_obj)
  : owner_(owner)
{
  const gchar* s;
  auto preview = glib::object_cast<UnityProtocolMusicPaymentPreview>(proto_obj);

  s = unity_protocol_music_payment_preview_get_title(preview);
  if (s) title_ = s;
  s = unity_protocol_music_payment_preview_get_subtitle(preview);
  if (s) subtitle_ = s;
  s = unity_protocol_music_payment_preview_get_header(preview);
  if (s) header_ = s;
  s = unity_protocol_music_payment_preview_get_email(preview);
  if (s) email_ = s;
  s = unity_protocol_music_payment_preview_get_email(preview);
  if (s) email_ = s;
  s = unity_protocol_music_payment_preview_get_payment_method(preview);
  if (s) payment_method_ = s;
  s = unity_protocol_music_payment_preview_get_purchase_prize(preview);
  if (s) purchase_prize_ = s;
  s = unity_protocol_music_payment_preview_get_purchase_type(preview);
  if (s) purchase_type_ = s;

  SetupGetters();
}

void MusicPaymentPreview::Impl::SetupGetters()
{
  owner_->title.SetGetterFunction(
            sigc::mem_fun(this, &MusicPaymentPreview::Impl::get_title));
  owner_->subtitle.SetGetterFunction(
            sigc::mem_fun(this, &MusicPaymentPreview::Impl::get_subtitle));
  owner_->header.SetGetterFunction(
            sigc::mem_fun(this, &MusicPaymentPreview::Impl::get_header));
  owner_->email.SetGetterFunction(
            sigc::mem_fun(this, &MusicPaymentPreview::Impl::get_email));
  owner_->payment_method.SetGetterFunction(
            sigc::mem_fun(this, &MusicPaymentPreview::Impl::get_payment_method));
  owner_->purchase_prize.SetGetterFunction(
            sigc::mem_fun(this, &MusicPaymentPreview::Impl::get_purchase_prize));
  owner_->purchase_type.SetGetterFunction(
            sigc::mem_fun(this, &MusicPaymentPreview::Impl::get_purchase_type));
}

MusicPaymentPreview::MusicPaymentPreview(unity::glib::Object<GObject> const& proto_obj)
  : PaymentPreview(proto_obj)
{
}

MusicPaymentPreview::~MusicPaymentPreview()
{
}


}
}
