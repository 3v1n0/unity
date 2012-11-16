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

#include "unity-shared/VScrollBarOverlayWindow.h"
#include "unity-shared/PlacesOverlayVScrollBar.h"
#include "unity-shared/UScreen.h"

using namespace testing;

namespace
{

class TestOverlayScrollbar : public Test
{
public:
  TestOverlayScrollbar()
  {
    overlay_window_ = new VScrollBarOverlayWindow(nux::Geometry(0,0,100,100));
  }

  ~TestOverlayScrollbar()
  {
    overlay_window_.Release();
  }

  int GetProxListSize() const
  {
    return nux::GetWindowThread()->GetWindowCompositor().GetProximityListSize();
  }

  nux::ObjectPtr<VScrollBarOverlayWindow> overlay_window_;
};

TEST_F(TestOverlayScrollbar, TestOverlayShows)
{
  overlay_window_->MouseNear();
  EXPECT_TRUE(overlay_window_->IsVisible());
}

TEST_F(TestOverlayScrollbar, TestOverlayHides)
{
  overlay_window_->MouseNear();
  EXPECT_TRUE(overlay_window_->IsVisible());

  overlay_window_->MouseBeyond();
  EXPECT_FALSE(overlay_window_->IsVisible());
}

TEST_F(TestOverlayScrollbar, TestOverlayStaysOpenWhenMouseDown)
{
  overlay_window_->MouseNear();
  overlay_window_->MouseDown();

  overlay_window_->MouseBeyond();
  EXPECT_TRUE(overlay_window_->IsVisible());
}

TEST_F(TestOverlayScrollbar, TestOverlayMouseDrags)
{
  overlay_window_->MouseDown();
  EXPECT_FALSE(overlay_window_->IsMouseBeingDragged());

  overlay_window_->SetThumbOffsetY(10);
  EXPECT_TRUE(overlay_window_->IsMouseBeingDragged());
}

TEST_F(TestOverlayScrollbar, TestOverlayStopDraggingOnMouseUp)
{
  overlay_window_->MouseDown();
  EXPECT_FALSE(overlay_window_->IsMouseBeingDragged());

  overlay_window_->SetThumbOffsetY(10);
  EXPECT_TRUE(overlay_window_->IsMouseBeingDragged());

  overlay_window_->MouseUp();
  EXPECT_FALSE(overlay_window_->IsMouseBeingDragged());
}

TEST_F(TestOverlayScrollbar, TestOverlaySetsOffsetY)
{
  int const offset_y = 30;

  overlay_window_->SetThumbOffsetY(offset_y);
  EXPECT_EQ(overlay_window_->GetThumbOffsetY(), offset_y);
}

TEST_F(TestOverlayScrollbar, TestOverlaySetsOffsetYOutOfBoundsLower)
{
  int const offset_y = -40;

  overlay_window_->SetThumbOffsetY(offset_y);
  EXPECT_EQ(overlay_window_->GetThumbOffsetY(), 0);
}

TEST_F(TestOverlayScrollbar, TestOverlaySetsOffsetYOutOfBoundsUpper)
{
  int const offset_y = 1000;
  int const expected_offset = overlay_window_->GetBaseHeight() - overlay_window_->GetThumbHeight();

  overlay_window_->SetThumbOffsetY(offset_y);
  EXPECT_EQ(overlay_window_->GetThumbOffsetY(), expected_offset);
}

TEST_F(TestOverlayScrollbar, TestOverlayMouseIsInsideThumb)
{
  nux::Geometry const geo(0, 50, 50, 400);

  overlay_window_->UpdateGeometry(geo);
  EXPECT_TRUE(overlay_window_->IsMouseInsideThumb(0));
}

TEST_F(TestOverlayScrollbar, TestOverlayMouseIsInsideOnOffsetChange)
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

TEST_F(TestOverlayScrollbar, TestOverlayVScrollbarAddsToProxList)
{
  int const prox_size = GetProxListSize();

  unity::dash::PlacesOverlayVScrollBar* p_overlay = new unity::dash::PlacesOverlayVScrollBar(NUX_TRACKER_LOCATION);
  EXPECT_EQ(GetProxListSize(), prox_size+1);

  delete p_overlay;
}

TEST_F(TestOverlayScrollbar, TestOverlayVScrollbarRemovesFromProxList)
{
  int const prox_size = GetProxListSize();

  unity::dash::PlacesOverlayVScrollBar* p_overlay = new unity::dash::PlacesOverlayVScrollBar(NUX_TRACKER_LOCATION);
  EXPECT_EQ(GetProxListSize(), prox_size+1);

  delete p_overlay;
  EXPECT_EQ(GetProxListSize(), prox_size);
}

}
