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
#include "StandaloneWindowManager.h"
#include "test_utils.h"

using namespace testing;

namespace unity
{

class MockPanelMenuView : public PanelMenuView
{
  public:
    using PanelMenuView::_panel_title;
    using PanelMenuView::RefreshTitle;

  protected:
    virtual std::string GetActiveViewName(bool use_appname) const
    {
      return "<>'";
    }
};

struct TestPanelMenuView : public testing::Test
{
  virtual void SetUp()
  {
  }

  void ProcessUBusMessages()
  {
    bool expired = false;
    glib::Idle idle([&] { expired = true; return false; },
                    glib::Source::Priority::LOW);
    Utils::WaitUntil(expired);
  }

protected:

  // The order is important, i.e. PanelMenuView needs
  // panel::Style that needs Settings
  Settings settings;
  panel::Style panelStyle;
  MockPanelMenuView panelMenuView;
};

TEST_F(TestPanelMenuView, Escaping)
{
  static const char *escapedText = "Panel d&amp;Inici";
  EXPECT_EQ(panelMenuView._panel_title, "");

  UBusManager ubus;
  ubus.SendMessage(UBUS_LAUNCHER_START_KEY_NAV, NULL);
  ubus.SendMessage(UBUS_LAUNCHER_SELECTION_CHANGED,
                   g_variant_new_string(escapedText));
  ProcessUBusMessages();

  ubus.SendMessage(UBUS_LAUNCHER_END_KEY_NAV, NULL);
  ProcessUBusMessages();

  StandaloneWindowManager *wm = dynamic_cast<StandaloneWindowManager *>(&WindowManager::Default());
  EXPECT_TRUE(wm != nullptr);
  // Change the wm to trick PanelMenuView::RefreshTitle to call GetActiveViewName
  wm->SetScaleActive(true);
  wm->SetScaleActiveForGroup(true);
  panelMenuView.RefreshTitle();

  EXPECT_EQ(panelMenuView._panel_title, "&lt;&gt;&apos;");
}

}
