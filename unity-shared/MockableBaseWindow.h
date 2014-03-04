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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef MOCKABLE_BASEWINDOW_H
#define MOCKABLE_BASEWINDOW_H

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>

namespace unity
{
/**
 * A nux base window with forwarding functions that can be replaced by mocks in
 * the test suite.
 */
class MockableBaseWindow : public nux::BaseWindow
{
public:
  typedef nux::ObjectPtr<MockableBaseWindow> Ptr;

  MockableBaseWindow(char const* window_name = "", NUX_FILE_LINE_PROTO)
    : nux::BaseWindow(window_name, NUX_TRACKER_LOCATION)
    , struts_enabled_(false)
  {}

  /**
   * Sets the window opacity.
   * @param[in] opacity Window opacity (alpha) value on [0, 1).
   *
   * This function override is provided so the window can be mocked during
   * testing.
   */
  virtual void SetOpacity(float opacity) { BaseWindow::SetOpacity(opacity); }

  virtual void InputWindowEnableStruts(bool enable)
  {
    struts_enabled_ = enable;
    BaseWindow::InputWindowEnableStruts(enable);
  }

  virtual bool InputWindowStrutsEnabled()
  {
    if (!InputWindowEnabled())
      return struts_enabled_;

    return BaseWindow::InputWindowStrutsEnabled();
  }

  bool struts_enabled_;
};

}
#endif // MOCKABLE_BASEWINDOW_H
