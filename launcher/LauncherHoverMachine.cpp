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
 * Authored by: Didier Roche <didrocks@ubuntu.com>
 */
#include <boost/lexical_cast.hpp>

#include "LauncherHoverMachine.h"

LauncherHoverMachine::LauncherHoverMachine()
{
  _quirks = DEFAULT;
  _should_hover = false;
  _latest_emit_should_hover = false;
  _hover_changed_emit_handle = 0;

}

LauncherHoverMachine::~LauncherHoverMachine()
{
  if (_hover_changed_emit_handle)
  {
    g_source_remove(_hover_changed_emit_handle);
    _hover_changed_emit_handle = 0;
  }
}

/* == Quick Quirk Reference : please keep up to date ==
    DEFAULT                = 0,
    LAUNCHER_HIDDEN        = 1 << 0, 1
    MOUSE_OVER_LAUNCHER    = 1 << 1, 2
    MOUSE_OVER_BFB         = 1 << 2, 4
    SHORTCUT_KEYS_VISIBLE  = 1 << 3, 8
    QUICKLIST_OPEN         = 1 << 4, 16
    KEY_NAV_ACTIVE         = 1 << 5, 32
    LAUNCHER_IN_ACTION     = 1 << 6, 64
    PLACES_VISIBLE         = 1 << 7, 128
*/

void
LauncherHoverMachine::EnsureHoverState()
{
  bool should_hover;

  if (GetQuirk(LAUNCHER_HIDDEN))
  {
    SetShouldHover(false);
    return;
  }

  if (GetQuirk((HoverQuirk)(MOUSE_OVER_LAUNCHER | MOUSE_OVER_BFB |
                            SHORTCUT_KEYS_VISIBLE | KEY_NAV_ACTIVE |
                            QUICKLIST_OPEN | LAUNCHER_IN_ACTION)))
    should_hover = true;
  else
    should_hover = false;


  SetShouldHover(should_hover);
}

void
LauncherHoverMachine::SetShouldHover(bool value)
{
  _should_hover = value;

  if (_hover_changed_emit_handle)
    g_source_remove(_hover_changed_emit_handle);
  _hover_changed_emit_handle = g_idle_add_full (G_PRIORITY_DEFAULT, &EmitShouldHoverChanged, this, NULL);
}

gboolean
LauncherHoverMachine::EmitShouldHoverChanged(gpointer data)
{
  LauncherHoverMachine* self = static_cast<LauncherHoverMachine*>(data);

  self->_hover_changed_emit_handle = 0;
  if (self->_should_hover == self->_latest_emit_should_hover)
    return false;

  self->_latest_emit_should_hover = self->_should_hover;
  self->should_hover_changed.emit(self->_should_hover);

  return false;
}

void
LauncherHoverMachine::SetQuirk(LauncherHoverMachine::HoverQuirk quirk, bool active)
{
  if (GetQuirk(quirk) == active)
    return;

  if (active)
    _quirks = (HoverQuirk)(_quirks | quirk);
  else
    _quirks = (HoverQuirk)(_quirks & ~quirk);

  EnsureHoverState();
}

bool
LauncherHoverMachine::GetQuirk(LauncherHoverMachine::HoverQuirk quirk, bool allow_partial)
{
  if (allow_partial)
    return _quirks & quirk;
  return (_quirks & quirk) == quirk;
}

std::string
LauncherHoverMachine::DebugHoverQuirks()
{
  // Although I do wonder why we are returning a string representation
  // of the enum value as an integer anyway.
  return boost::lexical_cast<std::string>(_quirks);
}
