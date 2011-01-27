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

#ifndef PLACE_FACTORY_H
#define PLACE_FACTORY_H

#include <string>
#include <vector>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>

#include "Place.h"

class PlaceFactory : public sigc::trackable
{
public:

  static PlaceFactory * GetDefault ();

  // For testing
  static void           SetDefault (PlaceFactory *factory);

  // This could be empty, if the loading of places is async, so you
  // should be listening to the signals too
  virtual std::vector<Place *>& GetPlaces () = 0;

  // If the implementation supports it, you'll get a refreshed list of
  // Indicators (probably though a bunch of removed/added events)
  virtual void ForceRefresh () = 0;

  // For adding factory-specific properties
  virtual void AddProperties (GVariantBuilder *builder) = 0;

  // Signals
  sigc::signal<void, Place *> place_added;
  sigc::signal<void, Place *> place_removed;

protected:
  std::vector<Place *> _places;
};

#endif // PLACE_FACTORY_H
