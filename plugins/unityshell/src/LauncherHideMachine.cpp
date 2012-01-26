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

#include <boost/lexical_cast.hpp>

#include <NuxCore/Logger.h>

namespace
{

nux::logging::Logger logger("unity.launcher");

}

LauncherHideMachine::LauncherHideMachine()
{
  _mode  = HIDE_NEVER;
  _quirks = DEFAULT;
  _should_hide = false;
  _show_on_edge = true;

  _latest_emit_should_hide = false;
  _hide_changed_emit_handle = 0;

  _hide_delay_handle = 0;
  _hide_delay_timeout_length = 750;

  decaymulator_ = unity::ui::Decaymulator::Ptr(new unity::ui::Decaymulator());
  decaymulator_->rate_of_decay = 50000;

  edge_decay_rate.changed.connect(sigc::mem_fun (this, &LauncherHideMachine::OnDecayRateChanged));
}

void LauncherHideMachine::OnDecayRateChanged (int value)
{
  decaymulator_->rate_of_decay = value;  
}


LauncherHideMachine::~LauncherHideMachine()
{
  if (_hide_delay_handle)
  {
    g_source_remove(_hide_delay_handle);
    _hide_delay_handle = 0;
  }
  if (_hide_changed_emit_handle)
  {
    g_source_remove(_hide_changed_emit_handle);
    _hide_changed_emit_handle = 0;
  }
}

void
LauncherHideMachine::AddRevealPressure(int pressure)
{
  decaymulator_->value = decaymulator_->value + pressure;

  if (decaymulator_->value > reveal_pressure)
  {
    SetQuirk(REVEAL_PRESSURE_PASS, true);
    decaymulator_->value = 0;
  }
}

void
LauncherHideMachine::SetShouldHide(bool value, bool skip_delay)
{
  if (_should_hide == value)
    return;

  if (value && !skip_delay)
  {
    if (_hide_delay_handle)
      g_source_remove(_hide_delay_handle);

    _hide_delay_handle = g_timeout_add(_hide_delay_timeout_length, &OnHideDelayTimeout, this);
  }
  else
  {
    _should_hide = value;

    if (_hide_changed_emit_handle)
      g_source_remove(_hide_changed_emit_handle);
    _hide_changed_emit_handle = g_idle_add_full (G_PRIORITY_DEFAULT, &EmitShouldHideChanged, this, NULL);
  }
}

/* == Quick Quirk Reference : please keep up to date ==
    LAUNCHER_HIDDEN        = 1 << 0, 1
    MOUSE_OVER_LAUNCHER    = 1 << 1, 2
    QUICKLIST_OPEN         = 1 << 4, 16  #VISIBLE_REQUIRED
    EXTERNAL_DND_ACTIVE    = 1 << 5, 32  #VISIBLE_REQUIRED
    INTERNAL_DND_ACTIVE    = 1 << 6, 64  #VISIBLE_REQUIRED
    TRIGGER_BUTTON_SHOW    = 1 << 7, 128 #VISIBLE_REQUIRED
    ANY_WINDOW_UNDER       = 1 << 8, 256
    ACTIVE_WINDOW_UNDER    = 1 << 9, 512
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

void
LauncherHideMachine::EnsureHideState(bool skip_delay)
{
  bool should_hide;

  // early check to see if we are locking to hidden
  if (GetQuirk(LOCK_HIDE))
  {
    SetShouldHide(true, true);
    return;
  }

  if (_mode == HIDE_NEVER)
  {
    SetShouldHide(false, skip_delay);
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
    else if (_mode == DODGE_WINDOWS)
      hide_for_window = GetQuirk(ANY_WINDOW_UNDER);
    else if (_mode == DODGE_ACTIVE_WINDOW)
      hide_for_window = GetQuirk(ACTIVE_WINDOW_UNDER);

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

void
LauncherHideMachine::SetMode(LauncherHideMachine::HideMode mode)
{
  if (_mode == mode)
    return;

  _mode = mode;
  EnsureHideState(true);
}

LauncherHideMachine::HideMode
LauncherHideMachine::GetMode()
{
  return _mode;
}

#define SKIP_DELAY_QUIRK (EXTERNAL_DND_ACTIVE | DND_PUSHED_OFF | ACTIVE_WINDOW_UNDER | \
ANY_WINDOW_UNDER | EXPO_ACTIVE | SCALE_ACTIVE | MT_DRAG_OUT | TRIGGER_BUTTON_SHOW)

void
LauncherHideMachine::SetQuirk(LauncherHideMachine::HideQuirk quirk, bool active)
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

bool
LauncherHideMachine::GetQuirk(LauncherHideMachine::HideQuirk quirk, bool allow_partial)
{
  if (allow_partial)
    return _quirks & quirk;
  return (_quirks & quirk) == quirk;
}

bool
LauncherHideMachine::ShouldHide()
{
  return _should_hide;
}

void
LauncherHideMachine::SetShowOnEdge(bool value)
{
  if (value == _show_on_edge)
    return;

  _show_on_edge = value;

  LOG_DEBUG(logger) << "Shows on edge: " << _show_on_edge;
}

bool
LauncherHideMachine::GetShowOnEdge()
{
  return _show_on_edge;
}

gboolean
LauncherHideMachine::OnHideDelayTimeout(gpointer data)
{
  LauncherHideMachine* self = static_cast<LauncherHideMachine*>(data);
  self->EnsureHideState(true);

  self->_hide_delay_handle = 0;
  return false;
}

gboolean
LauncherHideMachine::EmitShouldHideChanged(gpointer data)
{
  LauncherHideMachine* self = static_cast<LauncherHideMachine*>(data);

  self->_hide_changed_emit_handle = 0;
  if (self->_should_hide == self->_latest_emit_should_hide)
    return false;

  self->_latest_emit_should_hide = self->_should_hide;
  self->should_hide_changed.emit(self->_should_hide);

  return false;
}

std::string
LauncherHideMachine::DebugHideQuirks()
{
  // Although I do wonder why we are returning a string representation
  // of the enum value as an integer anyway.
  return boost::lexical_cast<std::string>(_quirks);
}
