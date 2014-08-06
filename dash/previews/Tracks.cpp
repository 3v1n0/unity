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

#include "Tracks.h"
#include <NuxCore/Logger.h>
#include <Nux/VLayout.h>
#include "unity-shared/IntrospectableWrappers.h"
#include "unity-shared/PreviewStyle.h"
#include <UnityCore/Track.h>

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
DECLARE_LOGGER(logger, "unity.dash.preview.music.tracks");
const RawPixel CHILDREN_SPACE = 1_em;
}

NUX_IMPLEMENT_OBJECT_TYPE(Tracks);

Tracks::Tracks(dash::Tracks::Ptr tracks, NUX_FILE_LINE_DECL)
  : ScrollView(NUX_FILE_LINE_PARAM)
  , tracks_(tracks)
{
  SetupViews();

  if (tracks_)
  {
    sig_conn_.Add(tracks_->track_added.connect(sigc::mem_fun(this, &Tracks::OnTrackAdded)));
    sig_conn_.Add(tracks_->track_changed.connect(sigc::mem_fun(this, &Tracks::OnTrackUpdated)));
    sig_conn_.Add(tracks_->track_removed.connect(sigc::mem_fun(this, &Tracks::OnTrackRemoved)));

    // Add what we've got.
    for (std::size_t i = 0; i < tracks_->count.Get(); ++i)
      OnTrackAdded(tracks_->RowAtIndex(i));
  }

  UpdateScale(scale);
  scale.changed.connect(sigc::mem_fun(this, &Tracks::UpdateScale));
}

std::string Tracks::GetName() const
{
  return "Tracks";
}

void Tracks::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
    .add("track-count", m_tracks.size());
}

void Tracks::SetupViews()
{
  EnableHorizontalScrollBar(false);
  layout_ = new nux::VLayout();
  layout_->SetPadding(0, previews::Style::Instance().GetDetailsRightMargin().CP(scale), 0, 0);
  layout_->SetSpaceBetweenChildren(CHILDREN_SPACE.CP(scale));
  SetLayout(layout_);
}

void Tracks::OnTrackUpdated(dash::Track const& track_row)
{
  auto pos = m_tracks.find(track_row.uri.Get());
  if (pos == m_tracks.end())
    return;

  pos->second->Update(track_row);
  pos->second->scale = scale();
}

void Tracks::OnTrackAdded(dash::Track const& track_row)
{
  LOG_TRACE(logger) << "OnTrackAdded for " << track_row.title.Get();

  std::string track_uri = track_row.uri.Get();
  if (m_tracks.find(track_uri) != m_tracks.end())
    return;

  previews::Style& style = dash::previews::Style::Instance();

  previews::Track::Ptr track_view(new previews::Track(NUX_TRACKER_LOCATION));
  AddChild(track_view.GetPointer());

  track_view->Update(track_row);
  track_view->SetMinimumHeight(style.GetTrackHeight().CP(scale));
  track_view->SetMaximumHeight(style.GetTrackHeight().CP(scale));
  track_view->scale = scale();
  layout_->AddView(track_view.GetPointer(), 0);

  m_tracks[track_uri] = track_view;
  ComputeContentSize();
}

void Tracks::OnTrackRemoved(dash::Track const& track_row)
{
  LOG_TRACE(logger) << "OnTrackRemoved for " << track_row.title.Get();

  auto pos = m_tracks.find(track_row.uri.Get());
  if (pos == m_tracks.end())
    return;

  RemoveChild(pos->second.GetPointer());
  layout_->RemoveChildObject(pos->second.GetPointer());
  ComputeContentSize();
}

void Tracks::UpdateScale(double scale)
{
  int track_height = Style::Instance().GetTrackHeight().CP(scale);

  for (auto const& track : m_tracks)
  {
    track.second->SetMinimumHeight(track_height);
    track.second->SetMaximumHeight(track_height);
    track.second->scale = scale;
  }

  if (layout_)
  {
    layout_->SetPadding(0, Style::Instance().GetDetailsRightMargin().CP(scale), 0, 0);
    layout_->SetSpaceBetweenChildren(CHILDREN_SPACE.CP(scale));
  }

  QueueRelayout();
  QueueDraw();
}

} // namespace previews
} // namespace dash
} // namespace unity
