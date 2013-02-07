/*
 * Copyright 2012-2013 Canonical Ltd.
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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 *
 */

#include <NuxCore/AnimationController.h>
#include <gmock/gmock.h>
#include <chrono>

#include "test_utils.h"
#include "DesktopLauncherIcon.h"
#include "SimpleLauncherIcon.h"
#include "SwitcherController.h"
#include "TimeUtil.h"
#include "unity-shared/UnitySettings.h"
#include "mock-base-window.h"

using namespace testing;
using namespace unity;
using namespace unity::switcher;
using namespace std::chrono;

namespace
{
typedef std::chrono::high_resolution_clock Clock;

#ifdef ENABLE_DELAYED_TWO_PHASE_CONSTRUCTION_TESTS
unsigned int DEFAULT_LAZY_CONSTRUCT_TIMEOUT = 20;
#endif

const unsigned FADE_DURATION = 80 * 1000; // in microseconds
const unsigned TICK_DURATION = 10 * 1000;


/**
 * A fake ApplicationWindow for verifying selection of the switcher.
 */
class FakeApplicationWindow : public ApplicationWindow
{
public:
  FakeApplicationWindow(Window xid) : xid_(xid) {}

  std::string title() const { return "FakeApplicationWindow"; }
  virtual std::string icon() const { return ""; }
  virtual std::string type() const { return "mock"; }

  virtual Window window_id() const { return xid_; }
  virtual int monitor() const { return -1; }
  virtual ApplicationPtr application() const { return ApplicationPtr(); }
  virtual bool Focus() const { return false; }
  virtual void Quit() const {}
private:
  Window xid_;
};

/**
 * A fake LauncherIcon for verifying selection operations of the switcher.
 */
class FakeLauncherIcon : public launcher::SimpleLauncherIcon
{
public:
  FakeLauncherIcon(std::string const& app_name, unsigned priority)
    : launcher::SimpleLauncherIcon(IconType::APPLICATION)
    , priority_(priority)
    , window_list{ std::make_shared<FakeApplicationWindow>(priority_ | 0x0001),
                   std::make_shared<FakeApplicationWindow>(priority_ | 0x0002) }
  { tooltip_text = app_name; }

  WindowList Windows()
  { return window_list; }

  unsigned long long SwitcherPriority()
  { return 0xffffffff - priority_; }

private:
  unsigned   priority_;
  WindowList window_list;
};


/**
 * The base test fixture for verifying the Switcher interface.
 */
class TestSwitcherController : public testing::Test
{
protected:
  TestSwitcherController()
    : animation_controller_(tick_source_)
    , mock_window_(new NiceMock<testmocks::MockBaseWindow>())
  {
    ON_CALL(*mock_window_, SetOpacity(_))
      .WillByDefault(Invoke(mock_window_.GetPointer(),
                     &testmocks::MockBaseWindow::RealSetOpacity));

    auto create_window = [this] { return mock_window_; };
    controller_.reset(new Controller(create_window));
    controller_->timeout_length = 0;

    icons_.push_back(launcher::AbstractLauncherIcon::Ptr(new launcher::DesktopLauncherIcon()));

    FakeLauncherIcon* first_app = new FakeLauncherIcon("First", 0x0100);
    icons_.push_back(launcher::AbstractLauncherIcon::Ptr(first_app));
    FakeLauncherIcon* second_app = new FakeLauncherIcon("Second", 0x0200);
    icons_.push_back(launcher::AbstractLauncherIcon::Ptr(second_app));
  }

  // required to create hidden secret global variables before test objects
  Settings unity_settings_;

  nux::animation::TickSource tick_source_;
  nux::animation::AnimationController animation_controller_;
  testmocks::MockBaseWindow::Ptr mock_window_;
  Controller::Ptr controller_;
  std::vector<unity::launcher::AbstractLauncherIcon::Ptr> icons_;
};


#ifdef ENABLE_DELAYED_TWO_PHASE_CONSTRUCTION_TESTS
TEST_F(TestSwitcherController, LazyConstructionTimeoutLength)
{
  EXPECT_EQ(controller_->GetConstructTimeout(), DEFAULT_LAZY_CONSTRUCT_TIMEOUT);
}

TEST_F(TestSwitcherController, LazyWindowConstruction)
{
  // Setting the timeout to a lower value to speed-up the test
  SwitcherController controller(2);

  EXPECT_EQ(controller.GetConstructTimeout(), 2);

  Utils::WaitForTimeout(controller.GetConstructTimeout()/2);
  ASSERT_FALSE(controller->window_constructed_);

  Utils::WaitUntil(controller.window_constructed_, controller.GetConstructTimeout() + 1);
  EXPECT_TRUE(controller.window_constructed_);
}
#endif

TEST_F(TestSwitcherController, InitialDetailTimeout)
{
  Clock::time_point start_time = Clock::now();
  static const int initial_details_timeout = 500;
  static const int details_timeout = 10 * initial_details_timeout;

  controller_->detail_on_timeout = true;
  controller_->initial_detail_timeout_length = initial_details_timeout;
  controller_->detail_timeout_length = details_timeout;

  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);
  Selection selection = controller_->GetCurrentSelection();
  EXPECT_EQ(selection.application_->tooltip_text(), "Second");
  EXPECT_EQ(selection.window_, 0);

  Utils::WaitForTimeoutMSec(initial_details_timeout * 1.1);
  selection = controller_->GetCurrentSelection();
  EXPECT_EQ(selection.application_->tooltip_text(), "Second");
  EXPECT_EQ(selection.window_, 0x0201);

  auto elapsed_time = Clock::now() - start_time;
  auto time_diff = duration_cast<milliseconds>(elapsed_time).count();
  EXPECT_TRUE(initial_details_timeout < time_diff);
  EXPECT_TRUE(time_diff < details_timeout);
}

TEST_F(TestSwitcherController, DetailTimeoutRemoval)
{
  Clock::time_point start_time = Clock::now();
  static const int details_timeout = 500;
  static const int initial_details_timeout = 10 * details_timeout;

  controller_->detail_on_timeout = true;
  controller_->detail_timeout_length = details_timeout;
  controller_->initial_detail_timeout_length = initial_details_timeout;

  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);
  Selection selection = controller_->GetCurrentSelection();
  EXPECT_EQ(selection.application_->tooltip_text(), "Second");
  EXPECT_EQ(selection.window_, 0);

  controller_->Next();
  selection = controller_->GetCurrentSelection();
  EXPECT_EQ(selection.application_->tooltip_text(), "Show Desktop");
  EXPECT_EQ(selection.window_, 0);

  controller_->Next();
  selection = controller_->GetCurrentSelection();
  EXPECT_EQ(selection.application_->tooltip_text(), "First");
  EXPECT_EQ(selection.window_, 0);

  Utils::WaitForTimeoutMSec(details_timeout * 1.1);
  selection = controller_->GetCurrentSelection();
  EXPECT_EQ(selection.application_->tooltip_text(), "First");
  EXPECT_EQ(selection.window_, 0x0101);


  auto elapsed_time = Clock::now() - start_time;
  auto time_diff = duration_cast<milliseconds>(elapsed_time).count();
  EXPECT_TRUE(details_timeout < time_diff);
  EXPECT_TRUE(time_diff < initial_details_timeout);
}

TEST_F(TestSwitcherController, DetailTimeoutOnDetailActivate)
{
  static const int initial_details_timeout = 500;
  static const int details_timeout = 10 * initial_details_timeout;

  controller_->detail_on_timeout = true;
  controller_->initial_detail_timeout_length = initial_details_timeout;
  controller_->detail_timeout_length = details_timeout;

  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);
  EXPECT_EQ(controller_->GetCurrentSelection().window_, 0);

  // Manually open-close the detail mode before that the timeout has occurred
  controller_->SetDetail(true);
  controller_->SetDetail(false);

  Utils::WaitForTimeoutMSec(initial_details_timeout * 1.1);
  EXPECT_EQ(controller_->GetCurrentSelection().window_, 0);
}

TEST_F(TestSwitcherController, ShowSwitcher)
{
  EXPECT_FALSE(controller_->Visible());
  EXPECT_CALL(*mock_window_, ShowWindow(true, _)).Times(AtLeast(1));

  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);
  Utils::WaitForTimeoutMSec(200);
  EXPECT_TRUE(controller_->Visible());
  EXPECT_TRUE(controller_->StartIndex() == 1);
}

TEST_F(TestSwitcherController, ShowSwitcherNoShowDeskop)
{
  EXPECT_CALL(*mock_window_, ShowWindow(true, _)).Times(AtLeast(1));

  controller_->SetShowDesktopDisabled(true);
  ASSERT_TRUE(controller_->IsShowDesktopDisabled());

  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);
  Utils::WaitForTimeoutMSec(200);
  ASSERT_TRUE(controller_->StartIndex() == 0);
  Selection selection = controller_->GetCurrentSelection();
  EXPECT_NE(selection.application_->tooltip_text(), "Show Desktop");

  controller_->Next();
  selection = controller_->GetCurrentSelection();
  EXPECT_NE(selection.application_->tooltip_text(), "Show Desktop");
}

TEST_F(TestSwitcherController, ShowSwitcherNoResults)
{
  controller_->SetShowDesktopDisabled(true);
  std::vector<unity::launcher::AbstractLauncherIcon::Ptr> results;
  EXPECT_CALL(*mock_window_, ShowWindow(true, _)).Times(0);

  controller_->Show(ShowMode::CURRENT_VIEWPORT, SortMode::FOCUS_ORDER, results);
  Utils::WaitForTimeoutMSec(200);
  ASSERT_FALSE(controller_->Visible());
  Selection selection = controller_->GetCurrentSelection();
  EXPECT_FALSE(selection.application_);
}

TEST_F(TestSwitcherController, ShowHideSwitcherFading)
{
  long long global_tick = 0, t;

  EXPECT_CALL(*mock_window_, ShowWindow(true, _)).Times(1);
  {
    InSequence showing;
    EXPECT_CALL(*mock_window_, SetOpacity(Eq(0.0f))).Times(AtLeast(1));

    EXPECT_CALL(*mock_window_, SetOpacity(AllOf(Gt(0.0f), Lt(1.0f))))
      .Times(AtLeast(FADE_DURATION/TICK_DURATION-1));

    EXPECT_CALL(*mock_window_, SetOpacity(Eq(1.0f))).Times(AtLeast(1));
  }

  controller_->Show(ShowMode::ALL, SortMode::LAUNCHER_ORDER, icons_);
  ASSERT_EQ(mock_window_->GetOpacity(), 0.0f);

  for (t = global_tick; t < global_tick + FADE_DURATION+1; t += TICK_DURATION)
    tick_source_.tick(t);
  global_tick += t;

  ASSERT_EQ(mock_window_->GetOpacity(), 1.0);
  Mock::VerifyAndClearExpectations(mock_window_.GetPointer());

  {
    InSequence hiding;
    EXPECT_CALL(*mock_window_, SetOpacity(Eq(1.0f))).Times(AtLeast(1));

    EXPECT_CALL(*mock_window_, SetOpacity(AllOf(Lt(1.0f), Gt(0.0f))))
      .Times(AtLeast(FADE_DURATION/TICK_DURATION-1));

    EXPECT_CALL(*mock_window_, SetOpacity(Eq(0.0f))).Times(AtLeast(1));
    EXPECT_CALL(*mock_window_, ShowWindow(false, _)).Times(1);
  }

  controller_->Hide(false);
  ASSERT_EQ(mock_window_->GetOpacity(), 1.0);

  for (t = global_tick; t < global_tick + FADE_DURATION+1; t += TICK_DURATION)
    tick_source_.tick(t);
  global_tick += t;

  EXPECT_EQ(mock_window_->GetOpacity(), 0.0f);
  Mock::VerifyAndClearExpectations(mock_window_.GetPointer());
}

}
