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
#include "unity-shared/GraphicsUtils.h"
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

NUX_IMPLEMENT_OBJECT_TYPE(FilterMultiRangeWidget);

FilterMultiRangeWidget::FilterMultiRangeWidget(NUX_FILE_LINE_DECL)
  : FilterExpanderLabel(_("Multi-range"), NUX_FILE_LINE_PARAM)
  , all_button_(nullptr)
  , dragging_(false)
{
  InitTheme();

  dash::Style& style = dash::Style::Instance();
  const int left_padding = 0;
  const int right_padding = 0;
  const int top_padding = style.GetSpaceBetweenFilterWidgets() - style.GetFilterHighlightPadding() - 2;
  const int bottom_padding = style.GetFilterHighlightPadding() - 1;

  layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout_->SetLeftAndRightPadding(left_padding, right_padding);
  layout_->SetTopAndBottomPadding(top_padding, bottom_padding);

  SetContents(layout_);
  OnActiveChanged(false);

  mouse_move.connect(sigc::mem_fun(this, &FilterMultiRangeWidget::RecvMouseMove));
  mouse_down.connect(sigc::mem_fun(this, &FilterMultiRangeWidget::RecvMouseDown));
  mouse_up.connect(sigc::mem_fun(this, &FilterMultiRangeWidget::RecvMouseUp));

  mouse_drag.connect(sigc::mem_fun(this, &FilterMultiRangeWidget::RecvMouseDrag));
}

void FilterMultiRangeWidget::SetFilter(Filter::Ptr const& filter)
{
  // Reset filter.
  layout_->Clear();
  buttons_.clear();
  mouse_down_button_.Release();
  mouse_down_left_active_button_.Release();
  mouse_down_right_active_button_.Release();

  filter_ = std::static_pointer_cast<MultiRangeFilter>(filter);

  // all button
  auto show_button_func = [this] (bool show_all_button)
  {
    all_button_ = show_all_button ? new FilterAllButton(NUX_TRACKER_LOCATION) : nullptr;
    SetRightHandView(all_button_);
    if (all_button_)
      all_button_->SetFilter(filter_);
  };
  show_button_func(filter_->show_all_button);
  filter_->show_all_button.changed.connect(show_button_func);
  
  expanded = !filter_->collapsed();

  filter_->option_added.connect(sigc::mem_fun(this, &FilterMultiRangeWidget::OnOptionAdded));
  filter_->option_removed.connect(sigc::mem_fun(this, &FilterMultiRangeWidget::OnOptionRemoved));

  // finally - make sure we are up-todate with our filter list
  for (auto it : filter_->options())
    OnOptionAdded(it);

  SetLabel(filter_->name);
}

void FilterMultiRangeWidget::OnActiveChanged(bool value)
{
  // go through all the buttons, and set the state :(
  int start = 2000;
  int end = 0;
  int index = 0;
  for (auto button : buttons_)
  {
    FilterOption::Ptr filter = button->GetFilter();
    if (filter != nullptr)
    {
      bool tmp_active = filter->active;
      button->SetActive(tmp_active);
      
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

void FilterMultiRangeWidget::OnOptionAdded(FilterOption::Ptr const& new_filter)
{
  FilterMultiRangeButtonPtr button(new FilterMultiRangeButton(NUX_TRACKER_LOCATION));
  button->SetFilter(new_filter);
  layout_->AddView(button.GetPointer());
  buttons_.push_back(button);
  new_filter->active.changed.connect(sigc::mem_fun(this, &FilterMultiRangeWidget::OnActiveChanged));
  OnActiveChanged(false);

  QueueRelayout();
}

void FilterMultiRangeWidget::OnOptionRemoved(FilterOption::Ptr const& removed_filter)
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

  QueueRelayout();
}

std::string FilterMultiRangeWidget::GetFilterType()
{
  return "FilterMultiRangeWidget";
}

void FilterMultiRangeWidget::InitTheme()
{
  //FIXME - build theme here - store images, cache them, fun fun fun
}

nux::Area* FilterMultiRangeWidget::FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
{
  bool mouse_inside = TestMousePointerInclusionFilterMouseWheel(mouse_position, event_type);
  if (!mouse_inside)
    return nullptr;

  nux::Area* area = View::FindAreaUnderMouse(mouse_position, nux::NUX_MOUSE_MOVE);
  if (area && area->Type().IsDerivedFromType(FilterMultiRangeButton::StaticObjectType))
  {
    return this;
  }

  return area;
}

void FilterMultiRangeWidget::RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  nux::Geometry geo = GetAbsoluteGeometry();
  nux::Point abs_cursor(geo.x + x, geo.y + y);
  UpdateMouseFocus(abs_cursor);
}

void FilterMultiRangeWidget::RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  mouse_down_button_.Release();
  mouse_down_left_active_button_.Release();
  mouse_down_right_active_button_.Release();

  dragging_ = false;
  nux::Geometry geo = GetAbsoluteGeometry();
  nux::Point abs_cursor(geo.x + x, geo.y + y);

  nux::Area* area = View::FindAreaUnderMouse(nux::Point(abs_cursor.x, abs_cursor.y), nux::NUX_MOUSE_PRESSED);

  if (!area || !area->Type().IsDerivedFromType(FilterMultiRangeButton::StaticObjectType))
    return;
  mouse_down_button_ = static_cast<FilterMultiRangeButton*>(area);

  // Cache the left/right selected buttons.
  FilterMultiRangeButtonPtr last_selected_button;
  for (FilterMultiRangeButtonPtr button : buttons_)
  {
    if (button->Active())
    {
      if (!mouse_down_left_active_button_.IsValid())
        mouse_down_left_active_button_ = button;
      last_selected_button = button;
    }
  }
  mouse_down_right_active_button_ = last_selected_button;
}

void FilterMultiRangeWidget::RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags)
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

void FilterMultiRangeWidget::RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  nux::Geometry geo = GetAbsoluteGeometry();
  nux::Point abs_cursor(geo.x + x, geo.y + y);
  UpdateMouseFocus(abs_cursor);

  if (!CheckDrag())
    return;

  nux::Area* area = View::FindAreaUnderMouse(nux::Point(abs_cursor.x, abs_cursor.y), nux::NUX_MOUSE_MOVE);
  if (!area || !area->Type().IsDerivedFromType(FilterMultiRangeButton::StaticObjectType))
    return;

  FilterMultiRangeButtonPtr drag_over_button;
  drag_over_button = static_cast<FilterMultiRangeButton*>(area);
  if (!drag_over_button.IsValid())
    return;
  dragging_ = true;

  auto end = buttons_.end();
  int found_buttons = 0;
  for (auto iter = buttons_.begin(); iter != end; ++iter)
  {
    FilterMultiRangeButtonPtr button = *iter;
    bool activate = false;

    // if we've dragged the left button, we want to activate everything between the "drag over button" and the "right button"
    if (mouse_down_button_ == mouse_down_left_active_button_ &&
        button == mouse_down_right_active_button_)
    {
      found_buttons++;
      activate = true;
    }
    // if we've dragged the right button, we want to activate everything between the "left button" and the "drag over button"
    else if (mouse_down_button_ == mouse_down_right_active_button_ &&
        button == mouse_down_left_active_button_)
    {
      found_buttons++;
      activate = true;
    }

    if (button == drag_over_button)
    {
      found_buttons++;
      activate = true;
    }

    if (activate || (found_buttons > 0 && found_buttons < 2))
    {
      button->Activate();
    }
    else
    {
      button->Deactivate();
    }
  }
}

bool FilterMultiRangeWidget::CheckDrag()
{
  if (!mouse_down_button_)
    return false;

  auto end = buttons_.end();
  bool between = false;
  bool active_found = false;
  for (auto iter = buttons_.begin(); iter != end; ++iter)
  {
    FilterMultiRangeButtonPtr button = *iter;
    if (button->Active())
    {
      active_found = true;
      if (button == mouse_down_button_)
      {
        between = true;
      }
    }
    else if (active_found)
    {
      active_found = false;
      break;
    }
  }

  if (mouse_down_button_ != mouse_down_left_active_button_ && mouse_down_button_ != mouse_down_right_active_button_)
  {
    if (between)
      return false;
    mouse_down_left_active_button_ = mouse_down_button_;
    mouse_down_right_active_button_ = mouse_down_button_;
  }

  return true;
}

void FilterMultiRangeWidget::UpdateMouseFocus(nux::Point const& abs_cursor_position)
{
  nux::Area* area = View::FindAreaUnderMouse(nux::Point(abs_cursor_position.x, abs_cursor_position.y), nux::NUX_MOUSE_MOVE);
  if (!area || !area->Type().IsDerivedFromType(FilterMultiRangeButton::StaticObjectType))
    return;

  nux::GetWindowCompositor().SetKeyFocusArea(static_cast<InputArea*>(area), nux::KEY_NAV_NONE);
}

void FilterMultiRangeWidget::Click(FilterMultiRangeButtonPtr const& activated_button)
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

void FilterMultiRangeWidget::ClearRedirectedRenderChildArea()
{
  for (auto button : buttons_)
  {
    if (button->IsRedrawNeeded())
      graphics::ClearGeometry(button->GetGeometry());
  }
}

} // namespace dash
} // namespace unity
