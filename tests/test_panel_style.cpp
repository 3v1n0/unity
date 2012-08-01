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

#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>
#include <gtest/gtest.h>

#include "unity-shared/PanelStyle.h"
#include "unity-shared/UnitySettings.h"

using namespace unity;
using namespace testing;

namespace
{

const std::string SCHEMA_DIRECTORY = BUILDDIR"/settings";
const std::string BASE_STORE_FILE = BUILDDIR"/settings/test-panel-style-gsettings.store";
const std::string BASE_STORE_CONTENTS = "[org/gnome/desktop/wm/preferences]\n" \
                                        "titlebar-font='%s'";
const std::string TITLEBAR_FONT = "Ubuntu Bold 11";


class TestPanelStyle : public Test
{
public:
  glib::Object<GSettingsBackend> backend;
  Settings unity_settings;
  std::unique_ptr<panel::Style> panel_style_instance;

  /* override */ void SetUp()
  {
    // set the data directory so gsettings can find the schema
    g_setenv("GSETTINGS_SCHEMA_DIR", SCHEMA_DIRECTORY.c_str(), false);

    glib::Error error;
    glib::String contents(g_strdup_printf(BASE_STORE_CONTENTS.c_str(), TITLEBAR_FONT.c_str()));

    g_file_set_contents(BASE_STORE_FILE.c_str(),
                        contents.Value(),
                        -1,
                        &error);

    ASSERT_FALSE(error);

    backend = g_keyfile_settings_backend_new(BASE_STORE_FILE.c_str(), "/", "root");
    panel_style_instance.reset(new panel::Style(backend));
  }
};

TEST_F(TestPanelStyle, TestAllocation)
{
  EXPECT_TRUE(G_IS_SETTINGS_BACKEND(backend.RawPtr()));
}

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

  glib::Object<GSettings> gsettings(g_settings_new_with_backend("org.gnome.desktop.wm.zXpreferences", backend));
  g_settings_set_string(gsettings, "titlebar-font", "Ubuntu Italic 11");

  sleep(1);

  ASSERT_TRUE(signal_received);
  ASSERT_EQ(panel_style_instance->GetFontDescription(panel::PanelItem::TITLE), "Ubuntu Italic 11");  
}

}
