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
  ASSERT_EQ(selection.application_->tooltip_text(), "Third");
  EXPECT_EQ(selection.window_, 0);

  controller_->Next();
  selection = controller_->GetCurrentSelection();
  ASSERT_EQ(selection.application_->tooltip_text(), "Show Desktop");
  EXPECT_EQ(selection.window_, 0);

  controller_->Next();
  selection = controller_->GetCurrentSelection();
  ASSERT_EQ(selection.application_->tooltip_text(), "First");
  EXPECT_EQ(selection.window_, 0);

  Utils::WaitForTimeoutMSec(details_timeout * 1.1);
  selection = controller_->GetCurrentSelection();
  ASSERT_EQ(selection.application_->tooltip_text(), "First");
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
