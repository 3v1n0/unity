/*
 * Copyright 2014 Canonical Ltd.
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 *
 */

#include <gmock/gmock.h>
#include <Nux/NuxTimerTickSource.h>
#include <NuxCore/AnimationController.h>

#include "SpreadFilter.h"
#include "UnitySettings.h"
#include "DashStyle.h"
#include "test_utils.h"

namespace unity
{
namespace spread
{
namespace
{
using namespace testing;

const unsigned ANIMATION_DURATION = 100 * 1000; // in microseconds

struct SigReceiver : sigc::trackable
{
  typedef NiceMock<SigReceiver> Nice;

  SigReceiver(Filter const& const_filter)
  {
    auto& filter = const_cast<Filter&>(const_filter);
    filter.text.changed.connect(sigc::mem_fun(this, &SigReceiver::TextChanged));
  }

  MOCK_CONST_METHOD1(TextChanged, void(std::string const&));
};

struct TestSpreadFilter : Test
{
  TestSpreadFilter()
    : animation_controller(tick_source)
    , big_tick_(0)
    , sig_receiver(filter)
  {}

  void Tick()
  {
    big_tick_ += ANIMATION_DURATION;
    tick_source.tick(big_tick_);
  }

  Settings settings_;
  dash::Style style_;
  nux::NuxTimerTickSource tick_source;
  nux::animation::AnimationController animation_controller;
  uint64_t big_tick_;
  Filter filter;
  SigReceiver::Nice sig_receiver;
};

TEST_F(TestSpreadFilter, Construction)
{
  EXPECT_FALSE(filter.Visible());
  EXPECT_TRUE(filter.text().empty());
}

TEST_F(TestSpreadFilter, VisibleWithText)
{
  std::string filter_string = "Unity is cool!";
  EXPECT_CALL(sig_receiver, TextChanged(_)).Times(0);
  filter.text = filter_string;

  EXPECT_CALL(sig_receiver, TextChanged(filter_string));
  Utils::WaitForTimeoutMSec();
  Tick();

  EXPECT_EQ(filter_string, filter.text());
  EXPECT_TRUE(filter.Visible());
}

TEST_F(TestSpreadFilter, InVisibleWithoutText)
{
  filter.text = "Really, Unity is cool!";
  Utils::WaitForTimeoutMSec();
  Tick();

  ASSERT_TRUE(filter.Visible());

  filter.text = "";
  EXPECT_CALL(sig_receiver, TextChanged(""));
  Utils::WaitForTimeoutMSec();
  Tick();

  EXPECT_TRUE(filter.text().empty());
  EXPECT_FALSE(filter.Visible());
}

} // anonymous namespace
} // spread namespace
} // unity namespace
