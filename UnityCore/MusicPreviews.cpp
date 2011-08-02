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
{
  /* FIXME: We don't unpack genres yet */
  renderer_name = "preview-track";
  number = PropertyToUnsignedInt(properties, "number");
  title = PropertyToString(properties, "title");
  artist = PropertyToString(properties, "artist");
  album = PropertyToString(properties, "album");
  length = PropertyToUnsignedInt(properties, "length");
  album_cover = PropertyToString(properties, "album-cover");
  primary_action_name = PropertyToString(properties, "primary-action-name");
  primary_action_icon_hint = PropertyToString(properties, "primary-action-icon-hint");
  primary_action_uri = PropertyToString(properties, "primary-action-uri");
  play_action_uri = PropertyToString(properties, "play-action-uri");
  pause_action_uri = PropertyToString(properties, "pause-action-uri");
}

}
}
