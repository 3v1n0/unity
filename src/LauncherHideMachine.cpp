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

LauncherHideMachine::LauncherHideMachine ()
{
  _mode  = HIDE_NEVER;
  _quirks = DEFAULT;
  _should_hide = false;
  
  _hide_delay_handle = 0;
  _hide_delay_timeout_length = 750;
}

LauncherHideMachine::~LauncherHideMachine ()
{
  if (_hide_delay_handle)
  {
    g_source_remove (_hide_delay_handle);
    _hide_delay_handle = 0;
  }
}

void
LauncherHideMachine::SetShouldHide (bool value, bool skip_delay)
{
  if (_should_hide == value)
    return;
    
  if (value && !skip_delay)
  {
    if (_hide_delay_handle)
      g_source_remove (_hide_delay_handle);
    
    _hide_delay_handle = g_timeout_add (_hide_delay_timeout_length, &OnHideDelayTimeout, this);
  }
  else
  {
    _should_hide = value;
    should_hide_changed.emit (value);
  }
}

/* == Quick Quirk Reference : please keep up to date ==
    LAUNCHER_HIDDEN        = 1 << 0, 1
    MOUSE_OVER_LAUNCHER    = 1 << 1, 2
    MOUSE_OVER_BFB         = 1 << 2, 4
    MOUSE_OVER_TRIGGER     = 1 << 3, 8
    QUICKLIST_OPEN         = 1 << 4, 16  #VISIBLE_REQUIRED
    EXTERNAL_DND_ACTIVE    = 1 << 5, 32  #VISIBLE_REQUIRED
    INTERNAL_DND_ACTIVE    = 1 << 6, 64  #VISIBLE_REQUIRED
    TRIGGER_BUTTON_DOWN    = 1 << 7, 128 #VISIBLE_REQUIRED
    ANY_WINDOW_UNDER       = 1 << 8, 256
    ACTIVE_WINDOW_UNDER    = 1 << 9, 512
    DND_PUSHED_OFF         = 1 << 10, 1024
    MOUSE_MOVE_POST_REVEAL = 1 << 10, 2k
    VERTICAL_SLIDE_ACTIVE  = 1 << 12, 4k  #VISIBLE_REQUIRED
    KEY_NAV_ACTIVE         = 1 << 13, 8k  #VISIBLE_REQUIRED
    PLACES_VISIBLE         = 1 << 14, 16k #VISIBLE_REQUIRED
    LAST_ACTION_ACTIVATE   = 1 << 15, 32k
    SCALE_ACTIVE           = 1 << 16, 64k  #VISIBLE_REQUIRED
    EXPO_ACTIVE            = 1 << 17, 128k #VISIBLE_REQUIRED
*/

#define VISIBLE_REQUIRED (QUICKLIST_OPEN | EXTERNAL_DND_ACTIVE | \
INTERNAL_DND_ACTIVE | TRIGGER_BUTTON_DOWN | VERTICAL_SLIDE_ACTIVE |\
KEY_NAV_ACTIVE | PLACES_VISIBLE | SCALE_ACTIVE | EXPO_ACTIVE)

void
LauncherHideMachine::EnsureHideState (bool skip_delay)
{
  bool should_hide;
  
  if (_mode == HIDE_NEVER)
  {
    SetShouldHide (false, skip_delay);
    return;
  }
  
  do
  {
    // first we check the condition where external DND is active and the push off has happened
    if (GetQuirk ((HideQuirk) (EXTERNAL_DND_ACTIVE | DND_PUSHED_OFF), false))
    {
      should_hide = true;
      break;
    }
    
    if (GetQuirk (LAST_ACTION_ACTIVATE))
    {
      should_hide = true;
      break;
    }
    
    HideQuirk _should_show_quirk;

    if (GetQuirk (LAUNCHER_HIDDEN))
      _should_show_quirk = (HideQuirk) (VISIBLE_REQUIRED | MOUSE_OVER_TRIGGER);
    else {
      _should_show_quirk = (HideQuirk) (VISIBLE_REQUIRED | MOUSE_OVER_BFB);
      // mouse position over launcher is only taken into account if we move it after the revealing state
      if (GetQuirk (MOUSE_MOVE_POST_REVEAL))
        _should_show_quirk = (HideQuirk) (_should_show_quirk | MOUSE_OVER_LAUNCHER); 
    }

    if (GetQuirk (_should_show_quirk))
    {
      should_hide = false;
      break;
    }

    if (_mode == AUTOHIDE)
      should_hide = true;
    else if (_mode == DODGE_WINDOWS)
      should_hide = GetQuirk (ANY_WINDOW_UNDER);
    else if (_mode == DODGE_ACTIVE_WINDOW)
      should_hide = GetQuirk (ACTIVE_WINDOW_UNDER);

  } while (false);
  
  SetShouldHide (should_hide, skip_delay);
}

void
LauncherHideMachine::SetMode (LauncherHideMachine::HideMode mode)
{
  if (_mode == mode)
    return;
  
  _mode = mode;
  EnsureHideState (true);
}

LauncherHideMachine::HideMode
LauncherHideMachine::GetMode ()
{
  return _mode;
}

#define SKIP_DELAY_QUIRK (EXTERNAL_DND_ACTIVE | DND_PUSHED_OFF | ACTIVE_WINDOW_UNDER | \
ANY_WINDOW_UNDER | EXPO_ACTIVE | SCALE_ACTIVE)

void
LauncherHideMachine::SetQuirk (LauncherHideMachine::HideQuirk quirk, bool active)
{
  if (GetQuirk (quirk) == active)
    return;
  
  if (active)
    _quirks = (HideQuirk) (_quirks | quirk);
  else
    _quirks = (HideQuirk) (_quirks & ~quirk);
  
  // no skipping when last action was activate on general case
  bool skip = quirk & SKIP_DELAY_QUIRK && !GetQuirk (LAST_ACTION_ACTIVATE);
  
  // but skip on last action if we were out of the launcher/bfb
  if (GetQuirk (LAST_ACTION_ACTIVATE) && !active && (quirk & (MOUSE_OVER_LAUNCHER | MOUSE_OVER_BFB)))
    skip = true;
  
  EnsureHideState (skip);
}

bool
LauncherHideMachine::GetQuirk (LauncherHideMachine::HideQuirk quirk, bool allow_partial)
{
  if (allow_partial)
    return _quirks & quirk;
  return (_quirks & quirk) == quirk;
}

bool
LauncherHideMachine::ShouldHide ()
{
  return _should_hide;
}

gboolean
LauncherHideMachine::OnHideDelayTimeout (gpointer data)
{
  LauncherHideMachine *self = static_cast<LauncherHideMachine *> (data);
  self->EnsureHideState (true);
  
  self->_hide_delay_handle = 0;
  return false;
}
