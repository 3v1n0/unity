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
#include "unity-shared/PanelStyle.h"
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
  nux::ObjectPtr<unity::PanelView> panel_view_;

  TestPanelView()
    : panel_view_(new unity::PanelView(std::make_shared<unity::indicator::DBusIndicators>()))
  {}

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

}
