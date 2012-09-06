// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010, 2011 Canonical Ltd
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 *              Tim Penhey <tim.penhey@canonical.com>
 */

#include "LauncherOptions.h"

namespace unity
{
namespace launcher
{

Options::Options()
    // defaults from XML file
  : hide_mode(LAUNCHER_HIDE_NEVER)
  , launch_animation(LAUNCH_ANIMATION_PULSE)
  , urgent_animation(URGENT_ANIMATION_WIGGLE)
  , auto_hide_animation(FADE_AND_SLIDE)
  , backlight_mode(BACKLIGHT_NORMAL)
  , reveal_trigger(RevealTrigger::EDGE)
  , icon_size(48)
  , tile_size(54)
  , background_alpha(0.6667)
  , edge_decay_rate(1500)
  , edge_overcome_pressure(2000)
  , edge_stop_velocity(6500)
  , edge_reveal_pressure(2000)
  , edge_responsiveness(2.0f)
  , edge_passed_disabled_ms(1000)
  , edge_resist(true)
  , show_for_all(false)
{
  auto_hide_animation.changed.connect    ([this] (AutoHideAnimation value) { option_changed.emit(); });
  background_alpha.changed.connect       ([this] (float value)             { option_changed.emit(); });
  backlight_mode.changed.connect         ([this] (BacklightMode value)     { option_changed.emit(); });
  edge_decay_rate.changed.connect        ([this] (int value)               { option_changed.emit(); });
  edge_overcome_pressure.changed.connect ([this] (int value)               { option_changed.emit(); });
  edge_responsiveness.changed.connect    ([this] (float value)             { option_changed.emit(); });
  edge_reveal_pressure.changed.connect   ([this] (int value)               { option_changed.emit(); });
  edge_stop_velocity.changed.connect     ([this] (int value)               { option_changed.emit(); });
  edge_passed_disabled_ms.changed.connect([this] (unsigned value)          { option_changed.emit(); });
  hide_mode.changed.connect              ([this] (LauncherHideMode value)  { option_changed.emit(); });
  icon_size.changed.connect              ([this] (int value)               { option_changed.emit(); });
  launch_animation.changed.connect       ([this] (LaunchAnimation value)   { option_changed.emit(); });
  reveal_trigger.changed.connect         ([this] (RevealTrigger vallue)    { option_changed.emit(); });
  tile_size.changed.connect              ([this] (int value)               { option_changed.emit(); });
  urgent_animation.changed.connect       ([this] (UrgentAnimation value)   { option_changed.emit(); });
  edge_resist.changed.connect            ([this] (bool value)              { option_changed.emit(); });
}

}
}
