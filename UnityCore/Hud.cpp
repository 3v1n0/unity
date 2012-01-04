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

#include "Hud.h"

#include <gio/gio.h>
#include <glib.h>
#include <NuxCore/Logger.h>
#include "GLibWrapper.h"

#include "config.h"

namespace unity
{
namespace hud
{

namespace
{
nux::logging::Logger logger("unity.hud.hud");
}
//
Hud::Hud(std::string const& dbus_name,
         std::string const& dbus_path)
    : proxy_(dbus_name, dbus_path, "com.canonical.hud")
{

}

Hud::~Hud()
{

}

void Hud::GetSuggestions(std::string const& search_string)
{
  LOG_DEBUG(logger) << "Getting suggestions: " << search_string;
  GVariant* paramaters = g_variant_new("(s)", search_string.c_str());
  proxy_.Call("GetSuggestions", paramaters, sigc::mem_fun(this, &Hud::SuggestionCallback));
}

void Hud::Execute(std::string const& execute_string)
{
  // we do a search and execute based on the results of that search
  LOG_DEBUG(logger) << "Executing string: " << execute_string;
  GVariant* paramaters = g_variant_new("(s)", execute_string.c_str());
  proxy_.Call("GetSuggestions", paramaters, sigc::mem_fun(this, &Hud::ExecuteSuggestionCallback));
}

void Hud::ExecuteSuggestionCallback(GVariant* suggests)
{
  GVariant* suggestions = g_variant_get_child_value(suggests, 2);
  GVariantIter iter;
  g_variant_iter_init(&iter, suggestions);
  glib::String suggestion;
  glib::String icon;
  GVariant* key = NULL;

  while (g_variant_iter_loop(&iter, "(ssv)", &suggestion, &icon, &key))
  {
    LOG_DEBUG(logger) << "Attempting to execute suggestion: " << suggestion;
    ExecuteByKey(key);
    break;
  }

  g_variant_unref(suggestions);
}

void Hud::ExecuteByKey(GVariant* key)
{
  LOG_DEBUG(logger) << "Executing by key";

  GVariantBuilder tuple;
  g_variant_builder_init(&tuple, G_VARIANT_TYPE_TUPLE);
  g_variant_builder_add_value(&tuple, g_variant_new_variant(key));
  g_variant_builder_add_value(&tuple, g_variant_new_uint32(0));

  proxy_.Call("ExecuteQuery", g_variant_builder_end(&tuple));
}


void Hud::SuggestionCallback(GVariant* suggests)
{
  // we first have to loop through the previous suggestions and unref
  // all the variants
  // !!FIXME!! - create a Variant class in GlibWrapper so scoping handles this for us

  for (auto it = suggestions_.begin(); it != suggestions_.end(); it++)
  {
    // get the third element in our tuple
    GVariant* key = std::get<2>((*it));
    g_variant_unref(key);
  }
  suggestions_.clear();

  // extract the information from the GVariants
  // target
  GVariant * vtarget = g_variant_get_child_value(suggests, 1);
  target = g_variant_get_string(vtarget, NULL);
  g_variant_unref(vtarget);

  LOG_DEBUG(logger) << "Got new target: " << target();

  // icon
  GVariant * vicon = g_variant_get_child_value(suggests, 0);
  target_icon = g_variant_get_string(vicon, NULL);
  g_variant_unref(vicon);

  LOG_DEBUG(logger) << "Got new icon: " << target_icon();

  GVariant* suggestions = g_variant_get_child_value(suggests, 2);
  GVariantIter iter;
  g_variant_iter_init(&iter, suggestions);
  glib::String suggestion;
  glib::String icon;
  GVariant* key = NULL;

  while (g_variant_iter_loop(&iter, "(ssv)", &suggestion, &icon, &key))
  {
    LOG_DEBUG(logger) << "Found suggestion: " << suggestion;
    g_variant_ref(key);
    suggestions_.push_back(Suggestion(std::string(suggestion), std::string(icon), key));
  }

  g_variant_unref(suggestions);

  suggestion_search_finished.emit(suggestions_);
}
}
}
