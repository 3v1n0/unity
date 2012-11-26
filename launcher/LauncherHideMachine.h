// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#ifndef LAUNCHERHIDEMACHINE
#define LAUNCHERHIDEMACHINE

#include <sigc++/sigc++.h>
#include <string>
#include <UnityCore/GLibSource.h>

#include "Decaymulator.h"

namespace unity
{
namespace launcher
{

class LauncherHideMachine : public sigc::trackable
{
public:
  typedef enum
  {
    HIDE_NEVER,
    AUTOHIDE,
    DODGE_WINDOWS,
    DODGE_ACTIVE_WINDOW,
  } HideMode;

  typedef enum
  {
    DEFAULT                = 0,
    LAUNCHER_HIDDEN        = 1 << 0,
    MOUSE_OVER_LAUNCHER    = 1 << 1,
    QUICKLIST_OPEN         = 1 << 2,
    EXTERNAL_DND_ACTIVE    = 1 << 3,
    INTERNAL_DND_ACTIVE    = 1 << 4,
    TRIGGER_BUTTON_SHOW    = 1 << 5,
    DND_PUSHED_OFF         = 1 << 6,
    MOUSE_MOVE_POST_REVEAL = 1 << 7,
    VERTICAL_SLIDE_ACTIVE  = 1 << 8,
    KEY_NAV_ACTIVE         = 1 << 9,
    PLACES_VISIBLE         = 1 << 10,
    SCALE_ACTIVE           = 1 << 11,
    EXPO_ACTIVE            = 1 << 12,
    MT_DRAG_OUT            = 1 << 13,
    REVEAL_PRESSURE_PASS   = 1 << 14,
    LAUNCHER_PULSE         = 1 << 15,
    LOCK_HIDE              = 1 << 16,
    SHORTCUT_KEYS_VISIBLE  = 1 << 17
  } HideQuirk;

  nux::Property<int> reveal_pressure;
  nux::Property<int> edge_decay_rate;
  nux::Property<float> reveal_progress;

  LauncherHideMachine();

  void     SetMode(HideMode mode);
  HideMode GetMode() const;

  void AddRevealPressure(int pressure);

  void SetQuirk(HideQuirk quirk, bool active);
  bool GetQuirk(HideQuirk quirk, bool allow_partial = true) const;

  bool ShouldHide() const;

  sigc::signal<void, bool> should_hide_changed;

  std::string DebugHideQuirks() const;

private:
  void EnsureHideState(bool skip_delay);
  void SetShouldHide(bool value, bool skip_delay);
  bool EmitShouldHideChanged();

  void OnDecayRateChanged (int value);

  ui::Decaymulator decaymulator_;
  HideMode  _mode;
  HideQuirk _quirks;
  bool      _should_hide;
  bool      _latest_emit_should_hide;

  glib::Source::UniquePtr _hide_delay_timeout;
  glib::Source::UniquePtr _hide_changed_emit_idle;
};

} // namespace launcher
} // namespace unity

#endif
