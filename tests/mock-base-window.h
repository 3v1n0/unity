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
 * version 3 along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef TESTS_MOCK_BASEWINDOW_H
#define TESTS_MOCK_BASEWINDOW_H

#include <gmock/gmock.h>
#include "ResizingBaseWindow.h"

namespace unity
{
namespace testmocks
{

using namespace testing;

class MockBaseWindow : public ResizingBaseWindow
{
public:
  typedef nux::ObjectPtr<MockBaseWindow> Ptr;
  typedef NiceMock<MockBaseWindow> Nice;

  MockBaseWindow(ResizingBaseWindow::GeometryAdjuster const& input_adjustment, const char *name = "Mock")
  	: ResizingBaseWindow(name, input_adjustment)
  {
    ON_CALL(*this, SetOpacity(_)).WillByDefault(Invoke([this] (float o) { ResizingBaseWindow::SetOpacity(o); }));
  }

  MockBaseWindow(const char *name = "Mock")
  	: MockBaseWindow([](nux::Geometry const& geo) { return geo; }, name)
  {}

  MOCK_METHOD2(ShowWindow, void(bool, bool));
  MOCK_METHOD1(SetOpacity, void(float));
};

} // namespace testmocks
} // namespace unity

#endif
