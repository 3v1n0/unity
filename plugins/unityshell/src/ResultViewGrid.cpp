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
#include <algorithm>
#include <Nux/Nux.h>
#include <NuxCore/Logger.h>
#include <Nux/VLayout.h>
#include <NuxImage/GdkGraphics.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "Timer.h"
#include "ubus-server.h"
#include "UBusMessages.h"
#include "ResultViewGrid.h"
#include "math.h"

namespace
{
nux::logging::Logger logger("unity.dash.results");
}

namespace unity
{
namespace dash
{
NUX_IMPLEMENT_OBJECT_TYPE(ResultViewGrid);

ResultViewGrid::ResultViewGrid(NUX_FILE_LINE_DECL)
  : ResultView(NUX_FILE_LINE_PARAM)
  , horizontal_spacing(0)
  , vertical_spacing(0)
  , padding(0)
  , mouse_over_index_(-1)
  , active_index_(-1)
  , selected_index_(-1)
  , preview_row_(0)
  , last_lazy_loaded_result_ (0)
  , lazy_load_handle_(0)
  , view_changed_handle_(0)
  , last_mouse_down_x_(-1)
  , last_mouse_down_y_(-1)
  , recorded_dash_width_(-1)
  , recorded_dash_height_(-1)
  , mouse_last_x_(-1)
  , mouse_last_y_(-1)
  , extra_horizontal_spacing_(0)
{
  SetAcceptKeyNavFocusOnMouseDown(false);

  auto needredraw_lambda = [&](int value)
  {
    NeedRedraw();
  };
  horizontal_spacing.changed.connect(needredraw_lambda);
  vertical_spacing.changed.connect(needredraw_lambda);
  padding.changed.connect(needredraw_lambda);

  key_nav_focus_change.connect(sigc::mem_fun(this, &ResultViewGrid::OnKeyNavFocusChange));
  key_nav_focus_activate.connect([&] (nux::Area *area) { UriActivated.emit (focused_uri_); });
  key_down.connect(sigc::mem_fun(this, &ResultViewGrid::OnKeyDown));
  mouse_move.connect(sigc::mem_fun(this, &ResultViewGrid::MouseMove));
  mouse_click.connect(sigc::mem_fun(this, &ResultViewGrid::MouseClick));

  mouse_down.connect([&](int x, int y, unsigned long mouse_state, unsigned long button_state)
  {
    last_mouse_down_x_ = x;
    last_mouse_down_y_ = y;
    uint index = GetIndexAtPosition(x, y);
    mouse_over_index_ = index;
  });

  mouse_leave.connect([&](int x, int y, unsigned long mouse_state, unsigned long button_state)
  {
    mouse_over_index_ = -1;
    mouse_last_x_ = -1;
    mouse_last_y_ = -1;
    NeedRedraw();
  });

  ubus_.RegisterInterest(UBUS_DASH_SIZE_CHANGED, [this] (GVariant* data) {
    // on dash size changed, we update our stored values, this sucks
    //FIXME in P - make dash size the size of our dash not the entire screen
    g_variant_get (data, "(ii)", &recorded_dash_width_, &recorded_dash_height_);
  });

  SetDndEnabled(true, false);
  NeedRedraw();
}

ResultViewGrid::~ResultViewGrid()
{
  if (lazy_load_handle_)
    g_source_remove(lazy_load_handle_);

  if (view_changed_handle_)
    g_source_remove(view_changed_handle_);
}

gboolean ResultViewGrid::OnLazyLoad (gpointer data)
{
  ResultViewGrid *self = (ResultViewGrid*)data;
  self->lazy_load_handle_ = 0;
  self->DoLazyLoad();
  return FALSE;
}

void ResultViewGrid::QueueLazyLoad()
{
  if (lazy_load_handle_ == 0)
  {
    lazy_load_handle_ = g_idle_add_full (G_PRIORITY_DEFAULT, (GSourceFunc)(&ResultViewGrid::OnLazyLoad), this, NULL);
  }
  last_lazy_loaded_result_ = 0; // we always want to reset the lazy load index here
}

void ResultViewGrid::QueueViewChanged()
{
  if (view_changed_handle_ == 0)
  {
    // using G_PRIORITY_HIGH because this needs to happen *before* next draw
    view_changed_handle_ = g_idle_add_full (G_PRIORITY_HIGH,
                                            [](gpointer data) -> gboolean
    {
      ResultViewGrid *self = (ResultViewGrid*)data;
      self->SizeReallocate();
      self->last_lazy_loaded_result_ = 0; // reset the lazy load index
      self->DoLazyLoad(); // also calls QueueDraw
      self->view_changed_handle_ = 0;
      return FALSE;
    }, this, NULL);
  }
}

void ResultViewGrid::DoLazyLoad()
{
  // FIXME - so this code was nice, it would only load the visible entries on the screen
  // however nux does not give us a good enough indicator right now that we are scrolling,
  // thus if you scroll more than a screen in one frame, you will end up with at least one frame where
  // no icons are displayed (they have not been preloaded yet) - it sucked. we should fix this next cycle when we can break api
  //~ int index = 0;
//~
  //~ ResultListBounds visible_bounds = GetVisableResults();
  //~ int lower_bound = std::get<0>(visible_bounds);
  //~ int upper_bound = std::get<1>(visible_bounds);
//~
  //~ ResultList::iterator it;
  //~ for (it = results_.begin(); it != results_.end(); it++)
  //~ {
    //~ if (index >= lower_bound && index <= upper_bound)
    //~ {
      //~ renderer_->Preload((*it));
    //~ }
    //~ index++;
  //~ }

  util::Timer timer;
  bool queue_additional_load = false; // if this is set, we will return early and start loading more next frame

  // instead we will just pre-load all the items if expanded or just one row if not
  int index = 0;
  int items_per_row = GetItemsPerRow();
  for (auto it = results_.begin() + last_lazy_loaded_result_; it != results_.end(); it++)
  {
    if ((!expanded && index < items_per_row) || expanded)
    {
      renderer_->Preload((*it));
      last_lazy_loaded_result_ = index;
    }

    if (timer.ElapsedSeconds() > 0.008)
    {
      queue_additional_load = true;
      break;
    }

    if (!expanded && index >= items_per_row)
      break; //early exit

    index++;
  }

  if (queue_additional_load)
  {
    //we didn't load all the results because we exceeded our time budget, so queue another lazy load
    if (lazy_load_handle_ == 0)
    {
      lazy_load_handle_ = g_timeout_add(1000/60 - 8, (GSourceFunc)(&ResultViewGrid::OnLazyLoad), this);
    }
  }

  QueueDraw();
}


int ResultViewGrid::GetItemsPerRow()
{
  int items_per_row = (GetGeometry().width - (padding * 2) + horizontal_spacing) / (renderer_->width + horizontal_spacing);
  return (items_per_row) ? items_per_row : 1; // always at least one item per row
}

void ResultViewGrid::SetPreview(PreviewBase* preview, Result& related_result)
{
  ResultView::SetPreview(preview, related_result);

  int items_per_row = GetItemsPerRow();
  uint row_size = renderer_->height + vertical_spacing;

  uint index = 0;
  ResultList::iterator it;
  for (it = results_.begin(); it != results_.end(); it++)
  {
    if ((*it).uri == preview_result_uri_)
    {
      break;
    }
    index++;
  }

  uint row_number = index / items_per_row;
  int y_position = row_number * row_size;

  preview_layout_->SetMinimumHeight(600);
  preview_layout_->SetGeometry(GetGeometry().x, y_position,
                               GetGeometry().width, 600);

  preview_layout_->SetBaseY(160);
  preview_layout_->SetMaximumHeight(600);

  SizeReallocate();

}

void ResultViewGrid::SetModelRenderer(ResultRenderer* renderer)
{
  ResultView::SetModelRenderer(renderer);
  SizeReallocate();
}

void ResultViewGrid::AddResult(Result& result)
{
  results_.push_back(result);
  QueueViewChanged();
}

void ResultViewGrid::RemoveResult(Result& result)
{
  ResultView::RemoveResult(result);
  QueueViewChanged();
}

void ResultViewGrid::SizeReallocate()
{
  //FIXME - needs to use the geometry assigned to it, but only after a layout
  int items_per_row = GetItemsPerRow();

  int total_rows = std::ceil(results_.size() / (double)items_per_row) ;
  int total_height = 0;

  if (expanded)
  {
    total_height = (total_rows * renderer_->height) + (total_rows * vertical_spacing);

    if (!preview_result_uri_.empty())
    {
      total_height += preview_layout_->GetGeometry().height + vertical_spacing;
    }
  }
  else
  {
    total_height = renderer_->height + vertical_spacing;
  }

  int width = (items_per_row * renderer_->width) + (padding*2) + ((items_per_row - 1) * horizontal_spacing);
  int geo_width = GetBaseWidth();
  int extra_width = geo_width - (width + 25-3);

  if (items_per_row != 1)
    extra_horizontal_spacing_ = extra_width / (items_per_row - 1);
  if (extra_horizontal_spacing_ < 0)
    extra_horizontal_spacing_ = 0;

  SetMinimumHeight(total_height + (padding * 2));
  SetMaximumHeight(total_height + (padding * 2));
  PositionPreview();

  mouse_over_index_ = GetIndexAtPosition(mouse_last_x_, mouse_last_y_);
  results_per_row = items_per_row;
}

void ResultViewGrid::PositionPreview()
{
  if (preview_layout_ == NULL)
    return;

  if (expanded == false)
    return;

  int items_per_row = GetItemsPerRow();
  int total_rows = (results_.size() / items_per_row) + 1;

  int row_size = renderer_->height + vertical_spacing;

  int y_position = padding;
  for (int row_index = 0; row_index <= total_rows; row_index++)
  {
    bool preview_in_this_row = false;

    for (int column_index = 0; column_index < items_per_row; column_index++)
    {
      uint index = (row_index * items_per_row) + column_index;
      if (index >= results_.size())
        break;

      if (results_[index].uri == preview_result_uri_)
        preview_in_this_row = true;
    }

    y_position += row_size;

    if (preview_in_this_row)
    {
      // the preview is in this row, so we want to position it below here
      preview_spacer_->SetMinimumHeight(y_position);
      preview_spacer_->SetMaximumHeight(y_position);
      preview_row_ = row_index;
      break;
    }
  }
}

bool ResultViewGrid::InspectKeyEvent(unsigned int eventType, unsigned int keysym, const char* character)
{
  nux::KeyNavDirection direction = nux::KEY_NAV_NONE;
  switch (keysym)
  {
    case NUX_VK_UP:
      direction = nux::KeyNavDirection::KEY_NAV_UP;
      break;
    case NUX_VK_DOWN:
      direction = nux::KeyNavDirection::KEY_NAV_DOWN;
      break;
    case NUX_VK_LEFT:
      direction = nux::KeyNavDirection::KEY_NAV_LEFT;
      break;
    case NUX_VK_RIGHT:
      direction = nux::KeyNavDirection::KEY_NAV_RIGHT;
      break;
    case NUX_VK_LEFT_TAB:
      direction = nux::KeyNavDirection::KEY_NAV_TAB_PREVIOUS;
      break;
    case NUX_VK_TAB:
      direction = nux::KeyNavDirection::KEY_NAV_TAB_NEXT;
      break;
    case NUX_VK_ENTER:
    case NUX_KP_ENTER:
      direction = nux::KeyNavDirection::KEY_NAV_ENTER;
      break;
    default:
      direction = nux::KeyNavDirection::KEY_NAV_NONE;
      break;
  }

  if (direction == nux::KeyNavDirection::KEY_NAV_NONE
      || direction == nux::KeyNavDirection::KEY_NAV_TAB_NEXT
      || direction == nux::KeyNavDirection::KEY_NAV_TAB_PREVIOUS
      || direction == nux::KeyNavDirection::KEY_NAV_ENTER)
  {
    // we don't handle these cases
    return false;
  }

  int items_per_row = GetItemsPerRow();
  int total_rows = std::ceil(results_.size() / static_cast<float>(items_per_row)); // items per row is always at least 1
  total_rows = (expanded) ? total_rows : 1; // restrict to one row if not expanded

   // check for edge cases where we want the keynav to bubble up
  if (direction == nux::KEY_NAV_LEFT && (selected_index_ % items_per_row == 0))
    return false; // pressed left on the first item, no diiice
  else if (direction == nux::KEY_NAV_RIGHT && (selected_index_ == static_cast<int>(results_.size() - 1)))
    return false; // pressed right on the last item, nope. nothing for you
  else if (direction == nux::KEY_NAV_RIGHT  && (selected_index_ % items_per_row) == (items_per_row - 1))
    return false; // pressed right on the last item in the first row in non expanded mode. nothing doing.
  else if (direction == nux::KEY_NAV_UP && selected_index_ < items_per_row)
    return false; // key nav up when already on top row
  else if (direction == nux::KEY_NAV_DOWN && selected_index_ >= (total_rows-1) * items_per_row)
    return false; // key nav down when on bottom row

  return true;
}

bool ResultViewGrid::AcceptKeyNavFocus()
{
  return true;
}

void ResultViewGrid::OnKeyDown (unsigned long event_type, unsigned long event_keysym,
                                unsigned long event_state, const TCHAR* character,
                                unsigned short key_repeat_count)
{
  nux::KeyNavDirection direction = nux::KEY_NAV_NONE;
  switch (event_keysym)
  {
    case NUX_VK_UP:
      direction = nux::KeyNavDirection::KEY_NAV_UP;
      break;
    case NUX_VK_DOWN:
      direction = nux::KeyNavDirection::KEY_NAV_DOWN;
      break;
    case NUX_VK_LEFT:
      direction = nux::KeyNavDirection::KEY_NAV_LEFT;
      break;
    case NUX_VK_RIGHT:
      direction = nux::KeyNavDirection::KEY_NAV_RIGHT;
      break;
    case NUX_VK_LEFT_TAB:
      direction = nux::KeyNavDirection::KEY_NAV_TAB_PREVIOUS;
      break;
    case NUX_VK_TAB:
      direction = nux::KeyNavDirection::KEY_NAV_TAB_NEXT;
      break;
    case NUX_VK_ENTER:
    case NUX_KP_ENTER:
      direction = nux::KeyNavDirection::KEY_NAV_ENTER;
      break;
    default:
      direction = nux::KeyNavDirection::KEY_NAV_NONE;
      break;
  }

  // if we got this far, we definately got a keynav signal

  ResultList::iterator current_focused_result = results_.end();
  if (focused_uri_.empty())
    focused_uri_ = results_.front().uri;

  std::string next_focused_uri;
  ResultList::iterator it;
  int items_per_row = GetItemsPerRow();
  int total_rows = std::ceil(results_.size() / static_cast<float>(items_per_row)); // items per row is always at least 1
  total_rows = (expanded) ? total_rows : 1; // restrict to one row if not expanded

  // find the currently focused item
  for (it = results_.begin(); it != results_.end(); it++)
  {
    std::string result_uri = (*it).uri;
    if (result_uri == focused_uri_)
    {
      current_focused_result = it;
      break;
    }
  }

  if (direction == nux::KEY_NAV_LEFT && (selected_index_ == 0))
    return; // pressed left on the first item, no diiice

  if (direction == nux::KEY_NAV_RIGHT && (selected_index_ == static_cast<int>(results_.size() - 1)))
    return; // pressed right on the last item, nope. nothing for you

  if (direction == nux::KEY_NAV_RIGHT && !expanded && selected_index_ == items_per_row - 1)
    return; // pressed right on the last item in the first row in non expanded mode. nothing doing.

  switch (direction)
  {
    case (nux::KEY_NAV_LEFT):
    {
      --selected_index_;
      break;
    }
    case (nux::KEY_NAV_RIGHT):
    {
      ++selected_index_;
      break;
    }
    case (nux::KEY_NAV_UP):
    {
      selected_index_ -= items_per_row;
      break;
    }
    case (nux::KEY_NAV_DOWN):
    {
      selected_index_ += items_per_row;
      break;
    }
    default:
      break;
  }

  selected_index_ = std::max(0, selected_index_);
  selected_index_ = std::min(static_cast<int>(results_.size() - 1), selected_index_);
  focused_uri_ = results_[selected_index_].uri;

  int focused_x = (renderer_->width + horizontal_spacing + extra_horizontal_spacing_) * (selected_index_ % items_per_row);
  int focused_y = (renderer_->height + vertical_spacing) * (selected_index_ / items_per_row);

  ubus_.SendMessage(UBUS_RESULT_VIEW_KEYNAV_CHANGED,
                    g_variant_new("(iiii)", focused_x, focused_y, renderer_->width(), renderer_->height()));
  selection_change.emit();

  NeedRedraw();
}

nux::Area* ResultViewGrid::KeyNavIteration(nux::KeyNavDirection direction)
{
  return this;
}

void ResultViewGrid::OnKeyNavFocusChange(nux::Area *area, bool has_focus, nux::KeyNavDirection direction)
{
  if (HasKeyFocus())
  {
    if (selected_index_ < 0)
    {
        focused_uri_ = results_.front().uri;
        selected_index_ = 0;
    }

    int focused_x = 0;
    int focused_y = 0;

    int items_per_row = GetItemsPerRow();

    if (direction == nux::KEY_NAV_UP && expanded)
    {
      // This View just got focused through keyboard navigation and the
      // focus is comming from the bottom. We want to focus the
      // first item (on the left) of the last row in this grid.

      int total_rows = std::ceil(results_.size() / (double)items_per_row);
      selected_index_ = items_per_row * (total_rows-1);

      focused_x = (renderer_->width + horizontal_spacing + extra_horizontal_spacing_) * (selected_index_ % items_per_row);
      focused_y = (renderer_->height + vertical_spacing) * (selected_index_ / items_per_row);

    }
    else
    {
      focused_x = (renderer_->width + horizontal_spacing + extra_horizontal_spacing_) * (selected_index_ % items_per_row);
      focused_y = (renderer_->height + vertical_spacing) * (selected_index_ / items_per_row);
    }

    if (direction != nux::KEY_NAV_NONE)
    {
      ubus_.SendMessage(UBUS_RESULT_VIEW_KEYNAV_CHANGED,
                        g_variant_new("(iiii)", focused_x, focused_y, renderer_->width(), renderer_->height()));
    }

    selection_change.emit();

  }
  else
  {
    selected_index_ = -1;
    focused_uri_.clear();

    selection_change.emit();
  }

  NeedRedraw();
}

long ResultViewGrid::ComputeContentSize()
{
  SizeReallocate();
  QueueLazyLoad();
  long ret = ResultView::ComputeContentSize();
  return ret;
}


typedef std::tuple <int, int> ResultListBounds;
ResultListBounds ResultViewGrid::GetVisableResults()
{
  int items_per_row = GetItemsPerRow();
  int start, end;

  if (!expanded)
  {
    // we are not expanded, so the bounds are known
    start = 0;
    end = items_per_row - 1;
  }
  else
  {
    //find the row we start at
    int absolute_y = GetAbsoluteY() - GetToplevel()->GetAbsoluteY();
    uint row_size = renderer_->height + vertical_spacing;

    if (absolute_y < 0)
    {
      // we are scrolled up a little,
      int row_index = abs(absolute_y) / row_size;
      start = row_index * items_per_row;
    }
    else
    {
      start = 0;
    }

    if (absolute_y + GetAbsoluteHeight() > GetToplevel()->GetAbsoluteHeight())
    {
      // our elements overflow the visable viewport
      int visible_height = (GetToplevel()->GetAbsoluteHeight() - std::max(absolute_y, 0));
      visible_height = std::min(visible_height, absolute_y + GetAbsoluteHeight());

      int visible_rows = std::ceil(visible_height / static_cast<float>(row_size));
      end = start + (visible_rows * items_per_row) + items_per_row;
    }
    else
    {
      end = results_.size() - 1;
    }
  }

  start = std::max(start, 0);
  end = std::min(end, static_cast<int>(results_.size()) - 1);

  return ResultListBounds(start, end);
}

void ResultViewGrid::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  int items_per_row = GetItemsPerRow();
  uint total_rows = (!expanded) ? 0 : (results_.size() / items_per_row) + 1;

  ResultView::ResultList::iterator it;

  //find the row we start at
  int absolute_y = GetAbsoluteY();
  uint row_size = renderer_->height + vertical_spacing;

  int y_position = padding + GetGeometry().y;

  ResultListBounds visible_bounds = GetVisableResults();

  for (uint row_index = 0; row_index <= total_rows; row_index++)
  {
    int row_lower_bound = row_index * items_per_row;
    if (row_lower_bound >= std::get<0>(visible_bounds) &&
        row_lower_bound <= std::get<1>(visible_bounds))
    {
      int x_position = padding + GetGeometry().x;
      for (int column_index = 0; column_index < items_per_row; column_index++)
      {
        uint index = (row_index * items_per_row) + column_index;
        if (index >= results_.size())
          break;

        ResultRenderer::ResultRendererState state = ResultRenderer::RESULT_RENDERER_NORMAL;
        if ((int)(index) == selected_index_)
        {
          state = ResultRenderer::RESULT_RENDERER_SELECTED;
        }
        else if ((int)(index) == active_index_)
        {
          state = ResultRenderer::RESULT_RENDERER_ACTIVE;
        }

        int half_width = recorded_dash_width_ / 2;
        int half_height = recorded_dash_height_;

        int offset_x, offset_y;

        /* Guard against divide-by-zero. SIGFPEs are not mythological
         * contrary to popular belief */
        if (half_width >= 10)
          offset_x = MAX(MIN((x_position - half_width) / (half_width / 10), 5), -5);
        else
          offset_x = 0;

        if (half_height >= 10)
          offset_y = MAX(MIN(((y_position + absolute_y) - half_height) / (half_height / 10), 5), -5);
        else
          offset_y = 0;

        if (recorded_dash_width_ < 1 || recorded_dash_height_ < 1)
        {
          offset_x = 0;
          offset_y = 0;
        }
        nux::Geometry render_geo(x_position, y_position, renderer_->width, renderer_->height);
//nux::GetPainter().Paint2DQuadColor(GfxContext, render_geo, nux::color::Blue*0.20);
        renderer_->Render(GfxContext, results_[index], state, render_geo, offset_x, offset_y);

        x_position += renderer_->width + horizontal_spacing + extra_horizontal_spacing_;
      }
    }

    // if we have a preview displaying on this row, flow the rest of the items down
    if (preview_layout_ != NULL && preview_row_ == row_index)
    {
      y_position += preview_layout_->GetGeometry().height + vertical_spacing;

    }
    y_position += row_size;
  }
}

void ResultViewGrid::DrawContent(nux::GraphicsEngine& GfxContent, bool force_draw)
{
  nux::Geometry base = GetGeometry();
  GfxContent.PushClippingRectangle(base);

  if (GetCompositionLayout())
  {
    nux::Geometry geo = GetCompositionLayout()->GetGeometry();
    GetCompositionLayout()->ProcessDraw(GfxContent, force_draw);
  }

  GfxContent.PopClippingRectangle();
}


void ResultViewGrid::MouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  uint index = GetIndexAtPosition(x, y);

  if (mouse_over_index_ != index)
  {
    selected_index_ = mouse_over_index_ = index;
    nux::GetWindowCompositor().SetKeyFocusArea(this);
  }
  mouse_last_x_ = x;
  mouse_last_y_ = y;

  NeedRedraw();
}

void ResultViewGrid::MouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  uint index = GetIndexAtPosition(x, y);
  mouse_over_index_ = index;
  if (index >= 0 && index < results_.size())
  {
    // we got a click on a button so activate it
    Result result = results_[index];
    selected_index_ = index;
    focused_uri_ = result.uri;
    UriActivated.emit(result.uri);
  }
}

uint ResultViewGrid::GetIndexAtPosition(int x, int y)
{
  uint items_per_row = GetItemsPerRow();

  uint column_size = renderer_->width + horizontal_spacing + extra_horizontal_spacing_;
  uint row_size = renderer_->height + vertical_spacing;

  if (preview_layout_ != NULL && (y - padding) / row_size > preview_row_)
  {
    // because we are "above" the preview row, we can just fudge the number
    y -= preview_layout_->GetGeometry().height + vertical_spacing;
  }

  int x_bound = items_per_row * column_size + padding;

  if ((x < padding) || (x >= x_bound))
    return -1;

  if (y < padding)
    return -1;

  uint row_number = std::max((y - padding), 0) / row_size ;
  uint column_number = std::max((x - padding), 0) / column_size;

  return (row_number * items_per_row) + column_number;
}

/* --------
   DND code
   --------
*/
bool
ResultViewGrid::DndSourceDragBegin()
{

  uint drag_index = GetIndexAtPosition(last_mouse_down_x_, last_mouse_down_y_);

  if (drag_index >= results_.size())
    return false;

  Reference();

  Result drag_result = results_[drag_index];

  current_drag_uri_ = drag_result.dnd_uri;
  if (current_drag_uri_ == "")
    current_drag_uri_ = drag_result.uri().substr(drag_result.uri().find(":") + 1);

  current_drag_icon_name_ = drag_result.icon_hint;

  LOG_DEBUG (logger) << "Dnd begin at " <<
                     last_mouse_down_x_ << ", " << last_mouse_down_y_ << " - using; "
                     << current_drag_uri_ << " - "
                     << current_drag_icon_name_;

  return true;
}

GdkPixbuf *
_icon_hint_get_drag_pixbuf (std::string icon_hint)
{
  GdkPixbuf *pbuf;
  GtkIconTheme *theme;
  GtkIconInfo *info;
  GError *error = NULL;
  GIcon *icon;
  int size = 64;
  if (icon_hint.empty())
    icon_hint = "application-default-icon";
  if (g_str_has_prefix(icon_hint.c_str(), "/"))
  {
    pbuf = gdk_pixbuf_new_from_file_at_scale (icon_hint.c_str(),
                                              size, size, FALSE, &error);
    if (error != NULL || !pbuf || !GDK_IS_PIXBUF (pbuf))
    {
      icon_hint = "application-default-icon";
      g_error_free (error);
      error = NULL;
    }
    else
      return pbuf;
  }
  theme = gtk_icon_theme_get_default();
  icon = g_icon_new_for_string(icon_hint.c_str(), NULL);

  if (G_IS_ICON(icon))
  {
     info = gtk_icon_theme_lookup_by_gicon(theme, icon, size, (GtkIconLookupFlags)0);
      g_object_unref(icon);
  }
  else
  {
     info = gtk_icon_theme_lookup_icon(theme,
                                        icon_hint.c_str(),
                                        size,
                                        (GtkIconLookupFlags) 0);
  }

  if (!info)
  {
      info = gtk_icon_theme_lookup_icon(theme,
                                        "application-default-icon",
                                        size,
                                        (GtkIconLookupFlags) 0);
  }

  if (gtk_icon_info_get_filename(info) == NULL)
  {
      gtk_icon_info_free(info);
      info = gtk_icon_theme_lookup_icon(theme,
                                        "application-default-icon",
                                        size,
                                        (GtkIconLookupFlags) 0);
  }

  pbuf = gtk_icon_info_load_icon(info, &error);

  if (error != NULL)
  {
    LOG_WARN (logger) << "could not find a pixbuf for " << icon_hint;
    g_error_free (error);
    pbuf = NULL;
  }

  gtk_icon_info_free(info);
  return pbuf;
}

nux::NBitmapData*
ResultViewGrid::DndSourceGetDragImage()
{
  nux::NBitmapData* result = 0;
  GdkPixbuf* pbuf;
  pbuf = _icon_hint_get_drag_pixbuf (current_drag_icon_name_);

  if (pbuf && GDK_IS_PIXBUF(pbuf))
  {
    // we don't free the pbuf as GdkGraphics will do it for us will do it for us
    nux::GdkGraphics graphics(pbuf);
    result = graphics.GetBitmap();
  }

  return result;
}

std::list<const char*>
ResultViewGrid::DndSourceGetDragTypes()
{
  std::list<const char*> result;
  result.push_back("text/uri-list");
  return result;
}

const char*
ResultViewGrid::DndSourceGetDataForType(const char* type, int* size, int* format)
{
  *format = 8;

  if (!current_drag_uri_.empty())
  {
    *size = strlen(current_drag_uri_.c_str());
    return current_drag_uri_.c_str();
  }
  else
  {
    *size = 0;
    return 0;
  }
}

void
ResultViewGrid::DndSourceDragFinished(nux::DndAction result)
{
  UnReference();
  last_mouse_down_x_ = -1;
  last_mouse_down_y_ = -1;
  current_drag_uri_.clear();
  current_drag_icon_name_.clear();

  // We need this because the drag can start in a ResultViewGrid and can
  // end in another ResultViewGrid
  EmitMouseLeaveSignal(0, 0, 0, 0);

  // We need an extra mouse motion to highlight the icon under the mouse
  // as soon as dnd finish
  Display* display = nux::GetGraphicsDisplay()->GetX11Display();
  if (display)
  {
    XWarpPointer(display, None, None, 0, 0, 0, 0, 0, 0);
    XSync(display, 0);
  }
}

int
ResultViewGrid::GetSelectedIndex()
{
  return selected_index_;
}

}
}
