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

void Hud::Execute(std::string const& query)
{
  LOG_DEBUG(logger) << "Executing query: " << query;
  GVariant* paramaters = g_variant_new("(s)", query.c_str());
  proxy_.Call("ExecuteQuery", paramaters);
}


void Hud::SuggestionCallback(GVariant* data)
{
  LOG_DEBUG(logger) << "Suggestion callback";
  gchar *ctarget = NULL;
  gchar *suggestion = NULL;
  GVariantIter *iter = NULL;

  suggestions_.clear();

  g_variant_get(data, "(sas)", &ctarget, &iter);
  while (g_variant_iter_loop(iter, "s", &suggestion))
    suggestions_.push_back(std::string(suggestion));

  g_variant_iter_free(iter);

  suggestion_search_finished.emit(suggestions_);
}
}
}
