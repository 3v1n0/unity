/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 *
 */

#include <gtest/gtest.h>

#include "SwitcherController.h"
#include "test_utils.h"


using namespace unity::switcher;

namespace
{

class MockSwitcherController : public Controller
{
public:
  MockSwitcherController()
    : Controller()
    , window_constructed_(false)
    , view_constructed_(false)
    , view_shown_(false)
  {};

  virtual void ConstructWindow()
  {
    window_constructed_ = true;
  }

  virtual void ConstructView()
  {
    view_constructed_ = true;
  }

  virtual void ShowView()
  {
    view_shown_ = true;
  }

  bool window_constructed_;
  bool view_constructed_;
  bool view_shown_;
};

TEST(TestSwitcherController, TestConstructor)
{
  Controller controller;
}

TEST(TestSwitcherController, TestLazyWindowConstruct)
{
  MockSwitcherController controller;

  g_timeout_add_seconds(10, [] (gpointer data) -> gboolean {
    auto controller = static_cast<MockSwitcherController*>(data);
    EXPECT_FALSE(controller->window_constructed_);

    return FALSE;
  }, &controller);

  Utils::WaitUntil(controller.window_constructed_, 22);
  EXPECT_TRUE(controller.window_constructed_);
}

}
