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

int const CONTENT_WIDTH = 200;
int const CONTENT_HEIGHT = 2000;
int const CONTAINER_WIDTH = 200;
int const CONTAINER_HEIGHT = 200;

class TestOverlayScrollBar : public Test
{
public:
  class MockScrollBar : public unity::dash::PlacesOverlayVScrollBar
  {
    public:
      MockScrollBar(NUX_FILE_LINE_DECL)
      : unity::dash::PlacesOverlayVScrollBar(NUX_FILE_LINE_PARAM)
      {
        OnScrollUp.connect([&] (float step, int dy) {
          printf("SCROLL UP %i\n", dy);
        });

        OnScrollDown.connect([&] (float step, int dy) {
          printf("SCROLL Down%i\n", dy);
        });
        auto geo = _track->GetGeometry();
        //printf("%i %i %i %i\n", geo.x, geo.y, geo.width, geo.height);
      }
      void test()
      {
        SetContentSize(0,0, CONTENT_WIDTH, CONTENT_HEIGHT);
        SetContainerSize(0,0, CONTAINER_WIDTH, CONTAINER_HEIGHT);
        printf("CONTENT: %i -- CONTAINER: %i\n", content_height_, container_height_);
        auto m = _track->GetGeometry();
        printf("S: %i %i %i %i\n", m.x, m.y, m.width, m.height);

        int x = _track->GetBaseX();
        int y = _track->GetBaseY();

        int temp = _slider->GetBaseY();

        int i;
        MoveMouse(x,y+40);
        MoveDown(x,y+40);
        for (i = 0; i < 50; i++)
        {
          MoveMouse(x,y+40+i);
        }
        MoveUp(x,y+i);
        printf("%i %i\n", temp, _slider->GetBaseY());
        //printf("%i %i\n", temp, _slider->GetBaseY());
        //printf("%i %i\n");
        /*

        MoveMouse(x,y+20);
        printf("%i %i\n", temp, _slider->GetBaseY());
        MoveUp(x,y+20);
        */
      }

      void MoveDown(int x, int y)
      {
        nux::Event event;
        event.type = nux::NUX_MOUSE_PRESSED;
        event.x = x;
        event.y = y;
        event.mouse_state |= 0x00010000;
        nux::GetWindowCompositor().ProcessEvent(event);
      }

      void MoveUp(int x, int y)
      {
        nux::Event event;
        event.type = nux::NUX_MOUSE_RELEASED;
        event.x = x;
        event.y = y;
        event.mouse_state |= 0x00010000;
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
  };

  class MockScrollView : public nux::ScrollView
  {
    public:
      MockScrollView(NUX_FILE_LINE_DECL)
      : nux::ScrollView(NUX_FILE_LINE_PARAM)
      {
        scroll_bar_ = std::make_shared<MockScrollBar>(NUX_TRACKER_LOCATION);
        SetVScrollBar(scroll_bar_.get());

      }

      std::shared_ptr<MockScrollBar> scroll_bar_;
  };

  TestOverlayScrollBar()
  {
     nux::VLayout* scroll_layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
     scroll_layout_->SetGeometry(0,0,1000,1000);
     scroll_view_ = std::make_shared<MockScrollView>(NUX_TRACKER_LOCATION);
     scroll_view_->EnableVerticalScrollBar(true);
     scroll_view_->EnableHorizontalScrollBar(false);
     scroll_view_->SetLayout(scroll_layout_);
  }

  int GetProxListSize() const
  {
    return nux::GetWindowThread()->GetWindowCompositor().GetProximityListSize();
  }

  std::shared_ptr<MockScrollView> scroll_view_;
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

TEST_F(TestOverlayScrollBar, TestOverlayVScrollbarAddsToProxList)
{
  int const prox_size = GetProxListSize();

  printf("%i..\n", scroll_view_->scroll_bar_->AtMinimum());
  printf("SIZE: %i\n", scroll_view_->scroll_bar_->GetBaseHeight());
  
  scroll_view_->scroll_bar_->test();
  EXPECT_EQ(GetProxListSize(), prox_size);

  //delete p_overlay;
}

TEST_F(TestOverlayScrollBar, TestOverlayVScrollbarRemovesFromProxList)
{
  int const prox_size = GetProxListSize();

  //unity::dash::PlacesOverlayVScrollBar* p_overlay = new unity::dash::PlacesOverlayVScrollBar(NUX_TRACKER_LOCATION);
  //EXPECT_EQ(GetProxListSize(), prox_size+1);

  //delete p_overlay;
  EXPECT_EQ(GetProxListSize(), prox_size);
}

}
