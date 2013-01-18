// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */


#include "ScopeData.h"
#include <unity-protocol.h>
#include <NuxCore/Logger.h>

#include "GLibWrapper.h"

namespace unity
{
namespace dash
{
DECLARE_LOGGER(logger, "unity.dash.scopedata");

namespace
{
  static void safe_delete_scope_metadata(UnityProtocolScopeRegistryScopeMetadata* data)
  {
    if (!data) return;
    unity_protocol_scope_registry_scope_metadata_unref(data);
  }
}

ScopeData::ScopeData()
: visible(true)
, always_search(false)
{
}

ScopeData::Ptr ScopeData::ReadProtocolDataForId(std::string const& scope_id)
{
  ScopeData::Ptr data(new ScopeData());

  glib::Error error;
  std::shared_ptr<UnityProtocolScopeRegistryScopeMetadata> meta_data(unity_protocol_scope_registry_scope_metadata_for_id(scope_id.c_str(), &error),
                                                                     safe_delete_scope_metadata);

  if (error)
  {
    data->id = scope_id;
    LOG_DEBUG(logger) << "Error fetching metadata for scope: " << scope_id << " : " << error.Message();
  }
  else if (meta_data)
  {
    data->dbus_name = glib::StringRef(meta_data->dbus_name);
    data->dbus_path = glib::StringRef(meta_data->dbus_path);

    data->id = glib::StringRef(meta_data->id);
    data->full_path = glib::StringRef(meta_data->full_path);
    data->name = glib::StringRef(meta_data->name);
    data->icon_hint = glib::StringRef(meta_data->icon);
    data->category_icon_hint = glib::StringRef(meta_data->category_icon);
    data->type = meta_data->type;
    data->description = glib::StringRef(meta_data->description);
    data->shortcut = glib::StringRef(meta_data->shortcut);
    data->search_hint = glib::StringRef(meta_data->search_hint);
    data->is_master = meta_data->is_master;
    data->query_pattern = glib::StringRef(meta_data->query_pattern);

    std::vector<std::string> keywords;
    for (GSList* v = meta_data->keywords; v; v = g_slist_next(v))
    {
      std::string value(static_cast<gchar*>(v->data));
      if (value.empty())
        continue;

      keywords.push_back(value);
    }
    data->keywords = keywords;

    LOG_DEBUG(logger) << "Creating scope: " << scope_id << " (" << data->dbus_path()  << " @ " << data->dbus_name() << ")";
  }
  return data;
}

}
}