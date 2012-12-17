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
 * Authored by: Marco Trevisan (Treviño) <marco@ubuntu.com>
 *
 */

#include <gmock/gmock.h>

#include <Nux/Nux.h>
#include "LauncherOptions.h"
#include "test_utils.h"

namespace unity
{
namespace
{

TEST(TestLauncherOptions, ChangingOneValueEmitsOnlyOneGlobalChange)
{
  launcher::Options options;
  unsigned changes = 0;
  options.option_changed.connect([&changes] {++changes;});

  options.hide_mode.changed.emit(decltype(options.hide_mode())());
  options.launch_animation.changed.emit(decltype(options.launch_animation())());
  options.urgent_animation.changed.emit(decltype(options.urgent_animation())());
  options.auto_hide_animation.changed.emit(decltype(options.auto_hide_animation())());
  options.backlight_mode.changed.emit(decltype(options.backlight_mode())());
  options.reveal_trigger.changed.emit(decltype(options.reveal_trigger())());
  options.icon_size.changed.emit(decltype(options.icon_size())());
  options.tile_size.changed.emit(decltype(options.tile_size())());
  options.background_alpha.changed.emit(decltype(options.background_alpha())());
  options.edge_decay_rate.changed.emit(decltype(options.edge_decay_rate())());
  options.edge_overcome_pressure.changed.emit(decltype(options.edge_overcome_pressure())());
  options.edge_stop_velocity.changed.emit(decltype(options.edge_stop_velocity())());
  options.edge_reveal_pressure.changed.emit(decltype(options.edge_reveal_pressure())());
  options.edge_responsiveness.changed.emit(decltype(options.edge_responsiveness())());
  options.edge_passed_disabled_ms.changed.emit(decltype(options.edge_passed_disabled_ms())());
  options.edge_resist.changed.emit(decltype(options.edge_resist())());
  options.show_for_all.changed.emit(decltype(options.show_for_all())());

  Utils::WaitUntilMSec([&changes] {return changes == 1;}, true, 1000);
  Utils::WaitUntilMSec([&changes] {return changes > 1;}, false, 15);
}

}
}
