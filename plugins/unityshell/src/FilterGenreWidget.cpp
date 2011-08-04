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
#include "Nux/Nux.h"

#include "FilterGenreWidget.h"
#include "FilterGenreButton.h"
#include "FilterBasicButton.h"

namespace unity {

  FilterGenre::FilterGenre (NUX_FILE_LINE_DECL)
      : FilterExpanderLabel ("Categories", NUX_FILE_LINE_PARAM)
      , all_selected (false) {
    InitTheme();

    all_button_ = new FilterBasicButton("Any", NUX_TRACKER_LOCATION);
    all_button_->Activated.connect(sigc::mem_fun(this, &FilterGenre::OnAllActivated));
    all_button_->Reference();

    genre_layout_ = new nux::GridHLayout(NUX_TRACKER_LOCATION);
    genre_layout_->SetVerticalInternalMargin (6);
    genre_layout_->SetHorizontalInternalMargin (6);
    genre_layout_->EnablePartialVisibility (false);
    genre_layout_->SetChildrenSize (140, 30);
    genre_layout_->Reference();
    BuildGenreLayout();

    SetRightHandView(all_button_);
    SetContents(genre_layout_);
  }

  FilterGenre::~FilterGenre() {
    all_button_->UnReference();
  }

  void FilterGenre::SetFilter(void *filter)
  {
    _filter = filter;
    //FIXME - we need to get detail from the filter,
    //such as name and link to its signals
  }

  std::string FilterGenre::GetFilterType ()
  {
    return "FilterBasicButton";
  }

  void FilterGenre::InitTheme()
  {
    //FIXME - build theme here - store images, cache them, fun fun fun
  }

  void FilterGenre::OnAllActivated(nux::View *view)
  {
    all_selected = true;
  }

  void FilterGenre::BuildGenreLayout()
  {
    //FIXME
    // blah blah dont have filters integrated so faking it for now
    FilterGenreButton *button = new FilterGenreButton ("Genre 1", NUX_TRACKER_LOCATION);
    genre_layout_->AddView (button, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

    button = new FilterGenreButton ("Genre 2", NUX_TRACKER_LOCATION);
    genre_layout_->AddView (button, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

    button = new FilterGenreButton ("Genre 3", NUX_TRACKER_LOCATION);
    genre_layout_->AddView (button, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

    button = new FilterGenreButton ("Genre 4", NUX_TRACKER_LOCATION);
    genre_layout_->AddView (button, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

    button = new FilterGenreButton ("Genre 5", NUX_TRACKER_LOCATION);
    genre_layout_->AddView (button, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

    button = new FilterGenreButton ("Genre 6", NUX_TRACKER_LOCATION);
    genre_layout_->AddView (button, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

    button = new FilterGenreButton ("Genre 7", NUX_TRACKER_LOCATION);
    genre_layout_->AddView (button, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

    button = new FilterGenreButton ("Genre 8", NUX_TRACKER_LOCATION);
    genre_layout_->AddView (button, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

    button = new FilterGenreButton ("Genre 9", NUX_TRACKER_LOCATION);
    genre_layout_->AddView (button, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

  }

  void FilterGenre::OnGenreActivated(nux::View *view)
  {
    // just check to see if all the filters are active or not
    // if they are, then set the all status to true
  }


  long int FilterGenre::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return PostProcessEvent2 (ievent, TraverseInfo, ProcessEventInfo);
  }

  void FilterGenre::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
  }

  void FilterGenre::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Geometry base = GetGeometry ();
    GfxContext.PushClippingRectangle (base);

    if (GetCompositionLayout ())
      GetCompositionLayout ()->ProcessDraw (GfxContext, force_draw);

    GfxContext.PopClippingRectangle();
  }

  void FilterGenre::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::View::PostDraw(GfxContext, force_draw);
  }

};
