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
#ifndef PLACE_ENTRY_H
#define PLACE_ENTRY_H

#include <map>
#include <string>
#include <vector>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>

class PlaceEntry : public sigc::trackable
{
public:
  virtual const gchar * GetName        () = 0;
  virtual const gchar * GetIcon        () = 0;
  virtual const gchar * GetDescription () = 0;

  // For ordering entries within a place
  virtual guint32        GetPosition  () = 0;

  // For DND, what can this entry handle
  virtual const gchar ** GetMimetypes () = 0;

  virtual const std::map<gchar *, gchar *>& GetHints () = 0;

  // Should be shown on launcher
  virtual bool IsSensitive () = 0;

  // This is not really useful for views
  virtual bool IsActive    () = 0;

  // Include as part of global search results
  // NOTE: IsSensitive has no effect, i.e. a entry might only ever want to be
  // shown in global search and never on the launcher
  virtual bool ShowInGlobal () = 0;

  // Signals
  sigc::signal<void, const gchar *> name_changed;
};

#endif // PLACE_ENTRY_H
