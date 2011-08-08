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
#include <Nux/Layout.h>

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
    : ResultView (NUX_FILE_LINE_PARAM)
    , horizontal_spacing(6)
    , vertical_spacing(6)
    , padding (6)
    , mouse_over_index_ (-1)
    , active_index_ (-1)
    , selected_index_ (-1)
{
  auto needredraw_lambda = [&] (int value) { NeedRedraw(); };
  horizontal_spacing.changed.connect (needredraw_lambda);
  vertical_spacing.changed.connect (needredraw_lambda);
  padding.changed.connect (needredraw_lambda);

  mouse_move.connect (sigc::mem_fun (this, &ResultViewGrid::MouseMove));
  mouse_click.connect (sigc::mem_fun (this, &ResultViewGrid::MouseClick));

  NeedRedraw();
}

ResultViewGrid::~ResultViewGrid()
{
}

void ResultViewGrid::SetPreview (PreviewBase *preview, Result& related_result)
{
  ResultView::SetPreview(preview, related_result);

  nux::Layout *preview_layout = GetCompositionLayout();
  uint items_per_row = (GetGeometry().width - (padding * 2)) / (renderer_->width + horizontal_spacing);

  uint row_size = renderer_->height + vertical_spacing;

  uint index = 0;
  ResultList::iterator it;
  for (it = results_.begin(); it != results_.end(); it++)
  {
    Result* result = *it;
    if (result == &related_result)
    {
      break;
    }
    index++;
  }

  uint row_number = index / items_per_row;
  int y_position = row_number * row_size;

  preview_layout->SetGeometry (GetGeometry().x, y_position,
                               GetGeometry().width, 600);

  preview_layout->SetBaseY(160);
  preview_layout->SetMaximumHeight(600);

  SizeReallocate();

}

void ResultViewGrid::SetModelRenderer(ResultRenderer* renderer)
{
  ResultView::SetModelRenderer(renderer);
  SizeReallocate ();
}

void ResultViewGrid::AddResult (Result& result)
{
  ResultView::AddResult (result);
  SizeReallocate ();
}

void ResultViewGrid::RemoveResult (Result& result)
{
  ResultView::RemoveResult (result);
  SizeReallocate ();
}

void ResultViewGrid::SizeReallocate ()
{
  //FIXME - needs to use the geometry assigned to it, but only after a layout
  int items_per_row = (800 - (padding * 2)) / (renderer_->width + horizontal_spacing);
  items_per_row = (items_per_row) ? items_per_row : 1;

  int total_rows = ceil ((float)results_.size () / items_per_row);
  int total_height = 0;

  if (expanded)
  {
    total_height = (total_rows * renderer_->height) + (total_rows * vertical_spacing);

    if (preview_result_ != NULL)
    {
      total_height += 600 + vertical_spacing;
    }
  }
  else
  {
    total_height = renderer_->height;
  }
  SetMinimumHeight (total_height + (padding * 2));

}

long int ResultViewGrid::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
  return TraverseInfo;
}

void ResultViewGrid::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle(GetGeometry());

  gPainter.PaintBackground(GfxContext, GetGeometry());

  uint items_per_row = (GetGeometry().width - (padding * 2)) / (renderer_->width + horizontal_spacing);
  items_per_row = (items_per_row) ? items_per_row : 1;

  gint64 time_start = g_get_monotonic_time ();
  ResultView::ResultList::iterator it;

  //find the row we start at
  int absolute_y = abs(std::min(GetAbsoluteY(), 0)) ;
  uint row_size = renderer_->height + vertical_spacing;
  uint absolute_row = (absolute_y + padding) / row_size;

  //FIXME - replace 1000 with the hight of the displayed viewport
  uint displayed_rows = 1000 / (renderer_->height + vertical_spacing);

  int y_position = padding + (absolute_row * renderer_->height) + (absolute_row * vertical_spacing);
  for (uint row_index = absolute_row; row_index <= absolute_row + displayed_rows; row_index++)
  {
    int x_position = padding;

    for (uint column_index = 0; column_index < items_per_row; column_index++)
    {
      uint index = (row_index * items_per_row) + column_index;
      if (index >= results_.size())
        return;

      ResultRenderer::ResultRendererState state = ResultRenderer::RESULT_RENDERER_NORMAL;
      if ((int)(index) == mouse_over_index_)
      {
        state = ResultRenderer::RESULT_RENDERER_PRELIGHT;
      }
      else if ((int)(index) == active_index_)
      {
        state = ResultRenderer::RESULT_RENDERER_ACTIVE;
      }
      else if ((int)(index) == selected_index_)
      {
        state = ResultRenderer::RESULT_RENDERER_SELECTED;
      }

      nux::Geometry render_geo (x_position, y_position, renderer_->width, renderer_->height);
      renderer_->Render(GfxContext, *results_[index], state, render_geo);

      // we also need to set the preview if we have to
      if (results_[index] == preview_result_)
      {
        render_geo.y += renderer_->height + vertical_spacing;
        render_geo.width = GetGeometry().width;
        render_geo.height = 600;
        //int preview_height = y_position + renderer_->height + vertical_spacing;
        //GetCompositionLayout()->SetGeometry (render_geo);
      }

      x_position += renderer_->width + horizontal_spacing;
    }

    y_position += renderer_->height + vertical_spacing;
  }

  g_debug ("took %f seconds to draw the view", (g_get_monotonic_time () - time_start) / 1000000.0f);
  time_start = g_get_monotonic_time();

  GfxContext.PopClippingRectangle();
}

void ResultViewGrid::DrawContent (nux::GraphicsEngine &GfxContent, bool force_draw) {
  nux::Geometry base = GetGeometry ();
  GfxContent.PushClippingRectangle (base);

  if (GetCompositionLayout ())
    GetCompositionLayout ()->ProcessDraw (GfxContent, force_draw);

  GfxContent.PopClippingRectangle();
}


void ResultViewGrid::MouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  uint index = GetIndexAtPosition (x, y);
  mouse_over_index_ = index;
  g_debug ("set mouse over index to %i", index);
  NeedRedraw();
}

void ResultViewGrid::MouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  uint index = GetIndexAtPosition (x, y);
  if (index >= 0)
  {
    // we got a click on a button so activate it
    Result* result = results_[index];
    UriActivated.emit (result->uri);
  }
  g_debug ("got click at %i - %i", x, y);
}

uint ResultViewGrid::GetIndexAtPosition (int x, int y)
{
  uint items_per_row = (GetGeometry().width - (padding * 2)) / (renderer_->width + horizontal_spacing);

  uint column_size = renderer_->width + horizontal_spacing;
  uint row_size = renderer_->height + vertical_spacing;

  int x_bound = items_per_row * column_size + padding;

  if (x < padding || x > x_bound)
    return -1;

  if (y < padding)
    return -1;

  uint row_number = std::max((y - padding), 0) / row_size ;
  uint column_number = std::max((x - padding), 0) / column_size;

  return (row_number * items_per_row) + column_number;
}

}
}
