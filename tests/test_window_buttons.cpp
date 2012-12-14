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
#include "WindowButtons.h"
#include "PanelStyle.h"
#include "UnitySettings.h"
#include "StandaloneWindowManager.h"

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
}
}
