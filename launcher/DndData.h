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
* Authored by: Andrea Azzarone <azzaronea@gmail.com>
*/

#ifndef DNDDATA_H
#define DNDDATA_H

#include <map>
#include <set>
#include <string>

namespace unity {

class DndData
{
public:
  /**
   * Fills the object given a list of uris.
   **/
  void Fill(const char* uris);
  
  /**
   * Resets the object. Call this function when no longer need data
   **/
  void Reset();
  
  /**
   * Returns a std::set<std::string> with all the uris.
   **/
  std::set<std::string> Uris() const { return uris_; }
  
  /**
   * Returns a std::set<std::string> with all the types.
   **/
  std::set<std::string> Types() const { return types_; }
  
  /**
   * Returns a std::set<std::string> with all uris of a given type.
   **/
  std::set<std::string> UrisByType(const std::string& type) const { return types_to_uris_.find(type)->second; }
  
  /**
   * Returns a std::set<std::string> with all types of a given uri.
   **/
  std::string TypeByUri(const std::string& uris) { return uris_to_types_.find(uris)->second; }

private:
  std::set<std::string> uris_;
  std::set<std::string> types_;
  std::map<std::string, std::string> uris_to_types_;
  std::map<std::string, std::set<std::string>> types_to_uris_;
};

} // namespace unity

#endif // DNDDATA_H
