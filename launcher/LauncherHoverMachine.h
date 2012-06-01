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

#ifndef LAUNCHERHOVERMACHINE
#define LAUNCHERHOVERMACHINE

#include <sigc++/sigc++.h>
#include <glib.h>
#include <string>

class LauncherHoverMachine : public sigc::trackable
{
public:

  typedef enum
  {
    DEFAULT                = 0,
    LAUNCHER_HIDDEN        = 1 << 0,
    MOUSE_OVER_LAUNCHER    = 1 << 1,
    MOUSE_OVER_BFB         = 1 << 2,
    SHORTCUT_KEYS_VISIBLE  = 1 << 3,
    QUICKLIST_OPEN         = 1 << 4,
    KEY_NAV_ACTIVE         = 1 << 5,
    LAUNCHER_IN_ACTION     = 1 << 6,
    PLACES_VISIBLE         = 1 << 7,
  } HoverQuirk;

  LauncherHoverMachine();
  virtual ~LauncherHoverMachine();

  void SetQuirk(HoverQuirk quirk, bool active);
  bool GetQuirk(HoverQuirk quirk, bool allow_partial = true);

  sigc::signal<void, bool> should_hover_changed;

  std::string DebugHoverQuirks();

private:
  void EnsureHoverState();
  void SetShouldHover(bool value);

  static gboolean EmitShouldHoverChanged(gpointer data);

  bool       _should_hover;
  bool       _latest_emit_should_hover;
  HoverQuirk _quirks;

  guint _hover_changed_emit_handle;

};

#endif
