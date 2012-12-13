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
#include "config.h"
#include <glib/gi18n-lib.h>

namespace unity
{
namespace dash
{

NUX_IMPLEMENT_OBJECT_TYPE(FilterMultiRange);

FilterMultiRange::FilterMultiRange(NUX_FILE_LINE_DECL)
  : FilterExpanderLabel(_("Multi-range"), NUX_FILE_LINE_PARAM)
  , dragging_(false)
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

  mouse_move.connect(sigc::mem_fun(this, &FilterMultiRange::RecvMouseMove));
  mouse_leave.connect(sigc::mem_fun(this, &FilterMultiRange::RecvMouseLeave));
  mouse_down.connect(sigc::mem_fun(this, &FilterMultiRange::RecvMouseDown));
  mouse_up.connect(sigc::mem_fun(this, &FilterMultiRange::RecvMouseUp));

  mouse_drag.connect(sigc::mem_fun(this, &FilterMultiRange::RecvMouseDrag));
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
  FilterMultiRangeButtonPtr button(new FilterMultiRangeButton(NUX_TRACKER_LOCATION));
  button->SetFilter(new_filter);
  layout_->AddView(button.GetPointer());
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
      layout_->RemoveChildObject(it->GetPointer());
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

nux::Area* FilterMultiRange::FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
{
  bool mouse_inside = TestMousePointerInclusionFilterMouseWheel(mouse_position, event_type);
  if (mouse_inside == false)
    return NULL;

  nux::Area* area = View::FindAreaUnderMouse(mouse_position, nux::NUX_MOUSE_MOVE);
  if (area && area->Type().IsDerivedFromType(FilterMultiRangeButton::StaticObjectType))
  {
    return this;
  }

  return area;
}

void FilterMultiRange::RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  nux::Geometry geo = GetAbsoluteGeometry();
  nux::Point abs_cursor(geo.x + x, geo.y + y);
  UpdateMouseFocus(abs_cursor);
}

void FilterMultiRange::RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
}

void FilterMultiRange::RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  dragging_ = false;
  nux::Geometry geo = GetAbsoluteGeometry();
  nux::Point abs_cursor(geo.x + x, geo.y + y);
  nux::Area* area = View::FindAreaUnderMouse(nux::Point(abs_cursor.x, abs_cursor.y), nux::NUX_MOUSE_PRESSED);
  if (!area || !area->Type().IsDerivedFromType(FilterMultiRangeButton::StaticObjectType))
    return;

  mouse_down_button_ = static_cast<FilterMultiRangeButton*>(area);
}

void FilterMultiRange::RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  FilterMultiRangeButtonPtr mouse_down_button(mouse_down_button_);
  mouse_down_button_.Release();

  if (dragging_)
  {
    dragging_ = false;
    return;
  }

  nux::Geometry geo = GetAbsoluteGeometry();
  nux::Area* area = View::FindAreaUnderMouse(nux::Point(geo.x + x, geo.y + y), nux::NUX_MOUSE_RELEASED);
  if (!area || !area->Type().IsDerivedFromType(FilterMultiRangeButton::StaticObjectType))
    return;

  FilterMultiRangeButtonPtr mouse_up_button;
  mouse_up_button = static_cast<FilterMultiRangeButton*>(area);
  if (mouse_up_button == mouse_down_button)
    Click(mouse_up_button);
}

void FilterMultiRange::RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  if (!mouse_down_button_)
    return;

  nux::Geometry geo = GetAbsoluteGeometry();
  nux::Point abs_cursor(geo.x + x, geo.y + y);
  UpdateMouseFocus(abs_cursor);

  nux::Area* area = View::FindAreaUnderMouse(nux::Point(abs_cursor.x, abs_cursor.y), nux::NUX_MOUSE_MOVE);
  if (!area || !area->Type().IsDerivedFromType(FilterMultiRangeButton::StaticObjectType))
    return;

  FilterMultiRangeButtonPtr drag_over_button;
  drag_over_button = static_cast<FilterMultiRangeButton*>(area);
  if (!drag_over_button.IsValid())
    return;
  dragging_ = true;

  nux::Geometry const& mouse_down_button_geometry = mouse_down_button_->GetAbsoluteGeometry();

  for (FilterMultiRangeButtonPtr button : buttons_)
  {
    nux::Geometry const& button_geometry = button->GetAbsoluteGeometry();
    if (button_geometry.x <= mouse_down_button_geometry.x && button_geometry.x+button_geometry.width >= abs_cursor.x)
      button->Activate();
    else if (button_geometry.x >= mouse_down_button_geometry.x && button_geometry.x <= abs_cursor.x)
      button->Activate();
    else
      button->Deactivate();
  }
}

void FilterMultiRange::UpdateMouseFocus(nux::Point const& abs_cursor_position)
{
  nux::Area* area = View::FindAreaUnderMouse(nux::Point(abs_cursor_position.x, abs_cursor_position.y), nux::NUX_MOUSE_MOVE);
  if (!area || !area->Type().IsDerivedFromType(FilterMultiRangeButton::StaticObjectType))
    return;

  nux::GetWindowCompositor().SetKeyFocusArea(static_cast<InputArea*>(area), nux::KEY_NAV_NONE);
}

void FilterMultiRange::Click(FilterMultiRangeButtonPtr const& activated_button)
{
  bool current_activated = activated_button->Active();
  bool any_others_active = false;

  for (FilterMultiRangeButtonPtr button : buttons_)
  {
    if (button != activated_button)
    {
      if (button->Active())
        any_others_active = true;
      button->Deactivate();
    }
  }

  if (!any_others_active && current_activated)
      activated_button->Deactivate();
  else
    activated_button->Activate();
}

} // namespace dash
} // namespace unity
