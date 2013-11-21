// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#include "LayoutSystem.h"

namespace unity {
namespace ui {

LayoutSystem::LayoutSystem()
  : spacing(8)
  , max_row_height(400)
{}

void LayoutSystem::LayoutWindows(LayoutWindow::Vector const& windows, nux::Geometry const& max_bounds, nux::Geometry& final_bounds)
{
  if (windows.empty())
    return;

  LayoutGridWindows(windows, max_bounds, final_bounds);
}

nux::Size LayoutSystem::GridSizeForWindows(LayoutWindow::Vector const& windows, nux::Geometry const& max_bounds) const
{
  unsigned count = windows.size();

  unsigned width = 1;
  unsigned height = 1;

  if (count == 2)
  {
    float stacked_aspect = std::max (windows[0]->geo.width, windows[1]->geo.width) / (float)(windows[0]->geo.height + windows[1]->geo.height);
    float row_aspect = (float)(windows[0]->geo.width + windows[1]->geo.width) / std::max(windows[0]->geo.height, windows[1]->geo.height);
    float box_aspect = (float)max_bounds.width / max_bounds.height;
    if (abs(row_aspect - box_aspect) > abs(stacked_aspect - box_aspect))
    {
      height = 2;
    }
    else
    {
      width = 2;
    }
  }
  else
  {
    while (width * height < count)
    {
      if (height < width)
        height++;
      else
        width++;
    }
  }

  return nux::Size(width, height);
}

nux::Geometry LayoutSystem::CompressAndPadRow(LayoutWindow::Vector const& windows, nux::Geometry const& max_bounds)
{
  int total_width = 0;
  int max_height = 0;
  for (LayoutWindow::Ptr const& window : windows)
  {
    window->result.x = total_width;
    total_width += spacing + window->result.width;
    max_height = std::max (window->result.height, max_height);
  }

  total_width -= spacing;

  int x1 = std::numeric_limits<int>::max();
  int y1 = std::numeric_limits<int>::max();
  int x2 = std::numeric_limits<int>::min();
  int y2 = std::numeric_limits<int>::min();

  int offset = std::max (0, (max_bounds.width - total_width) / 2);
  for (LayoutWindow::Ptr const& window : windows)
  {
    window->result.x += max_bounds.x + offset;
    window->result.y = max_bounds.y + (max_height - window->result.height) / 2;

    x1 = std::min (window->result.x, x1);
    y1 = std::min (window->result.y, y1);
    x2 = std::max (window->result.x + window->result.width, x2);
    y2 = std::max (window->result.y + window->result.height, y2);
  }

  return nux::Geometry (x1, y1, x2 - x1, y2 - y1);
}

nux::Geometry LayoutSystem::LayoutRow(LayoutWindow::Vector const& row, nux::Geometry const& row_bounds)
{
  nux::Geometry unpadded_bounds = row_bounds;
  unpadded_bounds.width -= spacing * (row.size () - 1);

  int combined_width = 0;
  for (LayoutWindow::Ptr const& window : row)
  {
    float scalar = unpadded_bounds.height / (float)window->geo.height;
    combined_width += window->geo.width * scalar;
  }

  float global_scalar = std::min (1.0f, unpadded_bounds.width / (float)combined_width);

  int x = unpadded_bounds.x;
  int y = unpadded_bounds.y;

  // precision of X,Y is relatively unimportant as the Compression stage will fix any issues, sizing
  // is however set at this point.
  for (LayoutWindow::Ptr const& window : row)
  {
    // we dont allow scaling up
    float final_scalar = std::min(1.0f, (unpadded_bounds.height / (float)window->geo.height) * global_scalar);

    window->result.x = x;
    window->result.y = y;
    window->result.width = window->geo.width * final_scalar;
    window->result.height = window->geo.height * final_scalar;

    x += window->result.width;
  }

  return CompressAndPadRow (row, row_bounds);
}

std::vector<int> LayoutSystem::GetRowSizes(LayoutWindow::Vector const& windows, nux::Geometry const& max_bounds) const
{
  std::vector<LayoutWindow::Vector> const& rows = GetRows(windows, max_bounds);
  std::vector<int> row_sizes;

  for (auto r : rows)
    row_sizes.push_back(r.size());

  return row_sizes;
}

std::vector<LayoutWindow::Vector> LayoutSystem::GetRows(LayoutWindow::Vector const& windows, nux::Geometry const& max_bounds) const
{
  std::vector<LayoutWindow::Vector> rows;

  int size = (int)windows.size();

  float total_aspect = 0;
  for (LayoutWindow::Ptr const& window : windows)
  {
    total_aspect += window->aspect_ratio;
  }

  if (total_aspect < 1.8f * ((float)max_bounds.width / max_bounds.height))
  {
    // If the total aspect ratio is < 1.8 the max, we fairly safely assume a double row configuration wont be better
    rows.push_back(windows);
  }
  else
  {
    nux::Size const& grid_size = GridSizeForWindows(windows, max_bounds);

    int width = grid_size.width;
    int height = grid_size.height;

    float row_height = std::min (max_bounds.height / height, max_row_height());
    float ideal_aspect = (float)max_bounds.width / row_height;

    int x = 0;
    int y = 0;

    int spare_slots = (width * height) - size;

    float row_aspect = 0.0f;

    LayoutWindow::Vector row_accum;
    
    int i;
    for (i = 0; i < size; ++i)
    {
      LayoutWindow::Ptr window = windows[i];

      row_accum.push_back (window);
      row_aspect += window->aspect_ratio;
      
      ++x;
      if (x == width - 1 && spare_slots)
      {
        bool skip = false;

        if (spare_slots == height - y)
          skip = true;
        else if (i < size - 1)
          skip = row_aspect + windows[i+1]->aspect_ratio >= ideal_aspect;

        if (skip)
        {
          ++x;
          spare_slots--;
        }
      }

      if (x >= width)
      {
        // end of row
        x = 0;
        ++y;
        row_aspect = 0;

        rows.push_back(row_accum);
        row_accum.clear ();
      }
    }

    if (!row_accum.empty())
      rows.push_back(row_accum);
  }

  return rows;
}

void LayoutSystem::LayoutGridWindows(LayoutWindow::Vector const& windows, nux::Geometry const& max_bounds, nux::Geometry& final_bounds)
{
  std::vector<LayoutWindow::Vector> const& rows = GetRows(windows, max_bounds);

  int height = rows.size();
  int non_spacing_height = max_bounds.height - ((height - 1) * spacing);
  int row_height = std::min (max_row_height(), non_spacing_height / height);
  int start_y = max_bounds.y;
  int low_y = 0;

  for (LayoutWindow::Vector const& row : rows)
  {
    nux::Geometry row_max_bounds(max_bounds.x, start_y, max_bounds.width, row_height);
    nux::Geometry const& row_final_bounds = LayoutRow(row, row_max_bounds);

    low_y = row_final_bounds.y + row_final_bounds.height;

    start_y += row_final_bounds.height + spacing;
  }

  int x1 = std::numeric_limits<int>::max();
  int y1 = std::numeric_limits<int>::max();
  int x2 = std::numeric_limits<int>::min();
  int y2 = std::numeric_limits<int>::min();

  int offset = (max_bounds.height - (low_y - max_bounds.y)) / 2;

  for (auto const& window : windows)
  {
    window->result.y += offset;

  	x1 = std::min (window->result.x, x1);
    y1 = std::min (window->result.y, y1);
    x2 = std::max (window->result.x + window->result.width, x2);
    y2 = std::max (window->result.y + window->result.height, y2);
  }

  final_bounds = nux::Geometry (x1, y1, x2 - x1, y2 - y1);
}

LayoutWindow::LayoutWindow(Window xid)
 : xid(xid)
 , geo(WindowManager::Default().GetWindowGeometry(xid))
 , decoration_height(0)
 , selected(false)
 , aspect_ratio(geo.width / static_cast<float>(geo.height))
 , alpha(0.0f)
{
  auto& wm = WindowManager::Default();

  if (wm.IsWindowMaximized(xid) && !wm.IsWindowMinimized(xid))
  {
    // Maximized windows are not decorated, so we define an extra decoration
    // height to be used to correctly render the window with a fake decoration
    auto const& deco_size = wm.GetWindowDecorationSize(xid, WindowManager::Edge::TOP);
    decoration_height = deco_size.height;
    geo.height += decoration_height;
    aspect_ratio = geo.width / static_cast<float>(geo.height);
  }
}

// Introspectable methods
std::string LayoutWindow::GetName() const
{
  return "LayoutWindow";
}

void LayoutWindow::AddProperties(debug::IntrospectionData& introspection)
{
  introspection.add(result);
}

}
}
