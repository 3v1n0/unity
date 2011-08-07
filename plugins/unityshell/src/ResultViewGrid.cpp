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
#include <NuxCore/Logger.h>

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
{
  auto needredraw_lambda = [&] (int value) { NeedRedraw(); };
  horizontal_spacing.changed.connect (needredraw_lambda);
  vertical_spacing.changed.connect (needredraw_lambda);
  padding.changed.connect (needredraw_lambda);

  NeedRedraw();
}

ResultViewGrid::~ResultViewGrid()
{
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
  int total_height = (total_rows * renderer_->height) + (total_rows * vertical_spacing);

  g_debug ("setting minimum height to %i", total_height);
  SetMinimumHeight (total_height);

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

      nux::Geometry render_geo (x_position, y_position, renderer_->width, renderer_->height);
      renderer_->Render(GfxContext, *results_[index], ResultRenderer::RESULT_RENDERER_NORMAL, render_geo);

      x_position += renderer_->width + horizontal_spacing;
    }

    y_position += renderer_->height + vertical_spacing;
  }

  /*
    uint index = 0;
  for (it = results_.begin (); it != results_.end(); it++)
  {
    int row = index / items_per_row;
    int column = index % items_per_row;

    int y_position = (row * renderer_->height) + (row * vertical_spacing) + padding;
    int x_position = (column * renderer_->width) + (column * horizontal_spacing) + padding;

    nux::Geometry render_geo (x_position, y_position, renderer_->width, renderer_->height);
    renderer_->Render(GfxContext, *(*it), ResultRenderer::RESULT_RENDERER_NORMAL, render_geo);

    index++;
  }
  */
  g_debug ("took %f seconds to draw the view", (g_get_monotonic_time () - time_start) / 1000000.0f);
  time_start = g_get_monotonic_time();

  GfxContext.PopClippingRectangle();
}

//~ void ResultViewGrid::OnModelChanged()
//~ {
  //~ int items_per_row = GetGeometry().width / ((_renderer->width + horizontal_spacing) - horizontal_spacing);
  //~ int total_height = ceil ((float)_model.count / items_per_row);
  //~ SetMinMaxSize (GetGeometry().width, total_height);
//~ }

}
}
