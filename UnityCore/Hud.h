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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 */

#ifndef UNITY_HUD_H
#define UNITY_HUD_H

#include <deque>
#include <string>
#include <memory>
#include <NuxCore/Property.h>
#include <glib.h>
#include "Variant.h"

namespace unity
{
namespace hud
{


class Query
{
public:
  typedef std::shared_ptr<Query> Ptr;

  Query(std::string const& formatted_text_, std::string const& icon_name_,
        std::string const& item_icon_, std::string const& completion_text_,
        std::string const& shortcut_, GVariant* key_)
  : formatted_text(formatted_text_)
  , icon_name(icon_name_)
  , item_icon(item_icon_)
  , completion_text(completion_text_)
  , shortcut(shortcut_)
  , key(key_)
  {}

  Query(const Query &rhs);
  Query& operator=(Query);

  std::string formatted_text;   // Pango formatted text
  std::string icon_name;        // icon name using standard lookups
  std::string item_icon;        // Future API
  std::string completion_text;  // Non formatted text f or completion
  std::string shortcut;         // Shortcut key
  glib::Variant key;
};


class HudImpl;
class Hud
{
public:
  typedef std::shared_ptr<Hud> Ptr;
  typedef std::deque<Query::Ptr> Queries;

  /*
   * Constructor for the hud
   * \param dbus_name string that specifies the name of the hud service
   * \param dbus_path string that specifies the path of the hud service
   */
  Hud(std::string const& dbus_name,
      std::string const& dbus_path);

  ~Hud();

  Hud(const Hud &rhs);
  Hud& operator=(Hud);

  nux::Property<std::string> target;
  nux::Property<bool> connected;

  /*
   * Queries the service for new suggestions, will fire off the 
   * suggestion_search_finished signal when the suggestions are returned
   */
  void RequestQuery(std::string const& search_string);
  
  /*
   * Executes a Query
   */
  void ExecuteQuery(Query::Ptr query, unsigned int timestamp);

  /*
   * Executes a query that returns from a search,
   * Implicitly calls CloseQuery();
   */
  void ExecuteQueryBySearch(std::string execute_string, unsigned int timestamp);

  /*
   * Closes the query connection, call when the hud closes
   */
  void CloseQuery();

  /*
   * Returns a deque of Query types when the service provides them
   */
  sigc::signal<void, Queries> queries_updated;

private:
  HudImpl *pimpl_;
};

}
}

#endif /* UNITY_HUD_H */
