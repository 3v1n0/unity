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

namespace unity
{
namespace dash
{

class MusicPreview::Impl
{
public:
  Impl(MusicPreview* owner, glib::Object<GObject> const& proto_obj);

  void PlayUri(std::string const& uri) const;
  void PauseUri(std::string const& uri) const;

  MusicPreview* owner_;

  glib::Object<UnityProtocolMusicPreview> raw_preview_;
  Tracks::Ptr tracks_model;
};

MusicPreview::Impl::Impl(MusicPreview* owner,
                         glib::Object<GObject> const& proto_obj)
  : owner_(owner)
  , tracks_model(new Tracks())
{
  const gchar* s;
  raw_preview_ = glib::object_cast<UnityProtocolMusicPreview>(proto_obj);

  s = unity_protocol_music_preview_get_track_data_swarm_name(raw_preview_);
  std::string swarm_name(s != NULL ? s : "");
  s = unity_protocol_music_preview_get_track_data_address(raw_preview_);
  std::string peer_address(s != NULL ? s : "");

  // TODO: we're not using private connection yet
  if (!swarm_name.empty())
  {
    tracks_model->swarm_name = swarm_name;
  }
}

void MusicPreview::Impl::PlayUri(std::string const& uri) const
{
  UnityProtocolPreview *preview = UNITY_PROTOCOL_PREVIEW(raw_preview_.RawPtr());

  unity_protocol_preview_begin_updates(preview);
  unity_protocol_music_preview_play_uri(raw_preview_, uri.c_str());
  glib::Variant properties(unity_protocol_preview_end_updates(preview),
                           glib::StealRef());
  owner_->Update(properties);
}

void MusicPreview::Impl::PauseUri(std::string const& uri) const
{
  UnityProtocolPreview *preview = UNITY_PROTOCOL_PREVIEW(raw_preview_.RawPtr());

  unity_protocol_preview_begin_updates(preview);
  unity_protocol_music_preview_pause_uri(raw_preview_, uri.c_str());
  glib::Variant properties(unity_protocol_preview_end_updates(preview),
                           glib::StealRef());
  owner_->Update(properties);
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

void MusicPreview::PlayUri(std::string const& uri) const
{
  pimpl->PlayUri(uri);
}

void MusicPreview::PauseUri(std::string const& uri) const
{
  pimpl->PauseUri(uri);
}

}
}
