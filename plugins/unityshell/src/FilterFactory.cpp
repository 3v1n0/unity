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
#include "Nux/View.h"

#include "FilterBasicButton.h"
#include "FilterRatingsWidget.h"

#include "FilterFactory.h"
namespace unity {

  FilterFactory::FilterFactory() {

  }

  FilterFactory::~FilterFactory() {

  }

  enum TMP_FILTER_TYPE {
    FILTER_GENRE = 0,
    FILTER_RATING = 1,
    FILTER_DATERANGE = 2,
    FILTER_TOGGLE = 3,
  };

  nux::View *FilterFactory::WidgetForFilter (void *filter)
  {
    /*
     * Pseudo code:
     * UnityFilter *filter = filter;
     * nux::View *view = NULL:
     * switch (filter.type) {
     *   case (UNITY_FILTER_GENRE):
     *    unity::FilterGenre *genre = new unity::FilterGenre ();
     *    genre->SetFilter (filter);
     *    view = genre;
     *    break;
     *
     *   case (UNITY_FILTER_RATING):
     *    unity::FilterRating *rating = new unity::FilterRating ();
     *    rating->SetFilter (filter);
     *    view = rating;
     *    break;
     * ...
     * ...
     * ...
     *
     * }
     * return view;
     */

    //TODO - needs to be hooked up to filters
    uint filter_type = FILTER_TOGGLE;
    nux::View *view = NULL;



    //TODO this does not conform to GCS i assume - I'll format it later
    switch (filter_type) {
      case (FILTER_GENRE): {
        //FIXME - removed the old code here, re-add it for the new widget
        break;
      }

      case (FILTER_RATING): {
        view = static_cast<nux::View *> (new FilterRatings ());
        break;
      };
      case (FILTER_DATERANGE): {
        //TODO - date range widget
        break;
      }

      case (FILTER_TOGGLE): {
        view = static_cast<nux::View *> (new FilterBasicButton ());
        break;
      };
    }

    dynamic_cast<FilterWidget *>(view)->SetFilter (filter);

    return view;
  }
}