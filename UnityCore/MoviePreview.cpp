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

#include "MoviePreview.h"

namespace unity
{
namespace dash
{

class MoviePreview::Impl
{
public:
  Impl(MoviePreview* owner, glib::Object<GObject> const& proto_obj);

  void SetupGetters();
  std::string get_year() const { return year_; };
  float get_rating() const { return rating_; };
  unsigned int get_num_ratings() const { return num_ratings_; };

  MoviePreview* owner_;

  std::string year_;
  float rating_;
  unsigned int num_ratings_;
};

MoviePreview::Impl::Impl(MoviePreview* owner, 
                         glib::Object<GObject> const& proto_obj)
  : owner_(owner)
{
  auto preview = glib::object_cast<UnityProtocolMoviePreview>(proto_obj);
  const gchar* s;
  s = unity_protocol_movie_preview_get_year(preview);
  if (s) year_ = s;
  rating_ = unity_protocol_movie_preview_get_rating(preview);
  num_ratings_ = unity_protocol_movie_preview_get_num_ratings(preview);

  SetupGetters();
}

void MoviePreview::Impl::SetupGetters()
{
  owner_->year.SetGetterFunction(
      sigc::mem_fun(this, &MoviePreview::Impl::get_year));
  owner_->rating.SetGetterFunction(
      sigc::mem_fun(this, &MoviePreview::Impl::get_rating));
  owner_->num_ratings.SetGetterFunction(
      sigc::mem_fun(this, &MoviePreview::Impl::get_num_ratings));
}

MoviePreview::MoviePreview(unity::glib::Object<GObject> const& proto_obj)
  : Preview(proto_obj)
  , pimpl(new Impl(this, proto_obj))
{
}

MoviePreview::~MoviePreview()
{
}

}
}
