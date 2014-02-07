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
#include <Nux/HLayout.h>
#include "PanelIndicatorsView.h"
#include "PanelStyle.h"
#include "UnitySettings.h"
#include "mock_indicators.h"

namespace unity
{
namespace panel
{
namespace
{

struct MockPanelIndicatorsView : PanelIndicatorsView
{
  MOCK_METHOD0(QueueDraw, void());

  PanelIndicatorEntryDropdownView::Ptr GetDropdown() const
  {
    for (auto* area : layout_->GetChildren())
    {
      if (PanelIndicatorEntryDropdownView* dropdown = dynamic_cast<PanelIndicatorEntryDropdownView*>(area))
        return PanelIndicatorEntryDropdownView::Ptr(dropdown);
    }

    return PanelIndicatorEntryDropdownView::Ptr();
  }
};

struct TestPanelIndicatorsView : testing::Test
{
  Settings settings_;
  Style style_;
  testing::NiceMock<MockPanelIndicatorsView> indicators;
};

TEST_F(TestPanelIndicatorsView, Construction)
{
  EXPECT_EQ(indicators.opacity(), 1.0f);
  EXPECT_EQ(PanelIndicatorEntryDropdownView::Ptr(), indicators.GetDropdown());
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

TEST_F(TestPanelIndicatorsView, EnableDropdownMenuInvalid)
{
  indicators.EnableDropdownMenu(false, nullptr);
  EXPECT_EQ(PanelIndicatorEntryDropdownView::Ptr(), indicators.GetDropdown());

  indicators.EnableDropdownMenu(false, std::make_shared<indicator::MockIndicators::Nice>());
  EXPECT_EQ(PanelIndicatorEntryDropdownView::Ptr(), indicators.GetDropdown());

  indicators.EnableDropdownMenu(true, nullptr);
  EXPECT_EQ(PanelIndicatorEntryDropdownView::Ptr(), indicators.GetDropdown());
}

TEST_F(TestPanelIndicatorsView, EnableDropdownMenu)
{
  indicators.EnableDropdownMenu(true, std::make_shared<indicator::MockIndicators::Nice>());
  EXPECT_NE(PanelIndicatorEntryDropdownView::Ptr(), indicators.GetDropdown());
}

} // anonymous namespace
} // panel namespace
} // unity namespace
