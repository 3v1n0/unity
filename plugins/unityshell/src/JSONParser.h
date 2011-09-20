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
 * Authored by: Tim Penhey <tim.penhey@canonical.com>
 *              Mirco Müller <mirco.mueller@canonical.com
 */
#ifndef UNITY_CORE_JSON_PARSER_H
#define UNITY_CORE_JSON_PARSER_H

#include <string>
#include <vector>
#include <map>

#include <boost/algorithm/string.hpp>

#include <NuxCore/Color.h>

#include <json-glib/json-glib.h>

#include "UnityCore/GLibWrapper.h"

namespace unity
{
namespace json
{

class Parser
{
public:
  Parser();

  bool Open(std::string const& filename);

  void ReadInt(std::string const& node_name,
               std::string const& member_name,
               int& value) const;
  void ReadInts(std::string const& node_name,
                std::string const& member_name,
                std::vector<int>& values) const;

  void ReadDouble(std::string const& node_name,
                  std::string const& member_name,
                  double& value) const;
  void ReadDoubles(std::string const& node_name,
                   std::string const& member_name,
                   std::vector<double>& values) const;

  void ReadColor(std::string const& node_name,
                 std::string const& member_name,
                 std::string const& opacity_name,
                 nux::Color& color) const;
  void ReadColors(std::string const& node_name,
                  std::string const& member_name,
                  std::string const& opacity_name,
                  std::vector<nux::Color>& colors) const;

  template <typename T>
  void ReadMappedString(std::string const& node_name,
                        std::string const& member_name,
                        std::map<std::string, T> const& mapping,
                        T& value) const;
  template <typename T>
  void ReadMappedStrings(std::string const& node_name,
                         std::string const& member_name,
                         std::map<std::string, T> const& mapping,
                         std::vector<T>& values) const;

private:
  JsonObject* GetNodeObject(std::string const& node_name) const;
  JsonArray* GetArray(std::string const& node_name,
                      std::string const& member_name) const;

private:
  glib::Object<JsonParser> parser_;
  JsonNode* root_;
};


template <typename T>
void Parser::ReadMappedStrings(std::string const& node_name,
                               std::string const& member_name,
                               std::map<std::string, T> const& mapping,
                               std::vector<T>& values) const
{
  JsonArray* array = GetArray(node_name, member_name);
  if (!array)
    return;

  std::size_t size = std::min<size_t>(json_array_get_length(array),
                                      values.size());
  for (std::size_t i = 0; i < size; ++i)
  {
    std::string key(json_array_get_string_element(array, i));
    boost::to_lower(key);
    auto it = mapping.find(key);
    if (it != mapping.end())
      values[i] = it->second;
  }
}

template <typename T>
void Parser::ReadMappedString(std::string const& node_name,
                              std::string const& member_name,
                              std::map<std::string, T> const& mapping,
                              T& value) const
{
  JsonObject* object = GetNodeObject(node_name);
  if (!object)
    return;

  std::string key(json_object_get_string_member(object, member_name.c_str()));
  boost::to_lower(key);
  auto it = mapping.find(key);
  if (it != mapping.end())
    value = it->second;
}


}
}


#endif
