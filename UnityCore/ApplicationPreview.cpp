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

#include "ApplicationPreview.h"

namespace unity
{
namespace dash
{

class ApplicationPreview::Impl
{
public:
  Impl(ApplicationPreview* owner, glib::Object<GObject> const& proto_obj);

  void SetupGetters();
  std::string get_last_update() { return last_update_; };
  std::string get_copyright() { return copyright_; };
  std::string get_license() { return license_; };
  glib::Object<GIcon> get_app_icon() { return app_icon_; };
  float get_rating() const { return rating_; };
  unsigned int get_num_ratings() const { return num_ratings_; };

  ApplicationPreview* owner_;

  std::string last_update_;
  std::string copyright_;
  std::string license_;
  glib::Object<GIcon> app_icon_;
  float rating_;
  unsigned int num_ratings_;
};

ApplicationPreview::Impl::Impl(ApplicationPreview* owner, glib::Object<GObject> const& proto_obj)
  : owner_(owner)
{
  const gchar* s;
  auto preview = glib::object_cast<UnityProtocolApplicationPreview>(proto_obj);

  s = unity_protocol_application_preview_get_last_update(preview);
  if (s) last_update_ = s;
  s = unity_protocol_application_preview_get_copyright(preview);
  if (s) copyright_ = s;
  s = unity_protocol_application_preview_get_license(preview);
  if (s) license_ = s;

  glib::Object<GIcon> icon(unity_protocol_application_preview_get_app_icon(preview),
                           glib::AddRef());
  app_icon_ = icon;
  rating_ = unity_protocol_application_preview_get_rating(preview);
  num_ratings_ = unity_protocol_application_preview_get_num_ratings(preview);
      
  SetupGetters();
}

void ApplicationPreview::Impl::SetupGetters()
{
  owner_->last_update.SetGetterFunction(
            sigc::mem_fun(this, &ApplicationPreview::Impl::get_last_update));
  owner_->copyright.SetGetterFunction(
            sigc::mem_fun(this, &ApplicationPreview::Impl::get_copyright));
  owner_->license.SetGetterFunction(
            sigc::mem_fun(this, &ApplicationPreview::Impl::get_license));
  owner_->app_icon.SetGetterFunction(
            sigc::mem_fun(this, &ApplicationPreview::Impl::get_app_icon));
  owner_->rating.SetGetterFunction(
            sigc::mem_fun(this, &ApplicationPreview::Impl::get_rating));
  owner_->num_ratings.SetGetterFunction(
            sigc::mem_fun(this, &ApplicationPreview::Impl::get_num_ratings));
}

ApplicationPreview::ApplicationPreview(unity::glib::Object<GObject> const& proto_obj)
  : Preview(proto_obj)
  , pimpl(new Impl(this, proto_obj))
{
}

ApplicationPreview::~ApplicationPreview()
{
}

}
}
