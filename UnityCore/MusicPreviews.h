// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 */

#ifndef UNITY_MUSIC_PREVIEWS_H
#define UNITY_MUSIC_PREVIEWS_H

#include <memory>

#include <sigc++/trackable.h>
#include <NuxCore/Property.h>

#include "Preview.h"

namespace unity
{
namespace dash
{

class TrackPreview : public Preview
{
public:
  typedef std::shared_ptr<TrackPreview> Ptr;
  typedef std::vector<std::string> Genres;

  TrackPreview(Preview::Properties& properties);


  nux::Property<unsigned int> number;
  nux::Property<std::string> title;
  nux::Property<std::string> artist;
  nux::Property<std::string> album;
  nux::Property<unsigned int> length;
  nux::Property<Genres> genres;
  nux::Property<std::string> album_cover;
  nux::Property<std::string> primary_action_name;
  nux::Property<std::string> primary_action_icon_hint;
  nux::Property<std::string> primary_action_uri;
  nux::Property<std::string> play_action_uri;
  nux::Property<std::string> pause_action_uri;
};


}
}

#endif
