/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#ifndef RESIZEDINPUTWINDOW_BASEWINDOW_H
#define RESIZEDINPUTWINDOW_BASEWINDOW_H

#include "MockableBaseWindow.h"

namespace unity
{

/**
 * A base window with a separate input area overlay.
 *
 * This Unity class extends the base Nux class with a bound function to
 * recalculate the input window geometry.
 */
class ResizingBaseWindow : public MockableBaseWindow
{
public:
  /**
   * A function for adjusting the input window geometry relative to the
   * displayed window geometry in some way.
   */
  typedef std::function<nux::Geometry (nux::Geometry const&)> GeometryAdjuster;
  typedef nux::ObjectPtr<ResizingBaseWindow> Ptr;

public:
  /**
   * Constructs display and input regions.
   * @param[in] window_name       The name of the window.
   * @param[in] input_adjustment  A bound function for adjusting the input area
   *                              geometry.
   */
  ResizingBaseWindow(char const* window_name,
                     GeometryAdjuster const& input_adjustment);

  /**
   * Recalculates the input window geometry.
   *
   * Useful if the underlying base window geometry has changed.
   */
  void UpdateInputWindowGeometry();

  /**
   * Sets the base window and input window geometry.
   * @param[in] geometry The new window geometry.
   */
  virtual void SetGeometry(const nux::Geometry &geometry);

private:
  GeometryAdjuster input_adjustment_;
};

}
#endif // RESIZEDINPUTWINDOW_BASEWINDOW_H
