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

#ifndef UNITY_MUSIC_PREVIEW_H
#define UNITY_MUSIC_PREVIEW_H

#include <memory>

#include <sigc++/trackable.h>

#include "Preview.h"

namespace unity
{
namespace dash
{

class MusicPreview : public Preview
{
public:
  typedef std::shared_ptr<MusicPreview> Ptr;

  struct Track
  {
  public:

    Track(unsigned int number_, std::string title_, unsigned int length_,
          std::string play_action_uri_, std::string pause_action_uri_)
      : number(number_)
      , title(title_)
      , length(length_)
      , play_action_uri(play_action_uri_)
      , pause_action_uri(pause_action_uri_)
    {}

    unsigned int number;
    std::string title;
    unsigned int length;
    std::string play_action_uri;
    std::string pause_action_uri;
  };
  
  typedef std::vector<Track> Tracks;

  MusicPreview(unity::glib::Object<UnityProtocolPreview> const& proto_obj);
};

}
}

#endif
