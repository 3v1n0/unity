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

#ifndef UNITY_FAVORITE_STORE_H
#define UNITY_FAVORITE_STORE_H

#include <list>
#include <string>

#include <boost/utility.hpp>
#include <sigc++/signal.h>


namespace unity
{

// An abstract object that facilitates getting and modifying the list of favorites
// Use GetDefault () to get the correct store for the session
typedef std::list<std::string> FavoriteList;

class FavoriteStore : public sigc::trackable, boost::noncopyable
{
public:
  FavoriteStore();
  virtual ~FavoriteStore();

  static FavoriteStore& Instance();

  virtual FavoriteList const& GetFavorites() = 0;

  // These will NOT emit the relevant signals, so bare that in mind
  // i.e. don't hope that you can add stuff and hook the view up to
  // favorite_added events to update the view. The signals only emit if
  // there has been a change on the GSettings object from an external
  // source
  virtual void AddFavorite(std::string const& desktop_path, int position) = 0;
  virtual void RemoveFavorite(std::string const& desktop_path) = 0;
  virtual void MoveFavorite(std::string const& desktop_path, int position) = 0;
  virtual void SetFavorites(FavoriteList const& desktop_paths) = 0;

  // Signals
  // These only emit if something has changed the GSettings object externally

  //desktop_path, position, before/after
  sigc::signal<void, std::string const&, std::string const&, bool> favorite_added;
  //desktop_path
  sigc::signal<void, std::string const&> favorite_removed;
  sigc::signal<void> reordered;
};

}

#endif // FAVORITE_STORE_H
