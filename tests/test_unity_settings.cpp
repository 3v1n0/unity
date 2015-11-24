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
#include <gmock/gmock.h>

#include "UnitySettings.h"
#include "test_utils.h"

#include <NuxCore/Logger.h>
#include <UnityCore/GLibWrapper.h>

namespace
{
struct SigReceiver : sigc::trackable
{
  typedef testing::NiceMock<SigReceiver> Nice;

  SigReceiver(std::shared_ptr<unity::Settings> const& settings)
  {
   settings->form_factor.changed.connect(sigc::mem_fun(this, &SigReceiver::FormFactorChanged));
  }

  MOCK_CONST_METHOD1(FormFactorChanged, void(unity::FormFactor));
};

struct TestUnitySettings : testing::Test
{
  unity::glib::Object<GSettings> gsettings;
  std::shared_ptr<unity::Settings> unity_settings;
  SigReceiver::Nice sig_receiver;

  TestUnitySettings()
   : gsettings(g_settings_new("com.canonical.Unity"))
   , unity_settings(std::make_shared<unity::Settings>())
   , sig_receiver(unity_settings)
  {
    g_settings_set_enum(gsettings, "form-factor", static_cast<int>(unity::FormFactor::DESKTOP));
    g_settings_set_enum(gsettings, "desktop-type", static_cast<int>(unity::DesktopType::UBUNTU));
  }

  ~TestUnitySettings()
  {
    sig_receiver.notify_callbacks();
    g_settings_reset(gsettings, "form-factor");
    g_settings_reset(gsettings, "desktop-type");
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
  ASSERT_NE(unity_settings->form_factor(), unity::FormFactor::NETBOOK);

  g_settings_set_enum(gsettings, "form-factor", static_cast<int>(unity::FormFactor::NETBOOK));
  EXPECT_EQ(unity_settings->form_factor(), unity::FormFactor::NETBOOK);
}

TEST_F(TestUnitySettings, GetDesktopType)
{
  ASSERT_NE(unity_settings->desktop_type(), unity::DesktopType::UBUNTUKYLIN);

  g_settings_set_enum(gsettings, "desktop-type", static_cast<int>(unity::DesktopType::UBUNTUKYLIN));
  EXPECT_EQ(unity_settings->desktop_type(), unity::DesktopType::UBUNTUKYLIN);
}

TEST_F(TestUnitySettings, FormFactorChangedSignal_Extern)
{
  EXPECT_CALL(sig_receiver, FormFactorChanged(unity::FormFactor::NETBOOK));

  g_settings_set_enum(gsettings, "form-factor", static_cast<int>(unity::FormFactor::NETBOOK));
}

TEST_F(TestUnitySettings, FormFactorChangedSignal_Extern_OtherKeys)
{
  EXPECT_CALL(sig_receiver, FormFactorChanged(testing::_)).Times(0);

  g_settings_set_int(gsettings, "minimize-count", 0);
  Utils::WaitForTimeoutMSec(100);
}

TEST_F(TestUnitySettings, FormFactorChangedSignal_Inter)
{
  EXPECT_CALL(sig_receiver, FormFactorChanged(unity::FormFactor::NETBOOK));

  unity_settings->form_factor = unity::FormFactor::NETBOOK;
}

}
