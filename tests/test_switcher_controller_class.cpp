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

#include "test_switcher_controller.h"

using namespace testing;
using namespace unity;
using namespace unity::switcher;
using namespace std::chrono;

FakeApplicationWindow::FakeApplicationWindow(Window xid, uint64_t active_number)
  : xid_(xid)
{
  auto WM = dynamic_cast<StandaloneWindowManager*>(&WindowManager::Default());
  auto standalone_window = std::make_shared<StandaloneWindow>(xid_);
  standalone_window->active_number = active_number;
  WM->AddStandaloneWindow(standalone_window);

  title.SetGetterFunction([this] { return "FakeApplicationWindow"; });
  icon.SetGetterFunction([this] { return ""; });
}

FakeApplicationWindow::~FakeApplicationWindow()
{
  auto WM = dynamic_cast<StandaloneWindowManager*>(&WindowManager::Default());
  WM->Close(xid_);
}

std::string FakeApplicationWindow::type() const { return "mock"; }

Window FakeApplicationWindow::window_id() const { return xid_; }
int FakeApplicationWindow::monitor() const { return -1; }
ApplicationPtr FakeApplicationWindow::application() const { return ApplicationPtr(); }
bool FakeApplicationWindow::Focus() const { return false; }
void FakeApplicationWindow::Quit() const { WindowManager::Default().Close(xid_); }

FakeLauncherIcon::FakeLauncherIcon(std::string const& app_name, bool allow_detail_view, uint64_t priority)
  : launcher::SimpleLauncherIcon(IconType::APPLICATION)
  , allow_detail_view_(allow_detail_view)
  , priority_(priority)
  , window_list{ std::make_shared<FakeApplicationWindow>(priority_ | 0x0001, SwitcherPriority()),
                 std::make_shared<FakeApplicationWindow>(priority_ | 0x0002, priority_) }
{
  tooltip_text = app_name;
}

WindowList FakeLauncherIcon::Windows()
{
  return window_list;
}

bool FakeLauncherIcon::AllowDetailViewInSwitcher() const
{
  return allow_detail_view_;
}

uint64_t FakeLauncherIcon::SwitcherPriority()
{
  return std::numeric_limits<uint64_t>::max() - priority_;
}

/**
 * The base test fixture for verifying the Switcher interface.
 */
//class TestSwitcherController : public testing::Test
TestSwitcherController::TestSwitcherController()
  : WM(dynamic_cast<StandaloneWindowManager*>(&WindowManager::Default()))
  , animation_controller_(tick_source_)
  , mock_window_(new NiceMock<testmocks::MockBaseWindow>())
  , controller_(std::make_shared<Controller>([this] { return mock_window_; }))
{
  controller_->timeout_length = 0;

  icons_.push_back(launcher::AbstractLauncherIcon::Ptr(new launcher::DesktopLauncherIcon()));

  FakeLauncherIcon* first_app = new FakeLauncherIcon("First", true, 0x0100);
  icons_.push_back(launcher::AbstractLauncherIcon::Ptr(first_app));
  FakeLauncherIcon* second_app = new FakeLauncherIcon("Second", true, 0x0200);
  icons_.push_back(launcher::AbstractLauncherIcon::Ptr(second_app));
  FakeLauncherIcon* third_app = new FakeLauncherIcon("Third", false, 0x0300);
  icons_.push_back(launcher::AbstractLauncherIcon::Ptr(third_app));
}

TestSwitcherController::~TestSwitcherController()
{
  WM->ResetStatus();
}