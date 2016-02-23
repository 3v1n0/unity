/*
 * Copyright 2012,2015 Canonical Ltd.
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
 *              Brandon Schaefer <brandon.schaefer@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <config.h>
#include <gmock/gmock.h>
#include <zeitgeist.h>

#include <UnityCore/GLibWrapper.h>

#include "ApplicationLauncherIcon.h"
#include "FavoriteStore.h"
#include "UBusWrapper.h"
#include "UBusMessages.h"
#include "mock-application.h"
#include "test_utils.h"
#include "test_standalone_wm.h"

using namespace testing;
using namespace testmocks;
using namespace unity;
using namespace unity::launcher;

namespace
{
const std::string DEFAULT_EMPTY_ICON = "application-default-icon";
const std::string GS_DESKTOP = BUILDDIR"/tests/data/applications/org.gnome.Software.desktop";
const std::string UM_DESKTOP = BUILDDIR"/tests/data/applications/update-manager.desktop";
const std::string NO_ICON_DESKTOP = BUILDDIR"/tests/data/applications/no-icon.desktop";

struct MockApplicationLauncherIcon : ApplicationLauncherIcon
{
  typedef nux::ObjectPtr<MockApplicationLauncherIcon> Ptr;

  MockApplicationLauncherIcon(ApplicationPtr const& app)
    : WindowedLauncherIcon(IconType::APPLICATION)
    , ApplicationLauncherIcon(app)
  {
    ON_CALL(*this, Stick(_)).WillByDefault(Invoke([this] (bool save) { ApplicationLauncherIcon::Stick(save); }));
    ON_CALL(*this, Stick()).WillByDefault(Invoke([this] { ApplicationLauncherIcon::Stick(); }));
    ON_CALL(*this, UnStick()).WillByDefault(Invoke([this] { ApplicationLauncherIcon::UnStick(); }));
    ON_CALL(*this, GetRemoteMenus()).WillByDefault(Invoke([] { return glib::Object<DbusmenuMenuitem>(); }));
  }

  MOCK_METHOD1(ActivateLauncherIcon, void(ActionArg));
  MOCK_METHOD1(Stick, void(bool));
  MOCK_METHOD0(Stick, void());
  MOCK_METHOD0(UnStick, void());
  MOCK_CONST_METHOD0(GetRemoteMenus, glib::Object<DbusmenuMenuitem>());

  bool LauncherIconIsSticky() const { return LauncherIcon::IsSticky(); }
  void LocalActivate(ActionArg a) { ApplicationLauncherIcon::ActivateLauncherIcon(a); }

  using ApplicationLauncherIcon::LogUnityEvent;
  using ApplicationLauncherIcon::Remove;
  using ApplicationLauncherIcon::SetApplication;
  using ApplicationLauncherIcon::GetApplication;
  using ApplicationLauncherIcon::PerformScroll;
  using LauncherIcon::BackgroundColor;
  using LauncherIcon::GetRemoteUri;
  using LauncherIcon::AllowDetailViewInSwitcher;
};

MATCHER_P(AreArgsEqual, a, "")
{
  return arg.source == a.source &&
         arg.button == a.button &&
         arg.timestamp == a.timestamp &&
         arg.target == a.target &&
         arg.monitor == a.monitor;
}

struct TestApplicationLauncherIcon : testmocks::TestUnityAppBase
{
  virtual ~TestApplicationLauncherIcon() {}

  virtual void SetUp() override
  {
    usc_app = std::make_shared<MockApplication::Nice>(GS_DESKTOP, "org.gnome.Software");
    usc_icon = new MockApplicationLauncherIcon(usc_app);
    ASSERT_EQ(usc_icon->DesktopFile(), GS_DESKTOP);

    empty_app = std::make_shared<MockApplication::Nice>(NO_ICON_DESKTOP);
    empty_icon = new MockApplicationLauncherIcon(empty_app);
    ASSERT_EQ(empty_icon->DesktopFile(), NO_ICON_DESKTOP);

    mock_app = std::make_shared<MockApplication::Nice>();
    mock_icon = new MockApplicationLauncherIcon(mock_app);
    ASSERT_TRUE(mock_icon->DesktopFile().empty());
  }

  void AddMockWindow(Window xid, int monitor, int desktop)
  {
    auto app_window = std::make_shared<MockApplicationWindow::Nice>(xid);
    app_window->monitor_ =  monitor;
    mock_app->windows_.push_back(app_window);

    auto standalone_window = std::make_shared<unity::StandaloneWindow>(xid);
    standalone_window->current_desktop = desktop;

    WM->AddStandaloneWindow(standalone_window);
  }

  glib::Object<DbusmenuMenuitem> GetMenuItemWithLabel(AbstractLauncherIcon::MenuItemsVector const& menus,
                                                      std::string const& label)
  {
    auto menu_it = std::find_if(menus.begin(), menus.end(), [label] (glib::Object<DbusmenuMenuitem> it) {
      if (glib::gchar_to_string(dbusmenu_menuitem_property_get(it, DBUSMENU_MENUITEM_PROP_TYPE)) == DBUSMENU_CLIENT_TYPES_SEPARATOR)
        return false;

      auto* menu_label = dbusmenu_menuitem_property_get(it, DBUSMENU_MENUITEM_PROP_LABEL);
      return (glib::gchar_to_string(menu_label) == label);
    });

    return (menu_it != menus.end()) ? *menu_it : glib::Object<DbusmenuMenuitem>();
  }

  glib::Object<DbusmenuMenuitem> GetMenuItemWithLabel(MockApplicationLauncherIcon::Ptr const& icon,
                                                      std::string const& label)
  {
    return GetMenuItemWithLabel(icon->Menus(), label);
  }

  bool HasMenuItemWithLabel(AbstractLauncherIcon::MenuItemsVector const& menus,
                            std::string const& label)
  {
    return GetMenuItemWithLabel(menus, label) != nullptr;
  }

  bool HasMenuItemWithLabel(MockApplicationLauncherIcon::Ptr const& icon, std::string const& label)
  {
    return HasMenuItemWithLabel(icon->Menus(), label);
  }

  void VerifySignalsDisconnection(MockApplication::Ptr const& app)
  {
    EXPECT_TRUE(app->closed.empty());
    EXPECT_TRUE(app->window_opened.empty());
    EXPECT_TRUE(app->window_moved.empty());
    EXPECT_TRUE(app->window_closed.empty());
    EXPECT_TRUE(app->visible.changed.empty());
    EXPECT_TRUE(app->active.changed.empty());
    EXPECT_TRUE(app->running.changed.empty());
    EXPECT_TRUE(app->urgent.changed.empty());
    EXPECT_TRUE(app->desktop_file.changed.empty());
    EXPECT_TRUE(app->title.changed.empty());
    EXPECT_TRUE(app->icon.changed.empty());
  }

  testwrapper::StandaloneWM WM;
  MockApplication::Ptr usc_app;
  MockApplication::Ptr empty_app;
  MockApplication::Ptr mock_app;
  MockApplicationLauncherIcon::Ptr usc_icon;
  MockApplicationLauncherIcon::Ptr empty_icon;
  MockApplicationLauncherIcon::Ptr mock_icon;
};

TEST_F(TestApplicationLauncherIcon, ApplicationSignalDisconnection)
{
  std::shared_ptr<MockApplication> app = std::make_shared<MockApplication::Nice>(GS_DESKTOP);
  {
    MockApplicationLauncherIcon::Ptr icon(new MockApplicationLauncherIcon(app));
    EXPECT_FALSE(app->closed.empty());
  }

  VerifySignalsDisconnection(app);
}

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
  EXPECT_EQ(usc_icon->icon_name(), "org.gnome.Software");
  EXPECT_EQ(empty_icon->icon_name(), DEFAULT_EMPTY_ICON);
  EXPECT_EQ(mock_icon->icon_name(), DEFAULT_EMPTY_ICON);
}

TEST_F(TestApplicationLauncherIcon, StickDesktopApp)
{
  bool saved = false;
  usc_icon->position_saved.connect([&saved] {saved = true;});
  EXPECT_CALL(*unity_app_, LogEvent(_, _)).Times(0);

  usc_icon->Stick(false);
  EXPECT_TRUE(usc_app->sticky());
  EXPECT_TRUE(usc_icon->IsSticky());
  EXPECT_TRUE(usc_icon->IsVisible());
  EXPECT_FALSE(saved);

  EXPECT_CALL(*unity_app_, LogEvent(ApplicationEventType::ACCESS, _)).Times(1);
  usc_icon->Stick(true);
  EXPECT_TRUE(saved);
}

TEST_F(TestApplicationLauncherIcon, StickDesktopLessApp)
{
  bool saved = false;
  mock_icon->position_saved.connect([&saved] {saved = true;});
  EXPECT_CALL(*unity_app_, LogEvent(_, _)).Times(0);

  mock_icon->Stick(false);
  EXPECT_TRUE(mock_app->sticky());
  EXPECT_FALSE(mock_icon->IsSticky());
  EXPECT_FALSE(mock_icon->IsVisible());
  EXPECT_FALSE(saved);

  mock_icon->Stick(true);
  EXPECT_FALSE(saved);
}

TEST_F(TestApplicationLauncherIcon, StickAndSaveDesktopApp)
{
  bool saved = false;
  usc_icon->position_saved.connect([&saved] {saved = true;});
  EXPECT_CALL(*unity_app_, LogEvent(ApplicationEventType::ACCESS, _));

  usc_icon->Stick(true);
  EXPECT_TRUE(usc_app->sticky());
  EXPECT_TRUE(usc_icon->IsSticky());
  EXPECT_TRUE(usc_icon->IsVisible());
  EXPECT_TRUE(saved);
}

TEST_F(TestApplicationLauncherIcon, StickAndSaveDesktopLessApp)
{
  bool saved = false;
  mock_icon->position_saved.connect([&saved] {saved = true;});
  EXPECT_CALL(*unity_app_, LogEvent(_, _)).Times(0);

  mock_icon->Stick(true);
  EXPECT_TRUE(mock_app->sticky());
  EXPECT_FALSE(mock_icon->IsSticky());
  EXPECT_FALSE(mock_icon->IsVisible());
  EXPECT_FALSE(saved);
}

TEST_F(TestApplicationLauncherIcon, StickStickedDesktopApp)
{
  auto app = std::make_shared<MockApplication::Nice>(GS_DESKTOP);
  app->sticky = true;
  app->desktop_file_ = UM_DESKTOP;
  MockApplicationLauncherIcon::Ptr icon(new MockApplicationLauncherIcon(app));
  ASSERT_TRUE(icon->IsSticky());
  EXPECT_TRUE(icon->LauncherIconIsSticky());
}

TEST_F(TestApplicationLauncherIcon, StickStickedDesktopLessApp)
{
  auto app = std::make_shared<MockApplication::Nice>();
  app->sticky = true;
  MockApplicationLauncherIcon::Ptr icon(new MockApplicationLauncherIcon(app));
  ASSERT_FALSE(icon->IsSticky());
  EXPECT_FALSE(icon->LauncherIconIsSticky());
}

TEST_F(TestApplicationLauncherIcon, StickAndSaveDesktopAppDontCreateNewDesktop)
{
  EXPECT_CALL(*usc_app, CreateLocalDesktopFile()).Times(0);
  usc_icon->Stick(true);

  EXPECT_TRUE(usc_icon->IsSticky());
}

TEST_F(TestApplicationLauncherIcon, StickAndSaveDesktopLessAppCreatesNewDesktop)
{
  auto app = std::make_shared<MockApplication::Nice>();
  MockApplicationLauncherIcon::Ptr icon(new MockApplicationLauncherIcon(app));

  EXPECT_CALL(*app, CreateLocalDesktopFile());
  icon->Stick(true);

  EXPECT_TRUE(app->sticky());
  EXPECT_FALSE(icon->IsSticky());
}

TEST_F(TestApplicationLauncherIcon, UnstickNotRunning)
{
  usc_app->SetRunState(false);
  usc_app->SetVisibility(true);

  bool forgot = false;
  bool removed = false;
  usc_icon->Stick();
  usc_icon->position_forgot.connect([&forgot] {forgot = true;});
  usc_icon->remove.connect([&removed] (AbstractLauncherIcon::Ptr const&) { removed = true; });

  usc_icon->UnStick();
  EXPECT_FALSE(usc_app->sticky());
  EXPECT_FALSE(usc_icon->IsSticky());
  EXPECT_FALSE(usc_icon->IsVisible());
  EXPECT_TRUE(forgot);
  EXPECT_TRUE(removed);
}

TEST_F(TestApplicationLauncherIcon, UnstickRunning)
{
  usc_app->SetRunState(true);
  usc_app->SetVisibility(true);

  bool forgot = false;
  bool removed = false;
  usc_icon->Stick();
  usc_icon->position_forgot.connect([&forgot] {forgot = true;});
  usc_icon->remove.connect([&removed] (AbstractLauncherIcon::Ptr const&) { removed = true; });

  usc_icon->UnStick();
  EXPECT_FALSE(usc_app->sticky());
  EXPECT_FALSE(usc_icon->IsSticky());
  EXPECT_TRUE(usc_icon->IsVisible());
  EXPECT_TRUE(forgot);
  EXPECT_FALSE(removed);
}

TEST_F(TestApplicationLauncherIcon, UnstickDesktopAppLogEvents)
{
  usc_icon->Stick();

  {
  InSequence order;
  EXPECT_CALL(*unity_app_, LogEvent(ApplicationEventType::ACCESS, _));
  EXPECT_CALL(*unity_app_, LogEvent(ApplicationEventType::LEAVE, _));
  }

  usc_icon->UnStick();
}

TEST_F(TestApplicationLauncherIcon, UnstickDesktopLessAppLogEvent)
{
  auto app = std::make_shared<MockApplication::Nice>();
  MockApplicationLauncherIcon::Ptr icon(new MockApplicationLauncherIcon(app));

  EXPECT_CALL(*unity_app_, LogEvent(ApplicationEventType::ACCESS, _)).Times(0);
  icon->UnStick();
}

TEST_F(TestApplicationLauncherIcon, VisibleChanged)
{
  usc_app->visible_ = true;
  usc_app->visible.changed(usc_app->visible_);
  ASSERT_TRUE(usc_icon->IsVisible());

  usc_app->visible_ = false;
  usc_app->visible.changed(usc_app->visible_);
  EXPECT_FALSE(usc_icon->IsVisible());
}

TEST_F(TestApplicationLauncherIcon, VisibleChangedSticky)
{
  usc_icon->Stick();
  usc_app->visible_ = true;
  usc_app->visible.changed(usc_app->visible_);
  ASSERT_TRUE(usc_icon->IsVisible());

  usc_app->visible_ = false;
  usc_app->visible.changed(usc_app->visible_);
  EXPECT_TRUE(usc_icon->IsVisible());
}

TEST_F(TestApplicationLauncherIcon, UpdateDesktopFile)
{
  usc_app->desktop_file_ = UM_DESKTOP;
  usc_app->desktop_file.changed.emit(usc_app->desktop_file_);

  EXPECT_EQ(UM_DESKTOP, usc_icon->DesktopFile());
}

TEST_F(TestApplicationLauncherIcon, UpdateDesktopFileRemoteUri)
{
  usc_app->desktop_file_ = UM_DESKTOP;
  usc_app->desktop_file.changed.emit(usc_app->desktop_file_);

  EXPECT_EQ(FavoriteStore::URI_PREFIX_APP + UM_DESKTOP, usc_icon->RemoteUri());
}

TEST_F(TestApplicationLauncherIcon, UpdateDesktopStaticQuicklistEmpty)
{
  usc_app->desktop_file_ = NO_ICON_DESKTOP;
  usc_app->desktop_file.changed.emit(usc_app->desktop_file_);

  EXPECT_FALSE(HasMenuItemWithLabel(usc_icon, "Test Action"));
}

TEST_F(TestApplicationLauncherIcon, UpdateDesktopStaticQuicklist)
{
  usc_app->desktop_file_ = UM_DESKTOP;
  usc_app->desktop_file.changed.emit(usc_app->desktop_file_);

  EXPECT_FALSE(HasMenuItemWithLabel(usc_icon, "Test Action"));
  EXPECT_TRUE(HasMenuItemWithLabel(usc_icon, "Update Action"));
}

TEST_F(TestApplicationLauncherIcon, UpdateDesktopReSavesIconPosition)
{
  mock_icon->Stick(true);

  bool saved = false;
  mock_icon->position_saved.connect([&saved] {saved = true;});
  mock_app->desktop_file_ = UM_DESKTOP;
  mock_app->desktop_file.changed.emit(usc_app->desktop_file_);

  EXPECT_TRUE(mock_app->sticky());
  EXPECT_TRUE(mock_icon->IsSticky());
  EXPECT_TRUE(saved);
}

TEST_F(TestApplicationLauncherIcon, UpdateDesktopEmptyForgetsIconPosition)
{
  usc_icon->Stick(true);

  bool forgot = false;
  usc_icon->position_forgot.connect([&forgot] {forgot = true;});
  usc_app->desktop_file_ = "";
  usc_app->desktop_file.changed.emit(usc_app->desktop_file_);

  EXPECT_FALSE(mock_app->sticky());
  EXPECT_FALSE(mock_icon->IsSticky());
  EXPECT_TRUE(forgot);
}

TEST_F(TestApplicationLauncherIcon, UpdateDesktopEmptyForgetsIconPositionAndUpdatesUri)
{
  usc_icon->Stick(true);

  bool forgot = false;
  bool uri_updated = false;
  bool saved = false;

  usc_icon->position_forgot.connect([&forgot, &uri_updated] {
    ASSERT_FALSE(uri_updated);
    forgot = true;
  });
  usc_icon->uri_changed.connect([&forgot, &uri_updated] (std::string const&) {
    ASSERT_TRUE(forgot);
    uri_updated = true;
  });
  usc_icon->position_saved.connect([&saved] { saved = false; });

  usc_app->desktop_file_ = "";
  usc_app->desktop_file.changed.emit(usc_app->desktop_file_);

  EXPECT_FALSE(usc_app->sticky());
  EXPECT_FALSE(usc_icon->IsSticky());
  EXPECT_TRUE(forgot);
  EXPECT_TRUE(uri_updated);
  EXPECT_FALSE(saved);
}

TEST_F(TestApplicationLauncherIcon, UpdateDesktopUpdatesIconUri)
{
  bool updated = false;
  usc_icon->uri_changed.connect([this, &updated] (std::string const& new_uri) {
    updated = true;
    EXPECT_EQ(usc_icon->RemoteUri(), new_uri);
  });

  usc_app->desktop_file_ = "";
  usc_app->desktop_file.changed.emit(usc_app->desktop_file_);
  EXPECT_TRUE(updated);

  updated = false;
  usc_app->desktop_file_ = UM_DESKTOP;
  usc_app->desktop_file.changed.emit(usc_app->desktop_file_);
  EXPECT_TRUE(updated);
}

TEST_F(TestApplicationLauncherIcon, UpdateDesktopDoesntUpdatesIconUri)
{
  bool updated = false;
  usc_icon->uri_changed.connect([&updated] (std::string const&) { updated = true; });
  usc_app->desktop_file.changed.emit(usc_app->desktop_file_);

  EXPECT_FALSE(updated);
}

TEST_F(TestApplicationLauncherIcon, UpdateDesktopForgetsOldPositionUpdatesUriAndSavesAgain)
{
  usc_icon->Stick(true);

  bool forgot = false;
  bool uri_updated = false;
  bool saved = false;
  bool removed = false;

  usc_icon->position_forgot.connect([&] {
    ASSERT_FALSE(uri_updated);
    ASSERT_FALSE(saved);
    forgot = true;
  });
  usc_icon->uri_changed.connect([&] (std::string const&) {
    ASSERT_FALSE(saved);
    ASSERT_TRUE(forgot);
    uri_updated = true;
  });
  usc_icon->position_saved.connect([&] {
    ASSERT_TRUE(forgot);
    ASSERT_TRUE(uri_updated);
    saved = true;
  });
  usc_icon->remove.connect([&removed] (AbstractLauncherIcon::Ptr const&) { removed = true; });

  usc_app->desktop_file_ = UM_DESKTOP;
  usc_app->desktop_file.changed.emit(usc_app->desktop_file_);

  ASSERT_FALSE(removed);
  EXPECT_TRUE(usc_app->sticky());
  EXPECT_TRUE(usc_icon->IsSticky());
  EXPECT_TRUE(forgot);
  EXPECT_TRUE(uri_updated);
  EXPECT_TRUE(saved);
}

TEST_F(TestApplicationLauncherIcon, RemoteUri)
{
  EXPECT_EQ(usc_icon->RemoteUri(), FavoriteStore::URI_PREFIX_APP + GS_DESKTOP);
  EXPECT_TRUE(mock_icon->RemoteUri().empty());
}

TEST_F(TestApplicationLauncherIcon, TooltipUpdates)
{
  ASSERT_TRUE(mock_icon->tooltip_text().empty());
  mock_app->title_ = "Got Name";
  ASSERT_TRUE(mock_icon->tooltip_text().empty());

  mock_app->title.changed.emit(mock_app->title_);
  EXPECT_EQ(mock_icon->tooltip_text(), "Got Name");

  mock_app->SetTitle("New Name");
  EXPECT_EQ(mock_icon->tooltip_text(), "New Name");
}

TEST_F(TestApplicationLauncherIcon, IconUpdates)
{
  ASSERT_EQ(mock_icon->icon_name(), DEFAULT_EMPTY_ICON);
  mock_app->icon_ = "icon-name";

  ASSERT_EQ(mock_icon->icon_name(), DEFAULT_EMPTY_ICON);

  mock_app->icon.changed.emit(mock_app->icon_);
  EXPECT_EQ(mock_icon->icon_name(), "icon-name");

  mock_app->SetIcon("new-icon-name");
  EXPECT_EQ(mock_icon->icon_name(), "new-icon-name");
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

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 200);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 1, 2));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 400);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 2, 1, 3));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 600);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 3, 2, 1, 4));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 800);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 4, 3, 2, 1, 5));

  // Make sure it wraps
  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 1000);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 2, 1));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 1200);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 1, 2));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 1400);
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

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 200);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 4, 3, 2, 1, 5));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 400);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 3, 2, 1, 4));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 600);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 2, 1, 3));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 800);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 1, 2));

  // Make sure it wraps
  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 1000);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 2, 1));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 1200);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 4, 3, 2, 1, 5));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 1400);
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

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 200);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 1, 2));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 400);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 2, 1, 3));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 600);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 1, 2));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 800);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 2, 1));
}

TEST_F(TestApplicationLauncherIcon, PerformScrollNoWindows)
{
  // Just to make sure it does not crash.
  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 200);
  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::UP, 400);
}

TEST_F(TestApplicationLauncherIcon, PerformScrollTooFast)
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

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 200);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 1, 2));

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 205); /* Too fast! */
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 1, 2));
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

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 200);
  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(7, 6, 5, 4, 3, 2, 8, 1));
  ASSERT_EQ(WM->GetActiveWindow(), 1);
}

TEST_F(TestApplicationLauncherIcon, PerformScrollSingleUnfocusedWindow)
{
  AddMockWindow(1, 0, 0);

  auto external_window = std::make_shared<unity::StandaloneWindow>(2);
  WM->AddStandaloneWindow(external_window);
  mock_icon->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, false);

  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(1, 2));
  ASSERT_EQ(WM->GetActiveWindow(), 2);

  mock_icon->PerformScroll(AbstractLauncherIcon::ScrollDirection::DOWN, 200);

  EXPECT_THAT(WM->GetWindowsInStackingOrder(), testing::ElementsAre(2, 1));
  ASSERT_EQ(WM->GetActiveWindow(), 1);
}

TEST_F(TestApplicationLauncherIcon, ActiveQuirkWMCrossCheck)
{
  auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  mock_app->windows_ = { win };
  ASSERT_FALSE(mock_icon->IsActive());

  mock_app->SetActiveState(true);
  ASSERT_FALSE(mock_icon->IsActive());

  WM->AddStandaloneWindow(std::make_shared<StandaloneWindow>(win->window_id()));
  EXPECT_TRUE(mock_icon->IsActive());
}

TEST_F(TestApplicationLauncherIcon, NoWindowListMenusWithOneWindow)
{
  auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  mock_app->windows_ = { win };
  EXPECT_FALSE(HasMenuItemWithLabel(mock_icon, win->title()));
}

TEST_F(TestApplicationLauncherIcon, WindowListMenusWithTwoWindows)
{
  auto win1 = std::make_shared<MockApplicationWindow::Nice>(1);
  auto wm_win1 = std::make_shared<StandaloneWindow>(win1->window_id());
  auto win2 = std::make_shared<MockApplicationWindow::Nice>(2);
  auto wm_win2 = std::make_shared<StandaloneWindow>(win2->window_id());

  mock_app->windows_ = { win1, win2 };
  WM->AddStandaloneWindow(wm_win1);
  WM->AddStandaloneWindow(wm_win2);

  win2->Focus();
  ASSERT_TRUE(wm_win2->active());
  ASSERT_TRUE(win2->active());

  auto const& menus = mock_icon->Menus();
  auto const& menu1 = GetMenuItemWithLabel(menus, win1->title());
  ASSERT_NE(menu1, nullptr);

  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menu1, DBUSMENU_MENUITEM_PROP_ENABLED));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menu1, DBUSMENU_MENUITEM_PROP_VISIBLE));
  ASSERT_STREQ(NULL, dbusmenu_menuitem_property_get(menu1, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE));
  EXPECT_EQ(dbusmenu_menuitem_property_get_int(menu1, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE),
  	DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menu1, QuicklistMenuItem::MARKUP_ACCEL_DISABLED_PROPERTY));
  EXPECT_EQ(dbusmenu_menuitem_property_get_int(menu1, QuicklistMenuItem::MAXIMUM_LABEL_WIDTH_PROPERTY), 300);

  auto const& menu2 = GetMenuItemWithLabel(menus, win2->title());
  ASSERT_NE(menu2, nullptr);

  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menu2, DBUSMENU_MENUITEM_PROP_ENABLED));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menu2, DBUSMENU_MENUITEM_PROP_VISIBLE));
  ASSERT_STREQ(DBUSMENU_MENUITEM_TOGGLE_RADIO, dbusmenu_menuitem_property_get(menu2, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE));
  EXPECT_EQ(dbusmenu_menuitem_property_get_int(menu2, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE),
  	DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED);
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menu2, QuicklistMenuItem::MARKUP_ACCEL_DISABLED_PROPERTY));
  EXPECT_EQ(dbusmenu_menuitem_property_get_int(menu2, QuicklistMenuItem::MAXIMUM_LABEL_WIDTH_PROPERTY), 300);

  bool activated = false;
  wm_win1->active.changed.connect([&activated] (bool a) { activated = a; });
  g_signal_emit_by_name(menu1, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, 0);

  EXPECT_TRUE(wm_win1->active());
  EXPECT_TRUE(activated);

  activated = false;
  wm_win2->active.changed.connect([&activated] (bool a) { activated = a; });
  g_signal_emit_by_name(menu2, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, 0);

  EXPECT_TRUE(wm_win2->active());
  EXPECT_TRUE(activated);
}

TEST_F(TestApplicationLauncherIcon, WindowListMenusWithEmptyTitles)
{
  auto win1 = std::make_shared<MockApplicationWindow::Nice>(1);
  auto win2 = std::make_shared<MockApplicationWindow::Nice>(2);
  win1->SetTitle("");

  mock_app->windows_ = { win1, win2 };

  EXPECT_FALSE(HasMenuItemWithLabel(mock_icon, win1->title()));
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuHasAppName)
{
  mock_app->SetTitle("MockApplicationTitle");
  EXPECT_TRUE(HasMenuItemWithLabel(mock_icon, "<b>"+mock_app->title()+"</b>"));
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuHasLockToLauncher)
{
  usc_app->sticky = false;
  EXPECT_TRUE(HasMenuItemWithLabel(usc_icon, "Lock to Launcher"));
  EXPECT_FALSE(HasMenuItemWithLabel(usc_icon, "Unlock from Launcher"));
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuItemLockToLauncher)
{
  bool saved = false;
  usc_icon->position_saved.connect([&saved] {saved = true;});

  auto const& menu_item = GetMenuItemWithLabel(usc_icon, "Lock to Launcher");
  ASSERT_NE(menu_item, nullptr);

  EXPECT_CALL(*usc_icon, Stick(true));
  dbusmenu_menuitem_handle_event(menu_item, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);

  EXPECT_TRUE(saved);
  EXPECT_TRUE(usc_app->sticky());
  EXPECT_TRUE(usc_icon->IsSticky());
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuHasUnLockToLauncher)
{
  usc_icon->Stick();
  EXPECT_TRUE(HasMenuItemWithLabel(usc_icon, "Unlock from Launcher"));
  EXPECT_FALSE(HasMenuItemWithLabel(usc_icon, "Lock to Launcher"));
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuItemUnLockFromLauncher)
{
  bool forgot = false;
  usc_icon->position_forgot.connect([&forgot] {forgot = true;});
  usc_icon->Stick();

  auto const& menu_item = GetMenuItemWithLabel(usc_icon, "Unlock from Launcher");
  ASSERT_NE(menu_item, nullptr);

  EXPECT_CALL(*usc_icon, UnStick());
  dbusmenu_menuitem_handle_event(menu_item, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);

  EXPECT_TRUE(forgot);
  EXPECT_FALSE(usc_app->sticky());
  EXPECT_FALSE(usc_icon->IsSticky());
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuHasNotQuit)
{
  mock_app->SetRunState(false);
  EXPECT_FALSE(HasMenuItemWithLabel(mock_icon, "Quit"));
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuHasQuit)
{
  mock_app->SetRunState(true);
  EXPECT_TRUE(HasMenuItemWithLabel(mock_icon, "Quit"));
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuItemQuit)
{
  mock_app->SetRunState(true);
  auto const& menu_item = GetMenuItemWithLabel(mock_icon, "Quit");
  ASSERT_NE(menu_item, nullptr);

  EXPECT_CALL(*mock_app, Quit());
  dbusmenu_menuitem_handle_event(menu_item, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuItemUpdatesWithAppName)
{
  mock_app->SetTitle("MockApplicationTitle");
  ASSERT_TRUE(HasMenuItemWithLabel(mock_icon, "<b>"+mock_app->title()+"</b>"));

  mock_app->SetTitle("MockApplicationNewTitle");
  EXPECT_TRUE(HasMenuItemWithLabel(mock_icon, "<b>"+mock_app->title()+"</b>"));
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuItemForAppName)
{
  mock_app->SetTitle("MockApplicationTitle");
  auto app_menu = GetMenuItemWithLabel(mock_icon, "<b>"+mock_app->title_+"</b>");
  ASSERT_NE(app_menu, nullptr);

  bool method_called = false;
  ON_CALL(*mock_icon, ActivateLauncherIcon(_)).WillByDefault(Invoke([&method_called] (ActionArg arg) {
    method_called = true;
  }));

  unsigned time = g_random_int();
  EXPECT_CALL(*mock_icon, ActivateLauncherIcon(AreArgsEqual(ActionArg(ActionArg::Source::LAUNCHER, 0, time))));
  dbusmenu_menuitem_handle_event(app_menu, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, time);

  Utils::WaitUntilMSec(method_called);
  EXPECT_TRUE(method_called);
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuItemDesktopAction)
{
  EXPECT_TRUE(HasMenuItemWithLabel(usc_icon, "Test Action"));
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuItemDesktopActionOverridesQuitNotRunning)
{
  mock_app->SetRunState(false);
  EXPECT_FALSE(HasMenuItemWithLabel(usc_icon, "Quit"));
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuItemDesktopActionOverridesQuitRunning)
{
  usc_app->SetRunState(true);
  auto const& item = GetMenuItemWithLabel(usc_icon, "Quit");
  ASSERT_NE(item, nullptr);
  EXPECT_CALL(*usc_app, Quit()).Times(0);

  dbusmenu_menuitem_handle_event(item, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuItemRemote)
{
  unsigned time = g_random_int();
  glib::Object<DbusmenuMenuitem> root(dbusmenu_menuitem_new());
  glib::Object<DbusmenuMenuitem> item(dbusmenu_menuitem_new());
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, "RemoteLabel");
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
  dbusmenu_menuitem_child_append(root, item);

  bool cb_called = false;
  glib::Signal<void, DbusmenuMenuitem*, unsigned> signal(item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
  [&cb_called, item, time] (DbusmenuMenuitem* menu, unsigned timestamp) {
    cb_called = true;
    EXPECT_EQ(menu, item);
    EXPECT_EQ(timestamp, time);
  });

  ON_CALL(*mock_icon, GetRemoteMenus()).WillByDefault(Invoke([&root] { return root; }));
  EXPECT_CALL(*mock_icon, GetRemoteMenus());
  EXPECT_TRUE(HasMenuItemWithLabel(mock_icon, "RemoteLabel"));

  dbusmenu_menuitem_handle_event(item, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, time);
  EXPECT_TRUE(cb_called);
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuItemRemoteDisablesMarkup)
{
  glib::Object<DbusmenuMenuitem> root(dbusmenu_menuitem_new());
  glib::Object<DbusmenuMenuitem> item(dbusmenu_menuitem_new());
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, "RemoteLabel");
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
  dbusmenu_menuitem_property_set_bool(item, QuicklistMenuItem::MARKUP_ENABLED_PROPERTY, TRUE);
  ASSERT_TRUE(dbusmenu_menuitem_property_get_bool(item, QuicklistMenuItem::MARKUP_ENABLED_PROPERTY));
  dbusmenu_menuitem_child_append(root, item);

  ON_CALL(*mock_icon, GetRemoteMenus()).WillByDefault(Invoke([&root] { return root; }));

  item = GetMenuItemWithLabel(mock_icon, "RemoteLabel");
  ASSERT_NE(item, nullptr);
  EXPECT_FALSE(dbusmenu_menuitem_property_get_bool(item, QuicklistMenuItem::MARKUP_ENABLED_PROPERTY));
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuItemRemoteIgnoresInvisible)
{
  glib::Object<DbusmenuMenuitem> root(dbusmenu_menuitem_new());
  glib::Object<DbusmenuMenuitem> item(dbusmenu_menuitem_new());
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, "InvisibleLabel");
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_VISIBLE, FALSE);
  ASSERT_FALSE(dbusmenu_menuitem_property_get_bool(item, DBUSMENU_MENUITEM_PROP_VISIBLE));
  dbusmenu_menuitem_child_append(root, item);

  ON_CALL(*mock_icon, GetRemoteMenus()).WillByDefault(Invoke([&root] { return root; }));

  EXPECT_FALSE(HasMenuItemWithLabel(mock_icon, "InvisibleLabel"));
}

struct QuitLabel : TestApplicationLauncherIcon, testing::WithParamInterface<std::string>
{
  virtual ~QuitLabel() {}
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

INSTANTIATE_TEST_CASE_P(TestApplicationLauncherIcon, QuitLabel, testing::Values("Quit", "Exit", "Close"));

TEST_P(/*TestApplicationLauncherIcon*/QuitLabel, QuicklistMenuItemRemoteOverridesQuitByLabelNotRunning)
{
  mock_app->SetRunState(false);
  glib::Object<DbusmenuMenuitem> root(dbusmenu_menuitem_new());
  glib::Object<DbusmenuMenuitem> item(dbusmenu_menuitem_new());
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, GetParam().c_str());
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
  dbusmenu_menuitem_child_append(root, item);

  ON_CALL(*mock_icon, GetRemoteMenus()).WillByDefault(Invoke([&root] { return root; }));

  EXPECT_FALSE(HasMenuItemWithLabel(mock_icon, GetParam()));
}

TEST_P(/*TestApplicationLauncherIcon*/QuitLabel, QuicklistMenuItemRemoteOverridesQuitByLabel)
{
  mock_app->SetRunState(true);
  unsigned time = g_random_int();
  glib::Object<DbusmenuMenuitem> root(dbusmenu_menuitem_new());
  glib::Object<DbusmenuMenuitem> item(dbusmenu_menuitem_new());
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, GetParam().c_str());
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
  dbusmenu_menuitem_child_append(root, item);

  bool cb_called = false;
  glib::Signal<void, DbusmenuMenuitem*, unsigned> signal(item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
  [&cb_called, item, time] (DbusmenuMenuitem* menu, unsigned timestamp) {
    cb_called = true;
    EXPECT_EQ(menu, item);
    EXPECT_EQ(timestamp, time);
  });

  ON_CALL(*mock_icon, GetRemoteMenus()).WillByDefault(Invoke([&root] { return root; }));

  if (GetParam() != "Quit")
    ASSERT_FALSE(HasMenuItemWithLabel(mock_icon, GetParam()));

  item = GetMenuItemWithLabel(mock_icon, "Quit");
  ASSERT_NE(item, nullptr);
  EXPECT_CALL(*mock_app, Quit()).Times(0);

  dbusmenu_menuitem_handle_event(item, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, time);
  EXPECT_TRUE(cb_called);
}

#pragma GCC diagnostic pop

TEST_F(TestApplicationLauncherIcon, QuicklistMenuItemRemoteOverridesQuitByPropertyNotRunning)
{
  mock_app->SetRunState(false);
  glib::Object<DbusmenuMenuitem> root(dbusmenu_menuitem_new());
  glib::Object<DbusmenuMenuitem> item(dbusmenu_menuitem_new());
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, "Custom Quit Label");
  dbusmenu_menuitem_property_set_bool(item, QuicklistMenuItem::QUIT_ACTION_PROPERTY, TRUE);
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
  dbusmenu_menuitem_child_append(root, item);

  ON_CALL(*mock_icon, GetRemoteMenus()).WillByDefault(Invoke([&root] { return root; }));

  EXPECT_FALSE(HasMenuItemWithLabel(mock_icon, "Custom Quit Label"));
  EXPECT_FALSE(HasMenuItemWithLabel(mock_icon, "Quit"));
}

TEST_F(TestApplicationLauncherIcon, QuicklistMenuItemRemoteOverridesQuitByProperty)
{
  mock_app->SetRunState(true);
  glib::Object<DbusmenuMenuitem> root(dbusmenu_menuitem_new());
  glib::Object<DbusmenuMenuitem> item(dbusmenu_menuitem_new());
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_LABEL, "Custom Quit Label");
  dbusmenu_menuitem_property_set_bool(item, QuicklistMenuItem::QUIT_ACTION_PROPERTY, TRUE);
  dbusmenu_menuitem_property_set_bool(item, DBUSMENU_MENUITEM_PROP_ENABLED, TRUE);
  dbusmenu_menuitem_child_append(root, item);

  ON_CALL(*mock_icon, GetRemoteMenus()).WillByDefault(Invoke([&root] { return root; }));

  EXPECT_FALSE(HasMenuItemWithLabel(mock_icon, "Custom Quit Label"));
  auto new_item = GetMenuItemWithLabel(mock_icon, "Quit");
  ASSERT_EQ(new_item, item);

  EXPECT_CALL(*mock_app, Quit()).Times(0);
  dbusmenu_menuitem_handle_event(item, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);
}

TEST_F(TestApplicationLauncherIcon, AllowDetailViewInSwitcher)
{
  mock_app->type_ = AppType::NORMAL;
  EXPECT_TRUE(mock_icon->AllowDetailViewInSwitcher());

  mock_app->type_ = AppType::WEBAPP;
  EXPECT_FALSE(mock_icon->AllowDetailViewInSwitcher());
}

TEST_F(TestApplicationLauncherIcon, LogUnityEventDesktopLess)
{
  EXPECT_CALL(*unity_app_, LogEvent(_, _)).Times(0);
  mock_icon->LogUnityEvent(ApplicationEventType::ACCESS);
}

MATCHER_P(ApplicationSubjectEquals, other, "") { return *arg == *other; }

TEST_F(TestApplicationLauncherIcon, LogUnityEventDesktop)
{
  auto subject = std::make_shared<testmocks::MockApplicationSubject>();
  subject->uri = usc_icon->RemoteUri();
  subject->current_uri = subject->uri();
  subject->text = usc_icon->tooltip_text();
  subject->interpretation = ZEITGEIST_NFO_SOFTWARE;
  subject->manifestation = ZEITGEIST_NFO_SOFTWARE_ITEM;
  subject->mimetype = "application/x-desktop";

  EXPECT_CALL(*unity_app_, LogEvent(ApplicationEventType::ACCESS, ApplicationSubjectEquals(subject)));
  usc_icon->LogUnityEvent(ApplicationEventType::ACCESS);
}

TEST_F(TestApplicationLauncherIcon, RemoveLogEvent)
{
  EXPECT_CALL(*unity_app_, LogEvent(ApplicationEventType::LEAVE, _));
  usc_icon->Remove();
}

TEST_F(TestApplicationLauncherIcon, ClosesOverlayOnActivation)
{
  bool closes_overlay = false;
  UBusManager manager;
  manager.RegisterInterest(UBUS_OVERLAY_CLOSE_REQUEST, [&closes_overlay] (GVariant* v) {
    ASSERT_THAT(v, IsNull());
    closes_overlay = true;
  });

  // XXX: we should abstract the application activation into application::Manager
  ON_CALL(*mock_icon, ActivateLauncherIcon(_)).WillByDefault(Invoke([this] (ActionArg a) { mock_icon->LocalActivate(a); }));
  mock_icon->Activate(ActionArg());
  Utils::WaitUntil(closes_overlay);

  EXPECT_TRUE(closes_overlay);
}

TEST_F(TestApplicationLauncherIcon, RemovedPropertyOnRemove)
{
  ASSERT_FALSE(mock_icon->removed);
  mock_icon->Remove();
  EXPECT_TRUE(mock_icon->removed);
}

TEST_F(TestApplicationLauncherIcon, RemoveDisconnectsSignals)
{
  ASSERT_FALSE(mock_app->closed.empty());
  mock_icon->Remove();
  VerifySignalsDisconnection(mock_app);
}

TEST_F(TestApplicationLauncherIcon, RemoveUnsetsAppParameters)
{
  usc_icon->Stick();
  ASSERT_TRUE(usc_app->seen);
  ASSERT_TRUE(usc_app->sticky);

  usc_icon->Remove();
  EXPECT_FALSE(usc_app->seen);
  EXPECT_FALSE(usc_app->sticky);
}

TEST_F(TestApplicationLauncherIcon, DestructionUnsetsAppParameters)
{
  usc_icon->Stick();
  ASSERT_TRUE(usc_app->seen);
  ASSERT_TRUE(usc_app->sticky);

  usc_icon = nullptr;
  EXPECT_FALSE(usc_app->seen);
  EXPECT_FALSE(usc_app->sticky);
}

TEST_F(TestApplicationLauncherIcon, DestructionDontUnsetsAppSeenIfRemoved)
{
  ASSERT_TRUE(mock_app->seen);

  mock_icon->Remove();
  ASSERT_FALSE(mock_app->seen);

  mock_app->seen = true;
  mock_icon = nullptr;

  EXPECT_TRUE(mock_app->seen);
}

TEST_F(TestApplicationLauncherIcon, DestructionDontUnsetsAppSeenIfReplaced)
{
  ASSERT_TRUE(mock_app->seen);

  mock_icon->Remove();
  ASSERT_FALSE(mock_app->seen);

  MockApplicationLauncherIcon::Ptr new_icon(new MockApplicationLauncherIcon(mock_app));
  mock_icon = nullptr;

  EXPECT_TRUE(mock_app->seen);
}

TEST_F(TestApplicationLauncherIcon, SetApplicationEqual)
{
  ASSERT_TRUE(usc_app->seen);
  usc_icon->SetApplication(usc_app);
  EXPECT_EQ(usc_app, usc_icon->GetApplication());
  EXPECT_TRUE(usc_app->seen);
  EXPECT_FALSE(usc_icon->removed);
}

TEST_F(TestApplicationLauncherIcon, SetApplicationNull)
{
  usc_icon->Stick();
  ASSERT_TRUE(usc_app->seen);
  ASSERT_TRUE(usc_app->sticky);

  usc_icon->SetApplication(nullptr);

  EXPECT_EQ(usc_app, usc_icon->GetApplication());
  EXPECT_FALSE(usc_app->seen);
  EXPECT_FALSE(usc_app->sticky);
  EXPECT_TRUE(usc_icon->removed);
  VerifySignalsDisconnection(usc_app);
}

TEST_F(TestApplicationLauncherIcon, SetApplicationNewUpdatesApp)
{
  usc_icon->Stick();
  ASSERT_TRUE(usc_app->seen);
  ASSERT_TRUE(usc_app->sticky);
  ASSERT_TRUE(usc_icon->IsSticky());

  auto new_app = std::make_shared<MockApplication::Nice>(UM_DESKTOP);
  ASSERT_FALSE(new_app->seen);
  ASSERT_FALSE(new_app->sticky);

  usc_icon->SetApplication(new_app);

  EXPECT_EQ(new_app, usc_icon->GetApplication());
  EXPECT_FALSE(usc_app->seen);
  EXPECT_FALSE(usc_app->sticky);
  EXPECT_FALSE(usc_icon->removed);
  VerifySignalsDisconnection(usc_app);

  EXPECT_TRUE(new_app->seen);
  EXPECT_TRUE(new_app->sticky);
  EXPECT_TRUE(usc_icon->IsSticky());
}

TEST_F(TestApplicationLauncherIcon, SetApplicationNewUpdatesFlags)
{
  auto new_app = std::make_shared<MockApplication::Nice>(UM_DESKTOP, "um_icon", "um_title");
  new_app->active_ = true;
  new_app->visible_ = true;
  new_app->running_ = true;

  // This is needed to really make the new_app active
  auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  new_app->windows_ = { win };
  WM->AddStandaloneWindow(std::make_shared<StandaloneWindow>(win->window_id()));

  ASSERT_NE(usc_app->title(), new_app->title());
  ASSERT_NE(usc_app->icon(), new_app->icon());
  ASSERT_NE(usc_app->visible(), new_app->visible());
  ASSERT_NE(usc_app->active(), new_app->active());
  ASSERT_NE(usc_app->running(), new_app->running());
  ASSERT_NE(usc_app->desktop_file(), new_app->desktop_file());

  usc_icon->SetApplication(new_app);

  EXPECT_EQ(new_app->title(), usc_icon->tooltip_text());
  EXPECT_EQ(new_app->icon(), usc_icon->icon_name());
  EXPECT_EQ(new_app->visible(), usc_icon->IsVisible());
  EXPECT_EQ(new_app->active(), usc_icon->IsActive());
  EXPECT_EQ(new_app->running(), usc_icon->IsRunning());
  EXPECT_EQ(new_app->desktop_file(), usc_icon->DesktopFile());
}

TEST_F(TestApplicationLauncherIcon, SetApplicationEmitsChangedSignalsInOrder)
{
  auto new_app = std::make_shared<MockApplication::Nice>(UM_DESKTOP, "um_icon", "um_title");
  new_app->active_ = true;
  new_app->visible_ = true;
  new_app->running_ = true;

  struct SignalsHandler
  {
    MOCK_METHOD1(TitleUpdated, void(std::string const&));
    MOCK_METHOD1(IconUpdated, void(std::string const&));
    MOCK_METHOD1(DesktopUpdated, void(std::string const&));
    MOCK_METHOD1(ActiveUpdated, void(bool));
    MOCK_METHOD1(VisibleUpdated, void(bool));
    MOCK_METHOD1(RunningUpdated, void(bool));
  };

  SignalsHandler handler;
  new_app->title.changed.connect(sigc::mem_fun(handler, &SignalsHandler::TitleUpdated));
  new_app->icon.changed.connect(sigc::mem_fun(handler, &SignalsHandler::IconUpdated));
  new_app->visible.changed.connect(sigc::mem_fun(handler, &SignalsHandler::VisibleUpdated));
  new_app->active.changed.connect(sigc::mem_fun(handler, &SignalsHandler::ActiveUpdated));
  new_app->running.changed.connect(sigc::mem_fun(handler, &SignalsHandler::RunningUpdated));
  new_app->desktop_file.changed.connect(sigc::mem_fun(handler, &SignalsHandler::DesktopUpdated));

  // The desktop file must be updated as last item!
  ExpectationSet unordered_calls;
  unordered_calls += EXPECT_CALL(handler, TitleUpdated(new_app->title()));
  unordered_calls += EXPECT_CALL(handler, IconUpdated(new_app->icon()));
  unordered_calls += EXPECT_CALL(handler, VisibleUpdated(new_app->visible()));
  unordered_calls += EXPECT_CALL(handler, ActiveUpdated(new_app->active()));
  unordered_calls += EXPECT_CALL(handler, RunningUpdated(new_app->running()));
  EXPECT_CALL(handler, DesktopUpdated(new_app->desktop_file())).After(unordered_calls);

  usc_icon->SetApplication(new_app);
}

} // anonymous namespace
