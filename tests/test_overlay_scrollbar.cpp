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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 *
 */

#include <gtest/gtest.h>

#include <Nux/Nux.h>
#include <NuxCore/ObjectPtr.h>
#include <Nux/VLayout.h>

#include "unity-shared/VScrollBarOverlayWindow.h"
#include "unity-shared/PlacesOverlayVScrollBar.h"
#include "unity-shared/UScreen.h"

using namespace testing;

namespace
{

class TestOverlayWindow : public Test
{
public:
  TestOverlayWindow()
  {
    overlay_window_ = new VScrollBarOverlayWindow(nux::Geometry(0,0,100,100));
  }

  int GetProxListSize() const
  {
    return nux::GetWindowThread()->GetWindowCompositor().GetProximityListSize();
  }

  nux::ObjectPtr<VScrollBarOverlayWindow> overlay_window_;
};

class TestOverlayScrollBar : public Test
{
public:
  class MockScrollBar : public unity::dash::PlacesOverlayVScrollBar
  {
    public:
      MockScrollBar(NUX_FILE_LINE_DECL)
      : unity::dash::PlacesOverlayVScrollBar(NUX_FILE_LINE_PARAM)
      , scroll_dy(0)
      , scroll_up_signal_(false)
      , scroll_down_signal_(false)
      {
        SetGeometry(nux::Geometry(0,0,200,500));
        SetContainerSize(0,0,200,200);
        SetContentSize(0,0,200,2000);
        ComputeContentSize();

        OnScrollUp.connect([&] (float step, int dy) {
          scroll_dy = dy;
          scroll_up_signal_ = true;
        });

        OnScrollDown.connect([&] (float step, int dy) {
          scroll_dy = dy;
          scroll_down_signal_ = true;
        });
      }

      void ScrollDown(int scroll_dy)
      {
        // Shows we are over the Overlay Thumb
        int x = _track->GetBaseX() + _track->GetBaseWidth() + 5;
        int y = _track->GetBaseY();

        MoveMouse(x,y);
        MoveDown(x,y);

        MoveMouse(x,y+scroll_dy);
        MoveUp(x,y+scroll_dy);
      }

      void ScrollUp(int scroll_dy)
      {
        ScrollDown(scroll_dy);

        // Shows we are over the Overlay Thumb
        int x = _track->GetBaseX() + _track->GetBaseWidth() + 5;
        int y = _track->GetBaseY();

        MoveMouse(x,y+scroll_dy);
        MoveDown(x,y+scroll_dy);

        MoveMouse(x,y);
        MoveUp(x,y);
      }

      void MoveDown(int x, int y)
      {
        nux::Event event;
        event.type = nux::NUX_MOUSE_PRESSED;
        event.x = x;
        event.y = y;
        nux::GetWindowCompositor().ProcessEvent(event);
      }

      void MoveUp(int x, int y)
      {
        nux::Event event;
        event.type = nux::NUX_MOUSE_RELEASED;
        event.x = x;
        event.y = y;
        nux::GetWindowCompositor().ProcessEvent(event);
      }

      void MoveMouse(int x, int y)
      {
        nux::Event event;
        event.type = nux::NUX_MOUSE_MOVE;
        event.x = x;
        event.y = y;
        nux::GetWindowCompositor().ProcessEvent(event);
      }

      using nux::VScrollBar::AtMinimum;
      using nux::VScrollBar::GetBaseHeight;

      int scroll_dy;
      bool scroll_up_signal_;
      bool scroll_down_signal_;
  };

  TestOverlayScrollBar()
  {
     scroll_bar_ = std::make_shared<MockScrollBar>(NUX_TRACKER_LOCATION);
  }

  std::shared_ptr<MockScrollBar> scroll_bar_;
};

TEST_F(TestOverlayWindow, TestOverlayShows)
{
  overlay_window_->MouseNear();
  EXPECT_TRUE(overlay_window_->IsVisible());
}

TEST_F(TestOverlayWindow, TestOverlayHides)
{
  overlay_window_->MouseNear();
  EXPECT_TRUE(overlay_window_->IsVisible());

  overlay_window_->MouseBeyond();
  EXPECT_FALSE(overlay_window_->IsVisible());
}

TEST_F(TestOverlayWindow, TestOverlayStaysOpenWhenMouseDown)
{
  overlay_window_->MouseNear();
  overlay_window_->MouseDown();

  overlay_window_->MouseBeyond();
  EXPECT_TRUE(overlay_window_->IsVisible());
}

TEST_F(TestOverlayWindow, TestOverlayMouseDrags)
{
  overlay_window_->MouseDown();
  EXPECT_FALSE(overlay_window_->IsMouseBeingDragged());

  overlay_window_->SetThumbOffsetY(10);
  EXPECT_TRUE(overlay_window_->IsMouseBeingDragged());
}

TEST_F(TestOverlayWindow, TestOverlayStopDraggingOnMouseUp)
{
  overlay_window_->MouseDown();
  EXPECT_FALSE(overlay_window_->IsMouseBeingDragged());

  overlay_window_->SetThumbOffsetY(10);
  EXPECT_TRUE(overlay_window_->IsMouseBeingDragged());

  overlay_window_->MouseUp();
  EXPECT_FALSE(overlay_window_->IsMouseBeingDragged());
}

TEST_F(TestOverlayWindow, TestOverlaySetsOffsetY)
{
  int const offset_y = 30;

  overlay_window_->SetThumbOffsetY(offset_y);
  EXPECT_EQ(overlay_window_->GetThumbOffsetY(), offset_y);
}

TEST_F(TestOverlayWindow, TestOverlaySetsOffsetYOutOfBoundsLower)
{
  int const offset_y = -40;

  overlay_window_->SetThumbOffsetY(offset_y);
  EXPECT_EQ(overlay_window_->GetThumbOffsetY(), 0);
}

TEST_F(TestOverlayWindow, TestOverlaySetsOffsetYOutOfBoundsUpper)
{
  int const offset_y = 1000;
  int const expected_offset = overlay_window_->GetBaseHeight() - overlay_window_->GetThumbHeight();

  overlay_window_->SetThumbOffsetY(offset_y);
  EXPECT_EQ(overlay_window_->GetThumbOffsetY(), expected_offset);
}

TEST_F(TestOverlayWindow, TestOverlayMouseIsInsideThumb)
{
  nux::Geometry const geo(0, 50, 50, 400);

  overlay_window_->UpdateGeometry(geo);
  EXPECT_TRUE(overlay_window_->IsMouseInsideThumb(0));
}

TEST_F(TestOverlayWindow, TestOverlayMouseIsInsideOnOffsetChange)
{
  nux::Geometry const geo(0, 50, 50, 400);
  int const offset_y = 50;
  int const thumb_height = overlay_window_->GetThumbHeight();

  overlay_window_->UpdateGeometry(geo);
  overlay_window_->SetThumbOffsetY(offset_y);

  EXPECT_FALSE(overlay_window_->IsMouseInsideThumb(offset_y - 1));
  EXPECT_TRUE(overlay_window_->IsMouseInsideThumb(offset_y + thumb_height/2));
  EXPECT_FALSE(overlay_window_->IsMouseInsideThumb(offset_y + thumb_height + 1));
}

TEST_F(TestOverlayScrollBar, TestScrollDownSignal)
{
  scroll_bar_->ScrollDown(10);
  EXPECT_TRUE(scroll_bar_->scroll_down_signal_);
}

TEST_F(TestOverlayScrollBar, TestScrollUpSignal)
{
  scroll_bar_->ScrollUp(10);
  EXPECT_TRUE(scroll_bar_->scroll_up_signal_);
}

TEST_F(TestOverlayScrollBar, TestScrollDownDeltaY)
{
  int scroll_down = 15;
  scroll_bar_->ScrollDown(scroll_down);
  EXPECT_EQ(scroll_bar_->scroll_dy, scroll_down);
}

TEST_F(TestOverlayScrollBar, TestScrollUpDeltaY)
{
  int scroll_up = 7;
  scroll_bar_->ScrollUp(scroll_up);
  EXPECT_EQ(scroll_bar_->scroll_dy, scroll_up);
}

TEST_F(TestOverlayScrollBar, TestScrollsSlowlyDeltaY)
{
  int scroll_down = 10;
  for (int i = 0; i < scroll_down; i++)
  {
    scroll_bar_->ScrollDown(1);
    EXPECT_EQ(scroll_bar_->scroll_dy, 1);
  }
}

}
