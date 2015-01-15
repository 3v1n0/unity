/*
 * Copyright 2013 Canonical Ltd.
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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 *
 */

#include <gmock/gmock.h>

#include "PanelView.h"
#include "unity-shared/MockableBaseWindow.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UBusWrapper.h"

#include "mock_menu_manager.h"
#include "test_standalone_wm.h"
#include "test_utils.h"

namespace
{
using namespace unity;
using namespace unity::panel;

class TestPanelView : public testing::Test
{
public:
  Style panel_style_;
  UBusManager ubus_manager_;
  nux::ObjectPtr<MockableBaseWindow> window_;
  nux::ObjectPtr<PanelView> panel_view_;
  testwrapper::StandaloneWM WM;

  TestPanelView()
    : window_(new MockableBaseWindow())
    , panel_view_(new PanelView(window_.GetPointer(), std::make_shared<menu::MockManager>()))
  {}
};

TEST_F(TestPanelView, Construction)
{
  EXPECT_FALSE(panel_view_->InOverlayMode());
}

TEST_F(TestPanelView, StoredDashWidth)
{
  auto check_function = [this] (int value) {
    return panel_view_->GetStoredDashWidth() == value;
  };

  int width = 500;
  int height =  600;

  GVariant* info = g_variant_new(UBUS_OVERLAY_FORMAT_STRING, "dash", TRUE, 0, width,  height);
  ubus_manager_.SendMessage(UBUS_OVERLAY_SHOWN, info);
  Utils::WaitUntil(std::bind(check_function, width));

  width =  150;

  ubus_manager_.SendMessage(UBUS_DASH_SIZE_CHANGED, g_variant_new("(ii)", width, height));
  Utils::WaitUntil(std::bind(check_function, width));
}

TEST_F(TestPanelView, HandleBarrierEvent)
{
  auto barrier = std::make_shared<ui::PointerBarrierWrapper>();
  auto event = std::make_shared<ui::BarrierEvent>(0, 0, 0, 100);

  WM->SetIsAnyWindowMoving(false);
  EXPECT_EQ(panel_view_->HandleBarrierEvent(barrier, event),
            ui::EdgeBarrierSubscriber::Result::NEEDS_RELEASE);

  WM->SetIsAnyWindowMoving(true);
  EXPECT_EQ(panel_view_->HandleBarrierEvent(barrier, event),
            ui::EdgeBarrierSubscriber::Result::IGNORED);
}

TEST_F(TestPanelView, InOverlayModeDash)
{
  GVariant* info = g_variant_new(UBUS_OVERLAY_FORMAT_STRING, "dash", TRUE, 0, 10, 20);
  ubus_manager_.SendMessage(UBUS_OVERLAY_SHOWN, info);
  Utils::WaitUntil([this] {return panel_view_->InOverlayMode();});
}

TEST_F(TestPanelView, InOverlayModeSpread)
{
  WM->SetScaleActive(true);
  EXPECT_TRUE(panel_view_->InOverlayMode());
}

TEST_F(TestPanelView, InOverlayModeSpreadThenDash)
{
  WM->SetScaleActive(true);
  ASSERT_TRUE(panel_view_->InOverlayMode());

  glib::Variant info(g_variant_new(UBUS_OVERLAY_FORMAT_STRING, "dash", TRUE, 0, 10, 20));
  ubus_manager_.SendMessage(UBUS_OVERLAY_SHOWN, info);
  Utils::WaitPendingEvents(100);
  ASSERT_TRUE(panel_view_->InOverlayMode());

  WM->SetScaleActive(false);
  EXPECT_TRUE(panel_view_->InOverlayMode());

  ubus_manager_.SendMessage(UBUS_OVERLAY_HIDDEN, info);
  Utils::WaitUntil([this] {return !panel_view_->InOverlayMode();});
  EXPECT_FALSE(panel_view_->InOverlayMode());
}

TEST_F(TestPanelView, InOverlayModeDashThenSpread)
{
  glib::Variant info(g_variant_new(UBUS_OVERLAY_FORMAT_STRING, "dash", TRUE, 0, 10, 20));
  ubus_manager_.SendMessage(UBUS_OVERLAY_SHOWN, info);
  Utils::WaitUntil([this] {return panel_view_->InOverlayMode();});
  ASSERT_TRUE(panel_view_->InOverlayMode());

  WM->SetScaleActive(true);
  ASSERT_TRUE(panel_view_->InOverlayMode());

  ubus_manager_.SendMessage(UBUS_OVERLAY_HIDDEN, info);
  Utils::WaitPendingEvents(100);
  EXPECT_TRUE(panel_view_->InOverlayMode());

  WM->SetScaleActive(false);
  EXPECT_FALSE(panel_view_->InOverlayMode());
}

}
