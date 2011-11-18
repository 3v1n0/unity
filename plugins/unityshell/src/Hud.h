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
#include <UnityCore/GLibDBusProxy.h>
#include <UnityCore/GLibWrapper.h>

namespace unity
{
namespace hud
{

class Hud
{
public:
  typedef std::shared_ptr<Hud> Ptr;
  typedef std::tuple<std::string, std::string, GVariant*> Suggestion;
  typedef std::deque<Suggestion> Suggestions;

  Hud(std::string const& dbus_name,
      std::string const& dbus_path);

  ~Hud();

  nux::Property<std::string> target;

  void GetSuggestions(std::string const& search_string);
  void Execute(std::string const& execute_string);
  void ExecuteByKey(GVariant* key);

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
