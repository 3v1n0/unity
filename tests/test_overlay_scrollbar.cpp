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
#include "unity-shared/UScreen.h"


namespace
{

class MockOverlayScrollbarWindow
{
public:
  MockOverlayScrollbarWindow()
  {
    overlay_window_ = new VScrollBarOverlayWindow(nux::Geometry(0,0,100,100));
  }

  ~MockOverlayScrollbarWindow()
  {
    overlay_window_.Release();
  }

  nux::ObjectPtr<VScrollBarOverlayWindow>& GetOverlayWindow()
  {
    return overlay_window_;
  }
private:
  nux::ObjectPtr<VScrollBarOverlayWindow> overlay_window_;
};

TEST(TestOverlayScrollbar, TestOverlayShows)
{
  MockOverlayScrollbarWindow m_overlay;

  m_overlay.GetOverlayWindow()->MouseNear();
  EXPECT_TRUE(m_overlay.GetOverlayWindow()->IsVisible());
}

TEST(TestOverlayScrollbar, TestOverlayHides)
{
  MockOverlayScrollbarWindow m_overlay;

  m_overlay.GetOverlayWindow()->MouseNear();
  EXPECT_TRUE(m_overlay.GetOverlayWindow()->IsVisible());

  m_overlay.GetOverlayWindow()->MouseBeyond();
  EXPECT_FALSE(m_overlay.GetOverlayWindow()->IsVisible());
}

TEST(TestOverlayScrollbar, TestOverlayStaysOpenWhenMouseDown)
{
  MockOverlayScrollbarWindow m_overlay;

  m_overlay.GetOverlayWindow()->MouseNear();
  m_overlay.GetOverlayWindow()->MouseDown();

  m_overlay.GetOverlayWindow()->MouseBeyond();
  EXPECT_TRUE(m_overlay.GetOverlayWindow()->IsVisible());
}

TEST(TestOverlayScrollbar, TestOverlayMouseDrags)
{
  MockOverlayScrollbarWindow m_overlay;

  m_overlay.GetOverlayWindow()->MouseDown();
  EXPECT_FALSE(m_overlay.GetOverlayWindow()->IsMouseBeingDragged());

  m_overlay.GetOverlayWindow()->SetThumbOffsetY(10);
  EXPECT_TRUE(m_overlay.GetOverlayWindow()->IsMouseBeingDragged());
}

TEST(TestOverlayScrollbar, TestOverlayStopDraggingOnMouseUp)
{
  MockOverlayScrollbarWindow m_overlay;
  m_overlay.GetOverlayWindow()->MouseDown();
  EXPECT_FALSE(m_overlay.GetOverlayWindow()->IsMouseBeingDragged());

  m_overlay.GetOverlayWindow()->SetThumbOffsetY(10);
  EXPECT_TRUE(m_overlay.GetOverlayWindow()->IsMouseBeingDragged());

  m_overlay.GetOverlayWindow()->MouseUp();
  EXPECT_FALSE(m_overlay.GetOverlayWindow()->IsMouseBeingDragged());
}

TEST(TestOverlayScrollbar, TestOverlaySetsOffsetY)
{
  MockOverlayScrollbarWindow m_overlay;
  const int offset_y = 30;

  m_overlay.GetOverlayWindow()->SetThumbOffsetY(offset_y);
  EXPECT_EQ(m_overlay.GetOverlayWindow()->GetThumbOffsetY(), offset_y);
}

TEST(TestOverlayScrollbar, TestOverlaySetsOffsetYOutOfBoundsLower)
{
  MockOverlayScrollbarWindow m_overlay;
  const int offset_y = -40;

  m_overlay.GetOverlayWindow()->SetThumbOffsetY(offset_y);
  EXPECT_EQ(m_overlay.GetOverlayWindow()->GetThumbOffsetY(), 0);
}

TEST(TestOverlayScrollbar, TestOverlaySetsOffsetYOutOfBoundsUpper)
{
  MockOverlayScrollbarWindow m_overlay;
  const int offset_y = 1000;
  const int expected_offset = m_overlay.GetOverlayWindow()->GetBaseHeight() -
                              m_overlay.GetOverlayWindow()->GetThumbHeight();

  m_overlay.GetOverlayWindow()->SetThumbOffsetY(offset_y);
  EXPECT_EQ(m_overlay.GetOverlayWindow()->GetThumbOffsetY(), expected_offset);
}

TEST(TestOverlayScrollbar, TestOverlayMouseIsInsideThumb)
{
  MockOverlayScrollbarWindow m_overlay;
  const nux::Geometry geo(0, 50, 50, 400);

  m_overlay.GetOverlayWindow()->UpdateGeometry(geo);
  EXPECT_TRUE(m_overlay.GetOverlayWindow()->IsMouseInsideThumb(0));
}

TEST(TestOverlayScrollbar, TestOverlayMouseIsInsideOnOffsetChange)
{
  MockOverlayScrollbarWindow m_overlay;
  const nux::Geometry geo(0, 50, 50, 400);
  const int offset_y = 50;
  const int thumb_height = m_overlay.GetOverlayWindow()->GetThumbHeight();

  m_overlay.GetOverlayWindow()->UpdateGeometry(geo);
  m_overlay.GetOverlayWindow()->SetThumbOffsetY(offset_y);

  EXPECT_FALSE(m_overlay.GetOverlayWindow()->IsMouseInsideThumb(offset_y - 1));
  EXPECT_TRUE(m_overlay.GetOverlayWindow()->IsMouseInsideThumb(offset_y + thumb_height/2));
  EXPECT_FALSE(m_overlay.GetOverlayWindow()->IsMouseInsideThumb(offset_y + thumb_height + 1));
}

}
