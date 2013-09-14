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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 *
 */

#include <gio/gio.h>
#include <gtest/gtest.h>

#include "test_utils.h"
#include "unity-shared/UnitySettings.h"

#include <UnityCore/GLibWrapper.h>

namespace
{

class TestUnitySettings : public testing::Test
{
public:
  unity::glib::Object<GSettings> gsettings;
  std::unique_ptr<unity::Settings> unity_settings;

  void SetUp()
  {
    Utils::init_gsettings_test_environment();
    gsettings = g_settings_new("com.canonical.Unity");
    g_settings_set_enum(gsettings, "form-factor", static_cast<int>(unity::FormFactor::DESKTOP));

    unity_settings.reset(new unity::Settings);
  }

  void TearDown()
  {
    Utils::reset_gsettings_test_environment();
  }
};

TEST_F(TestUnitySettings, SetFormFactor)
{
  unity_settings->form_factor = unity::FormFactor::NETBOOK;

  int raw_form_factor = g_settings_get_enum(gsettings, "form-factor");
  EXPECT_EQ(raw_form_factor, static_cast<int>(unity::FormFactor::NETBOOK));
}

TEST_F(TestUnitySettings, GetFormFactor)
{
  EXPECT_EQ(unity_settings->form_factor(), unity::FormFactor::DESKTOP);

  g_settings_set_enum(gsettings, "form-factor", static_cast<int>(unity::FormFactor::NETBOOK));
  EXPECT_EQ(unity_settings->form_factor(), unity::FormFactor::NETBOOK);
}

TEST_F(TestUnitySettings, FormFactorChangedSignal_Extern)
{
  bool signal_received = false;
  unity::FormFactor new_form_factor;
  unity_settings->form_factor.changed.connect([&](unity::FormFactor form_factor) {
    signal_received = true;
    new_form_factor = form_factor;
  });

  g_settings_set_enum(gsettings, "form-factor", static_cast<int>(unity::FormFactor::NETBOOK));
  Utils::WaitUntilMSec(signal_received);
  EXPECT_EQ(new_form_factor, unity::FormFactor::NETBOOK);
}

TEST_F(TestUnitySettings, FormFactorChangedSignal_Extern_OtherKeys)
{
  bool signal_received = false;
  unity_settings->form_factor.changed.connect([&](unity::FormFactor form_factor) {
    signal_received = true;
  });

  g_settings_set_int(gsettings, "minimize-count", 0);
  Utils::WaitForTimeoutMSec(100);
  EXPECT_FALSE(signal_received);
}

TEST_F(TestUnitySettings, FormFactorChangedSignal_Inter)
{
  bool signal_received = false;
  unity::FormFactor new_form_factor;
  unity_settings->form_factor.changed.connect([&](unity::FormFactor form_factor) {
    signal_received = true;
    new_form_factor = form_factor;
  });

  unity_settings->form_factor = unity::FormFactor::NETBOOK;
  Utils::WaitUntilMSec(signal_received);
  EXPECT_EQ(new_form_factor, unity::FormFactor::NETBOOK);
}

}
