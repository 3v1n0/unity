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
  , background_color(nux::Color(95, 18, 45))
  , background_alpha(0.6667)
  , icon_size(48)
  , tile_size(54)
  , super_tap_duration(250)
  , edge_decay_rate(1500)
  , edge_overcome_pressure(2000)
  , edge_stop_velocity(6500)
  , edge_reveal_pressure(2000)
  , edge_responsiveness(2.0f)
  , edge_passed_disabled_ms(1000)
  , edge_resist(true)
  , show_for_all(false)
{
  auto changed_lambda = [this] {
    changed_idle_.reset(new glib::Idle(glib::Source::Priority::HIGH));
    changed_idle_->Run([this] { option_changed.emit(); return false; });
  };

  auto_hide_animation.changed.connect(sigc::hide(changed_lambda));
  background_alpha.changed.connect(sigc::hide(changed_lambda));
  background_color.changed.connect(sigc::hide(changed_lambda));
  backlight_mode.changed.connect(sigc::hide(changed_lambda));
  edge_decay_rate.changed.connect(sigc::hide(changed_lambda));
  edge_overcome_pressure.changed.connect(sigc::hide(changed_lambda));
  edge_responsiveness.changed.connect(sigc::hide(changed_lambda));
  edge_reveal_pressure.changed.connect(sigc::hide(changed_lambda));
  edge_stop_velocity.changed.connect(sigc::hide(changed_lambda));
  edge_passed_disabled_ms.changed.connect(sigc::hide(changed_lambda));
  hide_mode.changed.connect(sigc::hide(changed_lambda));
  icon_size.changed.connect(sigc::hide(changed_lambda));
  launch_animation.changed.connect(sigc::hide(changed_lambda));
  reveal_trigger.changed.connect(sigc::hide(changed_lambda));
  tile_size.changed.connect(sigc::hide(changed_lambda));
  super_tap_duration.changed.connect(sigc::hide(changed_lambda));
  urgent_animation.changed.connect(sigc::hide(changed_lambda));
  edge_resist.changed.connect(sigc::hide(changed_lambda));
}

}
}
