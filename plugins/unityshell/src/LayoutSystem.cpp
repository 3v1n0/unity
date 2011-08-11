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
#include "WindowManager.h"

namespace unity {
namespace ui {
	
LayoutSystem::LayoutSystem()
{
}

LayoutSystem::~LayoutSystem()
{
}

void LayoutSystem::LayoutWindows (LayoutWindowList windows, nux::Geometry const& max_bounds, nux::Geometry& final_bounds)
{
  unsigned int size = windows.size();

  if (size == 0)
    return;
  
  for (auto window : windows)
  {
  	window->geo = WindowManager::Default ()->GetWindowGeometry (window->xid);
    window->aspect_ratio = (float)window->geo.width / (float)window->geo.height;
  }
  
  // we special case 2 and 3 since they are the most common 
  // cases (other than 1) and can be made beautiful with relative ease
  switch (size)  
  {
    case 2:
      LayoutTwoWindows (windows, max_bounds, final_bounds);
      break;
    default:
      LayoutGridWindows (windows, max_bounds, final_bounds);
      break;
  }
}

void LayoutSystem::LayoutTwoWindows (LayoutWindowList windows, nux::Geometry const& max_bounds, nux::Geometry& final_bounds)
{
  LayoutWindow::Ptr first = windows[0];
  LayoutWindow::Ptr second = windows[1];

  float combined_aspect = first->aspect_ratio + second->aspect_ratio;

  final_bounds = max_bounds;

  if (combined_aspect >= 1.0f)
  {
    // aspect is too wide
    final_bounds.y += (final_bounds.height - (final_bounds.height / combined_aspect)) / 2;
    final_bounds.height = final_bounds.height / combined_aspect;

    first->result.x = final_bounds.x;
    first->result.y = final_bounds.y;
    first->result.height = final_bounds.height;
    first->result.width = first->result.height * first->aspect_ratio;

    second->result.x = final_bounds.x + first->result.width;
    second->result.y = final_bounds.y;
    second->result.height = final_bounds.height;
    second->result.width = second->result.height * second->aspect_ratio;
  }
  else if (combined_aspect < 1.0f)
  {
    // aspect is too tall
    final_bounds.x += (final_bounds.width - (final_bounds.width / combined_aspect)) / 2;
    final_bounds.width = final_bounds.width / combined_aspect;

    // same as above?
    first->result.x = final_bounds.x;
    first->result.y = final_bounds.y;
    first->result.height = final_bounds.height;
    first->result.width = first->result.height * first->aspect_ratio;

    second->result.x = final_bounds.x + first->result.width;
    second->result.y = final_bounds.y;
    second->result.height = final_bounds.height;
    second->result.width = second->result.height * second->aspect_ratio;
  }
}

void LayoutSystem::LayoutGridWindows (LayoutWindowList windows, nux::Geometry const& max_bounds, nux::Geometry& final_bounds)
{
  int width = 1;
  int height = 1;

  while (width * height < (int) windows.size ())
  {
    if (height < width)
      height++;
    else
      width++;
  }

  final_bounds = max_bounds;

  int block_width = final_bounds.width / width;
  int block_height = final_bounds.height / height;

  int x = 0;
  int y = 0;

  int start_x = final_bounds.x + final_bounds.width;
  int start_y = final_bounds.y + final_bounds.height;

  int x1 = G_MAXINT;
  int y1 = G_MAXINT;
  int x2 = 0;
  int y2 = 0;

  int block_x = start_x;
  int block_y = start_y;
  int vertical_offset = 0;

  LayoutWindowList row_accum;

  LayoutWindowList::reverse_iterator it;
  for (it = windows.rbegin (); it != windows.rend (); it++)
  {
  	auto window = *it;
    window->result = ScaleBoxIntoBox (nux::Geometry (block_x - block_width, block_y - block_height, block_width, block_height), window->geo);
    row_accum.push_back (window);

    x1 = MIN (window->result.x, x1);
    y1 = MIN (window->result.y, y1);
    x2 = MAX (window->result.x + window->result.width, x2);
    y2 = MAX (window->result.y + window->result.height, y2);

    ++x;
    block_x -= block_width;
    if (x >= width || x + y * width == (int) windows.size ())
    {
      x = 0;
	    block_x = start_x;
      ++y;

      if (y == height - 1)
      {
      	block_width += (width * height - windows.size ()) * block_width / (windows.size () - width * (height - 1)); 
      }	
	    	

      if (y2 >= block_y + block_height)
      {
        block_y -= block_height;
      }
      else
      {
        int this_savings = (y1 - (block_y - block_height)) * 2;
        block_y = block_y - (block_height - this_savings);

        for (auto w : row_accum)
        	w->result.y += this_savings / 2;
        
        vertical_offset += this_savings / 2;
      }

      row_accum.clear ();
    }
  }

  x1 = G_MAXINT;
  y1 = G_MAXINT;
  x2 = 0;
  y2 = 0;

  for (auto window : windows)
  {
  	window->result.y -= vertical_offset;

  	x1 = MIN (window->result.x, x1);
    y1 = MIN (window->result.y, y1);
    x2 = MAX (window->result.x + window->result.width, x2);
    y2 = MAX (window->result.y + window->result.height, y2);
  }

  final_bounds = nux::Geometry (x1, y1, x2 - x1, y2 - y1);
}

nux::Geometry LayoutSystem::ScaleBoxIntoBox (nux::Geometry const& bounds, nux::Geometry const& box)
{
	float bound_aspect = (float) bounds.width / (float) bounds.height;
	float box_aspect = (float) box.width / (float) box.height;

	nux::Geometry result;

	if (box_aspect > bound_aspect)
	{
		// box wider than bounds
		result.x = bounds.x;

		result.width = bounds.width;
		result.height = result.width / box_aspect;

		result.y = bounds.y + (bounds.height - result.height) / 2;
	}
	else
	{
		result.y = bounds.y;

		result.height = bounds.height;
		result.width = result.height * box_aspect;

		result.x = bounds.x + (bounds.width - result.width) / 2;
	}

	return result;
}

LayoutWindow::LayoutWindow(Window xid)
 : xid (xid)
{

}

}
}