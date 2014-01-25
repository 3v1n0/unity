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
#include "PanelIndicatorsView.h"

namespace unity
{
namespace panel
{
namespace
{

struct MockPanelIndicatorsView : PanelIndicatorsView
{
  MOCK_METHOD0(QueueDraw, void());
};

struct TestPanelIndicatorsView : public testing::Test
{
  testing::NiceMock<MockPanelIndicatorsView> indicators;
};

TEST_F(TestPanelIndicatorsView, Construction)
{
  EXPECT_EQ(indicators.opacity(), 1.0f);
}

TEST_F(TestPanelIndicatorsView, OpacitySet)
{
  indicators.opacity = 0.555f;
  EXPECT_EQ(indicators.opacity(), 0.555f);

  indicators.opacity = -0.355f;
  EXPECT_EQ(indicators.opacity(), 0.0f);

  indicators.opacity = 5.355f;
  EXPECT_EQ(indicators.opacity(), 1.0f);
}

TEST_F(TestPanelIndicatorsView, ChangingOpacityQueuesDraw)
{
  EXPECT_CALL(indicators, QueueDraw()).Times(1);
  indicators.opacity = 0.555f;

  EXPECT_CALL(indicators, QueueDraw()).Times(0);
  indicators.opacity = 0.555f;
}

} // anonymous namespace
} // panel namespace
} // unity namespace
