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
#include "StandaloneWindowManager.h"
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

  void AddMockWindow(Window xid, int monitor, int desktop)
  {
    auto app_window = std::make_shared<MockApplicationWindow>(xid);
    app_window->monitor_ =  monitor;
    mock_app->windows_.push_back(app_window);

    auto standalone_window = std::make_shared<unity::StandaloneWindow>(xid);
    standalone_window->current_desktop = desktop;

    WM->AddStandaloneWindow(standalone_window);
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

TEST_F(TestApplicationLauncherIcon, PerformScrollTowardsTheUser)
{
  AddMockWindow(7, 1, 1);
  AddMockWindow(6, 0, 1);
  AddMockWindow(5, 0, 0);
  AddMockWindow(4, 0, 0);
  AddMockWindow(3, 1, 0);
  AddMockWindow(2, 0, 0);
  AddMockWindow(1, 0, 0);

  mock_icon->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);
  
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 2, 1));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 0);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 1, 2));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 10);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 2, 1, 3));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 20);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 3, 2, 1, 4));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 30);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 4, 3, 2, 1, 5));

  // Make sure it wraps
  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 40);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 2, 1));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 50);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 1, 2));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 60);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 2, 1, 3));

  // Much later...
  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 100000);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 2, 3, 1));
}

TEST_F(TestApplicationLauncherIcon, PerformScrollAwayFromTheUser)
{
  AddMockWindow(7, 1, 1);
  AddMockWindow(6, 0, 1);
  AddMockWindow(5, 0, 0);
  AddMockWindow(4, 0, 0);
  AddMockWindow(3, 1, 0);
  AddMockWindow(2, 0, 0);
  AddMockWindow(1, 0, 0);

  mock_icon->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);

  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 2, 1));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 0);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 4, 3, 2, 1, 5));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 10);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 3, 2, 1, 4));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 20);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 2, 1, 3));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 30);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 1, 2));

  // Make sure it wraps
  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 40);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 2, 1));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 50);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 4, 3, 2, 1, 5));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 60);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 3, 2, 1, 4));

  // Much later...
  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 100000);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 3, 2, 1, 4, 5));
}

TEST_F(TestApplicationLauncherIcon, PerformScrollSwitchDirection)
{
  AddMockWindow(7, 1, 1);
  AddMockWindow(6, 0, 1);
  AddMockWindow(5, 0, 0);
  AddMockWindow(4, 0, 0);
  AddMockWindow(3, 1, 0);
  AddMockWindow(2, 0, 0);
  AddMockWindow(1, 0, 0);
  
  mock_icon->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);

  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 2, 1));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 0);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 1, 2));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 10);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 4, 3, 1, 2, 5));
}

TEST_F(TestApplicationLauncherIcon, PerformScrollNoWindows)
{
  // Just to make sure it does not crash.
  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 0);
  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 10);
}

TEST_F(TestApplicationLauncherIcon, PerformScrollInitiallyUnfocusedWindow)
{
  AddMockWindow(7, 1, 1);
  AddMockWindow(6, 0, 1);
  AddMockWindow(5, 0, 0);
  AddMockWindow(4, 0, 0);
  AddMockWindow(3, 1, 0);
  AddMockWindow(2, 0, 0);
  AddMockWindow(1, 0, 0);
  
  auto external_window = std::make_shared<unity::StandaloneWindow>(8);
  WM->AddStandaloneWindow(external_window);
  mock_icon->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, false);

  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 2, 1, 8));
  ASSERT_EQ(WM->GetActiveWindow(), 8);

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 0);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 2, 8, 1));
  ASSERT_EQ(WM->GetActiveWindow(), 1);
}

TEST_F(TestApplicationLauncherIcon, ActiveQuirkWMCrossCheck)
{
  auto win = std::make_shared<MockApplicationWindow>(g_random_int());
  mock_app->windows_ = { win };
  ASSERT_FALSE(mock_icon->IsActive());

  mock_app->SetActiveState(true);
  ASSERT_FALSE(mock_icon->IsActive());

  WM->AddStandaloneWindow(std::make_shared<StandaloneWindow>(win->window_id()));
  EXPECT_TRUE(mock_icon->IsActive());
}

}
