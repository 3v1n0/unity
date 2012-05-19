/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Gordon Allott <gord.alott@canonical.com>
 */

#ifndef BGHASH_H
#define BGHASH_H

#include <sigc++/sigc++.h>
#include <Nux/Nux.h>
#include <libgnome-desktop/gnome-bg.h>
#include <unity-misc/gnome-bg-slideshow.h>
#include <UnityCore/GLibSignal.h>
#include <UnityCore/GLibWrapper.h>
#include "Animator.h"
#include "UBusWrapper.h"

namespace unity {
namespace colors {
  const nux::Color Aubergine(0x3E, 0x20, 0x60);
};
};

namespace unity
{
  class BGHash
  {
  public:
    BGHash();
    ~BGHash();

    nux::Color CurrentColor();
    void OnBackgroundChanged(GnomeBG *bg);
    void OnGSettingsChanged(GSettings *settings, gchar *key);
    void OverrideColor(nux::Color color);
    void RefreshColor();

  private:
    void OnTransitionUpdated(double progress);
    void DoUbusColorEmit();
    void TransitionToNewColor(nux::Color new_color);
    nux::Color InterpolateColor(nux::Color colora, nux::Color colorb, float value);
    nux::Color MatchColor(nux::Color base_color);

  private:
    glib::Object<GnomeBG> background_monitor_;
    glib::Object<GSettings> client_;
    Animator transition_animator_;

    nux::Color _current_color; // the current colour, including steps in transitions
    nux::Color _new_color;     // in transitions, the next colour, otherwise the current colour
    nux::Color _old_color;     // the last colour chosen, used for transitions
    nux::Color _override_color;

    glib::SignalManager signal_manager_;
    UBusManager ubus_manager_;
  };
};

#endif // BGHASH_H
