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

  virtual const guint32  GetPosition  () = 0;
  virtual const gchar ** GetMimetypes () = 0;

  virtual const std::map<gchar *, gchar *>& GetHints () = 0;

  virtual const bool IsSensitive () = 0;
  virtual const bool IsActive    () = 0;
};

#endif // PLACE_ENTRY_H
