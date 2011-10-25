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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */

#include "config.h"

#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/Button.h>

#include "PreviewMusicTrackWidget.h"
#include "StaticCairoText.h"

namespace unity {

   PreviewMusicTrackWidget::PreviewMusicTrackWidget (std::string number,
                                                     std::string name,
                                                     std::string time,
                                                     std::string play_uri,
                                                     std::string pause_uri,
                                                     NUX_FILE_LINE_DECL)
    : nux::View (NUX_FILE_LINE_PARAM)
    , track_is_active (false)
    , is_paused (false)
    , number_ (number)
    , name_ (name)
    , time_ (time)
    , play_uri_ (play_uri)
    , pause_uri_ (pause_uri)

  {
    InitTheme();

    track_is_active.changed.connect ([&] (bool value) {
      is_paused = !value;
    });

    is_paused.changed.connect ([&] (bool value) {
      if (!track_is_active)
      {
        return;
      }
      else
      {
        if (value)
        {
          // play
          UriActivated.emit (pause_uri_);
        }
        else
        {
          //pause
          UriActivated.emit (play_uri_);
        }
      }
    });
  }

  PreviewMusicTrackWidget::~PreviewMusicTrackWidget() {

  }

  void PreviewMusicTrackWidget::InitTheme()
  {
    nux::HLayout *track_layout = new nux::HLayout(NUX_TRACKER_LOCATION);
    //FIXME - use a button subclass for absolute renderering
    play_button_ = new nux::Button(number_.c_str());

    play_button_->state_change.connect ([&] (nux::View *view) {
      if (track_is_active)
      {
        is_paused = !is_paused;
      }
      else
      {
        track_is_active = true;
        is_paused = false;
        UriActivated.emit (play_uri_);
      }
    });

    //FIXME - hook up pressing button to activation URI
    nux::StaticCairoText *track_title = new nux::StaticCairoText(name_.c_str(), NUX_TRACKER_LOCATION);
    nux::StaticCairoText *track_length = new nux::StaticCairoText(time_.c_str(), NUX_TRACKER_LOCATION);

    track_layout->AddView(play_button_, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
    track_layout->AddLayout (new nux::SpaceLayout(6,6,6,6), 1);
    track_layout->AddView(track_title, 1, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
    track_layout->AddLayout (new nux::SpaceLayout(6,6,6,6), 1);
    track_layout->AddSpace (0, 1);
    track_layout->AddView(track_length, 1, nux::MINOR_POSITION_RIGHT, nux::MINOR_SIZE_FULL);

    SetLayout(track_layout);
  }

  void PreviewMusicTrackWidget::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
  }

  void PreviewMusicTrackWidget::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Geometry base = GetGeometry ();
    GfxContext.PushClippingRectangle (GetGeometry());

    if (GetCompositionLayout ())
      GetCompositionLayout ()->ProcessDraw (GfxContext, force_draw);

    GfxContext.PopClippingRectangle();
  }

  void PreviewMusicTrackWidget::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::View::PostDraw(GfxContext, force_draw);
  }

}
