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
 *
 */

#include "Scope.h"
#include "MiscUtils.h"
#include "ScopeProxy.h"
#include <unity-protocol.h>


namespace unity
{
namespace dash
{
DECLARE_LOGGER(logger, "unity.dash.scope");

class Scope::Impl
{
public:
  Impl(Scope* owner, ScopeData::Ptr const& scope_data);
  ~Impl();

  void Init();

  void Activate(LocalResult const& result, guint action_type, glib::HintsMap const& hints, ActivateCallback const& callback, GCancellable* cancellable);
  void OnActivateResultReply(LocalResult const& result, ScopeHandledType handled_type, glib::HintsMap const& hints, glib::Error const& error);

  void Preview(LocalResult const& result, glib::HintsMap const& hints, PreviewCallback const& callback, GCancellable* cancellable);
  void OnPreviewReply(LocalResult const& result, ScopeHandledType handled, glib::HintsMap const& hints, glib::Error const& error, PreviewCallback const& callback);

  DeeFilter* GetFilterForCategory(unsigned category, DeeFilter* filter)  const;

  Scope* owner_;
  ScopeData::Ptr scope_data_;
  ScopeProxyInterface::Ptr proxy_;

  typedef std::shared_ptr<sigc::connection> ConnectionPtr;
  std::vector<ConnectionPtr> property_connections;
};

Scope::Impl::Impl(Scope* owner, ScopeData::Ptr const& scope_data)
: owner_(owner)
, scope_data_(scope_data)
{
  property_connections.push_back(utils::ConnectProperties(owner_->id, scope_data_->id));
}

Scope::Impl::~Impl()
{
  for_each(property_connections.begin(), property_connections.end(), [](ConnectionPtr const& con) { con->disconnect(); });
  property_connections.clear();
}

void Scope::Impl::Init()
{
  proxy_ = owner_->CreateProxyInterface();

  if (proxy_)
  {
    property_connections.push_back(utils::ConnectProperties(owner_->connected, proxy_->connected));
    property_connections.push_back(utils::ConnectProperties(owner_->is_master, proxy_->is_master));
    property_connections.push_back(utils::ConnectProperties(owner_->search_hint, proxy_->search_hint));
    property_connections.push_back(utils::ConnectProperties(owner_->view_type, proxy_->view_type));
    property_connections.push_back(utils::ConnectProperties(owner_->results, proxy_->results));
    property_connections.push_back(utils::ConnectProperties(owner_->filters, proxy_->filters));
    property_connections.push_back(utils::ConnectProperties(owner_->categories, proxy_->categories));
    property_connections.push_back(utils::ConnectProperties(owner_->category_order, proxy_->category_order));

    property_connections.push_back(utils::ConnectProperties(owner_->name, proxy_->name));
    property_connections.push_back(utils::ConnectProperties(owner_->description, proxy_->description));
    property_connections.push_back(utils::ConnectProperties(owner_->icon_hint, proxy_->icon_hint));
    property_connections.push_back(utils::ConnectProperties(owner_->category_icon_hint, proxy_->category_icon_hint));
    property_connections.push_back(utils::ConnectProperties(owner_->keywords, proxy_->keywords));
    property_connections.push_back(utils::ConnectProperties(owner_->type, proxy_->type));
    property_connections.push_back(utils::ConnectProperties(owner_->query_pattern, proxy_->query_pattern));
    property_connections.push_back(utils::ConnectProperties(owner_->shortcut, proxy_->shortcut));
    property_connections.push_back(utils::ConnectProperties(owner_->visible, proxy_->visible));

    property_connections.push_back(utils::ConnectProperties(owner_->results, proxy_->results));
    property_connections.push_back(utils::ConnectProperties(owner_->filters, proxy_->filters));
    property_connections.push_back(utils::ConnectProperties(owner_->categories, proxy_->categories));
  }
}

void Scope::Impl::Activate(LocalResult const& result, guint action_type, glib::HintsMap const& hints, ActivateCallback const& callback, GCancellable* cancellable)
{
  proxy_->Activate(result,
                   action_type,
                   hints,
                   [this, callback] (LocalResult const& result, ScopeHandledType handled_type, glib::HintsMap const& hints, glib::Error const& error) {
      if (callback)
        callback(result, handled_type, error);
      OnActivateResultReply(result, handled_type, hints, error);
    },
    cancellable);
}

void Scope::Impl::OnActivateResultReply(LocalResult const& result, ScopeHandledType handled, glib::HintsMap const& hints, glib::Error const& error)
{
  LOG_DEBUG(logger) << "Activation reply (handled:" << handled << ", error: " << (error ? "true" : "false") << ") for " <<  result.uri;

  if (static_cast<UnityProtocolHandledType>(handled) == UNITY_PROTOCOL_HANDLED_TYPE_SHOW_PREVIEW)
  {
    auto iter = hints.find("preview");
    if (iter != hints.end())
    {
      glib::Variant v = iter->second;

      Preview::Ptr preview(Preview::PreviewForVariant(v));
      if (preview)
      {
        // would be nice to make parent_scope_ a shared_ptr,
        // but that's not really doable from here
        preview->parent_scope = owner_;
        preview->preview_result = result;
        owner_->preview_ready.emit(result, preview);
        return;
      }
    }

    LOG_WARNING(logger) << "Unable to deserialize Preview";
  }
  else
  {
    owner_->activated.emit(result, handled, hints);
  }
}

void Scope::Impl::Preview(LocalResult const& result, glib::HintsMap const& hints, PreviewCallback const& callback, GCancellable* cancellable)
{
  proxy_->Activate(result,
                   UNITY_PROTOCOL_ACTION_TYPE_PREVIEW_RESULT,
                   hints,
                   [this, callback] (LocalResult const& result, ScopeHandledType handled_type, glib::HintsMap const& hints, glib::Error const& error) {
                      OnPreviewReply(result, handled_type, hints, error, callback);
                   },
                   cancellable);
}

void Scope::Impl::OnPreviewReply(LocalResult const& result, ScopeHandledType handled, glib::HintsMap const& hints, glib::Error const& error, PreviewCallback const& callback)
{
  LOG_DEBUG(logger) << "Activation reply (handled:" << handled << ", error: " << (error ? "true" : "false") << ") for " <<  result.uri;

  if (!error)
  {
    Preview::Ptr preview;
    if (static_cast<UnityProtocolHandledType>(handled) == UNITY_PROTOCOL_HANDLED_TYPE_SHOW_PREVIEW)
    {
      auto iter = hints.find("preview");
      if (iter != hints.end())
      {
        glib::Variant v = iter->second;
        preview = Preview::PreviewForVariant(v);
      }
      else
      {
        LOG_WARNING(logger) << "Unable to deserialize Preview";
      }
    }

    glib::Error err_local;
    if (preview)
    {
      // would be nice to make parent_scope_ a shared_ptr,
      // but that's not really doable from here
      preview->parent_scope = owner_;
      preview->preview_result = result;
      owner_->preview_ready.emit(result, preview);
    }
    else
    {
      owner_->activated.emit(result, handled, hints);

      GError** real_err = &err_local;
      *real_err = g_error_new_literal(G_SCOPE_ERROR, G_SCOPE_ERROR_INVALID_PREVIEW,
                                      "Not a preview");
    }
    if (callback)
      callback(result, preview, err_local);
  }
  else if (callback)
  {
    callback(result, Preview::Ptr(), error);
  }
}

Scope::Scope(ScopeData::Ptr const& scope_data)
: pimpl(new Impl(this, scope_data))
{
}

Scope::~Scope()
{
}

void Scope::Init()
{
  pimpl->Init();
}

void Scope::Connect()
{
  if (pimpl->proxy_->connected())
    return;
  
  pimpl->proxy_->ConnectProxy();
}

void Scope::Search(std::string const& search_hint, SearchCallback const& callback, GCancellable* cancellable)
{
  return pimpl->proxy_->Search(search_hint, callback, cancellable);
}

void Scope::Activate(LocalResult const& result, ActivateCallback const& callback, GCancellable* cancellable)
{
  pimpl->Activate(result, UNITY_PROTOCOL_ACTION_TYPE_ACTIVATE_RESULT, glib::HintsMap(), callback, cancellable);
}

void Scope::Preview(LocalResult const& result, PreviewCallback const& callback, GCancellable* cancellable)
{
  pimpl->Preview(result, glib::HintsMap(), callback, cancellable);
}

void Scope::ActivatePreviewAction(std::string const& action_id,
                                  LocalResult const& result,
                                  glib::HintsMap const& hints,
                                  ActivateCallback const& callback,
                                  GCancellable* cancellable)
{
  glib::HintsMap tmp_hints = hints;
  tmp_hints["preview-action-id"] = g_variant_new_string(action_id.c_str());
  pimpl->Activate(result, UNITY_PROTOCOL_ACTION_TYPE_PREVIEW_ACTION, tmp_hints, callback, cancellable);
}

Results::Ptr Scope::GetResultsForCategory(unsigned category) const
{
  return pimpl->proxy_->GetResultsForCategory(category);
}

ScopeProxyInterface::Ptr Scope::CreateProxyInterface() const
{
  return ScopeProxyInterface::Ptr(new ScopeProxy(pimpl->scope_data_));
}


} // namespace dash
} // namespace unity
