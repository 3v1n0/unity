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

namespace unity
{
namespace hud
{

namespace
{
nux::logging::Logger logger("unity.hud.hud");
}

// Impl classes
class HudImpl
{
public:
  HudImpl(std::string const& dbus_name,
          std::string const& dbus_path)
  : proxy_(dbus_name, dbus_path, "com.canonical.hud")
  {    
  }
  void SuggestionCallback(GVariant* data);
  void ExecuteSuggestionCallback(GVariant* suggests);
  void ExecuteByKey(GVariant* key);

  Hud::Suggestions suggestions_;
  glib::DBusProxy proxy_;
  Hud* parent_;
};

void HudImpl::ExecuteByKey(GVariant* key)
{
  LOG_DEBUG(logger) << "Executing by Key";
  
  GVariantBuilder tuple;
  g_variant_builder_init(&tuple, G_VARIANT_TYPE_TUPLE);
  g_variant_builder_add_value(&tuple, g_variant_new_variant(key));
  g_variant_builder_add_value(&tuple, g_variant_new_uint32(0));
  
  proxy_.Call("ExecuteQuery", g_variant_builder_end(&tuple));
}

void HudImpl::ExecuteSuggestionCallback(GVariant* suggests)
{
  if (suggests == nullptr)
  {
    LOG_ERROR(logger) << "received null suggestion value";
    return;
  }
  
  GVariant * target = g_variant_get_child_value(suggests, 0);
  g_variant_unref(target);
  
  GVariant* suggestions = g_variant_get_child_value(suggests, 1);
  GVariantIter iter;
  g_variant_iter_init(&iter, suggestions);
  glib::String suggestion;
  glib::String icon;
  glib::String item_icon;
  glib::String completion_text;
  GVariant* key = NULL;
  
  while (g_variant_iter_loop(&iter, "(ssssv)", &suggestion, &icon, &item_icon, &completion_text, &key))
  {
    LOG_DEBUG(logger) << "Attempting to execute suggestion: " << suggestion;
    ExecuteByKey(key);
    break;
  }
  
  g_variant_unref(suggestions);
}

void HudImpl::SuggestionCallback(GVariant* suggests)
{
  suggestions_.clear();
  
  // extract the information from the GVariants
  GVariant * target = g_variant_get_child_value(suggests, 0);
  g_variant_unref(target);
  
  GVariant* suggestions = g_variant_get_child_value(suggests, 1);
  GVariantIter iter;
  g_variant_iter_init(&iter, suggestions);
  glib::String suggestion;
  glib::String icon;
  glib::String item_icon;
  glib::String completion_text;
  GVariant* key = NULL;
  
  while (g_variant_iter_loop(&iter, "(ssssv)", &suggestion, &icon, &item_icon, &completion_text, &key))
  {
    LOG_DEBUG(logger) << "Found suggestion: " << suggestion;
    g_variant_ref(key);
    suggestions_.push_back(Suggestion::Ptr(new Suggestion(std::string(suggestion), std::string(icon),
                                                          std::string(item_icon), std::string(completion_text),
                                                          key)));
  }
  
  g_variant_unref(suggestions);
  
  parent_->suggestion_search_finished.emit(suggestions_);
}


Hud::Hud(std::string const& dbus_name,
         std::string const& dbus_path)
    : pimpl_(new HudImpl(dbus_name, dbus_path))
{
  pimpl_->parent_ = this;
}

Hud::~Hud()
{
  delete pimpl_;
}

void Hud::GetSuggestions(std::string const& search_string)
{
  LOG_DEBUG(logger) << "Getting suggestions: " << search_string;
  GVariant* paramaters = g_variant_new("(s)", search_string.c_str());
  pimpl_->proxy_.Call("GetSuggestions", paramaters, sigc::mem_fun(this->pimpl_, &HudImpl::SuggestionCallback));
}

void Hud::Execute(std::string const& execute_string)
{
  // we do a search and execute based on the results of that search
  LOG_DEBUG(logger) << "Executing string: " << execute_string;
  GVariant* paramaters = g_variant_new("(s)", execute_string.c_str());
  pimpl_->proxy_.Call("GetSuggestions", paramaters, sigc::mem_fun(this->pimpl_, &HudImpl::ExecuteSuggestionCallback));
}


void Hud::ExecuteBySuggestion(Suggestion const& suggestion)
{
  LOG_DEBUG(logger) << "Executing by Suggestion: " << suggestion.formatted_text;
  if (suggestion.key == nullptr)
  {
    LOG_ERROR(logger) << "Tried to execute suggestion with no key: " << suggestion.formatted_text;
  }
  else
  {
    pimpl_->ExecuteByKey(suggestion.key);
    pimpl_->suggestions_.clear();
  }
}

}
}
