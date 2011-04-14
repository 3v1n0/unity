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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef PLACE_H
#define PLACE_H

#include <string>
#include <vector>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>
#include <glib.h>

#include "PlaceEntry.h"

enum ActivationResult
{
  FALLBACK = 0,
  SHOW_DASH,
  HIDE_DASH
};

class Place : public sigc::trackable
{
public:
  // This could be empty, if the loading of entries is async, so you
  // should be listening to the signals too
  virtual std::vector<PlaceEntry *>& GetEntries  () = 0;
  virtual guint32                    GetNEntries () = 0;

  virtual void ActivateResult (const char *uri, const char *mimetype) = 0;

  virtual void Connect () = 0;

  // Signals
  sigc::signal<void, PlaceEntry *> entry_added;
  sigc::signal<void, PlaceEntry *> entry_removed;
  sigc::signal<void, const char *, ActivationResult> result_activated;

protected:
  std::vector<PlaceEntry *> _entries;
};

#endif // PLACE_H
