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
#include "GLibDBusProxy.h"
#include "GLibWrapper.h"  

namespace unity
{
namespace hud
{

class Hud
{
public:
  typedef std::shared_ptr<Hud> Ptr;
  typedef std::tuple<std::string, std::string, GVariant*> Suggestion; // target, icon, key
  typedef std::deque<Suggestion> Suggestions;

  /*
   * Constructor for the hud
   * \param dbus_name string that specifies the name of the hud service
   * \param dbus_path string that specifies the path of the hud service
   */
  Hud(std::string const& dbus_name,
      std::string const& dbus_path);

  ~Hud();

  nux::Property<std::string> target;
  nux::Property<std::string> target_icon;

  /*
   * Queries the service for new suggestions, will fire off the 
   * suggestion_search_finished signal when the suggestions are returned
   */
  void GetSuggestions(std::string const& search_string);

  /*
   * Sends the given string to the execute method of the service, does not 
   * require a key
   */
  void Execute(std::string const& execute_string);

  /*
   * Executes a suggestion associated with the given key
   */
  void ExecuteByKey(GVariant* key);

  /*
   * Returns a deque of Suggestion types when the service provides them
   */
  sigc::signal<void, Suggestions> suggestion_search_finished;

private:
  Suggestions suggestions_;
  std::string private_connection_name_;
  glib::DBusProxy proxy_;

  void SuggestionCallback(GVariant* data);
  void ExecuteSuggestionCallback(GVariant* suggests);
};

}
}

#endif /* UNITY_HUD_H */
