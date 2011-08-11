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

#include "MusicPreviews.h"

namespace unity
{
namespace dash
{

TrackPreview::TrackPreview(Preview::Properties& properties)
  : number(PropertyToUnsignedInt(properties, "number"))
  , title(PropertyToString(properties, "title"))
  , artist(PropertyToString(properties, "artist"))
  , album(PropertyToString(properties, "album"))
  , length(PropertyToUnsignedInt(properties, "length"))
  , genres(PropertyToStringVector(properties, "genres"))
  , album_cover(PropertyToString(properties, "album-cover"))
  , primary_action_name(PropertyToString(properties, "primary-action-name"))
  , primary_action_icon_hint(PropertyToString(properties, "primary-action-icon-hint"))
  , primary_action_uri(PropertyToString(properties, "primary-action-uri"))
  , play_action_uri(PropertyToString(properties, "play-action-uri"))
  , pause_action_uri(PropertyToString(properties, "pause-action-uri"))
{
 renderer_name = "preview-track";
}

AlbumPreview::AlbumPreview(Preview::Properties& properties)
  : name(PropertyToString(properties, "name"))
  , artist(PropertyToString(properties, "artist"))
  , year(PropertyToString(properties, "year"))
  , genres(PropertyToStringVector(properties, "genres"))
  , album_cover(PropertyToString(properties, "album-cover"))
  , primary_action_name(PropertyToString(properties, "primary-action-name"))
  , primary_action_icon_hint(PropertyToString(properties, "primary-action-icon-hint"))
  , primary_action_uri(PropertyToString(properties, "primary-action-uri"))
{
  renderer_name = "preview-album";
  LoadTracks(properties);
}

void AlbumPreview::LoadTracks(Properties& properties)
{
  GVariantIter *iter = NULL;
  unsigned int track_number = 0;
  char* track_title;
  unsigned int track_length = 0;
  char* track_play_uri;
  char* track_pause_uri;

  if (properties.find("tracks") == properties.end())
    return;

  g_variant_get(properties["tracks"], "(ususs)", &iter);

  while (g_variant_iter_loop(iter, "ususs",
                             &track_number,
                             &track_title,
                             &track_length,
                             &track_play_uri,
                             &track_pause_uri))
  {
    Track track(track_number, track_title, track_length, track_play_uri, track_pause_uri);
    tracks.push_back(track);

    length += track_length;
  }

  g_variant_iter_free(iter);
}

}
}
