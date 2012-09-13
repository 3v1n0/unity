/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef RESIZEDINPUTWINDOW_BASEWINDOW_H
#define RESIZEDINPUTWINDOW_BASEWINDOW_H

#include <Nux/BaseWindow.h>

namespace unity
{

class ResizedInputWindowBaseWindow : public nux::BaseWindow
{
public:
  ResizedInputWindowBaseWindow(const char *WindowName, std::function<nux::Geometry (nux::Geometry const&)> geo_func)
  : BaseWindow(WindowName, NUX_TRACKER_LOCATION)
  {
    geo_func_ = geo_func;
  }

  void UpdateInputWindowGeometry()
  {
    if (m_input_window)
      m_input_window->SetGeometry(geo_func_(GetGeometry()));
  }

  virtual void SetGeometry(const nux::Geometry &geo)
  {
     Area::SetGeometry(geo);
     UpdateInputWindowGeometry();
  }

private:
  std::function<nux::Geometry (nux::Geometry const&)> geo_func_;
};

}
#endif // RESIZEDINPUTWINDOW_BASEWINDOW_H
