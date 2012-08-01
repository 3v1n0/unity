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

#include <config.h>

#include <gio/gio.h>
#include <gtest/gtest.h>

#include "unity-shared/PanelStyle.h"
#include "unity-shared/UnitySettings.h"

using namespace unity;
using namespace testing;

namespace
{

const std::string TITLEBAR_FONT = "Ubuntu Bold 11";

class TestPanelStyle : public Test
{
public:
  glib::Object<GSettings> gsettings;
  Settings unity_settings;
  std::unique_ptr<panel::Style> panel_style_instance;

  /* override */ void SetUp()
  {
	g_setenv("GSETTINGS_BACKEND", "memory", TRUE);

    gsettings = g_settings_new("org.gnome.desktop.wm.preferences");
    g_settings_set_string(gsettings, "titlebar-font", TITLEBAR_FONT.c_str());

    panel_style_instance.reset(new panel::Style());
  }

  /* override */ void TearDown()
  {
    g_setenv("GSETTINGS_BACKEND", "", TRUE);
  }
};

TEST_F(TestPanelStyle, TestGetFontDescription)
{
  ASSERT_EQ(panel_style_instance->GetFontDescription(panel::PanelItem::TITLE), TITLEBAR_FONT);
}

TEST_F(TestPanelStyle, TestChangedSignal)
{
  bool signal_received = false;

  panel_style_instance->changed.connect([&](){
    signal_received = true;
  });

  g_settings_set_string(gsettings, "titlebar-font", "Ubuntu Italic 11");

  sleep(1);

  ASSERT_TRUE(signal_received);
  ASSERT_EQ(panel_style_instance->GetFontDescription(panel::PanelItem::TITLE), "Ubuntu Italic 11");  
}

}
