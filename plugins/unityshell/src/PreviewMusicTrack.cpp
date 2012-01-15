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
#include <glib/gi18n-lib.h>
#include <Nux/Nux.h>
#include <Nux/StaticText.h>
#include <Nux/VLayout.h>
#include <Nux/HLayout.h>
#include <Nux/ScrollView.h>
#include <sstream>

#include "PreviewBasicButton.h"
#include "PreviewBase.h"
#include "IconTexture.h"
#include "StaticCairoText.h"
#include "PreviewMusicTrackWidget.h"

#include "PreviewMusicTrack.h"

namespace unity {

    PreviewMusicTrack::PreviewMusicTrack (dash::Preview::Ptr preview, NUX_FILE_LINE_DECL)
      : PreviewBase(NUX_FILE_LINE_PARAM)
    {
      SetPreview (preview);
    }

    PreviewMusicTrack::~PreviewMusicTrack ()
    {
      //ROOOARRRR
    }

    void PreviewMusicTrack::SetPreview(dash::Preview::Ptr preview)
    {
      preview_ = std::static_pointer_cast<dash::TrackPreview>(preview);
      BuildLayout ();
    }

    void PreviewMusicTrack::BuildLayout()
    {
      IconTexture *cover = new IconTexture (preview_->album_cover.c_str(), 400);
      nux::StaticCairoText *title = new nux::StaticCairoText(preview_->title.c_str(), NUX_TRACKER_LOCATION);
      title->SetFont("Ubuntu 25");

      std::string artist_year = preview_->artist; // no year in model + ", " + preview_->year;
      nux::StaticCairoText *artist = new nux::StaticCairoText(artist_year.c_str(), NUX_TRACKER_LOCATION);
      artist->SetFont("Ubuntu 15");

      std::ostringstream album_length_string;

      album_length_string << preview_->length / 60 << ":" << preview_->length % 60
                          << " " << _("min");

      nux::StaticCairoText *album_length = new nux::StaticCairoText(album_length_string.str().c_str(), NUX_TRACKER_LOCATION);

      std::ostringstream genres_string;
      dash::AlbumPreview::Genres::iterator genre_it;
      bool is_first_genre = true;
      for (genre_it = preview_->genres.begin(); genre_it != preview_->genres.end(); genre_it++)
      {
        if (is_first_genre)
          is_first_genre = false;
        else
          genres_string << ", ";

        genres_string << (*genre_it);
      }

      nux::StaticCairoText *genres = new nux::StaticCairoText(genres_string.str().c_str(), NUX_TRACKER_LOCATION);

      nux::VLayout* tracks = new nux::VLayout(NUX_TRACKER_LOCATION);

      std::stringstream number;
      number << preview_->number;
      std::stringstream track_length_string;
      track_length_string << preview_->length / 60 << ":" << preview_->length % 60;

      PreviewMusicTrackWidget *track_widget = new PreviewMusicTrackWidget(number.str(),
                                                                          preview_->title,
                                                                          track_length_string.str(),
                                                                          preview_->play_action_uri,
                                                                          preview_->pause_action_uri,
                                                                          NUX_TRACKER_LOCATION);

      track_widget->UriActivated.connect ([&] (std::string uri) { UriActivated.emit(uri); });

      tracks->AddView(track_widget, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);

      PreviewBasicButton* primary_button = new PreviewBasicButton(preview_->primary_action_name.c_str(), NUX_TRACKER_LOCATION);
      //FIXME - add secondary action when we have the backend for it
      primary_button->state_change.connect ([&] (nux::View *view) { UriActivated.emit (preview_->primary_action_uri); });


      nux::HLayout *large_container = new nux::HLayout(NUX_TRACKER_LOCATION);
      nux::VLayout *screenshot_container = new nux::VLayout(NUX_TRACKER_LOCATION);
      content_layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
      nux::HLayout *button_container = new nux::HLayout(NUX_TRACKER_LOCATION);

      //set up the tracks scroller
      nux::ScrollView* track_scroller = new nux::ScrollView(NUX_TRACKER_LOCATION);
      track_scroller->EnableHorizontalScrollBar(false);
      track_scroller->SetLayout(tracks);


      //set up the button layout
      nux::VLayout *track_genre_layout = new nux::VLayout(NUX_TRACKER_LOCATION);
      track_genre_layout->AddView(album_length, 0, nux::MINOR_POSITION_LEFT);
      track_genre_layout->AddView(genres, 0, nux::MINOR_POSITION_LEFT);

      button_container->AddLayout(track_genre_layout, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);

      button_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 1);
      button_container->AddSpace (0, 1);
      button_container->AddView (primary_button, 0, nux::MINOR_POSITION_RIGHT, nux::MINOR_SIZE_FULL);

      //bring it all together
      content_layout_->AddLayout (new nux::SpaceLayout(6,6,6,6), 1);
      content_layout_->AddView(title, 0, nux::MINOR_POSITION_LEFT);
      content_layout_->AddLayout (new nux::SpaceLayout(6,6,6,6), 1);
      content_layout_->AddView(artist, 0, nux::MINOR_POSITION_LEFT);
      content_layout_->AddLayout (new nux::SpaceLayout(12,12,12,12), 1);
      content_layout_->AddView(track_scroller, 1, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_FULL);
      content_layout_->AddLayout(button_container, 0);
      content_layout_->AddLayout (new nux::SpaceLayout(6,6,6,6), 1);

      screenshot_container->AddView (cover, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
      large_container->AddLayout(screenshot_container, 1);
      large_container->AddLayout (new nux::SpaceLayout(12,12,12,12), 0);
      large_container->AddLayout(content_layout_, 1);
      large_container->AddLayout (new nux::SpaceLayout(6,6,6,6), 0);

      SetLayout(large_container);
    }

  void PreviewMusicTrack::Draw (nux::GraphicsEngine &GfxContext, bool force_draw) {
    PreviewBase::Draw(GfxContext, force_draw);
  }

  void PreviewMusicTrack::DrawContent (nux::GraphicsEngine &GfxContent, bool force_draw) {
    nux::Geometry base = GetGeometry ();
    GfxContent.PushClippingRectangle (base);

    if (GetCompositionLayout ())
      GetCompositionLayout ()->ProcessDraw (GfxContent, force_draw);

    GfxContent.PopClippingRectangle();
  }

  void PreviewMusicTrack::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    PreviewBase::PostDraw(GfxContext, force_draw);
  }
}
