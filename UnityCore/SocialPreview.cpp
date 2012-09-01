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
 * along with this program.  If not, see <http://www.gnu.org/contents/>.
 *
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *              Michal Hruby <michal.hruby@canonical.com>
 */

#include <unity-protocol.h>

#include "SocialPreview.h"

namespace unity
{
namespace dash
{

class SocialPreview::Impl
{
public:
  Impl(SocialPreview* owner, glib::Object<GObject> const& proto_obj);

  void SetupGetters();
  std::string get_sender() { return sender_; };
  std::string get_content() { return content_; };
  glib::Object<GIcon> get_avatar() { return avatar_; };

  SocialPreview* owner_;

  std::string sender_;
  std::string content_;
  glib::Object<GIcon> avatar_;
};

SocialPreview::Impl::Impl(SocialPreview* owner, glib::Object<GObject> const& proto_obj)
  : owner_(owner)
{
  const gchar* s;
  auto preview = glib::object_cast<UnityProtocolSocialPreview>(proto_obj);

  s = unity_protocol_social_preview_get_sender(preview);
  if (s) sender_ = s;
  s = unity_protocol_social_preview_get_content(preview);
  if (s) content_ = s;

  glib::Object<GIcon> icon(unity_protocol_social_preview_get_avatar(preview),
                           glib::AddRef());
  avatar_ = icon;
      
  SetupGetters();
}

void SocialPreview::Impl::SetupGetters()
{
  owner_->sender.SetGetterFunction(
            sigc::mem_fun(this, &SocialPreview::Impl::get_sender));
  owner_->content.SetGetterFunction(
            sigc::mem_fun(this, &SocialPreview::Impl::get_content));
  owner_->avatar.SetGetterFunction(
            sigc::mem_fun(this, &SocialPreview::Impl::get_avatar));
}

SocialPreview::SocialPreview(unity::glib::Object<GObject> const& proto_obj)
  : Preview(proto_obj)
  , pimpl(new Impl(this, proto_obj))
{
}

SocialPreview::~SocialPreview()
{
}

}
}
