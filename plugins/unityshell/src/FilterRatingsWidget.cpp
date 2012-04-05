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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */

#include <Nux/Nux.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "DashStyle.h"
#include "FilterGenreWidget.h"
#include "FilterGenreButton.h"
#include "FilterBasicButton.h"
#include "FilterRatingsButton.h"
#include "FilterRatingsWidget.h"

namespace
{
const int star_size      = 28;
}

namespace unity
{
namespace dash
{

NUX_IMPLEMENT_OBJECT_TYPE(FilterRatingsWidget);

FilterRatingsWidget::FilterRatingsWidget(NUX_FILE_LINE_DECL)
  : FilterExpanderLabel(_("Rating"), NUX_FILE_LINE_PARAM)
{
  all_button_ = new FilterAllButton(NUX_TRACKER_LOCATION);

  dash::Style& style = dash::Style::Instance();
  const int top_padding    = style.GetSpaceBetweenFilterWidgets() - style.GetFilterHighlightPadding() - 1; // -1 (PNGs have an 1px top padding)
  const int bottom_padding = style.GetFilterHighlightPadding();

  nux::VLayout* layout = new nux::VLayout(NUX_TRACKER_LOCATION);
  layout->SetTopAndBottomPadding(top_padding, bottom_padding);
  ratings_ = new FilterRatingsButton(NUX_TRACKER_LOCATION);
  ratings_->SetMinimumHeight(star_size);

  layout->AddView(ratings_);

  SetRightHandView(all_button_);
  SetContents(layout);
}

FilterRatingsWidget::~FilterRatingsWidget()
{
}

void FilterRatingsWidget::SetFilter(Filter::Ptr const& filter)
{
  filter_ = std::static_pointer_cast<RatingsFilter>(filter);

  all_button_->SetFilter(filter_);
  expanded = !filter_->collapsed();
  ratings_->SetFilter(filter_);

  SetLabel(filter_->name);
  NeedRedraw();
}

std::string FilterRatingsWidget::GetFilterType()
{
  return "FilterRatingsWidget";
}

} // namespace dash
} // namespace unity
