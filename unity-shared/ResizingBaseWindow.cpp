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
 */
#include "Nux/Nux.h" // must come first because Nux headers are fuxxored
#include "ResizingBaseWindow.h"


namespace unity
{

ResizingBaseWindow::ResizingBaseWindow(char const* window_name,
                                       GeometryAdjuster const& input_adjustment)
: MockableBaseWindow(window_name)
, input_adjustment_(input_adjustment)
{ }


void ResizingBaseWindow::UpdateInputWindowGeometry()
{
#ifdef USE_X11
  if (m_input_window && m_input_window_enabled)
    m_input_window->SetGeometry(input_adjustment_(GetGeometry()));
#endif
}


void ResizingBaseWindow::SetGeometry(const nux::Geometry &geo)
{
   Area::SetGeometry(geo);
   UpdateInputWindowGeometry();
}

}
