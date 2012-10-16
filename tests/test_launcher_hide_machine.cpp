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

#include "launcher/LauncherHideMachine.h"
#include "test_utils.h"
namespace ul = unity::launcher;

namespace {

ul::LauncherHideMachine::HideQuirk QUIRKS [] {
    ul::LauncherHideMachine::QUICKLIST_OPEN,
    ul::LauncherHideMachine::EXTERNAL_DND_ACTIVE,
    ul::LauncherHideMachine::INTERNAL_DND_ACTIVE,
    ul::LauncherHideMachine::TRIGGER_BUTTON_SHOW,
    ul::LauncherHideMachine::DND_PUSHED_OFF,
    ul::LauncherHideMachine::MOUSE_MOVE_POST_REVEAL,
    ul::LauncherHideMachine::VERTICAL_SLIDE_ACTIVE,
    ul::LauncherHideMachine::KEY_NAV_ACTIVE,
    ul::LauncherHideMachine::PLACES_VISIBLE,
    ul::LauncherHideMachine::SCALE_ACTIVE,
    ul::LauncherHideMachine::EXPO_ACTIVE,
    ul::LauncherHideMachine::MT_DRAG_OUT,
    ul::LauncherHideMachine::REVEAL_PRESSURE_PASS,
    ul::LauncherHideMachine::LAUNCHER_PULSE,
    ul::LauncherHideMachine::LOCK_HIDE,
    ul::LauncherHideMachine::SHORTCUT_KEYS_VISIBLE };

struct HideModeNever : public TestWithParam<std::tr1::tuple<ul::LauncherHideMachine::HideQuirk, bool, bool>> {
  ul::LauncherHideMachine machine;
};

TEST_P(HideModeNever, Bool2Bool) {
  auto quirk = std::tr1::get<0>(GetParam());
  bool initial_value = std::tr1::get<1>(GetParam());
  bool final_value = std::tr1::get<2>(GetParam());

  machine.SetMode(ul::LauncherHideMachine::HIDE_NEVER);
  machine.SetQuirk(quirk, initial_value);

  bool sig_received = false;
  machine.should_hide_changed.connect([&sig_received] (bool /*value*/) {
    sig_received = true;
  });

  machine.SetQuirk(quirk, final_value);

  auto check_function = [&sig_received]() { return sig_received; };
  Utils::WaitUntil(check_function, false, 20/1000);
}

INSTANTIATE_TEST_CASE_P(TestLauncherHideMachine, HideModeNever,
    Combine(ValuesIn(QUIRKS), Bool(), Bool()));

// TODO: write tests for HideModeAutohide.

}
