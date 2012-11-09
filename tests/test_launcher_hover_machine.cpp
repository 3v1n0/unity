// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include <gtest/gtest.h>
using namespace testing;

#include "launcher/LauncherHoverMachine.h"
#include "test_utils.h"

namespace {

unity::LauncherHoverMachine::HoverQuirk QUIRKS [] {
  unity::LauncherHoverMachine::MOUSE_OVER_LAUNCHER,
  unity::LauncherHoverMachine::MOUSE_OVER_BFB,
  unity::LauncherHoverMachine::QUICKLIST_OPEN,
  unity::LauncherHoverMachine::KEY_NAV_ACTIVE,
  unity::LauncherHoverMachine::LAUNCHER_IN_ACTION };

struct SingleQuirk : public TestWithParam<std::tr1::tuple<unity::LauncherHoverMachine::HoverQuirk, bool, bool>> {
  unity::LauncherHoverMachine machine;
};

TEST_P(SingleQuirk, Bool2Bool) {
  auto quirk = std::tr1::get<0>(GetParam());
  bool initial_value = std::tr1::get<1>(GetParam());
  bool final_value = std::tr1::get<2>(GetParam());

  machine.SetQuirk(quirk, initial_value);
  Utils::WaitForTimeoutMSec(20); // ignore the first signal

  bool sig_received = false;
  bool hover = initial_value;
  machine.should_hover_changed.connect([&sig_received, &hover] (bool value) {
    sig_received = true;
    hover = value;
  });

  machine.SetQuirk(unity::LauncherHoverMachine::LAUNCHER_HIDDEN, false);
  machine.SetQuirk(quirk, final_value);

  if (initial_value != final_value)
  {
    Utils::WaitUntil(sig_received);
    ASSERT_EQ(hover, final_value);
  }
  else
  {
    Utils::WaitForTimeoutMSec(20);
    ASSERT_FALSE(sig_received);
  }
}

INSTANTIATE_TEST_CASE_P(TestLauncherHoverMachine, SingleQuirk,
    Combine(ValuesIn(QUIRKS), Bool(), Bool()));


struct MultipleQuirks : public TestWithParam<std::tr1::tuple<unity::LauncherHoverMachine::HoverQuirk, bool, bool,
                                                             unity::LauncherHoverMachine::HoverQuirk, bool, bool>> {
  unity::LauncherHoverMachine machine;
};

TEST_P(MultipleQuirks, DoubleBool2Bool) {
  auto quirk1 = std::tr1::get<0>(GetParam());
  auto quirk2 = std::tr1::get<3>(GetParam());

  if (quirk1 == quirk2)
    return;

  bool initial_value1 = std::tr1::get<1>(GetParam());
  bool final_value1 = std::tr1::get<2>(GetParam());
  bool initial_value2 = std::tr1::get<4>(GetParam());
  bool final_value2 = std::tr1::get<5>(GetParam());

  machine.SetQuirk(quirk1, initial_value1);
  machine.SetQuirk(quirk2, initial_value2);
  Utils::WaitForTimeoutMSec(20);

  bool sig_received = false;
  bool hover = initial_value1 || initial_value2;
  machine.should_hover_changed.connect([&sig_received, &hover] (bool value) {
    sig_received = true;
    hover = value;
  });

  machine.SetQuirk(unity::LauncherHoverMachine::LAUNCHER_HIDDEN, false);
  machine.SetQuirk(quirk1, final_value1);
  machine.SetQuirk(quirk2, final_value2);

  if ((initial_value1 || initial_value2) != (final_value1 || final_value2))
    Utils::WaitUntil(sig_received);

  auto check_function = [&]() { return hover == (final_value1 || final_value2); };
  Utils::WaitUntil(check_function, true, 20/1000);
}

INSTANTIATE_TEST_CASE_P(TestLauncherHoverMachine, MultipleQuirks,
    Combine(ValuesIn(QUIRKS), Bool(), Bool(), ValuesIn(QUIRKS), Bool(), Bool()));

}
