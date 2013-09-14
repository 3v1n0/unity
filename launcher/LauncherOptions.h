// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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

#ifndef LAUNCHEROPTIONS_H
#define LAUNCHEROPTIONS_H

#include <memory>
#include <vector>
#include <sigc++/sigc++.h>

#include <Nux/Nux.h>
#include <UnityCore/GLibSource.h>

namespace unity
{
namespace launcher
{

enum LauncherHideMode
{
  LAUNCHER_HIDE_NEVER,
  LAUNCHER_HIDE_AUTOHIDE,
};

enum LaunchAnimation
{
  LAUNCH_ANIMATION_NONE,
  LAUNCH_ANIMATION_PULSE,
  LAUNCH_ANIMATION_BLINK,
};

enum UrgentAnimation
{
  URGENT_ANIMATION_NONE,
  URGENT_ANIMATION_PULSE,
  URGENT_ANIMATION_WIGGLE,
};

enum AutoHideAnimation
{
  FADE_OR_SLIDE,
  SLIDE_ONLY,
  FADE_ONLY,
  FADE_AND_SLIDE,
};

enum BacklightMode
{
  BACKLIGHT_ALWAYS_ON,
  BACKLIGHT_NORMAL,
  BACKLIGHT_ALWAYS_OFF,
  BACKLIGHT_EDGE_TOGGLE,
  BACKLIGHT_NORMAL_EDGE_TOGGLE
};

enum RevealTrigger
{
  EDGE,
  CORNER,
};

class Options : public sigc::trackable
{
public:
  typedef std::shared_ptr<Options> Ptr;

  Options();

  nux::Property<LauncherHideMode> hide_mode;
  nux::Property<LaunchAnimation> launch_animation;
  nux::Property<UrgentAnimation> urgent_animation;
  nux::Property<AutoHideAnimation> auto_hide_animation;
  nux::Property<BacklightMode> backlight_mode;
  nux::Property<RevealTrigger> reveal_trigger;
  nux::Property<nux::Color> background_color;
  nux::Property<float> background_alpha;
  nux::Property<int> icon_size;
  nux::Property<int> tile_size;
  nux::Property<int> super_tap_duration;
  nux::Property<int> edge_decay_rate;
  nux::Property<int> edge_overcome_pressure;
  nux::Property<int> edge_stop_velocity;
  nux::Property<int> edge_reveal_pressure;
  nux::Property<float> edge_responsiveness;
  nux::Property<unsigned> edge_passed_disabled_ms;
  nux::Property<bool> edge_resist;
  nux::Property<bool> show_for_all;

  sigc::signal<void> option_changed;

private:
  glib::Source::UniquePtr changed_idle_;
};

}
}

#endif // LAUNCHEROPTIONS_H
