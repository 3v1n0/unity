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
 */

#include <gmock/gmock.h>

#include <Nux/Nux.h>
#include "PanelMenuView.h"
#include "PanelStyle.h"
#include "UnitySettings.h"
#include "UBusMessages.h"
#include "mock_menu_manager.h"
#include "test_standalone_wm.h"
#include "test_uscreen_mock.h"
#include "test_utils.h"

using namespace testing;

namespace unity
{
namespace panel
{

struct TestPanelMenuView : public testing::Test
{
  struct MockPanelMenuView : public PanelMenuView
  {
    MockPanelMenuView()
      : PanelMenuView(std::make_shared<menu::MockManager>())
    {}

    MOCK_METHOD0(QueueDraw, void());
    MOCK_CONST_METHOD1(GetActiveViewName, std::string(bool));

    using PanelMenuView::GetCurrentTitle;
    using PanelMenuView::window_buttons_;
    using PanelMenuView::titlebar_grab_area_;
    using PanelMenuView::we_control_active_;
  };

  nux::ObjectPtr<nux::BaseWindow> AddPanelToWindow(int monitor)
  {
    nux::ObjectPtr<nux::BaseWindow> panel_win(new nux::BaseWindow());
    auto const& monitor_geo = uscreen.GetMonitorGeometry(monitor);
    panel_win->SetGeometry(monitor_geo);
    panel_win->SetMaximumHeight(panelStyle.panel_height());
    panel_win->SetLayout(new nux::HLayout(NUX_TRACKER_LOCATION));
    panel_win->GetLayout()->AddView(&menu_view, 1);
    panel_win->GetLayout()->SetContentDistribution(nux::MAJOR_POSITION_START);
    panel_win->GetLayout()->SetVerticalExternalMargin(0);
    panel_win->GetLayout()->SetHorizontalExternalMargin(0);
    panel_win->ComputeContentSize();

    menu_view.SetMonitor(monitor);

    return panel_win;
  }

protected:
  // The order is important, i.e. menu_view needs
  // panel::Style that needs Settings
  MockUScreen uscreen;
  Settings settings;
  panel::Style panelStyle;
  testwrapper::StandaloneWM WM;
  testing::NiceMock<MockPanelMenuView> menu_view;
};

TEST_F(TestPanelMenuView, Escaping)
{
  ON_CALL(menu_view, GetActiveViewName(testing::_)).WillByDefault(Return("<>'"));
  auto escapedText = "Panel d'Inici";
  ASSERT_TRUE(menu_view.GetCurrentTitle().empty());

  UBusManager ubus;
  ubus.SendMessage(UBUS_LAUNCHER_START_KEY_NAV);
  ubus.SendMessage(UBUS_LAUNCHER_SELECTION_CHANGED, glib::Variant(escapedText));
  Utils::WaitUntilMSec([this, &escapedText] {return menu_view.GetCurrentTitle() == escapedText;});

  menu_view.we_control_active_ = true;
  ubus.SendMessage(UBUS_LAUNCHER_END_KEY_NAV);
  Utils::WaitUntilMSec([this] {return menu_view.GetCurrentTitle() == "<>'";});
}

TEST_F(TestPanelMenuView, QueuesDrawOnButtonsOpacityChange)
{
  EXPECT_CALL(menu_view, QueueDraw());
  menu_view.window_buttons_->opacity.changed.emit(0.5f);
}

struct ProgressTester : TestPanelMenuView, WithParamInterface<double> {};
INSTANTIATE_TEST_CASE_P(TestPanelMenuView, ProgressTester, Range(0.0, 1.0, 0.1));

TEST_P(ProgressTester, RestoreOnGrabInBiggerWorkArea)
{
  uscreen.SetupFakeMultiMonitor();
  unsigned monitor = uscreen.GetMonitors().size() - 1;
  auto const& monitor_geo = uscreen.GetMonitorGeometry(monitor);
  WM->SetWorkareaGeometry(monitor_geo);

  auto panel_win = AddPanelToWindow(monitor);

  auto max_window = std::make_shared<StandaloneWindow>(g_random_int());
  WM->AddStandaloneWindow(max_window);

  max_window->maximized = true;
  nux::Geometry win_geo(monitor_geo.x + monitor_geo.width/4, monitor_geo.y + monitor_geo.height/4,
                        monitor_geo.width/2, monitor_geo.height/2);
  max_window->geo = win_geo;

  bool restored = false;
  bool moved = false;
  WM->window_restored.connect([&] (Window xid) {restored = (max_window->Xid() == xid);});
  WM->window_moved.connect([&] (Window xid) {moved = (max_window->Xid() == xid);});

  // Grab the window outside the panel shape
  nux::Point mouse_pos(panel_win->GetX() + panel_win->GetWidth() * GetParam(), panel_win->GetY() + panel_win->GetHeight() + 1);
  menu_view.titlebar_grab_area_->grab_move(mouse_pos.x - panel_win->GetX(), mouse_pos.y - panel_win->GetY());

  nux::Geometry expected_geo(win_geo);
  expected_geo.SetPosition(mouse_pos.x - (win_geo.width * (mouse_pos.x - panel_win->GetX()) / panel_win->GetWidth()), mouse_pos.y);
  expected_geo.x = std::max<int>(expected_geo.x, monitor_geo.x);

  EXPECT_TRUE(restored);
  EXPECT_TRUE(moved);
  EXPECT_FALSE(max_window->maximized());
  EXPECT_EQ(max_window->geo(), expected_geo);
}

} // panel namespace
} // unity namespace
