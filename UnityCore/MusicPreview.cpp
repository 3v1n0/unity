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

#include "MusicPreview.h"
#include "Tracks.h"
#include "Scope.h"
#include "PreviewPlayer.h"

namespace unity
{
namespace dash
{
DECLARE_LOGGER(logger, "unity.dash.musicpreview");

class MusicPreview::Impl
{
public:
  Impl(MusicPreview* owner, glib::Object<GObject> const& proto_obj);

  MusicPreview* owner_;

  glib::Object<UnityProtocolMusicPreview> raw_preview_;
  Tracks::Ptr tracks_model;
};

MusicPreview::Impl::Impl(MusicPreview* owner,
                         glib::Object<GObject> const& proto_obj)
  : owner_(owner)
  , tracks_model(new Tracks())
{
  raw_preview_ = glib::object_cast<UnityProtocolMusicPreview>(proto_obj);

  if (raw_preview_)
  {
    glib::Object<DeeModel> track_dee_model(DEE_MODEL(unity_protocol_music_preview_get_track_model(raw_preview_)), glib::AddRef());
    tracks_model->SetModel(track_dee_model);
  }
}

MusicPreview::MusicPreview(unity::glib::Object<GObject> const& proto_obj)
  : Preview(proto_obj)
  , pimpl(new Impl(this, proto_obj))
{
}

MusicPreview::~MusicPreview()
{
}

Tracks::Ptr MusicPreview::GetTracksModel() const
{
  return pimpl->tracks_model;
}

}
}
