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

#ifndef PLACE_FACTORY_FILE_H
#define PLACE_FACTORY_FILE_H

#include <glib.h>
#include <gio/gio.h>

#include <string>
#include <vector>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>

#include "PlaceFactory.h"

class PlaceFactoryFile : public PlaceFactory
{
public:
  PlaceFactoryFile (const char *directory = NULL);
  ~PlaceFactoryFile ();

  std::vector<Place *>& GetPlaces     ();
  void                  ForceRefresh  ();
  void                  AddProperties (GVariantBuilder *builder);

  /* Callbacks, not interesting to others */
  void OnDirectoryEnumerationReady (GObject      *source,
                                    GAsyncResult *result);
private:
  static bool DoSortThemMister (Place *a, Place *b);
 
public:
  /* For Debugging */
  bool read_directory;

private:
  char  *_directory;
  GFile *_dir;
};

#endif // PLACE_FACTORY_FILE_H
