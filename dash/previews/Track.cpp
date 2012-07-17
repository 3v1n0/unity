// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2011 Canonical Ltd.
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

#include "Track.h"
#include "unity-shared/IntrospectableWrappers.h"
#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include <Nux/LayeredLayout.h>
#include <unity-shared/StaticCairoText.h>
#include <unity-shared/IconTexture.h>
#include <unity-shared/DashStyle.h>
#include <unity-shared/PreviewStyle.h>

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
nux::logging::Logger logger("unity.dash.previews.track");
}

NUX_IMPLEMENT_OBJECT_TYPE(Track);

Track::Track(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , track_state(TrackState::STOPPED)
{
  SetupViews();
}

Track::~Track()
{
}

std::string Track::GetName() const
{
  return "Track";
}

void Track::AddProperties(GVariantBuilder* builder)
{
}

void Track::Update(dash::Track const& track)
{
  uri_ = track.uri;
  progress_ = track.progress;
  if (track.track_number == 5) { progress_ = 0.5; }

  title_->SetText(track.title);

  std::stringstream ss_track_number;
  ss_track_number << track.track_number;
  track_number_->SetText(ss_track_number.str());

  gchar* duration = g_strdup_printf("%d:%.2d", track.length/60000, (track.length/1000) % 60);
  duration_->SetText(duration);
  g_free(duration);

  track_busy_ = track.is_playing.Get() || (progress_ > 0.0);

  if (track.is_playing)
  {
    track_status_layout_->SetActiveLayer(status_pause_layout_);
    track_state = TrackState::PLAYING;
  }
  else if (progress_ > 0.0)
  {
    track_status_layout_->SetActiveLayer(status_play_layout_);
    track_state = TrackState::PAUSED;
  }
  else
  {
    track_status_layout_->SetActiveLayer(track_number_layout_);
    track_state = TrackState::STOPPED;
  }

  QueueDraw();
}

void Track::SetupViews()
{
  previews::Style& style = previews::Style::Instance();
  nux::Layout* layout = new nux::HLayout();

  nux::BaseTexture* tex_play = style.GetPlayIcon();
  IconTexture* status_play = new IconTexture(tex_play, tex_play ? tex_play->GetWidth() : 25, tex_play ? tex_play->GetHeight() : 25);
  status_play->mouse_click.connect([&](int, int, unsigned long, unsigned long) { play.emit(uri_); });
  status_play->mouse_enter.connect(sigc::mem_fun(this, &Track::OnTrackControlMouseEnter));
  status_play->mouse_leave.connect(sigc::mem_fun(this, &Track::OnTrackControlMouseLeave));

  nux::BaseTexture* tex_pause = style.GetPauseIcon();
  IconTexture* status_pause = new IconTexture(tex_pause, tex_pause ? tex_pause->GetWidth() : 25, tex_pause ? tex_pause->GetHeight() : 25);
  status_pause->mouse_click.connect([&](int, int, unsigned long, unsigned long) { pause.emit(uri_); });
  status_pause->mouse_enter.connect(sigc::mem_fun(this, &Track::OnTrackControlMouseEnter));
  status_pause->mouse_leave.connect(sigc::mem_fun(this, &Track::OnTrackControlMouseLeave));

  track_number_ = new nux::StaticCairoText("", NUX_TRACKER_LOCATION);
  track_number_->SetTextEllipsize(nux::StaticCairoText::NUX_ELLIPSIZE_NONE);
  track_number_->SetTextAlignment(nux::StaticCairoText::NUX_ALIGN_CENTRE);
  track_number_->SetTextVerticalAlignment(nux::StaticCairoText::NUX_ALIGN_CENTRE);
  track_number_->SetLines(-1);
  track_number_->SetFont(style.track_font());
  track_number_->mouse_enter.connect(sigc::mem_fun(this, &Track::OnTrackControlMouseEnter));
  track_number_->mouse_leave.connect(sigc::mem_fun(this, &Track::OnTrackControlMouseLeave));

  title_ = new nux::StaticCairoText("", NUX_TRACKER_LOCATION);
  title_->SetTextEllipsize(nux::StaticCairoText::NUX_ELLIPSIZE_NONE);
  title_->SetTextAlignment(nux::StaticCairoText::NUX_ALIGN_LEFT);
  title_->SetTextVerticalAlignment(nux::StaticCairoText::NUX_ALIGN_CENTRE);
  title_->SetLines(-1);
  title_->SetFont(style.track_font());

  duration_ = new nux::StaticCairoText("", NUX_TRACKER_LOCATION);
  duration_->SetTextEllipsize(nux::StaticCairoText::NUX_ELLIPSIZE_NONE);
  duration_->SetTextAlignment(nux::StaticCairoText::NUX_ALIGN_RIGHT);
  duration_->SetTextVerticalAlignment(nux::StaticCairoText::NUX_ALIGN_CENTRE);
  duration_->SetLines(-1);
  duration_->SetFont(style.track_font());

  // Layouts
  // stick text fields in a layout so they don't alter thier geometry.
  status_play_layout_ = new nux::HLayout();
  status_play_layout_->AddSpace(0, 1);
  status_play_layout_->AddView(status_play, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  status_play_layout_->AddSpace(0, 1);

  status_pause_layout_ = new nux::HLayout();
  status_pause_layout_->AddSpace(0, 1);
  status_pause_layout_->AddView(status_pause, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  status_pause_layout_->AddSpace(0, 1);

  track_number_layout_ = new nux::HLayout();
  track_number_layout_->AddSpace(0, 1);
  track_number_layout_->AddView(track_number_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  track_number_layout_->AddSpace(0, 1);

  track_status_layout_ = new nux::LayeredLayout();
  track_status_layout_->AddLayer(status_play_layout_, true);
  track_status_layout_->AddLayer(status_pause_layout_, true);
  track_status_layout_->AddLayer(track_number_layout_, true);
  track_status_layout_->SetActiveLayer(track_number_layout_);
  track_status_layout_->SetMinimumWidth(25);
  track_status_layout_->SetMaximumWidth(25);

  title_layout_ = new nux::HLayout();
  title_layout_->SetLeftAndRightPadding(3);
  title_layout_->AddView(title_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  title_layout_->AddSpace(0, 0);

  duration_layout_ = new nux::HLayout();
  duration_layout_->SetMinimumWidth(60);
  duration_layout_->SetMaximumWidth(60);
  duration_layout_->AddSpace(0, 1);
  duration_layout_->AddView(duration_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  layout->AddLayout(track_status_layout_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  layout->AddLayout(title_layout_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  layout->AddLayout(duration_layout_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  SetLayout(layout);

  persistant_status_ = track_status_layout_->GetActiveLayer();
}

void Track::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  gfx_engine.PushClippingRectangle(base);
  nux::GetPainter().PaintBackground(gfx_engine, base);

  unsigned int alpha, src, dest = 0;
  gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
  gfx_engine.GetRenderStates().SetBlend(true);

  if (HasStatusFocus())
  {
    nux::Geometry geo_highlight(track_status_layout_->GetGeometry());

    gfx_engine.QRP_Color(geo_highlight.x,
                      geo_highlight.y,
                      geo_highlight.width,
                      geo_highlight.height,
                      nux::Color(0.15f, 0.15f, 0.15f, 0.15f));
  }

  int progress_width = progress_ * (duration_layout_->GetGeometry().x + duration_layout_->GetGeometry().width - title_layout_->GetGeometry().x);
  if (progress_width > 0.0)
   {

    nux::Geometry geo_progress(title_layout_->GetGeometry().x,
                              base.y,
                              progress_width,
                              base.height);
    
    gfx_engine.QRP_Color(geo_progress.x,
                      geo_progress.y,
                      geo_progress.width,
                      geo_progress.height,
                      nux::Color(0.15f, 0.15f, 0.15f, 0.15f),
                      nux::Color(0.15f, 0.15f, 0.15f, 0.15f),
                      nux::Color(0.05, 0.05, 0.05, 0.15f),
                      nux::Color(0.05, 0.05, 0.05, 0.15f));

    int current_progress_pos = progress_width > 2 ? (geo_progress.x+geo_progress.width) - 2 : geo_progress.x;


    gfx_engine.QRP_Color(current_progress_pos,
                      geo_progress.y,
                      MIN(2, progress_width),
                      geo_progress.height,
                      nux::Color(0.7f, 0.7f, 0.7f, 0.15f));

  }
  
  gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);

  gfx_engine.PopClippingRectangle();
}

void Track::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

  if (IsFullRedraw())
    nux::GetPainter().PushPaintLayerStack();

  unsigned int alpha, src, dest = 0;
  gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
  gfx_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);

  gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);

  if (IsFullRedraw())
    nux::GetPainter().PopPaintLayerStack();

  gfx_engine.PopClippingRectangle();
}

bool Track::HasStatusFocus() const
{
  return track_state == TrackState::PLAYING || track_state == TrackState::PAUSED;
}

void Track::OnTrackControlMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  switch (track_state)
  {
    case TrackState::PLAYING:
      track_status_layout_->SetActiveLayer(status_pause_layout_);
      break;
    case TrackState::PAUSED:
    case TrackState::STOPPED:
    default:
      track_status_layout_->SetActiveLayer(status_play_layout_);
      break;
  }
  QueueDraw();
}

void Track::OnTrackControlMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  switch (track_state)
  {
    case TrackState::PLAYING:
      track_status_layout_->SetActiveLayer(status_pause_layout_);
      break;
    case TrackState::PAUSED:
      track_status_layout_->SetActiveLayer(status_play_layout_);
      break;
    case TrackState::STOPPED:
    default:
      track_status_layout_->SetActiveLayer(track_number_layout_);
      break;
  }
  QueueDraw();
}


}
}
}
