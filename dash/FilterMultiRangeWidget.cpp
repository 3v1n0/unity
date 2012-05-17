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

#include "unity-shared/DashStyle.h"
#include "FilterMultiRangeWidget.h"
#include "FilterMultiRangeButton.h"
#include "FilterBasicButton.h"

#include <glib.h>
#include <glib/gi18n-lib.h>

namespace unity
{
namespace dash
{

NUX_IMPLEMENT_OBJECT_TYPE(FilterMultiRange);

FilterMultiRange::FilterMultiRange(NUX_FILE_LINE_DECL)
  : FilterExpanderLabel(_("Multi-range"), NUX_FILE_LINE_PARAM)
{
  InitTheme();

  dash::Style& style = dash::Style::Instance();
  const int left_padding = 0;
  const int right_padding = 0;
  const int top_padding = style.GetSpaceBetweenFilterWidgets() - style.GetFilterHighlightPadding() - 2;
  const int bottom_padding = style.GetFilterHighlightPadding() - 1;

  all_button_ = new FilterAllButton(NUX_TRACKER_LOCATION);

  layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout_->SetLeftAndRightPadding(left_padding, right_padding);
  layout_->SetTopAndBottomPadding(top_padding, bottom_padding);

  SetRightHandView(all_button_);
  SetContents(layout_);
  OnActiveChanged(false);
}

FilterMultiRange::~FilterMultiRange()
{
}

void FilterMultiRange::SetFilter(Filter::Ptr const& filter)
{
  filter_ = std::static_pointer_cast<MultiRangeFilter>(filter);

  all_button_->SetFilter(filter_);
  expanded = !filter_->collapsed();

  filter_->option_added.connect(sigc::mem_fun(this, &FilterMultiRange::OnOptionAdded));
  filter_->option_removed.connect(sigc::mem_fun(this, &FilterMultiRange::OnOptionRemoved));

  // finally - make sure we are up-todate with our filter list
  for (auto it : filter_->options())
    OnOptionAdded(it);

  SetLabel(filter_->name);
}

void FilterMultiRange::OnActiveChanged(bool value)
{
  // go through all the buttons, and set the state :(
  int start = 2000;
  int end = 0;
  int index = 0;
  for (auto button : buttons_)
  {
    FilterOption::Ptr filter = button->GetFilter();
    bool tmp_active = filter->active;
    button->SetActive(tmp_active);
    if (filter != nullptr)
    {
      if (filter->active)
      {
        if (index < start)
          start = index;
        if (index > end)
          end = index;
      }
    }
    index++;
  }

  index = 0;
  for (auto button : buttons_)
  {
    if (index == start && index == end)
      button->SetHasArrow(MultiRangeArrow::BOTH);
    else if (index == start)
      button->SetHasArrow(MultiRangeArrow::LEFT);
    else if (index == end && index != 0)
      button->SetHasArrow(MultiRangeArrow::RIGHT);
    else
      button->SetHasArrow(MultiRangeArrow::NONE);

    if (index == 0)
      button->SetVisualSide(MultiRangeSide::LEFT);
    else if (index == (int)buttons_.size() - 1)
      button->SetVisualSide(MultiRangeSide::RIGHT);
    else
      button->SetVisualSide(MultiRangeSide::CENTER);

    index++;
  }
}

void FilterMultiRange::OnOptionAdded(FilterOption::Ptr const& new_filter)
{
  FilterMultiRangeButton* button = new FilterMultiRangeButton(NUX_TRACKER_LOCATION);
  button->SetFilter(new_filter);
  layout_->AddView(button);
  buttons_.push_back(button);
  new_filter->active.changed.connect(sigc::mem_fun(this, &FilterMultiRange::OnActiveChanged));
  OnActiveChanged(false);

}

void FilterMultiRange::OnOptionRemoved(FilterOption::Ptr const& removed_filter)
{
  for (auto it=buttons_.begin() ; it != buttons_.end(); it++)
  {
    if ((*it)->GetFilter() == removed_filter)
    {
      layout_->RemoveChildObject(*it);
      buttons_.erase(it);
      break;
    }
  }

  OnActiveChanged(false);
}

std::string FilterMultiRange::GetFilterType()
{
  return "FilterMultiRange";
}

void FilterMultiRange::InitTheme()
{
  //FIXME - build theme here - store images, cache them, fun fun fun
}

} // namespace dash
} // namespace unity
