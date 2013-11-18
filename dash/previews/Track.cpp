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

#include "Track.h"
#include "unity-shared/IntrospectableWrappers.h"
#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include <Nux/LayeredLayout.h>
#include <unity-shared/StaticCairoText.h>
#include <unity-shared/IconTexture.h>
#include <unity-shared/DashStyle.h>
#include <unity-shared/PreviewStyle.h>
#include <UnityCore/Variant.h>

namespace unity
{
namespace dash
{
namespace previews
{

class TmpView : public nux::View
{
public:
  TmpView(NUX_FILE_LINE_PROTO): View(NUX_FILE_LINE_PARAM) {}
  virtual ~TmpView() {}

  virtual void Draw(nux::GraphicsEngine& gfx_engine, bool force_draw) {}
  virtual void DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
  {
    if (GetCompositionLayout())
      GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);
  }
  
  virtual bool AcceptKeyNavFocus() { return false; }

};

DECLARE_LOGGER(logger, "unity.dash.preview.music.track");
namespace
{
const int layout_spacing = 6;
}

NUX_IMPLEMENT_OBJECT_TYPE(Track);

class TrackProgressLayer : public nux::AbstractPaintLayer
{
public:
  TrackProgressLayer(nux::Color const& left_color, nux::Color const& right_color, nux::Color const& progress_color,
                    bool write_alpha = false, nux::ROPConfig const& ROP = nux::ROPConfig::Default)
  : left_color_(left_color)
  , right_color_(right_color)
  , progress_color_(progress_color)
  , write_alpha_(write_alpha)
  , rop_(ROP)
  {}
  virtual ~TrackProgressLayer() {}

  virtual void Renderlayer(nux::GraphicsEngine& graphics_engine)
  {
    unsigned int current_red_mask;
    unsigned int current_green_mask;
    unsigned int current_blue_mask;
    unsigned int current_alpha_mask;
    unsigned int current_alpha_blend;
    unsigned int current_src_blend_factor;
    unsigned int current_dest_blend_factor;

    // Get the current color mask and blend states. They will be restored later.
    graphics_engine.GetRenderStates().GetColorMask(current_red_mask, current_green_mask, current_blue_mask, current_alpha_mask);
    // Get the current blend states. They will be restored later.
    graphics_engine.GetRenderStates().GetBlend(current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
    
    graphics_engine.GetRenderStates().SetColorMask(GL_TRUE, GL_TRUE, GL_TRUE, write_alpha_ ? GL_TRUE : GL_FALSE);
    graphics_engine.GetRenderStates().SetBlend(rop_.Blend, rop_.SrcBlend, rop_.DstBlend);
    
    // Gradient to current pos.
    graphics_engine.QRP_Color(geometry_.x, geometry_.y, geometry_.width, geometry_.height, left_color_, left_color_, right_color_, right_color_);

    int current_progress_pos = geometry_.width > 2 ? (geometry_.x + geometry_.width) - 2 : geometry_.x;

    // current pos outline.
    graphics_engine.QRP_Color(current_progress_pos,
                      geometry_.y,
                      MIN(2, geometry_.width),
                      geometry_.height,
                      progress_color_);

    // Restore Color mask and blend states.
    graphics_engine.GetRenderStates().SetColorMask(current_red_mask, current_green_mask, current_blue_mask, current_alpha_mask);
    // Restore the blend state
    graphics_engine.GetRenderStates().SetBlend(current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
  }

  virtual nux::AbstractPaintLayer* Clone() const
  {
    return new TrackProgressLayer(*this);
  }

private:
  nux::Color left_color_;
  nux::Color right_color_;
  nux::Color progress_color_;
  bool write_alpha_;
  nux::ROPConfig rop_;
};

Track::Track(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , play_state_(PlayerState::STOPPED)
  , progress_(0.0)
  , mouse_over_(false)
{
  SetupBackground();
  SetupViews();
}

std::string Track::GetName() const
{
  return "Track";
}

void Track::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder)
    .add("uri", uri_)
    .add("play-state", (int)play_state_)
    .add("progress", progress_)
    .add("playpause-x", track_status_layout_->GetAbsoluteX())
    .add("playpause-y", track_status_layout_->GetAbsoluteX())
    .add("playpause-width", track_status_layout_->GetGeometry().width)
    .add("playpause-height", track_status_layout_->GetGeometry().height)
    .add("playpause-geo", track_status_layout_->GetAbsoluteGeometry());
}

void Track::Update(dash::Track const& track)
{
  uri_ = track.uri;
  title_->SetText(track.title, true);

  std::stringstream ss_track_number;
  ss_track_number << track.track_number;
  track_number_->SetText(ss_track_number.str());

  glib::String duration(g_strdup_printf("%d:%.2d", track.length/60, (track.length) % 60));
  duration_->SetText(duration);

  player_connection_ = player_.updated.connect([this](std::string const& uri, PlayerState player_state, double progress)
  {
    if (uri != uri_)
    {
      // If we're received an update for another track, we're obviously not playing this track anymore.
      if (progress_ != 0.0 || play_state_ != PlayerState::STOPPED)
      {
        progress_ = 0.0;
        play_state_ = PlayerState::STOPPED;
        UpdateTrackState();
      }
      return;
    }

    progress_ = progress;
    play_state_ = player_state;
    UpdateTrackState();
  });
  QueueDraw();
}

void Track::SetupBackground()
{ 
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  focus_layer_.reset(new nux::ColorLayer(nux::Color(0.15f, 0.15f, 0.15f, 0.15f), true, rop));
  progress_layer_.reset(new TrackProgressLayer(nux::Color(0.25f, 0.25f, 0.25f, 0.15f), nux::Color(0.05f, 0.05f, 0.05f, 0.15f), nux::Color(0.60f, 0.60f, 0.60f, 0.15f), true, rop));
}

void Track::SetupViews()
{
  previews::Style& style = previews::Style::Instance();
  nux::HLayout* layout = new nux::HLayout();
  layout->SetLeftAndRightPadding(0,0);

  nux::BaseTexture* tex_play = style.GetPlayIcon();
  IconTexture* status_play = new IconTexture(tex_play, style.GetStatusIconSize(), style.GetStatusIconSize());
  status_play->SetDrawMode(IconTexture::DrawMode::STRETCH_WITH_ASPECT);

  nux::BaseTexture* tex_pause = style.GetPauseIcon();
  IconTexture* status_pause = new IconTexture(tex_pause, style.GetStatusIconSize(), style.GetStatusIconSize());
  status_pause->SetDrawMode(IconTexture::DrawMode::STRETCH_WITH_ASPECT);

  track_number_ = new StaticCairoText("", NUX_TRACKER_LOCATION);
  track_number_->SetTextAlignment(StaticCairoText::NUX_ALIGN_CENTRE);
  track_number_->SetTextVerticalAlignment(StaticCairoText::NUX_ALIGN_CENTRE);
  track_number_->SetLines(-1);
  track_number_->SetFont(style.track_font());

  title_ = new StaticCairoText("", NUX_TRACKER_LOCATION);
  title_->SetTextAlignment(StaticCairoText::NUX_ALIGN_LEFT);
  title_->SetTextVerticalAlignment(StaticCairoText::NUX_ALIGN_CENTRE);
  title_->SetLines(-1);
  title_->SetFont(style.track_font());

  duration_ = new StaticCairoText("", NUX_TRACKER_LOCATION);
  duration_->SetTextEllipsize(StaticCairoText::NUX_ELLIPSIZE_NONE);
  duration_->SetTextAlignment(StaticCairoText::NUX_ALIGN_RIGHT);
  duration_->SetTextVerticalAlignment(StaticCairoText::NUX_ALIGN_CENTRE);
  duration_->SetLines(-1);
  duration_->SetFont(style.track_font());
  duration_->SetMaximumWidth(style.GetMusicDurationWidth());
  duration_->SetMaximumWidth(style.GetMusicDurationWidth());
  // Layouts
  // stick text fields in a layout so they don't alter thier geometry.
  status_play_layout_ = new TmpView();
  status_play_layout_->SetLayout(new nux::HLayout());
  status_play_layout_->GetLayout()->AddSpace(0, 1);
  status_play_layout_->GetLayout()->AddView(status_play, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  status_play_layout_->GetLayout()->AddSpace(0, 1);

  status_pause_layout_ = new TmpView();
  status_pause_layout_->SetLayout(new nux::HLayout());
  status_pause_layout_->GetLayout()->AddSpace(0, 1);
  status_pause_layout_->GetLayout()->AddView(status_pause, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  status_pause_layout_->GetLayout()->AddSpace(0, 1);

  track_number_layout_ = new TmpView();
  track_number_layout_->SetLayout(new nux::HLayout());
  track_number_layout_->GetLayout()->AddSpace(0, 1);
  track_number_layout_->GetLayout()->AddView(track_number_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  track_number_layout_->GetLayout()->AddSpace(0, 1);

  track_status_layout_ = new nux::LayeredLayout();
  track_status_layout_->AddLayer(status_play_layout_, true);
  track_status_layout_->AddLayer(status_pause_layout_, true);
  track_status_layout_->AddLayer(track_number_layout_, true);
  track_status_layout_->SetActiveLayer(track_number_layout_);

  title_layout_ = new nux::HLayout();
  title_layout_->SetLeftAndRightPadding(3);
  title_layout_->AddView(title_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  title_layout_->AddSpace(0, 0);

  duration_layout_ = new nux::HLayout();
  duration_layout_->AddSpace(0, 1);
  duration_layout_->AddView(duration_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  layout->AddLayout(track_status_layout_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  layout->AddLayout(title_layout_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  layout->AddLayout(duration_layout_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  SetLayout(layout);

  mouse_enter.connect(sigc::mem_fun(this, &Track::OnTrackControlMouseEnter));
  mouse_leave.connect(sigc::mem_fun(this, &Track::OnTrackControlMouseLeave));
  mouse_click.connect([this](int, int, unsigned long, unsigned long)
  {
    switch (play_state_)
    {
      case PlayerState::PLAYING:
        player_.Pause();
        break;

      case PlayerState::PAUSED:
        player_.Resume();
        break;

      case PlayerState::STOPPED:
      default:
        player_.Play(uri_);
        break;
    }
  });
}

void Track::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  unsigned int alpha, src, dest = 0;
  gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
  gfx_engine.GetRenderStates().SetBlend(true);

  if (HasStatusFocus())
  {
    focus_layer_->SetGeometry(track_status_layout_->GetGeometry());
    nux::GetPainter().RenderSinglePaintLayer(gfx_engine, focus_layer_->GetGeometry(), focus_layer_.get());
  }

  int progress_width = progress_ * (duration_layout_->GetGeometry().x + duration_layout_->GetGeometry().width - title_layout_->GetGeometry().x);
  if (progress_width > 0.0)
  {
    nux::Geometry geo_progress(title_layout_->GetGeometry().x,
                              base.y,
                              progress_width,
                              base.height);

    progress_layer_->SetGeometry(geo_progress);
    nux::GetPainter().RenderSinglePaintLayer(gfx_engine, progress_layer_->GetGeometry(), progress_layer_.get());
  }
  
  gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);
}

void Track::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

  int pop_layers = 0;
  if (!IsFullRedraw())
  {
    if (HasStatusFocus())
    {
      nux::GetPainter().PushLayer(gfx_engine, focus_layer_->GetGeometry(), focus_layer_.get());
      pop_layers++;
    }

    int progress_width = progress_ * (duration_layout_->GetGeometry().x + duration_layout_->GetGeometry().width - title_layout_->GetGeometry().x);
    if (progress_width > 0.0)
    {
      nux::GetPainter().PushLayer(gfx_engine, progress_layer_->GetGeometry(), progress_layer_.get());
      pop_layers++;
    }
  }
  else
    nux::GetPainter().PushPaintLayerStack();

  unsigned int alpha, src, dest = 0;
  gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
  gfx_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);

  gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);

  if (IsFullRedraw())
    nux::GetPainter().PopPaintLayerStack();
  else if (pop_layers > 0)
    nux::GetPainter().PopBackground(pop_layers);

  gfx_engine.PopClippingRectangle();
}

nux::Area* Track::FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
{
  bool mouse_inside = TestMousePointerInclusion(mouse_position, event_type);
  if (mouse_inside == false)
    return NULL;

  return this;
}

bool Track::HasStatusFocus() const
{
  return mouse_over_ || play_state_ == PlayerState::PLAYING || play_state_ == PlayerState::PAUSED;
}

void Track::OnTrackControlMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  mouse_over_ = true;
  UpdateTrackState();

  QueueDraw();
}

void Track::OnTrackControlMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  mouse_over_ = false;
  UpdateTrackState();

  QueueDraw();
}

void Track::UpdateTrackState()
{
  if (mouse_over_)
  {
    switch (play_state_)
    {
      case PlayerState::PLAYING:
        track_status_layout_->SetActiveLayer(status_pause_layout_);
        break;
      case PlayerState::PAUSED:
      case PlayerState::STOPPED:
      default:
        track_status_layout_->SetActiveLayer(status_play_layout_);
        break;
    }
  }
  else
  {
    switch (play_state_)
    {
      case PlayerState::PLAYING:
        track_status_layout_->SetActiveLayer(status_play_layout_);
        break;
      case PlayerState::PAUSED:
        track_status_layout_->SetActiveLayer(status_pause_layout_);
        break;
      case PlayerState::STOPPED:
      default:
        track_status_layout_->SetActiveLayer(track_number_layout_);
        break;
    }
  }
  QueueDraw();
}


void Track::PreLayoutManagement()
{
  previews::Style& style = previews::Style::Instance();
  nux::Geometry const& geo = GetGeometry();

  track_status_layout_->SetMinimumWidth(geo.height);
  track_status_layout_->SetMaximumWidth(geo.height);

  const int max_width = std::max(GetGeometry().width - geo.height - style.GetMusicDurationWidth() - layout_spacing*2, 0);
  title_->SetMaximumWidth(max_width);

  View::PreLayoutManagement();
}

} // namespace previews
} // namespace dash
} // namesapce unity
