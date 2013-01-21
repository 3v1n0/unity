// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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

#include "ScopeProxy.h"
#include "GLibSignal.h"
#include "MiscUtils.h"

#include <unity-protocol.h>
#include <NuxCore/Logger.h>
#include "GLibSource.h"

namespace unity
{
namespace dash
{

namespace
{
const int PROXY_CONNECT_TIMEOUT = 2000;

const unsigned CATEGORY_COLUMN = 2;
}


DECLARE_LOGGER(logger, "unity.dash.scopeproxy");

class ScopeProxy::Impl
{
public:
  Impl(ScopeProxy*const owner, ScopeData::Ptr const& scope_data);
  ~Impl();

  static ScopeData::Ptr CreateData(std::string const& dbus_name, std::string const& dbus_path);

  void CreateProxy();
  void DestroyProxy();

  void OnNewScope(GObject *source_object, GAsyncResult *res);
  void OnChannelOpened(GObject *source_object, GAsyncResult *res);

  void Search(std::string const& search_string, glib::HintsMap const& hints, SearchCallback const& callback, GCancellable* cancel);
  void Activate(std::string const& uri, uint activate_type, glib::HintsMap const& hints, ScopeProxy::ActivateCallback const& callback, GCancellable* cancellable);
  void UpdatePreviewProperty(std::string const& uri, glib::HintsMap const& hints, UpdatePreviewPropertyCallback const& callback, GCancellable* cancellable);

  bool set_view_type(ScopeViewType const& view_type)
  {
    if (scope_proxy_ && unity_protocol_scope_proxy_get_view_type(scope_proxy_) != static_cast<UnityProtocolViewType>(view_type))
    {
      unity_protocol_scope_proxy_set_view_type(scope_proxy_, static_cast<UnityProtocolViewType>(view_type));
      return true;
    }
    return false;
  }

  Results::Ptr results() { return results_; }
  Filters::Ptr filters() { return filters_; }
  Categories::Ptr categories() { return categories_; }

  DeeFilter* GetFilterForCategory(unsigned category, DeeFilter* filter) const;

  ScopeProxy*const owner_;
  ScopeData::Ptr scope_data_;

  // scope proxy properties
  nux::Property<bool> search_in_global;
  nux::Property<ScopeViewType> view_type;


  nux::Property<bool> connected;
  nux::Property<std::string> channel;
  std::string last_search_;

  glib::Object<UnityProtocolScopeProxy> scope_proxy_;
  glib::Object<GCancellable> cancel_scope_;

  bool proxy_created_;

  Results::Ptr results_;
  Filters::Ptr filters_;
  Categories::Ptr categories_;

  typedef std::shared_ptr<sigc::connection> ConnectionPtr;
  std::vector<ConnectionPtr> property_connections;
  sigc::connection filters_change_connection;

  /////////////////////////////////////////////////
  // DBus property signals.
  glib::Signal<void, UnityProtocolScopeProxy*> channels_invalidated_;
  glib::Signal<void, UnityProtocolScopeProxy*, GParamSpec*> visible_signal_;
  glib::Signal<void, UnityProtocolScopeProxy*, GParamSpec*> search_in_global_signal_;
  glib::Signal<void, UnityProtocolScopeProxy*, GParamSpec*> is_master_signal_;
  glib::Signal<void, UnityProtocolScopeProxy*, GParamSpec*> search_hint_signal_;
  glib::Signal<void, UnityProtocolScopeProxy*, GParamSpec*> view_type_signal_;
  glib::Signal<void, UnityProtocolScopeProxy*, GParamSpec*> filters_signal_;
  glib::Signal<void, UnityProtocolScopeProxy*, GParamSpec*> categories_signal_;
  glib::Signal<void, UnityProtocolScopeProxy*, GParamSpec*> metadata_signal_;
  glib::Signal<void, UnityProtocolScopeProxy*, GParamSpec*> optional_metadata_signal_;
  /////////////////////////////////////////////////

private:
  /////////////////////////////////////////////////
  // Signal Connections
  void OnScopeChannelsInvalidated(UnityProtocolScopeProxy* proxy);
  void OnScopeVisibleChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeIsMasterChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeSearchInGlobalChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeSearchHintChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeViewTypeChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeFiltersChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeCategoriesChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeMetadataChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeOptionalMetadataChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  /////////////////////////////////////////////////

  void WaitForProxyConnection(GCancellable* cancellable,
                                   int timeout_msec,
                                   std::function<void(glib::Error const&)> const& callback);

  /////////////////////////////////////
  // Search Callback
  struct SearchData
  {
    SearchCallback callback;
  };

  static void OnScopeSearchCallback(GObject *source_object, GAsyncResult *res, gpointer user_data)
  {
    std::unique_ptr<SearchData> data(static_cast<SearchData*>(user_data));
    glib::Error error;
    GHashTable* hint_ret = unity_protocol_scope_proxy_search_finish(UNITY_PROTOCOL_SCOPE_PROXY(source_object), res, &error);

    glib::HintsMap hints;
    glib::hintsmap_from_hashtable(hint_ret, hints);

    if (data->callback)
      data->callback(hints, error);
    if (hint_ret) { g_hash_table_destroy(hint_ret); }
  }
  /////////////////////////////////////

  /////////////////////////////////////
  // Search Callback
  struct UpdatePreviewPropertyData
  {
    UpdatePreviewPropertyCallback callback;
  };

  static void OnScopeUpdatePreviewPropertyCallback(GObject *source_object, GAsyncResult *res, gpointer user_data)
  {
    std::unique_ptr<UpdatePreviewPropertyData> data(static_cast<UpdatePreviewPropertyData*>(user_data));
    glib::Error error;
    GHashTable* hint_ret = unity_protocol_scope_proxy_update_preview_property_finish(UNITY_PROTOCOL_SCOPE_PROXY(source_object), res, &error);

    glib::HintsMap hints;
    glib::hintsmap_from_hashtable(hint_ret, hints);

    if (data->callback)
      data->callback(hints, error);

    if (hint_ret) { g_hash_table_destroy(hint_ret); }
  }
  /////////////////////////////////////

  /////////////////////////////////////
  // Activation Callback
  struct ActivateData
  {
    ScopeProxy::ActivateCallback callback;
  };
  static void OnScopeActivateCallback(GObject *source_object, GAsyncResult *res, gpointer user_data)
  {
    std::unique_ptr<ActivateData> data(static_cast<ActivateData*>(user_data));
    UnityProtocolActivationReplyRaw result;
    glib::Error error;
    unity_protocol_scope_proxy_activate_finish(UNITY_PROTOCOL_SCOPE_PROXY(source_object), res, &result, &error);

    if (data->callback)
    {
      std::string uri;
      ScopeHandledType handled = ScopeHandledType::NOT_HANDLED;

      uri = result.uri;
      handled = static_cast<ScopeHandledType>(result.handled);

      glib::HintsMap hints;
      glib::hintsmap_from_hashtable(result.hints, hints);

      data->callback(uri, handled, hints, error);
    }
  }
  /////////////////////////////////////

  /////////////////////////////////////
  // Async Calls
  struct ScopeAyncReplyData
  {
    typedef std::function<void(GObject *source_object, GAsyncResult *res)> ScopeAsyncReplyCallback;
    ScopeAyncReplyData(ScopeAsyncReplyCallback const& callback): callback(callback) {}

    ScopeAsyncReplyCallback callback;
  };
  static void OnScopeAsyncCallback(GObject *source_object, GAsyncResult *res, gpointer user_data)
  {
    std::unique_ptr<ScopeAyncReplyData> data(static_cast<ScopeAyncReplyData*>(user_data));
    if (data->callback)
      data->callback(source_object, res);    
  }
  /////////////////////////////////////

  static void OnCloseChannel(GObject *source_object, GAsyncResult *res, gpointer user_data)
  {
    glib::Error err;
    unity_protocol_scope_proxy_close_channel_finish(UNITY_PROTOCOL_SCOPE_PROXY(source_object), res, &err);
  }
};


ScopeProxy::Impl::Impl(ScopeProxy*const owner, ScopeData::Ptr const& scope_data)
: owner_(owner)
, scope_data_(scope_data)
, search_in_global(false)
, view_type(ScopeViewType::HIDDEN)
, connected(false)
, cancel_scope_(g_cancellable_new())
, proxy_created_(false)
, results_(new Results())
, filters_(new Filters())
, categories_(new Categories())
{
  // remote properties
  property_connections.push_back(utils::ConnectProperties(owner_->connected, connected));
  property_connections.push_back(utils::ConnectProperties(owner_->channel, channel));
  property_connections.push_back(utils::ConnectProperties(owner_->search_in_global, search_in_global));
  property_connections.push_back(utils::ConnectProperties(owner_->view_type, view_type));
  // shared properties
  property_connections.push_back(utils::ConnectProperties(owner_->visible, scope_data_->visible));
  property_connections.push_back(utils::ConnectProperties(owner_->is_master, scope_data_->is_master));
  property_connections.push_back(utils::ConnectProperties(owner_->search_hint, scope_data_->search_hint));
  // local properties
  property_connections.push_back(utils::ConnectProperties(owner_->dbus_name, scope_data_->dbus_name));
  property_connections.push_back(utils::ConnectProperties(owner_->dbus_path, scope_data_->dbus_path));
  property_connections.push_back(utils::ConnectProperties(owner_->name, scope_data_->name));
  property_connections.push_back(utils::ConnectProperties(owner_->description, scope_data_->description));
  property_connections.push_back(utils::ConnectProperties(owner_->shortcut, scope_data_->shortcut));
  property_connections.push_back(utils::ConnectProperties(owner_->icon_hint, scope_data_->icon_hint));
  property_connections.push_back(utils::ConnectProperties(owner_->category_icon_hint, scope_data_->category_icon_hint));
  property_connections.push_back(utils::ConnectProperties(owner_->keywords, scope_data_->keywords));
  property_connections.push_back(utils::ConnectProperties(owner_->type, scope_data_->type));
  property_connections.push_back(utils::ConnectProperties(owner_->query_pattern, scope_data_->query_pattern));

  owner_->filters.SetGetterFunction(sigc::mem_fun(this, &Impl::filters));
  owner_->categories.SetGetterFunction(sigc::mem_fun(this, &Impl::categories));
  owner_->results.SetGetterFunction(sigc::mem_fun(this, &Impl::results));
}

ScopeProxy::Impl::~Impl()
{
  filters_change_connection.disconnect();
  for_each(property_connections.begin(), property_connections.end(), [](ConnectionPtr const& con) { con->disconnect(); });
  property_connections.clear();

  g_cancellable_cancel(cancel_scope_);

  if (scope_proxy_ && connected)
    unity_protocol_scope_proxy_close_channel(scope_proxy_, channel().c_str(), nullptr, Impl::OnCloseChannel, nullptr);
}

ScopeData::Ptr ScopeProxy::Impl::CreateData(std::string const& dbus_name, std::string const& dbus_path)
{
  ScopeData::Ptr data(new ScopeData);
  data->dbus_path = dbus_path;
  data->dbus_name = dbus_name;

  return data;
}

void ScopeProxy::Impl::DestroyProxy()
{
  scope_proxy_.Release();
  if (cancel_scope_)
  {
    g_cancellable_cancel(cancel_scope_);
    cancel_scope_ = g_cancellable_new();
  } 
  cancel_scope_ = g_cancellable_new(); 
  connected = false;
  proxy_created_ = false;
}

void ScopeProxy::Impl::CreateProxy()
{
  if (proxy_created_)
    return;

  proxy_created_ = true;
  unity_protocol_scope_proxy_new_from_dbus(scope_data_->dbus_name().c_str(),
                                           scope_data_->dbus_path().c_str(),
                                           cancel_scope_,
                                           Impl::OnScopeAsyncCallback,
                                           new ScopeAyncReplyData(sigc::mem_fun(this, &Impl::OnNewScope)));
}

void ScopeProxy::Impl::OnNewScope(GObject *source_object, GAsyncResult *res)
{ 
  glib::Object<UnityProtocolScopeProxy> scope_proxy;
  glib::Error error;
  scope_proxy = unity_protocol_scope_proxy_new_from_dbus_finish(res, &error);

  channels_invalidated_.Disconnect();
  visible_signal_.Disconnect();
  search_in_global_signal_.Disconnect();
  is_master_signal_.Disconnect();
  search_hint_signal_.Disconnect();
  view_type_signal_.Disconnect();
  filters_signal_.Disconnect();
  categories_signal_.Disconnect();
  metadata_signal_.Disconnect();
  optional_metadata_signal_.Disconnect();


  if (error || !scope_proxy)
  {
    scope_proxy_.Release();
    LOG_ERROR(logger) << "Failed to connect to scope @ '" << scope_data_->dbus_path() << "'";
    return;
  }

  LOG_DEBUG(logger) << "Connected to scope @ '" << scope_data_->dbus_path() << "'";

  scope_proxy_ = scope_proxy;

  // shared properties
  scope_data_->visible = unity_protocol_scope_proxy_get_visible(scope_proxy_);
  scope_data_->is_master = unity_protocol_scope_proxy_get_is_master(scope_proxy_);
  scope_data_->search_hint = glib::StringRef(unity_protocol_scope_proxy_get_search_hint(scope_proxy_));
  // remote properties
  search_in_global = unity_protocol_scope_proxy_get_search_in_global(scope_proxy_);
  view_type = static_cast<ScopeViewType>(unity_protocol_scope_proxy_get_view_type(scope_proxy_));

  channels_invalidated_.Connect(scope_proxy_, "channels-invalidated", sigc::mem_fun(this, &Impl::OnScopeChannelsInvalidated));
  visible_signal_.Connect(scope_proxy_, "notify::visible", sigc::mem_fun(this, &Impl::OnScopeVisibleChanged));
  search_in_global_signal_.Connect(scope_proxy_, "notify::search-in-global", sigc::mem_fun(this, &Impl::OnScopeSearchInGlobalChanged));
  is_master_signal_.Connect(scope_proxy_, "notify::is-master", sigc::mem_fun(this, &Impl::OnScopeIsMasterChanged));
  search_hint_signal_.Connect(scope_proxy_, "notify::search-hint", sigc::mem_fun(this, &Impl::OnScopeSearchHintChanged));
  view_type_signal_.Connect(scope_proxy_, "notify::view-type", sigc::mem_fun(this, &Impl::OnScopeViewTypeChanged));
  filters_signal_.Connect(scope_proxy_, "notify::filters-model", sigc::mem_fun(this, &Impl::OnScopeFiltersChanged));
  categories_signal_.Connect(scope_proxy_, "notify::categories-model", sigc::mem_fun(this, &Impl::OnScopeCategoriesChanged));
  metadata_signal_.Connect(scope_proxy_, "notify::metadata", sigc::mem_fun(this, &Impl::OnScopeMetadataChanged));
  optional_metadata_signal_.Connect(scope_proxy_, "notify::optional-metadata", sigc::mem_fun(this, &Impl::OnScopeOptionalMetadataChanged));

  unity_protocol_scope_proxy_open_channel(scope_proxy_,
                                          UNITY_PROTOCOL_CHANNEL_TYPE_DEFAULT,
                                          UNITY_PROTOCOL_CHANNEL_FLAGS_NONE,
                                          cancel_scope_,
                                          OnScopeAsyncCallback,
                                          new ScopeAyncReplyData(sigc::mem_fun(this, &Impl::OnChannelOpened)));
}

void ScopeProxy::Impl::OnChannelOpened(GObject *source_object, GAsyncResult *res)
{
  if (!UNITY_PROTOCOL_IS_SCOPE_PROXY(source_object))
    return;

  glib::Object<UnityProtocolScopeProxy> scope_proxy;
  glib::Error error;
  DeeSerializableModel* serialisable_model = nullptr;
  glib::String tmp_channel(unity_protocol_scope_proxy_open_channel_finish(UNITY_PROTOCOL_SCOPE_PROXY(source_object), res, &serialisable_model, &error));

  glib::Object<DeeModel> results_dee_model(DEE_MODEL(serialisable_model));
  results_->SetModel(results_dee_model);

  glib::Object<DeeModel> filters_dee_model(DEE_MODEL(unity_protocol_scope_proxy_get_filters_model(scope_proxy_)), glib::AddRef());
  filters_->SetModel(filters_dee_model);
  filters_change_connection.disconnect();
  filters_change_connection = filters_->filter_changed.connect([this](Filter::Ptr const& filter)
  {
    glib::HintsMap hints;
    hints["changed-filter-row"] = filter->VariantValue();

    printf("Filter changed by user\n");
    Search(last_search_, hints, nullptr, cancel_scope_);
  });

  glib::Object<DeeModel> categories_dee_model(DEE_MODEL(unity_protocol_scope_proxy_get_categories_model(scope_proxy_)), glib::AddRef());
  categories_->SetModel(categories_dee_model);

  if (tmp_channel.Str().empty() || error)
  {
    channel = "";
    connected = false;

    LOG_ERROR(logger) << "Failed to open channel on '" << scope_data_->dbus_path() << "'";
    return;
  }

  channel = tmp_channel.Str();
  LOG_DEBUG(logger) << "Opened channel:" << channel() << " on scope @ '" << scope_data_->dbus_path() << "'";
  connected = true;
}

void ScopeProxy::Impl::WaitForProxyConnection(GCancellable* cancellable,
                                   int timeout_msec,
                                   std::function<void(glib::Error const&)> const& callback)
{
  if (!connected)
  {
    auto con = std::make_shared<sigc::connection>();
    auto canc = glib::Object<GCancellable>(cancellable, glib::AddRef());

    // add a timeout
    auto timeout = std::make_shared<glib::Timeout>(timeout_msec < 0 ? 30000 : timeout_msec, [con, canc, callback] ()
    {
      if (!g_cancellable_is_cancelled(canc))
      {
        glib::Error err;
        GError** real_err = &err;
        *real_err = g_error_new_literal(G_DBUS_ERROR, G_DBUS_ERROR_TIMED_OUT,
                                        "Timed out waiting for scope proxy connection");
        callback(err);
      }
      con->disconnect();
      return false;
    });
    // wait for the signal
    *con = connected.changed.connect([con, canc, timeout, callback] (bool connected)
    {
      if (!connected)
        return;

      if (!g_cancellable_is_cancelled(canc)) callback(glib::Error());
      timeout->Remove();
      con->disconnect();
    });
  }
  else
  {
    callback(glib::Error());
  }
}

void ScopeProxy::Impl::Search(std::string const& search_string, glib::HintsMap const& hints, SearchCallback const& callback, GCancellable* cancellable)
{
  GCancellable* target_canc = cancellable != NULL ? cancellable : cancel_scope_;

  if (!scope_proxy_)
  {
    if (!proxy_created_)
      CreateProxy();

    glib::Object<GCancellable> canc(target_canc, glib::AddRef());
    WaitForProxyConnection(canc, PROXY_CONNECT_TIMEOUT, [this, search_string, hints, callback, canc] (glib::Error const& err)
    {
      if (err)
      {
        callback(glib::HintsMap(), err);
        LOG_WARNING(logger) << "Could not search " << search_string
                            << ": " << err;
      }
      else
      {
        Search(search_string, hints, callback, canc);
      }
    });
    return;
  }

  SearchData* data = new SearchData();
  data->callback = callback;

  GHashTable* hints_table = glib::hashtable_from_hintsmap(hints, g_hash_table_new(g_direct_hash, g_direct_equal));

  last_search_ = search_string.c_str();
  unity_protocol_scope_proxy_search(scope_proxy_,
    channel().c_str(),
    search_string.c_str(),
    hints_table,
    target_canc,
    Impl::OnScopeSearchCallback,
    data);
}

void ScopeProxy::Impl::Activate(std::string const& uri, uint activate_type, glib::HintsMap const& hints, ScopeProxy::ActivateCallback const& callback, GCancellable* cancellable)
{
  GCancellable* target_canc = cancellable != NULL ? cancellable : cancel_scope_;

  if (!scope_proxy_)
  {
    if (!proxy_created_)
      CreateProxy();

    glib::Object<GCancellable> canc(target_canc, glib::AddRef());
    WaitForProxyConnection(canc, PROXY_CONNECT_TIMEOUT, [this, uri, activate_type, hints, callback, canc] (glib::Error const& err)
    {
      if (err)
      {
        callback(uri, ScopeHandledType::NOT_HANDLED, glib::HintsMap(), err);
        LOG_WARNING(logger) << "Could not activate '" << uri
                            << "' : " << err;
      }
      else
      {
        Activate(uri, activate_type, hints, callback, canc);
      }
    });
    return;
  }

  ActivateData* data = new ActivateData();
  data->callback = callback;

  GHashTable* hints_table = glib::hashtable_from_hintsmap(hints, g_hash_table_new(g_direct_hash, g_direct_equal));

  unity_protocol_scope_proxy_activate(scope_proxy_,
                                      channel().c_str(),
                                      uri.c_str(),
                                      (UnityProtocolActionType)activate_type,
                                      hints_table,
                                      target_canc,
                                      Impl::OnScopeActivateCallback,
                                      data);
}

void ScopeProxy::Impl::UpdatePreviewProperty(std::string const& uri, glib::HintsMap const& hints, ScopeProxy::UpdatePreviewPropertyCallback const& callback, GCancellable* cancellable)
{
  GCancellable* target_canc = cancellable != NULL ? cancellable : cancel_scope_;

  if (!scope_proxy_)
  {
    if (!proxy_created_)
      CreateProxy();

    glib::Object<GCancellable> canc(target_canc, glib::AddRef());
    WaitForProxyConnection(canc, PROXY_CONNECT_TIMEOUT, [this, uri, hints, callback, canc] (glib::Error const& err)
    {
      if (err)
      {
        callback(glib::HintsMap(), err);
        LOG_WARNING(logger) << "Could not update preview property '" << uri
                            << "' : " << err;
      }
      else
      {
        UpdatePreviewProperty(uri, hints, callback, canc);
      }
    });
    return;
  }

  UpdatePreviewPropertyData* data = new UpdatePreviewPropertyData();
  data->callback = callback;

  GHashTable* hints_table = glib::hashtable_from_hintsmap(hints, g_hash_table_new(g_direct_hash, g_direct_equal));

  unity_protocol_scope_proxy_update_preview_property(scope_proxy_,
                                                     channel().c_str(),
                                                     uri.c_str(),
                                                     hints_table,
                                                     target_canc,
                                                     Impl::OnScopeUpdatePreviewPropertyCallback,
                                                     data);
}

void ScopeProxy::Impl::OnScopeChannelsInvalidated(UnityProtocolScopeProxy* proxy)
{
  LOG_WARNING(logger) << "Channels for scope '" << scope_data_->dbus_path() << "' have been invalidated. Attempting proxy re-creation.";
  DestroyProxy();
  CreateProxy();
}

void ScopeProxy::Impl::OnScopeVisibleChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param)
{
  scope_data_->visible = unity_protocol_scope_proxy_get_visible(proxy);
}

void ScopeProxy::Impl::OnScopeIsMasterChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param)
{
  scope_data_->is_master = unity_protocol_scope_proxy_get_is_master(proxy);
}

void ScopeProxy::Impl::OnScopeSearchInGlobalChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param)
{
  search_in_global = unity_protocol_scope_proxy_get_search_in_global(proxy);
}

void ScopeProxy::Impl::OnScopeSearchHintChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param)
{
  scope_data_->search_hint = unity_protocol_scope_proxy_get_search_hint(proxy);
}

void ScopeProxy::Impl::OnScopeViewTypeChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param)
{
  view_type = static_cast<ScopeViewType>(unity_protocol_scope_proxy_get_view_type(proxy));
}

void ScopeProxy::Impl::OnScopeFiltersChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param)
{
  printf("Filter changed by server\n");

  bool blocked = filters_change_connection.block(true);

  glib::Object<DeeModel> filters_dee_model(DEE_MODEL(unity_protocol_scope_proxy_get_filters_model(scope_proxy_)), glib::AddRef());
  categories_->SetModel(filters_dee_model);

  filters_change_connection.block(blocked);

  owner_->filters.EmitChanged(filters());
}

void ScopeProxy::Impl::OnScopeCategoriesChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param)
{
  glib::Object<DeeModel> categories_dee_model(DEE_MODEL(unity_protocol_scope_proxy_get_categories_model(scope_proxy_)), glib::AddRef());
  categories_->SetModel(categories_dee_model);

  owner_->categories.EmitChanged(categories());
}

void ScopeProxy::Impl::OnScopeMetadataChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param)
{
}

void ScopeProxy::Impl::OnScopeOptionalMetadataChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param)
{
}

static void category_filter_map_func (DeeModel* orig_model,
                                      DeeFilterModel* filter_model,
                                      gpointer user_data)
{
  DeeModelIter* iter;
  DeeModelIter* end;
  unsigned index = GPOINTER_TO_UINT(user_data);

  iter = dee_model_get_first_iter(orig_model);
  end = dee_model_get_last_iter(orig_model);
  while (iter != end)
  {
    unsigned category_index = dee_model_get_uint32(orig_model, iter,
                                                   CATEGORY_COLUMN);
    if (index == category_index)
    {
      dee_filter_model_append_iter(filter_model, iter);
    }
    iter = dee_model_next(orig_model, iter);
  }
}

static gboolean category_filter_notify_func (DeeModel* orig_model,
                                             DeeModelIter* orig_iter,
                                             DeeFilterModel* filter_model,
                                             gpointer user_data)
{
  unsigned index = GPOINTER_TO_UINT(user_data);
  unsigned category_index = dee_model_get_uint32(orig_model, orig_iter,
                                                 CATEGORY_COLUMN);

  if (index != category_index)
    return FALSE;

  dee_filter_model_insert_iter_with_original_order(filter_model, orig_iter);
  return TRUE;
}

DeeFilter* ScopeProxy::Impl::GetFilterForCategory(unsigned category, DeeFilter* filter) const
{
  filter->map_func = category_filter_map_func;
  filter->map_notify = category_filter_notify_func;
  filter->destroy = nullptr;
  filter->userdata = GUINT_TO_POINTER(category);

  return filter;
}

ScopeProxy::ScopeProxy(std::string const& scope_id)
: pimpl(new Impl(this, ScopeData::ReadProtocolDataForId(scope_id)))
{
}


ScopeProxy::ScopeProxy(std::string const& dbus_name, std::string const& dbus_path)
: pimpl(new Impl(this, Impl::CreateData(dbus_name, dbus_path)))
{
}

ScopeProxy::~ScopeProxy()
{  
}

void ScopeProxy::CreateProxy()
{
  pimpl->CreateProxy();
}

void ScopeProxy::Search(std::string const& search_string, SearchCallback const& callback, GCancellable* cancellable)
{
  pimpl->Search(search_string, glib::HintsMap(), callback, cancellable);
}

void ScopeProxy::Activate(std::string const& uri, uint activate_type, glib::HintsMap const& hints, ScopeProxy::ActivateCallback const& callback, GCancellable* cancellable)
{
  pimpl->Activate(uri, activate_type, hints, callback, cancellable);
}

void ScopeProxy::UpdatePreviewProperty(std::string const& uri, glib::HintsMap const& hints, ScopeProxy::UpdatePreviewPropertyCallback const& callback, GCancellable* cancellable)
{
  pimpl->UpdatePreviewProperty(uri, hints, callback, cancellable);
}

Results::Ptr ScopeProxy::GetResultsForCategory(unsigned category) const
{
  Results::Ptr all_results = results;

  DeeFilter filter;
  pimpl->GetFilterForCategory(category, &filter);
  glib::Object<DeeModel> filter_model(dee_filter_model_new(all_results->model(), &filter));

  auto func = [all_results](glib::Object<DeeModel> const& model) { return all_results->GetTag(); };
  
  Results::Ptr results_category_model(new Results(ModelType::UNATTACHED));
  results_category_model->SetModel(filter_model, func);
  return results_category_model;
}


} // namespace dash
} // namespace unity