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
 * Authored by: Andrea Azzarone <azzarone@gmail.com>
 *              Brandon Schaefer <brandon.schaefer@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <config.h>
#include <gmock/gmock.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/DesktopUtilities.h>

#include "ApplicationLauncherIcon.h"
#include "FavoriteStore.h"
#include "mock-application.h"
#include "StandaloneWindowManager.h"

using namespace unity;
using namespace testmocks;
using namespace unity::launcher;

namespace
{
const std::string DEFAULT_EMPTY_ICON = "application-default-icon";
const std::string USC_DESKTOP = BUILDDIR"/tests/data/applications/ubuntu-software-center.desktop";
const std::string NO_ICON_DESKTOP = BUILDDIR"/tests/data/applications/no-icon.desktop";

class TestApplicationLauncherIcon : public testing::Test
{
public:
  virtual void SetUp()
  {
    WM = dynamic_cast<StandaloneWindowManager*>(&WindowManager::Default());
    usc_app.reset(new MockApplication(USC_DESKTOP, "softwarecenter"));
    usc_icon = new launcher::ApplicationLauncherIcon(usc_app);
    ASSERT_EQ(usc_icon->DesktopFile(), USC_DESKTOP);

    empty_app.reset(new MockApplication(NO_ICON_DESKTOP));
    empty_icon = new launcher::ApplicationLauncherIcon(empty_app);
    ASSERT_EQ(empty_icon->DesktopFile(), NO_ICON_DESKTOP);

    mock_app.reset(new MockApplication(""));
    mock_icon = new launcher::ApplicationLauncherIcon(mock_app);
    ASSERT_TRUE(mock_icon->DesktopFile().empty());
  }

  StandaloneWindowManager* WM;
  std::shared_ptr<MockApplication> usc_app;
  std::shared_ptr<MockApplication> empty_app;
  std::shared_ptr<MockApplication> mock_app;
  nux::ObjectPtr<launcher::ApplicationLauncherIcon> usc_icon;
  nux::ObjectPtr<launcher::ApplicationLauncherIcon> empty_icon;
  nux::ObjectPtr<launcher::ApplicationLauncherIcon> mock_icon;
};

TEST_F(TestApplicationLauncherIcon, Position)
{
  EXPECT_EQ(usc_icon->position(), AbstractLauncherIcon::Position::FLOATING);
}

TEST_F(TestApplicationLauncherIcon, TestCustomBackgroundColor)
{
  nux::Color const& color = usc_icon->BackgroundColor();

  EXPECT_EQ(color.red, 0xaa / 255.0f);
  EXPECT_EQ(color.green, 0xbb / 255.0f);
  EXPECT_EQ(color.blue, 0xcc / 255.0f);
  EXPECT_EQ(color.alpha, 0xff / 255.0f);
}

TEST_F(TestApplicationLauncherIcon, TestDefaultIcon)
{
  EXPECT_EQ(usc_icon->icon_name(), "softwarecenter");
  EXPECT_EQ(empty_icon->icon_name(), DEFAULT_EMPTY_ICON);
  EXPECT_EQ(mock_icon->icon_name(), DEFAULT_EMPTY_ICON);
}

TEST_F(TestApplicationLauncherIcon, Stick)
{
  bool saved = false;
  usc_icon->position_saved.connect([&saved] {saved = true;});

  usc_icon->Stick(false);
  EXPECT_TRUE(usc_app->sticky());
  EXPECT_TRUE(usc_icon->IsSticky());
  EXPECT_TRUE(usc_icon->IsVisible());
  EXPECT_FALSE(saved);

  usc_icon->Stick(true);
  EXPECT_FALSE(saved);
}

TEST_F(TestApplicationLauncherIcon, StickAndSave)
{
  bool saved = false;
  usc_icon->position_saved.connect([&saved] {saved = true;});

  usc_icon->Stick(true);
  EXPECT_TRUE(usc_app->sticky());
  EXPECT_TRUE(usc_icon->IsSticky());
  EXPECT_TRUE(usc_icon->IsVisible());
  EXPECT_TRUE(saved);
}

TEST_F(TestApplicationLauncherIcon, Unstick)
{
  bool forgot = false;
  usc_icon->position_forgot.connect([&forgot] {forgot = true;});

  usc_icon->Stick(false);
  usc_icon->UnStick();
  EXPECT_FALSE(usc_app->sticky());
  EXPECT_FALSE(usc_icon->IsSticky());
  EXPECT_FALSE(usc_icon->IsVisible());
  EXPECT_TRUE(forgot);
}

TEST_F(TestApplicationLauncherIcon, RemoteUri)
{
  EXPECT_EQ(usc_icon->RemoteUri(), FavoriteStore::URI_PREFIX_APP + DesktopUtilities::GetDesktopID(USC_DESKTOP));
  EXPECT_TRUE(mock_icon->RemoteUri().empty());
}

TEST_F(TestApplicationLauncherIcon, EmptyTooltipUpdatesOnRunning)
{
  ASSERT_TRUE(mock_icon->tooltip_text().empty());
  mock_app->title_ = "Got Name";
  ASSERT_TRUE(mock_icon->tooltip_text().empty());

  mock_app->SetRunState(true);
  EXPECT_EQ(mock_icon->tooltip_text(), "Got Name");

  mock_app->SetRunState(false);
  mock_app->title_ = "New Name";
  mock_app->SetRunState(true);
  EXPECT_EQ(mock_icon->tooltip_text(), "Got Name");
}

TEST_F(TestApplicationLauncherIcon, InvalidIconUpdatesOnRunning)
{
  ASSERT_EQ(mock_icon->icon_name(), DEFAULT_EMPTY_ICON);
  mock_app->icon_ = "icon-name";

  ASSERT_EQ(mock_icon->icon_name(), DEFAULT_EMPTY_ICON);

  mock_app->SetRunState(true);
  EXPECT_EQ(mock_icon->icon_name(), "icon-name");

  mock_app->SetRunState(false);
  mock_app->icon_ = "new-icon-name";
  mock_app->SetRunState(true);
  EXPECT_EQ(mock_icon->icon_name(), "icon-name");
}

TEST_F(TestApplicationLauncherIcon, ActiveQuirkWMCrossCheck)
{
  auto win = std::make_shared<MockApplicationWindow>(g_random_int());
  mock_app->window_list_ = { win };
  ASSERT_FALSE(mock_icon->IsActive());

  mock_app->SetActiveState(true);
  ASSERT_FALSE(mock_icon->IsActive());

  WM->AddStandaloneWindow(std::make_shared<StandaloneWindow>(win->window_id()));
  EXPECT_TRUE(mock_icon->IsActive());
}

TEST_F(TestApplicationLauncherIcon, NoWindowListMenusWithOneWindow)
{
  auto win = std::make_shared<MockApplicationWindow>(g_random_int());
  mock_app->window_list_ = { win };

  auto const& menus = mock_icon->Menus();
  auto menu_it = std::find_if(menus.begin(), menus.end(), [win] (glib::Object<DbusmenuMenuitem> it) {
    auto* label = dbusmenu_menuitem_property_get(it, DBUSMENU_MENUITEM_PROP_LABEL);
    return (label && std::string(label) == win->title());
  });

  EXPECT_EQ(menu_it, menus.end());
}

TEST_F(TestApplicationLauncherIcon, WindowListMenusWithTwoWindows)
{
  auto win1 = std::make_shared<MockApplicationWindow>(1);
  auto wm_win1 = std::make_shared<StandaloneWindow>(win1->window_id());
  auto win2 = std::make_shared<MockApplicationWindow>(2);
  auto wm_win2 = std::make_shared<StandaloneWindow>(win2->window_id());

  mock_app->window_list_ = { win1, win2 };
  WM->AddStandaloneWindow(wm_win1);
  WM->AddStandaloneWindow(wm_win2);
  ASSERT_TRUE(wm_win2->active());

  auto const& menus = mock_icon->Menus();

  auto menu1_it = std::find_if(menus.begin(), menus.end(), [win1] (glib::Object<DbusmenuMenuitem> it) {
    auto* label = dbusmenu_menuitem_property_get(it, DBUSMENU_MENUITEM_PROP_LABEL);
    return (label && std::string(label) == win1->title());
  });

  ASSERT_NE(menu1_it, menus.end());

  auto menu2_it = std::find_if(menus.begin(), menus.end(), [win2] (glib::Object<DbusmenuMenuitem> it) {
    auto* label = dbusmenu_menuitem_property_get(it, DBUSMENU_MENUITEM_PROP_LABEL);
    return (label && std::string(label) == win2->title());
  });

  ASSERT_NE(menu2_it, menus.end());

  bool activated = false;
  wm_win1->active.changed.connect([&activated] (bool a) { activated = a; });
  g_signal_emit_by_name(*menu1_it, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, 0);

  EXPECT_TRUE(wm_win1->active());
  EXPECT_TRUE(activated);

  activated = false;
  wm_win2->active.changed.connect([&activated] (bool a) { activated = a; });
  g_signal_emit_by_name(*menu2_it, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, 0);

  EXPECT_TRUE(wm_win2->active());
  EXPECT_TRUE(activated);
}

}
