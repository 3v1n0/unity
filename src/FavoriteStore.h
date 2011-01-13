// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#ifndef FAVORITE_STORE_H
#define FAVORITE_STORE_H

#include <NuxCore/NuxCore.h>
#include <NuxCore/ObjectType.h>
#include <NuxCore/Object.h>
#include <string>
#include <sigc++/signal.h>

#include <glib.h>

// An abstract object that facilitates getting and modifying the list of favorites
// Use GetDefault () to get the correct store for the session

class FavoriteStore : public nux::Object
{
public:
  FavoriteStore ();
  ~FavoriteStore ();

  // Returns a referenced FavoriteStore, make sure to UnReference () it
  static FavoriteStore * GetDefault ();

  // Methods

  // Get's a GSList of char * desktop paths
  // DO NOT FREE
  // DO NOT RELY ON THE CHAR *, STRDUP THEM
  virtual GSList * GetFavorites () = 0;
  
  // These will NOT emit the relevant signals, so bare that in mind
  // i.e. don't hope that you can add stuff and hook the view up to
  // favorite_added events to update the view. The signals only emit if
  // there has been a change on the GSettings object from an external
  // source
  virtual void AddFavorite    (const char *desktop_path, gint position) = 0;
  virtual void RemoveFavorite (const char *desktop_path) = 0;
  virtual void MoveFavorite   (const char *desktop_path, gint position) = 0;
  virtual void SetFavorites   (std::list<const char*> desktop_paths) = 0;

  // Signals
  // Therse only emit if something has changed the GSettings object externally

  //desktop_path, position
  sigc::signal<void, const char *, guint32> favorite_added;

  //desktop_path
  sigc::signal<void, const char *> favorite_removed;

  sigc::signal<void> reordered;

private:
};

#endif // FAVORITE_STORE_H
