// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 *
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
using namespace testing;

#include "ResultViewGrid.h"
using namespace unity;

namespace
{

class MockResultViewGrid : public dash::ResultViewGrid
{
public:
  MockResultViewGrid(NUX_FILE_LINE_DECL) : dash::ResultViewGrid(NUX_FILE_LINE_PARAM) {}

  MOCK_METHOD0(QueueDraw, void());
  MOCK_METHOD2(GetIndexAtPosition, uint(int x, int y));

  void FakeMouseMoveSignal(int x = 0, int y = 0, int dx = 0, int dy = 0, unsigned long mouse_button_state = 0, unsigned long special_keys_state = 0)
  {
    EmitMouseMoveSignal(x, y, dy, dy, mouse_button_state, special_keys_state);
  }
};

class TestResultViewGrid : public Test
{
public:
  virtual void SetUp()
  {
    view.Adopt(new NiceMock<MockResultViewGrid>(NUX_TRACKER_LOCATION));
    renderer.Adopt(new dash::ResultRenderer(NUX_TRACKER_LOCATION));

    view->SetModelRenderer(renderer.GetPointer());
    nux::GetWindowCompositor().SetKeyFocusArea(view.GetPointer());
  }

  nux::ObjectPtr<MockResultViewGrid> view;
  nux::ObjectPtr<dash::ResultRenderer> renderer;
};


TEST_F(TestResultViewGrid, TestQueueDrawMouseMoveInsideUnfocusedIcon)
{
  EXPECT_CALL(*view, QueueDraw())
    .Times(1);

  EXPECT_CALL(*view, GetIndexAtPosition(_, _))
    .WillOnce(Return(7));

  view->FakeMouseMoveSignal();
}


TEST_F(TestResultViewGrid, TestQueueDrawMouseMoveInsideFocusedIcon)
{
  EXPECT_CALL(*view, GetIndexAtPosition(_, _))
    .WillRepeatedly(Return(7));

  view->FakeMouseMoveSignal();

  EXPECT_CALL(*view, QueueDraw())
    .Times(0);

  view->FakeMouseMoveSignal();
}


TEST_F(TestResultViewGrid, TestQueueDrawMouseMoveOutside)
{
  EXPECT_CALL(*view, GetIndexAtPosition(_, _))
    .WillRepeatedly(Return(-1));

  view->FakeMouseMoveSignal();

  EXPECT_CALL(*view, QueueDraw())
    .Times(0);

  view->FakeMouseMoveSignal();
}

}
