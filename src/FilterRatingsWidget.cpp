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

#include "FilterRatingsWidget.h"

namespace unity {

  FilterRatings::FilterRatings (NUX_FILE_LINE_DECL)
      : nux::Button (NUX_FILE_LINE_PARAM)
      , rating (this, "rating") {
    InitTheme();

    OnMouseDown.connect (sigc::mem_fun (this, &FilterRatings::RecvMouseDown) );
    rating.changed.connect (sigc::mem_fun (this, &FilterRatings::OnRatingsChanged) );
  }

  FilterRatings::~FilterRatings() {

  }

  void FilterRatings::SetFilter(void *filter)
  {
    // we got a new filter
    FilterWidget::SetFilter (filter);

    //FIXME - we need to get detail from the filter,
    //such as name and link to its signals
  }


  void FilterRatings::InitTheme()
  {
    //FIXME - build theme here - store images, cache them, fun fun fun
    // ratings bar requires us to store/create three states for stares, half open, selected/unselected
  }


  long int FilterRatings::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return nux::Button::ProcessEvent(ievent, TraverseInfo, ProcessEventInfo);
  }

  void FilterRatings::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    //FIXME - i disabled the crappy drawing i was doing,
    //should just draw five sliced textures,
    nux::Button::Draw(GfxContext, force_draw);
  }

  void FilterRatings::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
    //FIXME - i swear i didn't change anything, but suddenly nux stopped drawing the contents of the button
    nux::Button::DrawContent(GfxContext, force_draw);
  }

  void FilterRatings::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Button::PostDraw(GfxContext, force_draw);
  }

  void FilterRatings::RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags) {
    //FIXME - ripped out the code that detected which star you clicked on, it was awful and hacky
    //FIXME - set the rating based on which star was clicked on
  }


  void FilterRatings::OnRatingsChanged (int rating) {
    QueueDraw(); // make sure the view reflects whatever the latest rating is
    // FIXME - once we have shared library objects, update those too
  }

};
