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
//
#include "Hud.h"

#include <gio/gio.h>
#include <glib.h>
#include <NuxCore/Logger.h>
#include "GLibWrapper.h"
#include "GLibDBusProxy.h"

#include "config.h"

#include <sigc++/bind.h>

namespace unity
{
namespace hud
{
DECLARE_LOGGER(logger, "unity.hud.impl");

namespace
{
const int request_number_of_results = 6;
}

// Impl classes
class HudImpl
{
public:
  HudImpl(std::string const& dbus_name,
          std::string const& dbus_path,
          Hud *parent)
  : query_key_(NULL)
  , proxy_(dbus_name, dbus_path, "com.canonical.hud")
  , parent_(parent)
  {
    LOG_DEBUG(logger) << "Hud init with name: " << dbus_name << "and path: " << dbus_path;
    proxy_.connected.connect([&]() {
      LOG_DEBUG(logger) << "Hud Connected";
      parent_->connected = true;
    });

    proxy_.Connect("UpdatedQuery", sigc::mem_fun(this, &HudImpl::UpdateQueryCallback));
  }

  void QueryCallback(GVariant* data);
  void UpdateQueryCallback(GVariant* data);
  void BuildQueries(GVariant* query_array);
  void ExecuteByKey(GVariant* key, unsigned int timestamp);
  void ExecuteQueryByStringCallback(GVariant* query, unsigned int timestamp);
  void CloseQuery();

  GVariant* query_key_;
  Hud::Queries queries_;
  glib::DBusProxy proxy_;
  Hud* parent_;
};

void HudImpl::ExecuteByKey(GVariant* key, unsigned int timestamp)
{
  if (!key)
    return;

  LOG_DEBUG(logger) << "Executing by Key";

  GVariantBuilder tuple;
  g_variant_builder_init(&tuple, G_VARIANT_TYPE_TUPLE);
  g_variant_builder_add_value(&tuple, g_variant_new_variant(key));
  g_variant_builder_add_value(&tuple, g_variant_new_uint32(timestamp));

  proxy_.Call("ExecuteQuery", g_variant_builder_end(&tuple));
}

void HudImpl::ExecuteQueryByStringCallback(GVariant* query, unsigned int timestamp)
{
  if (g_variant_n_children(query) < 3)
  {
    LOG_ERROR(logger) << "Received (" << g_variant_n_children(query) << ") children in a query, expected 3";
    return;
  }

  queries_.clear();

  GVariant* query_key = g_variant_get_child_value(query, 2);
  query_key_ = query_key;

  GVariant* queries = g_variant_get_child_value(query, 1);
  BuildQueries(queries);
  g_variant_unref(queries);

  if (queries_.empty() == false)
  {
    // we now execute based off the first result
    ExecuteByKey(queries_.front()->key, timestamp);
    CloseQuery();
  }
}

void HudImpl::QueryCallback(GVariant* query)
{
  if (g_variant_n_children(query) < 3)
  {
    LOG_ERROR(logger) << "Received (" << g_variant_n_children(query) << ") children in a query, expected 3";
    return;
  }
  queries_.clear();

  // extract the information from the GVariants
  GVariant* target = g_variant_get_child_value(query, 0);
  g_variant_unref(target);

  GVariant* query_key = g_variant_get_child_value(query, 2);
  query_key_ = query_key;

  GVariant* queries = g_variant_get_child_value(query, 1);
  BuildQueries(queries);
  g_variant_unref(queries);

  parent_->queries_updated.emit(queries_);
}

void HudImpl::UpdateQueryCallback(GVariant* query)
{
  if (g_variant_n_children(query) < 3)
  {
    LOG_ERROR(logger) << "Received (" << g_variant_n_children(query) << ") children in a query, expected 3";
    return;
  }
  // as we are expecting an update, we want to check
  // and make sure that we are the actual receivers of
  // the signal

  GVariant* query_key = g_variant_get_child_value(query, 2);
  if (query_key_ && g_variant_equal(query_key_, query_key))
  {
    queries_.clear();
    GVariant* queries = g_variant_get_child_value(query, 1);
    BuildQueries(queries);
    g_variant_unref(queries);
    parent_->queries_updated.emit(queries_);
  }
}

void HudImpl::BuildQueries(GVariant* query_array)
{
  GVariantIter iter;
  g_variant_iter_init(&iter, query_array);
  gchar* formatted_text;
  gchar* icon;
  gchar* item_icon;
  gchar* completion_text;
  gchar* shortcut;
  GVariant* key = NULL;

  while (g_variant_iter_loop(&iter, "(sssssv)",
         &formatted_text, &icon, &item_icon, &completion_text, &shortcut, &key))
  {
    queries_.push_back(Query::Ptr(new Query(formatted_text,
                                            icon,
                                            item_icon,
                                            completion_text,
                                            shortcut,
                                            key)));
  }
}

void HudImpl::CloseQuery()
{
  if (query_key_ == NULL)
  {
    LOG_WARN(logger) << "Attempted to close the hud connection without starting it";
  }
  else
  {
    GVariant* paramaters = g_variant_new("(v)", query_key_);
    proxy_.Call("CloseQuery", paramaters);
    g_variant_unref(query_key_);
    query_key_ = NULL;
    queries_.clear();
  }
}


Hud::Hud(std::string const& dbus_name,
         std::string const& dbus_path)
    : connected(false)
    , pimpl_(new HudImpl(dbus_name, dbus_path, this))
{
  pimpl_->parent_ = this;
}

Hud::~Hud()
{
  delete pimpl_;
}

void Hud::RequestQuery(std::string const& search_string)
{
  LOG_DEBUG(logger) << "Getting Query: " << search_string;
  if (pimpl_->query_key_ != NULL)
  {
    CloseQuery();
  }

  GVariant* paramaters = g_variant_new("(si)",
                                       search_string.c_str(),
                                       request_number_of_results);
  pimpl_->proxy_.Call("StartQuery", paramaters, sigc::mem_fun(this->pimpl_, &HudImpl::QueryCallback));
}


void Hud::ExecuteQuery(Query::Ptr query, unsigned int timestamp)
{
  LOG_DEBUG(logger) << "Executing query: " << query->formatted_text;
  pimpl_->ExecuteByKey(query->key, timestamp);
}

void Hud::ExecuteQueryBySearch(std::string execute_string, unsigned int timestamp)
{
  //Does a search then executes the result based on that search
  LOG_DEBUG(logger) << "Executing by string" << execute_string;
  if (pimpl_->query_key_ != NULL)
  {
    CloseQuery();
  }

  GVariant* paramaters = g_variant_new("(si)",
                                       execute_string.c_str(),
                                       1);

  auto functor = sigc::mem_fun(this->pimpl_, &HudImpl::ExecuteQueryByStringCallback);

  pimpl_->proxy_.Call("StartQuery", paramaters, sigc::bind(functor, timestamp));
}

void Hud::CloseQuery()
{
  if (pimpl_->query_key_)
    pimpl_->CloseQuery();
}

}
}
