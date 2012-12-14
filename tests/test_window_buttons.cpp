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
 * Authored by: Marco Trevisan (Treviño) <marco@ubuntu.com>
 *
 */

#include <gmock/gmock.h>

#include <Nux/Nux.h>
#include "PanelStyle.h"
#include "StandaloneWindowManager.h"
#include "UnitySettings.h"
#include "WindowButtons.h"
#include "WindowButtonPriv.h"

namespace unity
{
namespace
{

struct MockWindowButtons : WindowButtons
{
  MOCK_METHOD0(QueueDraw, void());
};

struct TestWindowButtons : public testing::Test
{
  TestWindowButtons()
    : wm(dynamic_cast<StandaloneWindowManager*>(&WindowManager::Default()))
  {}

  StandaloneWindow::Ptr AddFakeWindowToWM(Window xid)
  {
    auto fake_window = std::make_shared<StandaloneWindow>(xid);
    wm->AddStandaloneWindow(fake_window);

    return fake_window;
  }

  internal::WindowButton* GetWindowButtonByType(panel::WindowButtonType type)
  {
    for (auto* area : wbuttons.GetChildren())
    {
      auto button = dynamic_cast<internal::WindowButton*>(area);

      if (button && button->GetType() == type)
        return button;
    }

    return nullptr;
  }

  Settings settings;
  panel::Style panel_style;
  testing::NiceMock<MockWindowButtons> wbuttons;
  StandaloneWindowManager* wm = nullptr;
};

TEST_F(TestWindowButtons, Construction)
{
  EXPECT_EQ(wbuttons.monitor(), 0);
  EXPECT_EQ(wbuttons.controlled_window(), 0);
  EXPECT_EQ(wbuttons.opacity(), 1.0f);
  EXPECT_EQ(wbuttons.focused(), true);

  ASSERT_NE(GetWindowButtonByType(panel::WindowButtonType::CLOSE), nullptr);
  ASSERT_NE(GetWindowButtonByType(panel::WindowButtonType::MINIMIZE), nullptr);
  ASSERT_NE(GetWindowButtonByType(panel::WindowButtonType::MAXIMIZE), nullptr);
  ASSERT_NE(GetWindowButtonByType(panel::WindowButtonType::UNMAXIMIZE), nullptr);
}

TEST_F(TestWindowButtons, OpacitySet)
{
  wbuttons.opacity = 0.555f;
  EXPECT_EQ(wbuttons.opacity(), 0.555f);

  wbuttons.opacity = -0.355f;
  EXPECT_EQ(wbuttons.opacity(), 0.0f);

  wbuttons.opacity = 5.355f;
  EXPECT_EQ(wbuttons.opacity(), 1.0f);
}

TEST_F(TestWindowButtons, ChangingOpacityQueuesDraw)
{
  EXPECT_CALL(wbuttons, QueueDraw()).Times(1);
  wbuttons.opacity = 0.555f;

  EXPECT_CALL(wbuttons, QueueDraw()).Times(0);
  wbuttons.opacity = 0.555f;
}

TEST_F(TestWindowButtons, ChangingFocusedQueuesDraw)
{
  EXPECT_CALL(wbuttons, QueueDraw()).Times(1);
  wbuttons.focused = false;

  EXPECT_CALL(wbuttons, QueueDraw()).Times(0);
  wbuttons.focused = false;

  EXPECT_CALL(wbuttons, QueueDraw()).Times(1);
  wbuttons.focused = true;
}

TEST_F(TestWindowButtons, ChangingControlledWindowUpdatesCloseButton)
{
  Window xid = 12345;
  auto fake_win = AddFakeWindowToWM(xid);
  ASSERT_TRUE(fake_win->closable);
  wbuttons.controlled_window = xid;
  EXPECT_TRUE(GetWindowButtonByType(panel::WindowButtonType::CLOSE)->IsEnabled());

  xid = 54321;
  fake_win = AddFakeWindowToWM(xid);
  fake_win->closable = false;
  wbuttons.controlled_window = xid;
  EXPECT_FALSE(GetWindowButtonByType(panel::WindowButtonType::CLOSE)->IsEnabled());
}

TEST_F(TestWindowButtons, ChangingControlledWindowUpdatesMinimizeButton)
{
  Window xid = 12345;
  auto fake_win = AddFakeWindowToWM(xid);
  ASSERT_TRUE(fake_win->minimizable);
  wbuttons.controlled_window = xid;
  EXPECT_TRUE(GetWindowButtonByType(panel::WindowButtonType::MINIMIZE)->IsEnabled());

  xid = 54321;
  fake_win = AddFakeWindowToWM(xid);
  fake_win->minimizable = false;
  wbuttons.controlled_window = xid;
  EXPECT_FALSE(GetWindowButtonByType(panel::WindowButtonType::MINIMIZE)->IsEnabled());
}
}
}
