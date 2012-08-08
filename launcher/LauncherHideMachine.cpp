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

#include "LauncherHideMachine.h"

#include <NuxCore/Logger.h>

namespace unity
{
namespace launcher
{

namespace
{
nux::logging::Logger logger("unity.launcher");
const unsigned int HIDE_DELAY_TIMEOUT_LENGTH = 750;
}

LauncherHideMachine::LauncherHideMachine()
  : reveal_progress(0)
  , _mode(HIDE_NEVER)
  , _quirks(DEFAULT)
  , _should_hide(false)
  , _latest_emit_should_hide(false)
{
  decaymulator_.value.changed.connect([&](int value) { reveal_progress = value / static_cast<float>(reveal_pressure); });
  edge_decay_rate.changed.connect(sigc::mem_fun (this, &LauncherHideMachine::OnDecayRateChanged));
}

void LauncherHideMachine::OnDecayRateChanged(int value)
{
  decaymulator_.rate_of_decay = value;
}

void LauncherHideMachine::AddRevealPressure(int pressure)
{
  decaymulator_.value = decaymulator_.value + pressure;

  if (decaymulator_.value > reveal_pressure)
  {
    SetQuirk(REVEAL_PRESSURE_PASS, true);
    SetQuirk(MOUSE_MOVE_POST_REVEAL, true);
    decaymulator_.value = 0;
  }
}

void LauncherHideMachine::SetShouldHide(bool value, bool skip_delay)
{
  if (_should_hide == value)
    return;

  if (value && !skip_delay)
  {
    _hide_delay_timeout.reset(new glib::Timeout(HIDE_DELAY_TIMEOUT_LENGTH));
    _hide_delay_timeout->Run([&] () {
      EnsureHideState(true);
      return false;
    });
  }
  else
  {
    _should_hide = value;

    _hide_changed_emit_idle.reset(new glib::Idle(glib::Source::Priority::DEFAULT));
    _hide_changed_emit_idle->Run(sigc::mem_fun(this, &LauncherHideMachine::EmitShouldHideChanged));
  }
}

/* == Quick Quirk Reference : please keep up to date ==
    LAUNCHER_HIDDEN        = 1 << 0, 1
    MOUSE_OVER_LAUNCHER    = 1 << 1, 2
    QUICKLIST_OPEN         = 1 << 4, 16  #VISIBLE_REQUIRED
    EXTERNAL_DND_ACTIVE    = 1 << 5, 32  #VISIBLE_REQUIRED
    INTERNAL_DND_ACTIVE    = 1 << 6, 64  #VISIBLE_REQUIRED
    TRIGGER_BUTTON_SHOW    = 1 << 7, 128 #VISIBLE_REQUIRED
    DND_PUSHED_OFF         = 1 << 10, 1024
    MOUSE_MOVE_POST_REVEAL = 1 << 11, 2k
    VERTICAL_SLIDE_ACTIVE  = 1 << 12, 4k  #VISIBLE_REQUIRED
    KEY_NAV_ACTIVE         = 1 << 13, 8k  #VISIBLE_REQUIRED
    PLACES_VISIBLE         = 1 << 14, 16k #VISIBLE_REQUIRED
    LAST_ACTION_ACTIVATE   = 1 << 15, 32k
    SCALE_ACTIVE           = 1 << 16, 64k  #VISIBLE_REQUIRED
    EXPO_ACTIVE            = 1 << 17, 128k #VISIBLE_REQUIRED
    MT_DRAG_OUT            = 1 << 18, 256k #VISIBLE_REQUIRED
    LAUNCHER_PULSE         = 1 << 20, 1M   #VISIBLE_REQUIRED
    LOCK_HIDE              = 1 << 21, 2M
*/

#define VISIBLE_REQUIRED (QUICKLIST_OPEN | EXTERNAL_DND_ACTIVE | \
INTERNAL_DND_ACTIVE | TRIGGER_BUTTON_SHOW | VERTICAL_SLIDE_ACTIVE |\
KEY_NAV_ACTIVE | PLACES_VISIBLE | SCALE_ACTIVE | EXPO_ACTIVE |\
MT_DRAG_OUT | LAUNCHER_PULSE)

void LauncherHideMachine::EnsureHideState(bool skip_delay)
{
  bool should_hide;

  if (_mode == HIDE_NEVER)
  {
    SetShouldHide(false, skip_delay);
    return;
  }

  // early check to see if we are locking to hidden - but only if we are in non HIDE_NEVER
  if (GetQuirk(LOCK_HIDE))
  {
    SetShouldHide(true, true);
    return;
  }

  do
  {
    // first we check the condition where external DND is active and the push off has happened
    if (GetQuirk((HideQuirk)(EXTERNAL_DND_ACTIVE | DND_PUSHED_OFF), false))
    {
      should_hide = true;
      break;
    }

    // figure out if we are going to hide because of a window
    bool hide_for_window = false;
    if (_mode == AUTOHIDE)
      hide_for_window = true;

    // if we activated AND we would hide because of a window, go ahead and do it
    if (!_should_hide && GetQuirk(LAST_ACTION_ACTIVATE) && hide_for_window)
    {
      should_hide = true;
      break;
    }

    // Is there anything holding us open?
    HideQuirk _should_show_quirk;
    if (GetQuirk(LAUNCHER_HIDDEN))
    {
      _should_show_quirk = (HideQuirk) ((VISIBLE_REQUIRED) | REVEAL_PRESSURE_PASS);
    }
    else
    {
      _should_show_quirk = (HideQuirk)(VISIBLE_REQUIRED);
      // mouse position over launcher is only taken into account if we move it after the revealing state
      if (GetQuirk(MOUSE_MOVE_POST_REVEAL))
        _should_show_quirk = (HideQuirk)(_should_show_quirk | MOUSE_OVER_LAUNCHER);
    }

    if (GetQuirk(_should_show_quirk))
    {
      should_hide = false;
      break;
    }

    // nothing holding us open, any reason to hide?
    should_hide = hide_for_window;

  }
  while (false);

  SetShouldHide(should_hide, skip_delay);
}

void LauncherHideMachine::SetMode(LauncherHideMachine::HideMode mode)
{
  if (_mode == mode)
    return;

  _mode = mode;
  EnsureHideState(true);
}

LauncherHideMachine::HideMode
LauncherHideMachine::GetMode() const
{
  return _mode;
}

#define SKIP_DELAY_QUIRK (EXTERNAL_DND_ACTIVE | DND_PUSHED_OFF | EXPO_ACTIVE | SCALE_ACTIVE | MT_DRAG_OUT | TRIGGER_BUTTON_SHOW)

void LauncherHideMachine::SetQuirk(LauncherHideMachine::HideQuirk quirk, bool active)
{
  if (GetQuirk(quirk) == active)
    return;

  if (active)
    _quirks = (HideQuirk)(_quirks | quirk);
  else
    _quirks = (HideQuirk)(_quirks & ~quirk);

  // no skipping when last action was activate on general case
  bool skip = quirk & SKIP_DELAY_QUIRK && !GetQuirk(LAST_ACTION_ACTIVATE);

  // but skip on last action if we were out of the launcher/bfb
  if (GetQuirk(LAST_ACTION_ACTIVATE) && !active && (quirk & (MOUSE_OVER_LAUNCHER)))
    skip = true;

  EnsureHideState(skip);
}

bool LauncherHideMachine::GetQuirk(LauncherHideMachine::HideQuirk quirk, bool allow_partial) const
{
  if (allow_partial)
    return _quirks & quirk;
  return (_quirks & quirk) == quirk;
}

bool LauncherHideMachine::ShouldHide() const
{
  return _should_hide;
}

bool LauncherHideMachine::EmitShouldHideChanged()
{
  if (_should_hide == _latest_emit_should_hide)
    return false;

  _latest_emit_should_hide = _should_hide;
  should_hide_changed.emit(_should_hide);

  return false;
}

std::string LauncherHideMachine::DebugHideQuirks() const
{
  // Although I do wonder why we are returning a string representation
  // of the enum value as an integer anyway.
  return std::to_string(_quirks);
}

} // namespace launcher
} // namespace unity
