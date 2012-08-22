// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#ifndef TRACKS_H
#define TRACKS_H

#include <Nux/Nux.h>
#include <Nux/ScrollView.h>
#include <UnityCore/Tracks.h>
#include "unity-shared/Introspectable.h"
#include "Track.h"

namespace nux
{
class VLayout;
}

namespace unity
{
namespace dash
{
class Track;

namespace previews
{

class Tracks : public debug::Introspectable, public nux::ScrollView
{
public:
  typedef nux::ObjectPtr<Tracks> Ptr;
  NUX_DECLARE_OBJECT_TYPE(Tracks, nux::View);

  Tracks(dash::Tracks::Ptr tracks, NUX_FILE_LINE_PROTO);
  virtual ~Tracks();

  // From debug::Introspectable
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);  

  sigc::signal<void, std::string const&> play;
  sigc::signal<void, std::string const&> pause;

protected:
  virtual void Draw(nux::GraphicsEngine& gfx_engine, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw);
 
  virtual bool AcceptKeyNavFocus() { return false; }

  void SetupViews();

  void OnTrackUpdated(dash::Track const& track);
  void OnTrackAdded(dash::Track const& track);
  void OnTrackRemoved(dash::Track const&track);

  void onPlayTrack(std::string const& uri);
  void onPauseTrack(std::string const& uri);

protected:
  dash::Tracks::Ptr tracks_;

  nux::VLayout* layout_;
  std::map<std::string, previews::Track::Ptr> m_tracks;
  int track_count_;
};

}
}
}

#endif // TRACKS_H
