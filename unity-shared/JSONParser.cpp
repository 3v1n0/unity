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

#include "JSONParser.h"

#include <pango/pango.h>

#include <NuxCore/Logger.h>

namespace unity
{
namespace json
{
DECLARE_LOGGER(logger, "unity.json");
namespace
{
nux::Color ColorFromPango(const gchar* color_string)
{
  static const float PANGO_MAX = 0xffff;
  PangoColor color = {0, 0, 0};
  ::pango_color_parse(&color, color_string);
  return nux::Color(color.red / PANGO_MAX,
                    color.green / PANGO_MAX,
                    color.blue  / PANGO_MAX);
}

}

Parser::Parser()
  : root_(nullptr)
{
}

bool Parser::Open(std::string const& filename)
{
  glib::Error error;
  parser_ = json_parser_new();
  gboolean result = json_parser_load_from_file(parser_, filename.c_str(), &error);
  if (!result)
  {
    LOG_WARN(logger) << "Failure: " << error;
    return false;
  }

  root_ = json_parser_get_root(parser_); // not ref'ed

  if (JSON_NODE_TYPE(root_) != JSON_NODE_OBJECT)
  {
    LOG_WARN(logger) << "Root node is not an object, fail.  It's an: "
                     << json_node_type_name(root_);
    return false;
  }
  return true;
}

JsonObject* Parser::GetNodeObject(std::string const& node_name) const
{
  if (!root_)
    return nullptr;

  JsonObject* object = json_node_get_object(root_);
  JsonNode* node = json_object_get_member(object, node_name.c_str());
  return json_node_get_object(node);
}

JsonArray* Parser::GetArray(std::string const& node_name,
                            std::string const& member_name) const
{
  JsonObject* object = GetNodeObject(node_name);
  if (object)
    return json_object_get_array_member(object, member_name.c_str());
  return nullptr;
}


void Parser::ReadInt(std::string const& node_name,
                     std::string const& member_name,
                     int& value) const
{
  JsonObject* object = GetNodeObject(node_name);

  if (!object)
    return;

  value = json_object_get_int_member(object, member_name.c_str());
}

void Parser::ReadInts(std::string const& node_name,
                      std::string const& member_name,
                      std::vector<int>& values) const
{
  JsonArray* array = GetArray(node_name, member_name);

  if (!array)
    return;

  std::size_t size = std::min<size_t>(json_array_get_length(array),
                                      values.size());
  for (std::size_t i = 0; i < size; ++i)
    values[i] = json_array_get_int_element(array, i);
}

void Parser::ReadDouble(std::string const& node_name,
                        std::string const& member_name,
                        double& value) const
{
  JsonObject* object = GetNodeObject(node_name);

  if (!object)
    return;

  value = json_object_get_double_member(object, member_name.c_str());
}

void Parser::ReadDoubles(std::string const& node_name,
                         std::string const& member_name,
                         std::vector<double>& values) const
{
  JsonArray* array = GetArray(node_name, member_name);

  if (!array)
    return;

  std::size_t size = std::min<size_t>(json_array_get_length(array),
                                      values.size());
  for (std::size_t i = 0; i < size; ++i)
    values[i] = json_array_get_double_element(array, i);
}

void Parser::ReadColor(std::string const& node_name,
                       std::string const& member_name,
                       std::string const& opacity_name,
                       nux::Color& color) const
{
  JsonObject* object = GetNodeObject(node_name);

  if (!object)
    return;

  color = ColorFromPango(json_object_get_string_member(object, member_name.c_str()));
  color.alpha = json_object_get_double_member(object, opacity_name.c_str());
}

void Parser::ReadColors(std::string const& node_name,
                        std::string const& member_name,
                        std::string const& opacity_name,
                        std::vector<nux::Color>& colors) const
{
  JsonArray* array = GetArray(node_name, member_name);
  if (!array)
    return;

  std::size_t size = std::min<size_t>(json_array_get_length(array),
                                      colors.size());
  for (std::size_t i = 0; i < size; ++i)
  {
    colors[i] = ColorFromPango(json_array_get_string_element(array, i));
  }

  array = GetArray(node_name, opacity_name);
  if (!array)
    return;
  size = std::min<size_t>(json_array_get_length(array),
                          colors.size());
  for (std::size_t i = 0; i < size; ++i)
    colors[i].alpha = json_array_get_double_element(array, i);
}



} // namespace json
} // namespace unity
