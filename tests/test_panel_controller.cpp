// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include <gmock/gmock.h>

#include "PanelController.h"
#include "PanelStyle.h"
#include "PanelView.h"
#include "UnitySettings.h"
#include "test_uscreen_mock.h"
#include "launcher/LauncherOptions.h"

namespace {

struct TestPanelController : public testing::Test
{
  TestPanelController()
  : edge_barriers(std::make_shared<unity::ui::EdgeBarrierController>())
  , options(std::make_shared<unity::launcher::Options>())
  {
    edge_barriers->options = options;
  }

  unity::MockUScreen uscreen;
  unity::Settings settings;
  unity::panel::Style panel_style;
  unity::ui::EdgeBarrierController::Ptr edge_barriers;
  unity::launcher::Options::Ptr options;
};

TEST_F(TestPanelController, Construction)
{
  nux::ObjectPtr<unity::PanelView> panel_ptr;

  {
    unity::panel::Controller pc(edge_barriers);

    ASSERT_EQ(pc.panels().size(), 1);
    EXPECT_EQ(pc.panels()[0]->GetMonitor(), 0);
    panel_ptr = pc.panels()[0];
  }

  ASSERT_EQ(1, panel_ptr->GetReferenceCount());
}

TEST_F(TestPanelController, Multimonitor)
{
  uscreen.SetupFakeMultiMonitor();
  unity::panel::Controller::PanelVector panel_ptrs;

  {
    unity::panel::Controller pc(edge_barriers);

    ASSERT_EQ(pc.panels().size(), unity::monitors::MAX);

    for (unsigned int i = 0; i < unity::monitors::MAX; ++i)
    {
      ASSERT_EQ(pc.panels()[i]->GetMonitor(), i);
      panel_ptrs.push_back(pc.panels()[i]);
    }
  }

  for (auto const& panel_ptr : panel_ptrs)
  {
    ASSERT_EQ(1, panel_ptr->GetReferenceCount());
  }
}

TEST_F(TestPanelController, MultimonitorSwitchToSingleMonitor)
{
  uscreen.SetupFakeMultiMonitor();

  {
    unity::panel::Controller pc(edge_barriers);

    ASSERT_EQ(pc.panels().size(), unity::monitors::MAX);

    uscreen.Reset();
    EXPECT_EQ(pc.panels().size(), 1);
    EXPECT_EQ(pc.panels()[0]->GetMonitor(), 0);
  }
}

TEST_F(TestPanelController, MultimonitorRemoveMiddleMonitor)
{
  uscreen.SetupFakeMultiMonitor();

  {
    unity::panel::Controller pc(edge_barriers);

    ASSERT_EQ(pc.panels().size(), unity::monitors::MAX);

    std::vector<nux::Geometry> &monitors = uscreen.GetMonitors();
    monitors.erase(monitors.begin() + monitors.size()/2);
    uscreen.changed.emit(uscreen.GetPrimaryMonitor(), uscreen.GetMonitors());
    ASSERT_EQ(pc.panels().size(), unity::monitors::MAX - 1);

    for (unsigned int i = 0; i < unity::monitors::MAX - 1; ++i)
      ASSERT_EQ(pc.panels()[i]->GetMonitor(), i);
  }
}

TEST_F(TestPanelController, SingleMonitorSwitchToMultimonitor)
{
  {
    unity::panel::Controller pc(edge_barriers);

    ASSERT_EQ(pc.panels().size(), 1);

    uscreen.SetupFakeMultiMonitor();
    EXPECT_EQ(pc.panels().size(), unity::monitors::MAX);
  }
}

TEST_F(TestPanelController, MultimonitorGeometries)
{
  uscreen.SetupFakeMultiMonitor();

  {
    unity::panel::Controller pc(edge_barriers);

    for (unsigned int i = 0; i < unity::monitors::MAX; ++i)
    {
      auto const& monitor_geo = uscreen.GetMonitorGeometry(i);
      auto const& panel_geo = pc.panels()[i]->GetAbsoluteGeometry();
      ASSERT_EQ(panel_geo.x, monitor_geo.x);
      ASSERT_EQ(panel_geo.y, monitor_geo.y);
      ASSERT_EQ(panel_geo.width, monitor_geo.width);
      ASSERT_EQ(panel_geo.height, panel_style.PanelHeight(i));
    }
  }
}

TEST_F(TestPanelController, MonitorResizesPanels)
{
  unity::panel::Controller pc(edge_barriers);

  nux::Geometry monitor_geo = uscreen.GetMonitorGeometry(0);
  monitor_geo.SetSize(monitor_geo.width/2, monitor_geo.height/2);
  uscreen.SetMonitors({monitor_geo});
  nux::Geometry panel_geo = pc.panels()[0]->GetAbsoluteGeometry();
  ASSERT_EQ(panel_geo.x, monitor_geo.x);
  ASSERT_EQ(panel_geo.y, monitor_geo.y);
  ASSERT_EQ(panel_geo.width, monitor_geo.width);
  ASSERT_EQ(panel_geo.height, panel_style.PanelHeight());

  uscreen.Reset();
  monitor_geo = uscreen.GetMonitorGeometry(0);
  panel_geo = pc.panels()[0]->GetAbsoluteGeometry();
  ASSERT_EQ(panel_geo.x, monitor_geo.x);
  ASSERT_EQ(panel_geo.y, monitor_geo.y);
  ASSERT_EQ(panel_geo.width, monitor_geo.width);
  ASSERT_EQ(panel_geo.height, panel_style.PanelHeight());
}

TEST_F(TestPanelController, MultiMonitorEdgeBarrierSubscriptions)
{
  uscreen.SetupFakeMultiMonitor();

  {
    unity::panel::Controller pc(edge_barriers);

    for (unsigned i = 0; i < unity::monitors::MAX; ++i)
      ASSERT_EQ(edge_barriers->GetHorizontalSubscriber(i), pc.panels()[i].GetPointer());
  }
}

}
