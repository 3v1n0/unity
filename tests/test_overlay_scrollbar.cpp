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
#include <Nux/NuxTimerTickSource.h>
#include <Nux/VLayout.h>
#include <NuxCore/ObjectPtr.h>
#include <NuxCore/AnimationController.h>


#include "unity-shared/VScrollBarOverlayWindow.h"
#include "unity-shared/PlacesOverlayVScrollBar.h"
#include "unity-shared/UScreen.h"

using namespace unity::dash;
using namespace testing;

namespace unity
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

namespace dash
{

class MockScrollBar : public unity::dash::PlacesOverlayVScrollBar
{
  public:
    MockScrollBar(NUX_FILE_LINE_DECL)
    : PlacesOverlayVScrollBar(NUX_FILE_LINE_PARAM)
    , scroll_tick_(1000 * 401)
    , scroll_dy_(0)
    , thumbs_height_(overlay_window_->GetThumbGeometry().height)
    , scroll_up_signal_(false)
    , scroll_down_signal_(false)
    {
      OnScrollUp.connect([&] (float step, int dy) {
        scroll_dy_ = dy;
        scroll_up_signal_ = true;
      });

      OnScrollDown.connect([&] (float step, int dy) {
        scroll_dy_ = dy;
        scroll_down_signal_ = true;
      });
    }

    // ScrollDown/Up moves the mouse over the overlay scroll bar, then
    // moves it down/up by scroll_dy
    void ScrollDown(int scroll_dy)
    {
      UpdateStepY();

      auto geo = overlay_window_->GetThumbGeometry();
      int x = geo.x;
      int y = geo.y;

      MoveMouse(x, y);
      MoveDown(x, y);

      MoveMouse(x, y+scroll_dy);
      MoveUp(x, y+scroll_dy);
    }

    void ScrollUp(int scroll_dy)
    {
      UpdateStepY();

      auto geo = overlay_window_->GetThumbGeometry();
      int x = geo.x;
      int y = geo.y;

      MoveMouse(x, y);
      MoveDown(x, y);

      MoveMouse(x, y-scroll_dy);
      MoveUp(x, y-scroll_dy);
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

    void MoveMouseNear()
    {
      auto geo = overlay_window_->GetThumbGeometry();
      MoveMouse(geo.x, geo.y);
    }

    void ScrollUpAnimation(int scroll_dy)
    {
      nux::animation::AnimationController animation_controller(tick_source_);

      MoveMouseNear();
      UpdateStepY();

      StartScrollAnimation(ScrollDir::UP, scroll_dy);
      tick_source_.tick(scroll_tick_);
    }

    void ScrollDownAnimation(int scroll_dy)
    {
      nux::animation::AnimationController animation_controller(tick_source_);

      MoveMouseNear();
      UpdateStepY();

      StartScrollAnimation(ScrollDir::DOWN, scroll_dy);
      tick_source_.tick(scroll_tick_);
    }

    void SetThumbOffset(int y)
    {
      overlay_window_->SetThumbOffsetY(y);
      UpdateConnectorPosition();
    }

    void StartScrollThenConnectorAnimation()
    {
      nux::animation::AnimationController animation_controller(tick_source_);

      StartScrollAnimation(ScrollDir::DOWN, 20);
      MoveMouse(0,0);
      StartConnectorAnimation();

      tick_source_.tick(scroll_tick_);
    }

    void FakeDragDown()
    {
      OnMouseDrag(0, overlay_window_->GetThumbOffsetY() + 1, 0, 5, 0, 0);
    }

    nux::NuxTimerTickSource tick_source_;

    using PlacesOverlayVScrollBar::connector_height_;
    using VScrollBar::_slider;

    int scroll_tick_;
    int scroll_dy_;
    int thumbs_height_;
    bool scroll_up_signal_;
    bool scroll_down_signal_;
};

}

class MockScrollView : public nux::ScrollView
{
public:
  MockScrollView(NUX_FILE_LINE_DECL)
  : nux::ScrollView(NUX_FILE_LINE_PARAM)
  {
    scroll_bar_ = new MockScrollBar(NUX_TRACKER_LOCATION);
    SetVScrollBar(scroll_bar_.GetPointer());
  }

  nux::ObjectPtr<MockScrollBar> scroll_bar_;
};

class TestOverlayVScrollBar : public Test
{
public:
  TestOverlayVScrollBar()
  {
    nux::VLayout* scroll_layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
    scroll_layout_->SetGeometry(0,0,1000,5000);
    scroll_layout_->SetScaleFactor(0);

    scroll_view_ = new MockScrollView(NUX_TRACKER_LOCATION);
    scroll_view_->EnableVerticalScrollBar(true);
    scroll_view_->EnableHorizontalScrollBar(false);
    scroll_view_->SetLayout(scroll_layout_);

    scroll_view_->scroll_bar_->SetContentSize(0, 0, 201, 2000);
    scroll_view_->scroll_bar_->SetContainerSize(0, 0, 202, 400);
  }

  nux::ObjectPtr<MockScrollView> scroll_view_;
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
  overlay_window_->MouseLeave();
  EXPECT_FALSE(overlay_window_->IsVisible());
}

TEST_F(TestOverlayWindow, TestOverlayStaysOpenWhenMouseDown)
{
  overlay_window_->MouseNear();
  overlay_window_->MouseDown();

  overlay_window_->MouseBeyond();
  overlay_window_->MouseLeave();
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


TEST_F(TestOverlayVScrollBar, TestScrollDownSignal)
{
  scroll_view_->scroll_bar_->ScrollDown(10);
  EXPECT_TRUE(scroll_view_->scroll_bar_->scroll_down_signal_);
}

TEST_F(TestOverlayVScrollBar, TestScrollUpSignal)
{
  scroll_view_->scroll_bar_->ScrollDown(10);
  scroll_view_->scroll_bar_->ScrollUp(10);
  EXPECT_TRUE(scroll_view_->scroll_bar_->scroll_up_signal_);
}

TEST_F(TestOverlayVScrollBar, TestScrollDownDeltaY)
{
  int scroll_down = 15;
  scroll_view_->scroll_bar_->ScrollDown(scroll_down);
  EXPECT_EQ(scroll_view_->scroll_bar_->scroll_dy_, scroll_down);
}

TEST_F(TestOverlayVScrollBar, TestScrollUpDeltaY)
{
  int scroll_up = 7;
  scroll_view_->scroll_bar_->ScrollDown(scroll_up+1);
  scroll_view_->scroll_bar_->ScrollUp(scroll_up);
  EXPECT_EQ(scroll_view_->scroll_bar_->scroll_dy_, scroll_up);
}

TEST_F(TestOverlayVScrollBar, TestScrollDownBaseYMoves)
{
  int slider_y = scroll_view_->scroll_bar_->_slider->GetBaseY();
  int scroll_down = 10;
  scroll_view_->scroll_bar_->ScrollDown(scroll_down);
  EXPECT_EQ(scroll_view_->scroll_bar_->scroll_dy_, scroll_down);
  EXPECT_GT(scroll_view_->scroll_bar_->_slider->GetBaseY(), slider_y);
}

TEST_F(TestOverlayVScrollBar, TestScrollUpBaseYMoves)
{
  int scroll_up = 10;
  scroll_view_->scroll_bar_->ScrollDown(scroll_up+1);

  int slider_y = scroll_view_->scroll_bar_->_slider->GetBaseY();
  scroll_view_->scroll_bar_->ScrollUp(scroll_up);
  EXPECT_EQ(scroll_view_->scroll_bar_->scroll_dy_, scroll_up);
  EXPECT_LT(scroll_view_->scroll_bar_->_slider->GetBaseY(), slider_y);
}

TEST_F(TestOverlayVScrollBar, TestScrollsSlowlyDeltaY)
{
  int scroll_down = 10;
  for (int i = 0; i < scroll_down; i++)
  {
    scroll_view_->scroll_bar_->ScrollDown(1);
    EXPECT_EQ(scroll_view_->scroll_bar_->scroll_dy_, 1);
  }
}

TEST_F(TestOverlayVScrollBar, TestScrollUpAnimationMovesSlider)
{
  int scroll_up = 10;
  scroll_view_->scroll_bar_->ScrollDown(scroll_up+10);

  int slider_y = scroll_view_->scroll_bar_->_slider->GetBaseY();
  scroll_view_->scroll_bar_->ScrollUpAnimation(scroll_up);

  EXPECT_EQ(scroll_view_->scroll_bar_->scroll_dy_, scroll_up);
  EXPECT_LT(scroll_view_->scroll_bar_->_slider->GetBaseY(), slider_y);
}

TEST_F(TestOverlayVScrollBar, TestScrollDownAnimationMovesSlider)
{
  int scroll_down = 10;
  int slider_y = scroll_view_->scroll_bar_->_slider->GetBaseY();

  scroll_view_->scroll_bar_->ScrollDownAnimation(scroll_down);

  EXPECT_EQ(scroll_view_->scroll_bar_->scroll_dy_, scroll_down);
  EXPECT_GT(scroll_view_->scroll_bar_->_slider->GetBaseY(), slider_y);
}

TEST_F(TestOverlayVScrollBar, TestConnectorResetsDuringScrollAnimation)
{
  scroll_view_->scroll_bar_->MoveMouseNear();
  scroll_view_->scroll_bar_->SetThumbOffset(100);

  int connector_height = scroll_view_->scroll_bar_->connector_height_;
  EXPECT_GT(connector_height, 0);

  scroll_view_->scroll_bar_->StartScrollThenConnectorAnimation();

  connector_height = scroll_view_->scroll_bar_->connector_height_;
  EXPECT_EQ(connector_height, 0);
}


TEST_F(TestOverlayVScrollBar, TestAllowDragIfOverlayIsAtMaximum)
{
  // Offset that sets the thumb at the bottom of the scrollbar
  int thumb_offset = scroll_view_->scroll_bar_->GetHeight() -
                     scroll_view_->scroll_bar_->thumbs_height_;

  scroll_view_->scroll_bar_->SetThumbOffset(thumb_offset);
  scroll_view_->scroll_bar_->FakeDragDown();
  EXPECT_TRUE(scroll_view_->scroll_bar_->scroll_down_signal_);
}

}

