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
#include <NuxGraphics/GdkGraphics.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <unity-protocol.h>

#include <UnityCore/Variant.h>
#include "unity-shared/IntrospectableWrappers.h"
#include "unity-shared/Timer.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/GraphicsUtils.h"
#include "unity-shared/UnitySettings.h"
#include "ResultViewGrid.h"
#include "math.h"

DECLARE_LOGGER(logger, "unity.dash.results");

namespace unity
{
namespace dash
{

namespace
{
  const float UNFOCUSED_GHOST_ICON_OPACITY_REF = 0.3f;
  const float UNFOCUSED_ICON_SATURATION_REF = 0.05f;

  const float FOCUSED_GHOST_ICON_OPACITY_REF = 0.7f;
  const float FOCUSED_ICON_SATURATION_REF = 0.5f;

  const int DOUBLE_CLICK_SPEED = 500; //500 ms (double-click speed hardcoded to 400 ms in nux)
}

NUX_IMPLEMENT_OBJECT_TYPE(ResultViewGrid);

ResultViewGrid::ResultViewGrid(NUX_FILE_LINE_DECL)
  : ResultView(NUX_FILE_LINE_PARAM)
  , horizontal_spacing(0)
  , vertical_spacing(0)
  , padding(0)
  , mouse_over_index_(-1)
  , active_index_(-1)
  , selected_index_(-1)
  , last_lazy_loaded_result_(0)
  , all_results_preloaded_(true)
  , last_mouse_down_x_(-1)
  , last_mouse_down_y_(-1)
  , drag_index_(~0)
  , recorded_dash_width_(-1)
  , recorded_dash_height_(-1)
  , mouse_last_x_(-1)
  , mouse_last_y_(-1)
  , extra_horizontal_spacing_(0)
{
  EnableDoubleClick(true);
  SetAcceptKeyNavFocusOnMouseDown(false);

  auto needredraw_lambda = [&](int value) { NeedRedraw(); };
  horizontal_spacing.changed.connect(needredraw_lambda);
  vertical_spacing.changed.connect(needredraw_lambda);
  padding.changed.connect(needredraw_lambda);
  selected_index_.changed.connect(needredraw_lambda);
  expanded.changed.connect([&](bool value) { if (value) all_results_preloaded_ = false; });
  results_per_row.changed.connect([&](int value) { if (value > 0) all_results_preloaded_ = false; });

  key_nav_focus_change.connect(sigc::mem_fun(this, &ResultViewGrid::OnKeyNavFocusChange));
  key_nav_focus_activate.connect([&] (nux::Area *area)
  {
    Activate(focused_result_, selected_index_, ResultView::ActivateType::DIRECT);
  });
  key_down.connect(sigc::mem_fun(this, &ResultViewGrid::OnKeyDown));
  mouse_move.connect(sigc::mem_fun(this, &ResultViewGrid::MouseMove));
  mouse_click.connect(sigc::mem_fun(this, &ResultViewGrid::MouseClick));
  mouse_double_click.connect(sigc::mem_fun(this, &ResultViewGrid::MouseDoubleClick));

  mouse_down.connect([&](int x, int y, unsigned long mouse_state, unsigned long button_state)
  {
    last_mouse_down_x_ = x;
    last_mouse_down_y_ = y;
    unsigned index = GetIndexAtPosition(x, y);
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

  // We are interested in the color of the desktop background.
  ubus_.RegisterInterest(UBUS_BACKGROUND_COLOR_CHANGED, [this] (GVariant* data) {
    double red = 0.0f, green = 0.0f, blue = 0.0f, alpha = 0.0f;

    g_variant_get(data, "(dddd)", &red, &green, &blue, &alpha);
    background_color_ = nux::Color(red, green, blue, alpha);
    QueueDraw();
  });
  ubus_.SendMessage(UBUS_BACKGROUND_REQUEST_COLOUR_EMIT);

  ubus_.RegisterInterest(UBUS_DASH_PREVIEW_NAVIGATION_REQUEST, [&] (GVariant* data) {
    int nav_mode = 0;
    GVariant* local_result_variant = NULL;
    glib::String proposed_unique_id;
    g_variant_get(data, "(ivs)", &nav_mode, &local_result_variant, &proposed_unique_id);
    LocalResult local_result(LocalResult::FromVariant(local_result_variant));
    g_variant_unref(local_result_variant);

    if (proposed_unique_id.Str() != unique_id())
      return;

    unsigned num_results = GetNumResults();
    if (local_result == activated_result_)
    {
      int current_index = GetIndexForLocalResult(activated_result_);
      if (nav_mode == -1) // left
      {
        current_index--;
      }
      else if (nav_mode == 1) // right
      {
        current_index++;
      }

      if (current_index < 0 || static_cast<unsigned int>(current_index) >= num_results)
      {
        LOG_ERROR(logger) << "requested to activated a result that does not exist: " << current_index;
        return;
      }

      // closed
      if (nav_mode == 0)
      {
        activated_result_.clear();
      }
      else
      {
        selected_index_ = active_index_ = current_index;
        activated_result_ = GetLocalResultForIndex(current_index);
        LOG_DEBUG(logger) << "activating preview for index: "
                  << "(" << current_index << ")"
                  << " " << activated_result_.uri;
        Activate(activated_result_, current_index, ActivateType::PREVIEW);
      }
    }

  });

  SetDndEnabled(true, false);
}

void ResultViewGrid::Activate(LocalResult const& local_result, int index, ResultView::ActivateType type)
{
  activate_timer_.reset();
  unsigned num_results = GetNumResults();

  int left_results = index;
  int right_results = num_results ? (num_results - index) - 1 : 0;
  //FIXME - just uses y right now, needs to use the absolute position of the bottom of the result
  // (jay) Here is the fix: Compute the y position of the row where the item is located.
  nux::Geometry abs_geo = GetAbsoluteGeometry();
  int row_y = padding + abs_geo.y;
  int column_x = padding + abs_geo.x;
  int row_height = renderer_->height + vertical_spacing;
  int column_width = renderer_->width + horizontal_spacing;

  if (GetItemsPerRow())
  {
    int num_results = GetNumResults();
    int items_per_row = GetItemsPerRow();

    int num_row = num_results / items_per_row;
    if (num_results % items_per_row)
    {
      ++num_row;
    }
    int column_index = index % items_per_row;
    int row_index = index / items_per_row;

    column_x += column_index * column_width;
    row_y += row_index * row_height;
  }

  active_index_ = index;
  guint64 timestamp = nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp;
  glib::Variant data(g_variant_new("(tiiiiii)", timestamp, column_x, row_y, column_width, row_height, left_results, right_results));
  ResultActivated.emit(local_result, type, data);
}

void ResultViewGrid::QueueLazyLoad()
{
  if (all_results_preloaded_ || GetNumResults() == 0)
    return;

  if (results_changed_idle_)
    return;

  if (!lazy_load_source_)
  {
    lazy_load_source_.reset(new glib::Idle(glib::Source::Priority::DEFAULT));
      // dont need to reset the last start index as all the previous ones would have been preloaded already.
    lazy_load_source_->Run(sigc::mem_fun(this, &ResultViewGrid::DoLazyLoad));
  }
}

void ResultViewGrid::QueueResultsChanged()
{
  // even if we're not going to run the lazy load, we need to reset the start in case it's running already.
  last_lazy_loaded_result_ = 0;

  if (!results_changed_idle_)
  {
    // using glib::Source::Priority::HIGH because this needs to happen *before* next draw
    results_changed_idle_.reset(new glib::Idle(glib::Source::Priority::HIGH));
    results_changed_idle_->Run([this] () {
      SizeReallocate();
      results_changed_idle_.reset();
      lazy_load_source_.reset(); // no point doing this one as well.

      if (!all_results_preloaded_)
      {
        last_lazy_loaded_result_ = 0; // reset the lazy load index in case we got an insert
        DoLazyLoad(); // also calls QueueDraw
      }
      return false;
    });
  }
}

bool ResultViewGrid::DoLazyLoad()
{
  util::Timer timer;
  bool queue_additional_load = false; // if this is set, we will return early and start loading more next frame

  // instead we will just pre-load all the items if expanded or just one row if not
  int index = 0;
  int items_per_row = GetItemsPerRow();
  for (ResultIterator it(GetIteratorAtRow(last_lazy_loaded_result_)); !it.IsLast(); ++it)
  {
    if ((!expanded && index < items_per_row) || expanded)
    {
      renderer_->Preload(*it);
    }

    if (!expanded && index >= items_per_row)
      break; //early exit

    if (timer.ElapsedSeconds() > 0.008)
    {
      queue_additional_load = true;
      break;
    }

    last_lazy_loaded_result_++;
    index++;
  }

  if (!queue_additional_load)
  {
    all_results_preloaded_ = true;
    lazy_load_source_.reset();
  }
  else if (!lazy_load_source_)
  {
    lazy_load_source_.reset(new glib::Idle(glib::Source::Priority::DEFAULT));
    lazy_load_source_->Run(sigc::mem_fun(this, &ResultViewGrid::DoLazyLoad));
  }
  QueueDraw();

  return queue_additional_load;
}


int ResultViewGrid::GetItemsPerRow()
{
  int items_per_row = (GetGeometry().width - (padding * 2) + horizontal_spacing) / (renderer_->width + horizontal_spacing);
  return (items_per_row) ? items_per_row : 1; // always at least one item per row
}

void  ResultViewGrid::GetResultDimensions(int& rows, int& columns)
{
  columns = GetItemsPerRow();
  rows = result_model_ ? ceil(static_cast<double>(result_model_->count()) / static_cast<double>(std::max<int>(1, columns))) : 0.0;
}

void ResultViewGrid::SetModelRenderer(ResultRenderer* renderer)
{
  ResultView::SetModelRenderer(renderer);
  SizeReallocate();
}

void ResultViewGrid::AddResult(Result const& result)
{
  all_results_preloaded_ = false;
  QueueResultsChanged();
}

void ResultViewGrid::RemoveResult(Result const& result)
{
  ResultView::RemoveResult(result);
  // removing a result might make a non-preloaded one visible
  all_results_preloaded_ = false;
  QueueResultsChanged();
}

void ResultViewGrid::SizeReallocate()
{
  //FIXME - needs to use the geometry assigned to it, but only after a layout
  int items_per_row = GetItemsPerRow();
  unsigned num_results = GetNumResults();

  int total_rows = std::ceil(num_results / (double)items_per_row) ;
  int total_height = 0;

  if (expanded)
  {
    total_height = (total_rows * renderer_->height) + (total_rows * vertical_spacing);
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

  total_height += (padding * 2); // add padding

  SetMinimumHeight(total_height);
  SetMaximumHeight(total_height);

  mouse_over_index_ = GetIndexAtPosition(mouse_last_x_, mouse_last_y_);
  results_per_row = items_per_row;
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
    case XK_Menu:
      return true;
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
  unsigned num_results = GetNumResults();
  int total_rows = std::ceil(num_results / static_cast<float>(items_per_row)); // items per row is always at least 1
  total_rows = (expanded) ? total_rows : 1; // restrict to one row if not expanded

   // check for edge cases where we want the keynav to bubble up
  if (direction == nux::KEY_NAV_LEFT && (selected_index_ % items_per_row == 0))
    return false; // pressed left on the first item, no diiice
  else if (direction == nux::KEY_NAV_RIGHT && (selected_index_ == static_cast<int>(num_results - 1)))
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

  if (focused_result_.uri.empty())
    focused_result_ = (*GetIteratorAtRow(0));

  int items_per_row = GetItemsPerRow();
  unsigned num_results = GetNumResults();
  int total_rows = std::ceil(num_results / static_cast<float>(items_per_row)); // items per row is always at least 1
  total_rows = (expanded) ? total_rows : 1; // restrict to one row if not expanded

  if (direction == nux::KEY_NAV_LEFT && (selected_index_ == 0))
    return; // pressed left on the first item, no diiice

  if (direction == nux::KEY_NAV_RIGHT && (selected_index_ == static_cast<int>(num_results - 1)))
    return; // pressed right on the last item, nope. nothing for you

  if (direction == nux::KEY_NAV_RIGHT && !expanded && selected_index_ == items_per_row - 1)
    return; // pressed right on the last item in the first row in non expanded mode. nothing doing.

  switch (direction)
  {
    case (nux::KEY_NAV_LEFT):
    {
      selected_index_ = selected_index_ - 1;
      break;
    }
    case (nux::KEY_NAV_RIGHT):
    {
     selected_index_ = selected_index_ + 1;
      break;
    }
    case (nux::KEY_NAV_UP):
    {
      selected_index_ = selected_index_ - items_per_row;
      break;
    }
    case (nux::KEY_NAV_DOWN):
    {
      selected_index_ = selected_index_ + items_per_row;
      break;
    }
    default:
      break;
  }

  selected_index_ = std::max(0, selected_index_());
  selected_index_ = std::min(static_cast<int>(num_results - 1), selected_index_());
  ResultIterator iter(GetIteratorAtRow(selected_index_));
  focused_result_ = (*iter);

  std::tuple<int, int> focused_coord = GetResultPosition(selected_index_);

  int focused_x = std::get<0>(focused_coord);
  int focused_y = std::get<1>(focused_coord);

  ubus_.SendMessage(UBUS_RESULT_VIEW_KEYNAV_CHANGED,
                    g_variant_new("(iiii)", focused_x, focused_y, renderer_->width(), renderer_->height()));
  selection_change.emit();

  if (event_type == nux::NUX_KEYDOWN && event_keysym == XK_Menu)
  {
    Activate(focused_result_, selected_index_, ActivateType::PREVIEW);
  }
}

nux::Area* ResultViewGrid::KeyNavIteration(nux::KeyNavDirection direction)
{
  return this;
}

void ResultViewGrid::OnKeyNavFocusChange(nux::Area *area, bool has_focus, nux::KeyNavDirection direction)
{
  if (HasKeyFocus())
  {
    if (result_model_ && selected_index_ < 0 && GetNumResults())
    {
      ResultIterator first_iter(result_model_->model());
      focused_result_ = (*first_iter);
      selected_index_ = 0;
    }

    int items_per_row = GetItemsPerRow();
    unsigned num_results = GetNumResults();

    if (direction == nux::KEY_NAV_UP && expanded)
    {
      // This View just got focused through keyboard navigation and the
      // focus is comming from the bottom. We want to focus the
      // first item (on the left) of the last row in this grid.

      int total_rows = std::ceil(num_results / (double)items_per_row);
      selected_index_ = items_per_row * (total_rows-1);
    }

    if (direction != nux::KEY_NAV_NONE)
    {
      std::tuple<int, int> focused_coord = GetResultPosition(selected_index_);

      int focused_x = std::get<0>(focused_coord);
      int focused_y = std::get<1>(focused_coord);
      ubus_.SendMessage(UBUS_RESULT_VIEW_KEYNAV_CHANGED,
                        g_variant_new("(iiii)", focused_x, focused_y, renderer_->width(), renderer_->height()));
    }

    selection_change.emit();
  }
  else
  {
    selected_index_ = -1;
    focused_result_.clear();

    selection_change.emit();
  }
}

long ResultViewGrid::ComputeContentSize()
{
  SizeReallocate();
  QueueLazyLoad();

  return ResultView::ComputeContentSize();
}


typedef std::tuple <int, int> ResultListBounds;
ResultListBounds ResultViewGrid::GetVisableResults()
{
  int items_per_row = GetItemsPerRow();
  unsigned num_results = GetNumResults();
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
    unsigned row_size = renderer_->height + vertical_spacing;

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
      end = num_results - 1;
    }
  }

  start = std::max(start, 0);
  end = std::min(end, static_cast<int>(num_results - 1));

  return ResultListBounds(start, end);
}

void ResultViewGrid::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  int items_per_row = GetItemsPerRow();
  unsigned num_results = GetNumResults();
  int total_rows = (!expanded) ? 0 : (num_results / items_per_row) + 1;

  int row_size = renderer_->height + vertical_spacing;
  int y_position = padding + GetGeometry().y;

  ResultListBounds visible_bounds = GetVisableResults();

  nux::Geometry absolute_geometry(GetAbsoluteGeometry());

  for (int row_index = 0; row_index <= total_rows; row_index++)
  {
    DrawRow(GfxContext, visible_bounds, row_index, y_position, absolute_geometry);

    y_position += row_size;
  }
}

void ResultViewGrid::DrawRow(nux::GraphicsEngine& GfxContext, ResultListBounds const& visible_bounds, int row_index, int y_position, nux::Geometry const& absolute_position)
{
  unsigned int current_alpha_blend;
  unsigned int current_src_blend_factor;
  unsigned int current_dest_blend_factor;
  GfxContext.GetRenderStates().GetBlend(current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
  GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  int items_per_row = GetItemsPerRow();

  int row_lower_bound = row_index * items_per_row;
  if (row_lower_bound >= std::get<0>(visible_bounds) &&
      row_lower_bound <= std::get<1>(visible_bounds))
  {
    float saturation_progress = 1.0 - desaturation_progress();
    float saturation = 1.0;
    float opacity = 1.0;
    int x_position = padding + GetGeometry().x;
    for (int column_index = 0; column_index < items_per_row; column_index++)
    {
      int index = (row_index * items_per_row) + column_index;
      if (index < 0 || index >=  (int)GetNumResults())
        break;

      ResultRenderer::ResultRendererState state = ResultRenderer::RESULT_RENDERER_NORMAL;

      if (enable_texture_render() == false)
      {
        if (index == selected_index_)
        {
          state = ResultRenderer::RESULT_RENDERER_SELECTED;
        }
      }
      else if (index == active_index_)
      {
        state = ResultRenderer::RESULT_RENDERER_SELECTED;
      }

      int half_width = recorded_dash_width_ / 2;
      int half_height = recorded_dash_height_ / 2;

      int offset_x, offset_y;

      /* Guard against divide-by-zero. SIGFPEs are not mythological
       * contrary to popular belief */
      if (half_width >= 10)
        offset_x = std::max(std::min((x_position - half_width) / (half_width / 10), 5), -5);
      else
        offset_x = 0;

      if (half_height >= 10)
        offset_y = std::max(std::min(((y_position + absolute_position.y) - half_height) / (half_height / 10), 5), -5);
      else
        offset_y = 0;

      if (recorded_dash_width_ < 1 || recorded_dash_height_ < 1)
      {
        offset_x = 0;
        offset_y = 0;
      }

      // Color and saturation
      if (state == ResultRenderer::RESULT_RENDERER_SELECTED)
      {
        saturation = saturation_progress + (1.0-saturation_progress) * FOCUSED_ICON_SATURATION_REF;
        opacity = saturation_progress + (1.0-saturation_progress) * FOCUSED_GHOST_ICON_OPACITY_REF;
      }
      else
      {
        saturation = saturation_progress + (1.0-saturation_progress) * UNFOCUSED_ICON_SATURATION_REF;
        opacity = saturation_progress + (1.0-saturation_progress) * UNFOCUSED_GHOST_ICON_OPACITY_REF;
      }
      nux::Color tint(opacity + (1.0f-opacity) * background_color_.red,
                     opacity + (1.0f-opacity) * background_color_.green,
                     opacity + (1.0f-opacity) * background_color_.blue,
                     opacity);

      nux::Geometry render_geo(x_position, y_position, renderer_->width, renderer_->height);
      Result result(*GetIteratorAtRow(index));
      renderer_->Render(GfxContext,
                        result,
                        state,
                        render_geo,
                        offset_x,
                        offset_y,
                        tint,
                        saturation);

      x_position += renderer_->width + horizontal_spacing + extra_horizontal_spacing_;
    }
  }

  GfxContext.GetRenderStates().SetBlend(current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
}


void ResultViewGrid::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry base = GetGeometry();
  GfxContext.PushClippingRectangle(base);

  if (GetCompositionLayout())
  {
    GetCompositionLayout()->ProcessDraw(GfxContext, force_draw);
  }

  GfxContext.PopClippingRectangle();
}


void ResultViewGrid::MouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  unsigned index = GetIndexAtPosition(x, y);

  if (mouse_over_index_ != index)
  {
    selected_index_ = mouse_over_index_ = index;

    nux::GetWindowCompositor().SetKeyFocusArea(this);
  }
  mouse_last_x_ = x;
  mouse_last_y_ = y;
}

void ResultViewGrid::MouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  unsigned num_results = GetNumResults();
  unsigned index = GetIndexAtPosition(x, y);
  mouse_over_index_ = index;
  if (index < num_results)
  {
    // we got a click on a button so activate it
    ResultIterator it(GetIteratorAtRow(index));
    Result result = *it;
    selected_index_ = index;
    focused_result_ = result;
    activated_result_ = result;


    if (nux::GetEventButton(button_flags) == nux::NUX_MOUSE_BUTTON1)
    {
      if (unity::Settings::Instance().double_click_activate)
      {
        // delay activate for single left click. (for double click check)
        activate_timer_.reset(new glib::Timeout(DOUBLE_CLICK_SPEED, [this, index]() {
          Activate(activated_result_, index, ResultView::ActivateType::PREVIEW_LEFT_BUTTON);
          return false;
        }));
      }
      else
      {
        Activate(activated_result_, index, ResultView::ActivateType::DIRECT);
      }
    }
    else
    {
       Activate(activated_result_, index, ResultView::ActivateType::PREVIEW);
    }
  }
}

void ResultViewGrid::MouseDoubleClick(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  if (unity::Settings::Instance().double_click_activate == false)
    return;

  unsigned num_results = GetNumResults();
  unsigned index = GetIndexAtPosition(x, y);
  mouse_over_index_ = index;
  if (index < num_results && nux::GetEventButton(button_flags) == nux::NUX_MOUSE_BUTTON1)
  {
    // we got a click on a button so activate it
    ResultIterator it(GetIteratorAtRow(index));
    Result result = *it;
    selected_index_ = index;
    focused_result_ = result;

    activated_result_ = result;
    Activate(activated_result_, index, ResultView::ActivateType::DIRECT);
  }
}

unsigned ResultViewGrid::GetIndexAtPosition(int x, int y)
{
  if (x < 0 || y < 0)
     return -1;

  unsigned items_per_row = GetItemsPerRow();

  unsigned column_size = renderer_->width + horizontal_spacing + extra_horizontal_spacing_;
  unsigned row_size = renderer_->height + vertical_spacing;

  int x_bound = items_per_row * column_size + padding;

  if ((x < padding) || (x >= x_bound))
    return -1;

  if (y < padding)
    return -1;

  unsigned row_number = std::max((y - padding), 0) / row_size ;
  unsigned column_number = std::max((x - padding), 0) / column_size;

  return (row_number * items_per_row) + column_number;
}

std::tuple<int, int> ResultViewGrid::GetResultPosition(LocalResult const& local_result)
{
  unsigned int index = GetIndexForLocalResult(local_result);
  return GetResultPosition(index);
}

std::tuple<int, int> ResultViewGrid::GetResultPosition(const unsigned int& index)
{
  if (G_UNLIKELY(index >= static_cast<unsigned>(GetNumResults()) || index < 0))
  {
    LOG_ERROR(logger) << "index (" << index << ") does not exist in this category";
    return std::tuple<int, int>(0,0);
  } // out of bounds.

  int items_per_row = GetItemsPerRow();
  int column_size = renderer_->width + horizontal_spacing + extra_horizontal_spacing_;
  int row_size = renderer_->height + vertical_spacing;

  int y = row_size * (index / items_per_row) + padding;
  int x = column_size * (index % items_per_row) + padding;

  return std::tuple<int, int>(x, y);
}

/* --------
   DND code
   --------
*/
bool ResultViewGrid::DndSourceDragBegin()
{
#ifdef USE_X11
  drag_index_ = GetIndexAtPosition(last_mouse_down_x_, last_mouse_down_y_);

  if (drag_index_ >= GetNumResults())
    return false;

  Reference();

  ResultIterator iter(GetIteratorAtRow(drag_index_));
  current_drag_result_ = *iter;
  if (current_drag_result_.empty())
    current_drag_result_.uri = current_drag_result_.uri.substr(current_drag_result_.uri.find(":") + 1);

  LOG_DEBUG (logger) << "Dnd begin at " <<
                     last_mouse_down_x_ << ", " << last_mouse_down_y_ << " - using; "
                     << current_drag_result_.uri;

  return true;
#else
  return false;
#endif
}

nux::NBitmapData*
ResultViewGrid::DndSourceGetDragImage()
{
  if (drag_index_ >= GetNumResults())
    return nullptr;

  Result result(*GetIteratorAtRow(drag_index_));
  return renderer_->GetDndImage(result);
}

std::list<const char*>
ResultViewGrid::DndSourceGetDragTypes()
{
  std::list<const char*> result;
  result.push_back("text/uri-list");
  return result;
}

const char* ResultViewGrid::DndSourceGetDataForType(const char* type, int* size, int* format)
{
  *format = 8;

  if (!current_drag_result_.empty())
  {
    *size = strlen(current_drag_result_.uri.c_str());
    return current_drag_result_.uri.c_str();
  }
  else
  {
    *size = 0;
    return 0;
  }
}

void ResultViewGrid::DndSourceDragFinished(nux::DndAction result)
{
#ifdef USE_X11
  UnReference();
  last_mouse_down_x_ = -1;
  last_mouse_down_y_ = -1;
  current_drag_result_.clear();
  drag_index_ = ~0;

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
#endif
}

int
ResultViewGrid::GetSelectedIndex() const
{
  return selected_index_;
}

void
ResultViewGrid::SetSelectedIndex(int index)
{
  unsigned num_results = GetNumResults();
  if (num_results == 0)
  {
    focused_result_ = LocalResult();
    index = -1;
  }
  else
  {
    if (index >= 0 && (unsigned)index >= num_results)
      index = num_results-1;

    ResultIterator iter(GetIteratorAtRow(index));
    focused_result_ = (*iter);
  }

  selected_index_ = index;
}

void
ResultViewGrid::UpdateRenderTextures()
{
  nux::Geometry root_geo(GetAbsoluteGeometry());

  int items_per_row = GetItemsPerRow();
  unsigned num_results = GetNumResults();

  unsigned int total_rows = (!expanded) ? 1 : std::ceil(num_results / (double)items_per_row);
  int row_height = renderer_->height + vertical_spacing;

  int cumulative_height = 0;
  unsigned int row_index = 0;
  for (; row_index < total_rows; row_index++)
  {
    // only one texture for non-expanded.
    if (!expanded && row_index > 0)
      break;

    if (row_index >= result_textures_.size())
    {
      ResultViewTexture::Ptr result_texture(new ResultViewTexture);
      result_texture->abs_geo.x = root_geo.x;
      result_texture->abs_geo.y = root_geo.y + cumulative_height;
      result_texture->abs_geo.width = GetWidth();
      result_texture->abs_geo.height = row_height;
      result_texture->row_index = row_index;

      result_textures_.push_back(result_texture);
    }
    else
    {
      ResultViewTexture::Ptr const& result_texture(result_textures_[row_index]);

      result_texture->abs_geo.x = root_geo.x;
      result_texture->abs_geo.y = root_geo.y + cumulative_height;
      result_texture->abs_geo.width = GetWidth();
      result_texture->abs_geo.height = row_height;
      result_texture->row_index = row_index;
    }

    cumulative_height += row_height;
  }

  // get rid of old textures.
  for (; row_index < result_textures_.size(); row_index++)
  {
    result_textures_.pop_back();
  }
}

void ResultViewGrid::RenderResultTexture(ResultViewTexture::Ptr const& result_texture)
{
  int row_height = renderer_->height + vertical_spacing;

  // Do we need to re-create the texture?
  if (!result_texture->texture.IsValid() ||
       result_texture->texture->GetWidth() != GetWidth() ||
       result_texture->texture->GetHeight() != row_height)
  {
    result_texture->texture = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture(GetWidth(),
                                                                                                         row_height,
                                                                                                         1,
                                                                                                         nux::BITFMT_R8G8B8A8);
    if (!result_texture->texture.IsValid())
      return;
  }

  ResultListBounds visible_bounds(0, GetNumResults()-1);

  graphics::PushOffscreenRenderTarget(result_texture->texture);

    // clear the texture.
  CHECKGL(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
  CHECKGL(glClear(GL_COLOR_BUFFER_BIT));

  nux::GraphicsEngine& graphics_engine(nux::GetWindowThread()->GetGraphicsEngine());
  nux::Geometry offset_rect = graphics_engine.ModelViewXFormRect(GetGeometry());
  graphics_engine.PushModelViewMatrix(nux::Matrix4::TRANSLATE(-offset_rect.x, 0, 0));

  DrawRow(graphics_engine, visible_bounds, result_texture->row_index, 0, GetAbsoluteGeometry());

  graphics_engine.PopModelViewMatrix();

  graphics::PopOffscreenRenderTarget();
}

debug::ResultWrapper* ResultViewGrid::CreateResultWrapper(Result const& result, int index)
{
  int x_offset = GetAbsoluteX();
  int y_offset = GetAbsoluteY();

  std::tuple<int, int> result_coord = GetResultPosition(index);

  nux::Geometry geo(std::get<0>(result_coord) + x_offset,
    std::get<1>(result_coord) + y_offset,
    renderer_->width,
    renderer_->height);
  return new debug::ResultWrapper(result, geo);
}

void ResultViewGrid::UpdateResultWrapper(debug::ResultWrapper* wrapper, Result const& result, int index)
{
  if (!wrapper)
    return;

  int x_offset = GetAbsoluteX();
  int y_offset = GetAbsoluteY();

  std::tuple<int, int> result_coord = GetResultPosition(index);

  nux::Geometry geo(std::get<0>(result_coord) + x_offset,
    std::get<1>(result_coord) + y_offset,
    renderer_->width,
    renderer_->height);

  wrapper->UpdateGeometry(geo);
}


}
}
