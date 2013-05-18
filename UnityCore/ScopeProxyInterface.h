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

#ifndef UNITY_SCOPE_PROXY_INTERFACE_H
#define UNITY_SCOPE_PROXY_INTERFACE_H

#include "GLibWrapper.h"
#include <NuxCore/Property.h>
#include <dee.h>

#include "Results.h"
#include "Filters.h"
#include "Categories.h"
#include "GLibWrapper.h"
#include "ScopeData.h"

namespace unity
{
namespace dash
{

enum ScopeHandledType
{
  NOT_HANDLED=0,
  SHOW_DASH,
  HIDE_DASH,
  GOTO_DASH_URI,
  SHOW_PREVIEW
};

enum ScopeViewType
{
  HIDDEN=0,
  HOME_VIEW,
  SCOPE_VIEW
};

class ScopeProxyInterface : public sigc::trackable, boost::noncopyable
{
public:
  typedef std::shared_ptr<ScopeProxyInterface> Ptr;

  virtual ~ScopeProxyInterface() {}

  nux::ROProperty<std::string> dbus_name;
  nux::ROProperty<std::string> dbus_path;
  nux::ROProperty<bool> connected;
  nux::ROProperty<std::string> channel;

  nux::ROProperty<bool> visible;
  nux::ROProperty<bool> is_master;
  nux::ROProperty<std::string> search_hint;
  nux::RWProperty<ScopeViewType> view_type;

  nux::ROProperty<Results::Ptr> results;
  nux::ROProperty<Filters::Ptr> filters;
  nux::ROProperty<Categories::Ptr> categories;
  nux::ROProperty<std::vector<unsigned int>> category_order;

  nux::ROProperty<std::string> name;
  nux::ROProperty<std::string> description;
  nux::ROProperty<std::string> icon_hint;
  nux::ROProperty<std::string> category_icon_hint;
  nux::ROProperty<std::vector<std::string>> keywords;
  nux::ROProperty<std::string> type;
  nux::ROProperty<std::string> query_pattern;
  nux::ROProperty<std::string> shortcut;

  virtual void ConnectProxy() = 0;
  virtual void DisconnectProxy() = 0;

  typedef std::function<void(std::string const& search_string, glib::HintsMap const&, glib::Error const&)> SearchCallback;
  virtual void Search(std::string const& search_hint, SearchCallback const& callback, GCancellable* cancellable) = 0;

  typedef std::function<void(LocalResult const&, ScopeHandledType, glib::HintsMap const&, glib::Error const&)> ActivateCallback;
  virtual void Activate(LocalResult const& result, uint activate_type, glib::HintsMap const& hints, ActivateCallback const& callback, GCancellable* cancellable) = 0;

  virtual Results::Ptr GetResultsForCategory(unsigned category) const = 0;
};

} // namespace dash
} // namespace unity

#endif // UNITY_SCOPE_PROXY_INTERFACE_H