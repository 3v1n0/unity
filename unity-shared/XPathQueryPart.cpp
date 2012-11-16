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
 * Authored by: Thomi Richards <thomi.richards@canonical.com>
 */

#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/bind.hpp>
#include <NuxCore/Logger.h>
#include "XPathQueryPart.h"
#include "Introspectable.h"

namespace unity
{

namespace debug
{
DECLARE_LOGGER(logger, "unity.debug.xpath");

// Stores a part of an XPath query.
XPathQueryPart::XPathQueryPart(std::string const& query_part)
{
  std::vector<std::string> part_pieces;
  boost::algorithm::split(part_pieces, query_part, boost::algorithm::is_any_of("[]="));
  // Boost's split() implementation does not match it's documentation! According to the
  // docs, it's not supposed to add empty strings, but it does, which is a PITA. This
  // next line removes them:
  part_pieces.erase( std::remove_if( part_pieces.begin(),
                                      part_pieces.end(),
                                      boost::bind( &std::string::empty, _1 ) ),
                    part_pieces.end());
  if (part_pieces.size() == 1)
  {
    node_name_ = part_pieces.at(0);
  }
  else if (part_pieces.size() == 3)
  {
    node_name_ = part_pieces.at(0);
    param_name_ = part_pieces.at(1);
    param_value_ = part_pieces.at(2);
  }
  else
  {
    LOG_WARNING(logger) << "Malformed query part: " << query_part;
    // assume it's just a node name:
    node_name_ = query_part;
  }
}

bool XPathQueryPart::Matches(Introspectable* node) const
{
  bool matches = false;
  if (param_name_ == "")
  {
    matches = (node_name_ == "*" || node->GetName() == node_name_);
  }
  else
  {
    GVariantBuilder  child_builder;
    g_variant_builder_init(&child_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&child_builder, "{sv}", "id", g_variant_new_uint64(node->GetIntrospectionId()));
    node->AddProperties(&child_builder);
    GVariant* prop_dict = g_variant_builder_end(&child_builder);
    GVariant *prop_value = g_variant_lookup_value(prop_dict, param_name_.c_str(), NULL);

    if (prop_value != NULL)
    {
      GVariantClass prop_val_type = g_variant_classify(prop_value);
      // it'd be nice to be able to do all this with one method. However, the booleans need
      // special treatment, and I can't figure out how to group all the integer types together
      // without resorting to template functions.... and we all know what happens when you
      // start doing that...
      switch (prop_val_type)
      {
        case G_VARIANT_CLASS_STRING:
        {
          const gchar* prop_val = g_variant_get_string(prop_value, NULL);
          if (g_strcmp0(prop_val, param_value_.c_str()) == 0)
          {
            matches = true;
          }
        }
        break;
        case G_VARIANT_CLASS_BOOLEAN:
        {
          std::string value = boost::to_upper_copy(param_value_);
          bool p = value == "TRUE" ||
                    value == "ON" ||
                    value == "YES" ||
                    value == "1";
          matches = (g_variant_get_boolean(prop_value) == p);
        }
        break;
        case G_VARIANT_CLASS_BYTE:
        {
          // It would be nice if I could do all the integer types together, but I couldn't see how...
          std::stringstream stream(param_value_);
          int val; // changing this to guchar causes problems.
          stream >> val;
          matches = (stream.rdstate() & (stream.badbit|stream.failbit)) == 0 &&
                    val == g_variant_get_byte(prop_value);
        }
        break;
        case G_VARIANT_CLASS_INT16:
        {
          std::stringstream stream(param_value_);
          gint16 val;
          stream >> val;
          matches = (stream.rdstate() & (stream.badbit|stream.failbit)) == 0 &&
                    val == g_variant_get_int16(prop_value);
        }
        break;
        case G_VARIANT_CLASS_UINT16:
        {
          std::stringstream stream(param_value_);
          guint16 val;
          stream >> val;
          matches = (stream.rdstate() & (stream.badbit|stream.failbit)) == 0 &&
                    val == g_variant_get_uint16(prop_value);
        }
        break;
        case G_VARIANT_CLASS_INT32:
        {
          std::stringstream stream(param_value_);
          gint32 val;
          stream >> val;
          matches = (stream.rdstate() & (stream.badbit|stream.failbit)) == 0 &&
                    val == g_variant_get_int32(prop_value);
        }
        break;
        case G_VARIANT_CLASS_UINT32:
        {
          std::stringstream stream(param_value_);
          guint32 val;
          stream >> val;
          matches = (stream.rdstate() & (stream.badbit|stream.failbit)) == 0 &&
                    val == g_variant_get_uint32(prop_value);
        }
        break;
        case G_VARIANT_CLASS_INT64:
        {
          std::stringstream stream(param_value_);
          gint64 val;
          stream >> val;
          matches = (stream.rdstate() & (stream.badbit|stream.failbit)) == 0 &&
                    val == g_variant_get_int64(prop_value);
        }
        break;
        case G_VARIANT_CLASS_UINT64:
        {
          std::stringstream stream(param_value_);
          guint64 val;
          stream >> val;
          matches = (stream.rdstate() & (stream.badbit|stream.failbit)) == 0 &&
                    val == g_variant_get_uint64(prop_value);
        }
        break;
      default:
        LOG_WARNING(logger) << "Unable to match against property of unknown type.";
      };
    }
    g_variant_unref(prop_value);
    g_variant_unref(prop_dict);
  }

  return matches;
}

}
}
