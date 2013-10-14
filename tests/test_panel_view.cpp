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

#include <gtest/gtest.h>

#include "PanelView.h"
#include "unity-shared/MockableBaseWindow.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/StandaloneWindowManager.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/UnitySettings.h"

#include "test_utils.h"

namespace
{

class TestPanelView : public testing::Test
{
public:
  unity::Settings unity_settings_;
  unity::panel::Style panel_style_;
  unity::UBusManager ubus_manager_;
  nux::ObjectPtr<unity::MockableBaseWindow> window_;
  nux::ObjectPtr<unity::PanelView> panel_view_;
  unity::StandaloneWindowManager* WM;

  TestPanelView()
    : window_(new unity::MockableBaseWindow())
    , panel_view_(new unity::PanelView(window_.GetPointer(), std::make_shared<unity::indicator::DBusIndicators>()))
    , WM (dynamic_cast<unity::StandaloneWindowManager*>(&unity::WindowManager::Default()))
  {}

  ~TestPanelView()
  {
    WM->ResetStatus();
  }

};

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
  auto barrier = std::make_shared<unity::ui::PointerBarrierWrapper>();
  auto event = std::make_shared<ui::BarrierEvent>(0, 0, 0, 100);
 
  WM->SetIsAnyWindowMoving(false);
  EXPECT_EQ(panel_view_->HandleBarrierEvent(barrier.get(), event),
            ui::EdgeBarrierSubscriber::Result::NEEDS_RELEASE);

  WM->SetIsAnyWindowMoving(true);
  EXPECT_EQ(panel_view_->HandleBarrierEvent(barrier.get(), event),
            ui::EdgeBarrierSubscriber::Result::IGNORED);
}

}
