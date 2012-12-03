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
 * Authored by: Diego Sarmentero <diego.sarmentero@canonical.com>
 */

#include <unity-protocol.h>

#include "ErrorPreview.h"

namespace unity
{
namespace dash
{

class ErrorPreview::Impl
{
public:
  Impl(ErrorPreview* owner, glib::Object<GObject> const& proto_obj);

  void SetupGetters();

  // getters for the data properties

  std::string get_title() const { return title_; };
  std::string get_subtitle() const { return subtitle_; };
  std::string get_header() const { return header_; };
  std::string get_purchase_prize() const { return purchase_prize_; };
  std::string get_purchase_type() const { return purchase_type_; };

  // getters for the lables

  // instance vars
  ErrorPreview* owner_;

  std::string title_;
  std::string subtitle_;
  std::string header_;
  std::string purchase_prize_;
  std::string purchase_type_;
};

ErrorPreview::Impl::Impl(ErrorPreview* owner, glib::Object<GObject> const& proto_obj)
  : owner_(owner)
{
    const gchar* s;
    auto preview = glib::object_cast<UnityProtocolErrorPreview>(proto_obj);

    s = unity_protocol_error_preview_get_title(preview);
    if (s) title_ = s;
    s = unity_protocol_error_preview_get_subtitle(preview);
    if (s) subtitle_ = s;
    s = unity_protocol_error_preview_get_header(preview);
    if (s) header_ = s;
    s = unity_protocol_error_preview_get_purchase_prize(preview);
    if (s) purchase_prize_ = s;
    s = unity_protocol_error_preview_get_purchase_type(preview);
    if (s) purchase_type_ = s;

    SetupGetters();
}

void ErrorPreview::Impl::SetupGetters()
{
  owner_->title.SetGetterFunction(
            sigc::mem_fun(this, &ErrorPreview::Impl::get_title));
  owner_->subtitle.SetGetterFunction(
            sigc::mem_fun(this, &ErrorPreview::Impl::get_subtitle));
  owner_->header.SetGetterFunction(
            sigc::mem_fun(this, &ErrorPreview::Impl::get_header));
  owner_->purchase_prize.SetGetterFunction(
            sigc::mem_fun(this, &ErrorPreview::Impl::get_purchase_prize));
  owner_->purchase_type.SetGetterFunction(
            sigc::mem_fun(this, &ErrorPreview::Impl::get_purchase_type));
}

ErrorPreview::ErrorPreview(unity::glib::Object<GObject> const& proto_obj)
  : PaymentPreview(proto_obj)
  , pimpl(new Impl(this, proto_obj))
{
}

ErrorPreview::~ErrorPreview()
{
}


}
}
