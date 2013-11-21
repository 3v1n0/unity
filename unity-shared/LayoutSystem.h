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

#include "unity-shared/Introspectable.h"
#include "unity-shared/WindowManager.h"

namespace unity {
namespace ui {

class LayoutWindow : public debug::Introspectable
{
public:
  typedef std::shared_ptr<LayoutWindow> Ptr;
  typedef std::vector<LayoutWindow::Ptr> Vector;

  LayoutWindow(Window xid);

  Window xid;

  nux::Geometry geo;
  nux::Geometry result;
  unsigned decoration_height;

  bool selected;
  float aspect_ratio;
  float alpha;

protected:
  // Introspectable methods
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);
};

class LayoutSystem
{
public:
  nux::Property<int> spacing;
  nux::Property<int> max_row_height;

  LayoutSystem();

  void LayoutWindows(LayoutWindow::Vector const& windows, nux::Geometry const& max_bounds, nux::Geometry& final_bounds);
  std::vector<int> GetRowSizes(LayoutWindow::Vector const& windows, nux::Geometry const& max_bounds) const;

protected:
  void LayoutGridWindows(LayoutWindow::Vector const& windows, nux::Geometry const& max_bounds, nux::Geometry& final_bounds);

  nux::Geometry LayoutRow(LayoutWindow::Vector const& row, nux::Geometry const& row_bounds);
  nux::Geometry CompressAndPadRow(LayoutWindow::Vector const& windows, nux::Geometry const& max_bounds);

  nux::Size GridSizeForWindows(LayoutWindow::Vector const& windows, nux::Geometry const& max_bounds) const;

  std::vector<LayoutWindow::Vector> GetRows(LayoutWindow::Vector const& windows, nux::Geometry const& max_bounds) const;

  nux::Geometry ScaleBoxIntoBox(nux::Geometry const& bounds, nux::Geometry const& box);
};

}
}

#endif // UNITYSHELL_LAYOUTSYSTEM_H
