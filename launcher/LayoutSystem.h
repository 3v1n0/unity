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

#ifndef UNITYSHELL_LAYOUTSYSTEM_H
#define UNITYSHELL_LAYOUTSYSTEM_H

#include <memory>
#include <sigc++/sigc++.h>
#include <Nux/Nux.h>

#include "unity-shared/WindowManager.h"

namespace unity {
namespace ui {

struct LayoutWindow
{
  typedef std::shared_ptr<LayoutWindow> Ptr;

  LayoutWindow(Window xid);

  Window xid;

  nux::Geometry geo;
  nux::Geometry result;
  unsigned decoration_height;

  bool selected;
  float aspect_ratio;
  float alpha;
};

typedef std::vector<LayoutWindow::Ptr> LayoutWindowList;

class LayoutSystem
{
public:
  nux::Property<int> spacing;
  nux::Property<int> max_row_height;

  LayoutSystem();

  void LayoutWindows(LayoutWindowList windows, nux::Geometry const& max_bounds, nux::Geometry& final_bounds);

protected:
  void LayoutGridWindows(LayoutWindowList const& windows, nux::Geometry const& max_bounds, nux::Geometry& final_bounds);

  nux::Geometry LayoutRow(LayoutWindowList const& row, nux::Geometry const& row_bounds);
  nux::Geometry CompressAndPadRow(LayoutWindowList const& windows, nux::Geometry const& max_bounds);

  std::vector<LayoutWindowList> GetRows(LayoutWindowList const& windows, nux::Geometry const& max_bounds);

  nux::Size GridSizeForWindows(LayoutWindowList windows, nux::Geometry const& max_bounds);

  nux::Geometry ScaleBoxIntoBox(nux::Geometry const& bounds, nux::Geometry const& box);
};

}
}

#endif // UNITYSHELL_LAYOUTSYSTEM_H
