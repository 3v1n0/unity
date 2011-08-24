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

#include "ubus-server.h"
#include "UBusMessages.h"
#include "ResultViewGrid.h"
#include "math.h"

namespace
{
nux::logging::Logger logger("unity.dash.ResultViewGrid");
}

namespace unity
{
namespace dash
{
ResultViewGrid::ResultViewGrid(NUX_FILE_LINE_DECL)
  : ResultView(NUX_FILE_LINE_PARAM)
  , horizontal_spacing(6)
  , vertical_spacing(6)
  , padding(6)
  , mouse_over_index_(-1)
  , active_index_(-1)
  , selected_index_(-1)
  , preview_row_(0)
  , last_mouse_down_x_(-1)
  , last_mouse_down_y_(-1)
{
  auto needredraw_lambda = [&](int value)
  {
    NeedRedraw();
  };
  horizontal_spacing.changed.connect(needredraw_lambda);
  vertical_spacing.changed.connect(needredraw_lambda);
  padding.changed.connect(needredraw_lambda);

  OnKeyNavFocusChange.connect (sigc::mem_fun (this, &ResultViewGrid::OnOnKeyNavFocusChange));
  OnKeyNavFocusActivate.connect ([&] (nux::Area *area) { UriActivated.emit (focused_uri_); });
  key_down.connect (sigc::mem_fun (this, &ResultViewGrid::OnKeyDown));
  mouse_move.connect(sigc::mem_fun(this, &ResultViewGrid::MouseMove));
  mouse_click.connect(sigc::mem_fun(this, &ResultViewGrid::MouseClick));

  mouse_down.connect([&](int x, int y, unsigned long mouse_state, unsigned long button_state)
  {
    last_mouse_down_x_ = x;
    last_mouse_down_y_ = y;
  });

  mouse_leave.connect([&](int x, int y, unsigned long mouse_state, unsigned long button_state)
  {
    mouse_over_index_ = -1;
    NeedRedraw();
  });

  SetDndEnabled(true, false);
  NeedRedraw();
}

ResultViewGrid::~ResultViewGrid()
{
}

int ResultViewGrid::GetItemsPerRow()
{
  int items_per_row = (GetGeometry().width - (padding * 2)) / (renderer_->width + horizontal_spacing);
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
  ResultView::AddResult(result);
  SizeReallocate();
}

void ResultViewGrid::RemoveResult(Result& result)
{
  ResultView::RemoveResult(result);
  SizeReallocate();
}

void ResultViewGrid::SizeReallocate()
{
  //FIXME - needs to use the geometry assigned to it, but only after a layout
  int items_per_row = GetItemsPerRow();

  int total_rows = (results_.size() / items_per_row);
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
    total_height = renderer_->height;
  }
  SetMinimumHeight(total_height + (padding * 2));
  SetMaximumHeight(total_height + (padding * 2));
  PositionPreview();
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

long int ResultViewGrid::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo)
{
  return TraverseInfo;
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
  if (direction == nux::KEY_NAV_UP && selected_index_ < items_per_row)
    return false; // key nav up when already on top row
  else if (direction == nux::KEY_NAV_DOWN && selected_index_ >= (total_rows-1) * items_per_row)
    return false; // key nav down when on bottom row
  else
    return true;

  return false;
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
  NeedRedraw();
}

nux::Area* ResultViewGrid::KeyNavIteration(nux::KeyNavDirection direction)
{
  return this;
}

// crappy name.
void ResultViewGrid::OnOnKeyNavFocusChange(nux::Area *area)
{

  if (HasKeyFocus())
  {
    focused_uri_ = results_.front().uri;
    selected_index_ = 0;
    NeedRedraw();
  }
  else
  {
    selected_index_ = -1;
    focused_uri_.clear();
  }
}

long ResultViewGrid::ComputeLayout2()
{
  SizeReallocate();
  long ret = ResultView::ComputeLayout2();
  return ret;

}

void ResultViewGrid::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle(GetGeometry());

  gPainter.PaintBackground(GfxContext, GetGeometry());

  int items_per_row = GetItemsPerRow();

  uint total_rows = (!expanded) ? 0 : (results_.size() / items_per_row) + 1;

  ResultView::ResultList::iterator it;

  //find the row we start at
  int absolute_y = GetAbsoluteY();
  uint row_size = renderer_->height + vertical_spacing;

  int y_position = padding + GetGeometry().y;
  nux::Area* top_level_parent = GetToplevel();

  for (uint row_index = 0; row_index <= total_rows; row_index++)
  {
    // check if the row is displayed on the screen,
    // FIXME - optimisation - replace 2048 with the height of the displayed viewport
    // or at the very least, with the largest monitor resolution
    //~ if ((y_position + renderer_->height) + absolute_y >= 0
    //~ && (y_position - renderer_->height) + absolute_y <= top_level_parent->GetGeometry().height)
    if (1)
    {
      int x_position = padding + GetGeometry().x;
      for (int column_index = 0; column_index < items_per_row; column_index++)
      {
        uint index = (row_index * items_per_row) + column_index;
        if (index >= results_.size())
          break;

        ResultRenderer::ResultRendererState state = ResultRenderer::RESULT_RENDERER_NORMAL;
        if ((int)(index) == mouse_over_index_)
        {
          state = ResultRenderer::RESULT_RENDERER_PRELIGHT;
        }
        else if ((int)(index) == selected_index_)
        {
          state = ResultRenderer::RESULT_RENDERER_SELECTED;
        }
        else if ((int)(index) == active_index_)
        {
          state = ResultRenderer::RESULT_RENDERER_ACTIVE;
        }

        int half_width = top_level_parent->GetGeometry().width / 2;
        // FIXME - we assume the height of the viewport is 600
        int half_height = top_level_parent->GetGeometry().height;

        int offset_x = (x_position - half_width) / (half_width / 10);
        int offset_y = ((y_position + absolute_y) - half_height) / (half_height / 10);
        nux::Geometry render_geo(x_position, y_position, renderer_->width, renderer_->height);
        renderer_->Render(GfxContext, results_[index], state, render_geo, offset_x, offset_y);

        x_position += renderer_->width + horizontal_spacing;
      }
    }

    // if we have a preview displaying on this row, flow the rest of the items down
    if (preview_layout_ != NULL && preview_row_ == row_index)
    {
      y_position += preview_layout_->GetGeometry().height + vertical_spacing;

    }
    y_position += row_size;
  }

  GfxContext.PopClippingRectangle();
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
  mouse_over_index_ = index;
  NeedRedraw();
}

void ResultViewGrid::MouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  uint index = GetIndexAtPosition(x, y);
  if (index >= 0 && index < results_.size())
  {
    // we got a click on a button so activate it
    Result result = results_[index];
    UriActivated.emit(result.uri);
  }
}

uint ResultViewGrid::GetIndexAtPosition(int x, int y)
{
  uint items_per_row = GetItemsPerRow();

  uint column_size = renderer_->width + horizontal_spacing;
  uint row_size = renderer_->height + vertical_spacing;

  if (preview_layout_ != NULL && (y - padding) / row_size > preview_row_)
  {
    // because we are "above" the preview row, we can just fudge the number
    y -= preview_layout_->GetGeometry().height + vertical_spacing;
  }

  int x_bound = items_per_row * column_size + padding;

  if (x < padding || x > x_bound)
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
void
ResultViewGrid::DndSourceDragBegin()
{
  Reference();
  ubus_server_send_message(ubus_server_get_default(),
                           UBUS_PLACE_VIEW_CLOSE_REQUEST,
                           NULL);

  uint drag_index = GetIndexAtPosition(last_mouse_down_x_, last_mouse_down_y_);
  Result drag_result = results_[drag_index];
  current_drag_uri_ = drag_result.dnd_uri;
  current_drag_icon_name_ = drag_result.icon_hint;

  LOG_DEBUG (logger) << "Dnd begin at " <<
                     last_mouse_down_x_ << ", " << last_mouse_down_y_ << " - using; "
                     << current_drag_uri_ << " - "
                     << current_drag_icon_name_;
}

nux::NBitmapData*
ResultViewGrid::DndSourceGetDragImage()
{
  nux::NBitmapData* result = 0;
  GdkPixbuf* pbuf;
  GtkIconTheme* theme;
  GtkIconInfo* info;
  GError* error = NULL;
  GIcon* icon;

  std::string icon_name = current_drag_icon_name_.c_str();
  int size = 64;

  if (icon_name.empty())
    icon_name = "application-default-icon";

  theme = gtk_icon_theme_get_default();
  icon = g_icon_new_for_string(icon_name.c_str(), NULL);

  if (G_IS_ICON(icon))
  {
    info = gtk_icon_theme_lookup_by_gicon(theme, icon, size, (GtkIconLookupFlags)0);
    g_object_unref(icon);
  }
  else
  {
    info = gtk_icon_theme_lookup_icon(theme,
                                      icon_name.c_str(),
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
    LOG_WARN (logger) << "could not find a pixbuf for " << icon_name;
    g_error_free (error);
    result = NULL;
  }

  gtk_icon_info_free(info);

  if (GDK_IS_PIXBUF(pbuf))
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
}

}
}
