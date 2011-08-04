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
namespace { // FIXME - fill with actual renderer type strings
  const std::string renderer_type_ratings = "ratings";
  const std::string renderer_type_multirange = "multi-range";
  const std::string renderer_type_check_options = "check-options";
}

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

  nux::View *FilterFactory::WidgetForFilter (dash::Filter::Ptr filter)
  {
    //TODO - needs to be hooked up to filters
    std::string filter_type = filter->renderer_name;
    nux::View *view = NULL;

    switch (filter_type) {
      case (renderer_type_check_options): {
        view = static_cast<nux::View *> (new FilterGenre(NUX_TRACKER_LOCATION));
        break;
      }
      case (renderer_type_ratings): {
        view = static_cast<nux::View *> (new FilterRatingsWidget (NUX_TRACKER_LOCATION));
        break;
      };
      case (renderer_type_multirange): {
        //TODO - date range widget
        break;
      }
    }

    dynamic_cast<FilterWidget *>(view)->SetFilter (filter);

    return view;
  }
}
