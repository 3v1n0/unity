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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#include <functional>

#include <gmock/gmock.h>

#include "LauncherDragWindow.h"
#include "WindowManager.h"

using namespace unity::launcher;
using namespace testing;

namespace
{
const int ICON_SIZE = 15;
}

namespace unity
{
namespace launcher
{
namespace
{

struct StubLauncherDragWindow : public LauncherDragWindow
{
  struct DrawCallback
  {
    virtual ~DrawCallback() {}
    MOCK_METHOD0(callback, void());
  };

  StubLauncherDragWindow()
   : LauncherDragWindow(ICON_SIZE, std::bind(&DrawCallback::callback, &cb))
  {}

  bool DrawContentOnNuxLayer() const { return false; }
  using LauncherDragWindow::DrawContent;

  DrawCallback cb;
};

struct TestLauncherDragWindow : public testing::Test
{
  TestLauncherDragWindow()
    : drag_window(new StubLauncherDragWindow())
  {}

  nux::ObjectPtr<StubLauncherDragWindow> drag_window;
};
}

TEST_F(TestLauncherDragWindow, Construction)
{
  EXPECT_EQ(drag_window->GetBaseWidth(), ICON_SIZE);
  EXPECT_EQ(drag_window->GetBaseHeight(), ICON_SIZE);
  EXPECT_FALSE(drag_window->Animating());
  EXPECT_FALSE(drag_window->Cancelled());
}

TEST_F(TestLauncherDragWindow, NoDrawOnConstruction)
{
  EXPECT_CALL(drag_window->cb, callback()).Times(0);
  drag_window = new StubLauncherDragWindow();
}

TEST_F(TestLauncherDragWindow, DrawOnFirstPaint)
{
  EXPECT_CALL(drag_window->cb, callback()).Times(1);
  drag_window->DrawContent(*(nux::GetGraphicsDisplay()->GetGraphicsEngine()), false);
}

TEST_F(TestLauncherDragWindow, NoDrawOnSecondPaint)
{
  EXPECT_CALL(drag_window->cb, callback()).Times(1);
  drag_window->DrawContent(*(nux::GetGraphicsDisplay()->GetGraphicsEngine()), false);
  drag_window->DrawContent(*(nux::GetGraphicsDisplay()->GetGraphicsEngine()), false);
}

TEST_F(TestLauncherDragWindow, EscapeRequestsCancellation)
{
  nux::Event cancel;
  cancel.type = nux::NUX_KEYDOWN;
  cancel.x11_keysym = NUX_VK_ESCAPE;
  bool got_signal;

  drag_window->drag_cancel_request.connect([&got_signal] { got_signal = true; });
  drag_window->GrabKeyboard();
  nux::GetWindowCompositor().ProcessEvent(cancel);

  EXPECT_TRUE(got_signal);
  EXPECT_TRUE(drag_window->Cancelled());
}

TEST_F(TestLauncherDragWindow, CancelsOnWindowMapped)
{
  bool got_signal;
  drag_window->drag_cancel_request.connect([&got_signal] { got_signal = true; });
  WindowManager::Default().window_mapped.emit(0);

  EXPECT_TRUE(got_signal);
  EXPECT_TRUE(drag_window->Cancelled());
}

TEST_F(TestLauncherDragWindow, CancelsOnWindowUnmapped)
{
  bool got_signal;
  drag_window->drag_cancel_request.connect([&got_signal] { got_signal = true; });
  WindowManager::Default().window_unmapped.emit(0);

  EXPECT_TRUE(got_signal);
  EXPECT_TRUE(drag_window->Cancelled());
}

}
}
