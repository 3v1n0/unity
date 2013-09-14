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
#include "ConnectionManager.h"
#include "GLibSignal.h"
#include "GLibSource.h"
#include "MiscUtils.h"

#include <unity-protocol.h>
#include <NuxCore/Logger.h>

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

  void CreateProxy();
  void OnNewScope(glib::Object<UnityProtocolScopeProxy> const& scope_proxy, glib::Error const& error);
  void DestroyProxy();

  void OpenChannel();
  void OnChannelOpened(glib::String const& opened_channel, glib::Object<DeeModel> results_dee_model, glib::Error const& error);

  void CloseChannel();
  static void OnCloseChannel(GObject *source_object, GAsyncResult *res, gpointer user_data);

  void Search(std::string const& search_string, glib::HintsMap const& hints, SearchCallback const& callback, GCancellable* cancel);
  void Activate(LocalResult const& result, uint activate_type, glib::HintsMap const& hints, ScopeProxy::ActivateCallback const& callback, GCancellable* cancellable);

  bool set_view_type(ScopeViewType const& _view_type)
  {
    if (scope_proxy_ && view_type != _view_type)
    {
      view_type = _view_type;
      unity_protocol_scope_proxy_set_view_type(scope_proxy_, static_cast<UnityProtocolViewType>(_view_type));
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
  nux::Property<ScopeViewType> view_type;

  nux::Property<bool> connected;
  nux::Property<std::string> channel;
  nux::Property<std::vector<unsigned int>> category_order;
  std::string last_search_;

  glib::Object<UnityProtocolScopeProxy> scope_proxy_;
  glib::Cancellable cancel_scope_;
  bool proxy_created_;
  bool scope_proxy_connected_;
  bool searching_;

  Results::Ptr results_;
  Filters::Ptr filters_;
  Categories::Ptr categories_;

  connection::handle filters_change_connection_;
  connection::Manager signals_conn_;

  typedef glib::Signal<void, UnityProtocolScopeProxy*, GParamSpec*> ScopePropertySignal;
  glib::SignalManager gsignals_;

private:
  /////////////////////////////////////////////////
  // Signal Connections
  void OnScopeConnectedChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeIsMasterChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeSearchInGlobalChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeSearchHintChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeViewTypeChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeFiltersChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeCategoriesChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeMetadataChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeOptionalMetadataChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param);
  void OnScopeCategoryOrderChanged(UnityProtocolScopeProxy* sender, const gchar* channel_id, guint32* new_order, int new_order_length1);
  void OnScopeFilterSettingsChanged(UnityProtocolScopeProxy* sender, const gchar* channel_id, GVariant* filter_rows);
  /////////////////////////////////////////////////

  void WaitForProxyConnection(GCancellable* cancellable,
                                   int timeout_msec,
                                   std::function<void(glib::Error const&)> const& callback);

  /////////////////////////////////////
  // Search Callback
  struct SearchData
  {
    std::string search_string;
    SearchCallback callback;
  };

  static void g_hash_table_unref0(GHashTable* hash_table) { if (hash_table) g_hash_table_unref(hash_table); }

  static void OnScopeSearchCallback(GObject *source_object, GAsyncResult *res, gpointer user_data)
  {
    if (!UNITY_PROTOCOL_IS_SCOPE_PROXY(source_object))
      return;

    std::unique_ptr<SearchData> data(static_cast<SearchData*>(user_data));
    glib::Error error;
    std::unique_ptr<GHashTable, void(*)(GHashTable*)> hint_ret(unity_protocol_scope_proxy_search_finish(UNITY_PROTOCOL_SCOPE_PROXY(source_object), res, &error),
                                         g_hash_table_unref0);
    if (error && g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      LOG_DEBUG(logger) << "Search cancelled ('" << (data ? data->search_string : "[unknown]") << "').";
      return;
    }

    glib::HintsMap hints;
    glib::hintsmap_from_hashtable(hint_ret.get(), hints);

    if (data && data->callback)
      data->callback(data->search_string, hints, error);
  }
  /////////////////////////////////////

  /////////////////////////////////////
  // Activation Callback
  struct ActivateData
  {
    ScopeProxy::ActivateCallback callback;
    LocalResult result;
  };
  static void OnScopeActivateCallback(GObject *source_object, GAsyncResult *res, gpointer user_data)
  {
    if (!UNITY_PROTOCOL_IS_SCOPE_PROXY(source_object))
      return;

    std::unique_ptr<ActivateData> data(static_cast<ActivateData*>(user_data));
    UnityProtocolActivationReplyRaw result;
    memset(&result, 0, sizeof(UnityProtocolActivationReplyRaw));
    glib::Error error;
    unity_protocol_scope_proxy_activate_finish(UNITY_PROTOCOL_SCOPE_PROXY(source_object), res, &result, &error);

    if (error)
    {
      if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      {
        LOG_DEBUG(logger) << "Activate cancelled.";
        return;
      }
      if (data && data->callback)
        data->callback(data->result, ScopeHandledType::NOT_HANDLED, glib::HintsMap(), error);
    }
    else if (data && data->callback)
    {
      ScopeHandledType handled = static_cast<ScopeHandledType>(result.handled);

      glib::HintsMap hints;
      if (result.hints)
        glib::hintsmap_from_hashtable(result.hints, hints);

      data->result.uri = glib::gchar_to_string(result.uri);
      data->callback(data->result, handled, hints, error);
    }
    unity_protocol_activation_reply_raw_destroy(&result);
  }
  /////////////////////////////////////

  /////////////////////////////////////
  // Async Calls
  static void OnNewScopeCallback(GObject *source_object, GAsyncResult *res, gpointer user_data)
  {
    glib::Object<UnityProtocolScopeProxy> scope_proxy;
    glib::Error error;
    scope_proxy = unity_protocol_scope_proxy_new_from_dbus_finish(res, &error);
    if (error && g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      LOG_DEBUG(logger) << "Proxy create cancelled.";
      return;
    }
    
    if (user_data)
    {
      ScopeProxy::Impl* pimpl = (ScopeProxy::Impl*)user_data;
      pimpl->OnNewScope(scope_proxy, error);
    }
  }

  static void OnOpenChannelCallback(GObject *source_object, GAsyncResult *res, gpointer user_data)
  {
    if (!UNITY_PROTOCOL_IS_SCOPE_PROXY(source_object))
      return;

    glib::Error error;
    DeeSerializableModel* serialisable_model = nullptr;
    glib::String tmp_channel(unity_protocol_scope_proxy_open_channel_finish(UNITY_PROTOCOL_SCOPE_PROXY(source_object), res, &serialisable_model, &error));

    if (error && g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      LOG_DEBUG(logger) << "Open channel cancelled.";
      return;
    }
    if (user_data)
    {
      ScopeProxy::Impl* pimpl = (ScopeProxy::Impl*)user_data;

      glib::Object<DeeModel> results_dee_model(DEE_MODEL(serialisable_model));
      pimpl->OnChannelOpened(tmp_channel, results_dee_model, error);
    }
  }
  /////////////////////////////////////
};


ScopeProxy::Impl::Impl(ScopeProxy*const owner, ScopeData::Ptr const& scope_data)
: owner_(owner)
, scope_data_(scope_data)
, view_type(ScopeViewType::HIDDEN)
, connected(false)
, proxy_created_(false)
, scope_proxy_connected_(false)
, searching_(false)
, results_(new Results())
, filters_(new Filters())
, categories_(new Categories())
{
  // remote properties
  signals_conn_.Add(utils::ConnectProperties(owner_->connected, connected));
  signals_conn_.Add(utils::ConnectProperties(owner_->channel, channel));
  signals_conn_.Add(utils::ConnectProperties(owner_->category_order, category_order));

  // shared properties
  signals_conn_.Add(utils::ConnectProperties(owner_->is_master, scope_data_->is_master));
  signals_conn_.Add(utils::ConnectProperties(owner_->search_hint, scope_data_->search_hint));
  // local properties
  signals_conn_.Add(utils::ConnectProperties(owner_->dbus_name, scope_data_->dbus_name));
  signals_conn_.Add(utils::ConnectProperties(owner_->dbus_path, scope_data_->dbus_path));
  signals_conn_.Add(utils::ConnectProperties(owner_->name, scope_data_->name));
  signals_conn_.Add(utils::ConnectProperties(owner_->description, scope_data_->description));
  signals_conn_.Add(utils::ConnectProperties(owner_->shortcut, scope_data_->shortcut));
  signals_conn_.Add(utils::ConnectProperties(owner_->icon_hint, scope_data_->icon_hint));
  signals_conn_.Add(utils::ConnectProperties(owner_->category_icon_hint, scope_data_->category_icon_hint));
  signals_conn_.Add(utils::ConnectProperties(owner_->keywords, scope_data_->keywords));
  signals_conn_.Add(utils::ConnectProperties(owner_->type, scope_data_->type));
  signals_conn_.Add(utils::ConnectProperties(owner_->query_pattern, scope_data_->query_pattern));
  signals_conn_.Add(utils::ConnectProperties(owner_->visible, scope_data_->visible));
  // Writable properties.
  owner_->view_type.SetGetterFunction([this]() { return view_type.Get(); });
  owner_->view_type.SetSetterFunction(sigc::mem_fun(this, &Impl::set_view_type));
  
  owner_->filters.SetGetterFunction(sigc::mem_fun(this, &Impl::filters));
  owner_->categories.SetGetterFunction(sigc::mem_fun(this, &Impl::categories));
  owner_->results.SetGetterFunction(sigc::mem_fun(this, &Impl::results));
}

ScopeProxy::Impl::~Impl()
{
  if (scope_proxy_ && connected)
    unity_protocol_scope_proxy_close_channel(scope_proxy_, channel().c_str(), nullptr, Impl::OnCloseChannel, nullptr);
}

void ScopeProxy::Impl::DestroyProxy()
{
  CloseChannel();

  scope_proxy_.Release();
  cancel_scope_.Renew();
  connected = false;
  proxy_created_ = false;
  scope_proxy_connected_ = false;
}

void ScopeProxy::Impl::CreateProxy()
{
  if (proxy_created_)
    return;

  proxy_created_ = true;
  unity_protocol_scope_proxy_new_from_dbus(scope_data_->dbus_name().c_str(),
                                           scope_data_->dbus_path().c_str(),
                                           cancel_scope_,
                                           Impl::OnNewScopeCallback,
                                           this);
}

void ScopeProxy::Impl::OnNewScope(glib::Object<UnityProtocolScopeProxy> const& scope_proxy, glib::Error const& error)
{
  gsignals_.Disconnect(scope_proxy_);

  if (error || !scope_proxy)
  {
    proxy_created_ = false;
    scope_proxy_.Release();
    LOG_ERROR(logger) << "Failed to create scope proxy for " << scope_data_->id() << " =>  " << error;
    return;
  }

  LOG_DEBUG(logger) << "Created scope proxy for " << scope_data_->id();

  scope_proxy_ = scope_proxy;

  // shared properties
  scope_data_->is_master = unity_protocol_scope_proxy_get_is_master(scope_proxy_);
  scope_data_->search_hint = glib::gchar_to_string(unity_protocol_scope_proxy_get_search_hint(scope_proxy_));
  // remote properties
  view_type = static_cast<ScopeViewType>(unity_protocol_scope_proxy_get_view_type(scope_proxy_));

  auto sig = std::make_shared<ScopePropertySignal>(scope_proxy_, "notify::connected", sigc::mem_fun(this, &Impl::OnScopeConnectedChanged));
  gsignals_.Add(sig);
  sig = std::make_shared<ScopePropertySignal>(scope_proxy_, "notify::connected", sigc::mem_fun(this, &Impl::OnScopeConnectedChanged));
  gsignals_.Add(sig);
  sig = std::make_shared<ScopePropertySignal>(scope_proxy_, "notify::is-master", sigc::mem_fun(this, &Impl::OnScopeIsMasterChanged));
  gsignals_.Add(sig);
  sig = std::make_shared<ScopePropertySignal>(scope_proxy_, "notify::search-hint", sigc::mem_fun(this, &Impl::OnScopeSearchHintChanged));
  gsignals_.Add(sig);
  sig = std::make_shared<ScopePropertySignal>(scope_proxy_, "notify::view-type", sigc::mem_fun(this, &Impl::OnScopeViewTypeChanged));
  gsignals_.Add(sig);
  sig = std::make_shared<ScopePropertySignal>(scope_proxy_, "notify::filters-model", sigc::mem_fun(this, &Impl::OnScopeFiltersChanged));
  gsignals_.Add(sig);
  sig = std::make_shared<ScopePropertySignal>(scope_proxy_, "notify::categories-model", sigc::mem_fun(this, &Impl::OnScopeCategoriesChanged));
  gsignals_.Add(sig);
  sig = std::make_shared<ScopePropertySignal>(scope_proxy_, "notify::metadata", sigc::mem_fun(this, &Impl::OnScopeMetadataChanged));
  gsignals_.Add(sig);
  sig = std::make_shared<ScopePropertySignal>(scope_proxy_, "notify::optional-metadata", sigc::mem_fun(this, &Impl::OnScopeOptionalMetadataChanged));
  gsignals_.Add(sig);

  gsignals_.Add<void, UnityProtocolScopeProxy*, const gchar*, guint32*, int>(scope_proxy_, "category-order-changed", sigc::mem_fun(this, &Impl::OnScopeCategoryOrderChanged));
  gsignals_.Add<void, UnityProtocolScopeProxy*, const gchar*, GVariant*>(scope_proxy_, "filter-settings-changed", sigc::mem_fun(this, &Impl::OnScopeFilterSettingsChanged));

  scope_proxy_connected_ = unity_protocol_scope_proxy_get_connected(scope_proxy_);

  if (scope_proxy_connected_)
    OpenChannel();
  else
    LOG_WARN(logger) << "Proxy scope not connected for " << scope_data_->id();
}

void ScopeProxy::Impl::OpenChannel()
{
  if (channel != "")
    return;
  
  LOG_DEBUG(logger) << "Opening channel open for " << scope_data_->id();

  unity_protocol_scope_proxy_open_channel(scope_proxy_,
                                          UNITY_PROTOCOL_CHANNEL_TYPE_DEFAULT,
                                          UNITY_PROTOCOL_CHANNEL_FLAGS_DIFF_CHANGES,
                                          cancel_scope_,
                                          OnOpenChannelCallback,
                                          this);
}

void ScopeProxy::Impl::OnChannelOpened(glib::String const& opened_channel, glib::Object<DeeModel> results_dee_model, glib::Error const& error)
{
  results_->SetModel(results_dee_model);

  glib::Object<DeeModel> filters_dee_model(DEE_MODEL(unity_protocol_scope_proxy_get_filters_model(scope_proxy_)), glib::AddRef());
  filters_->SetModel(filters_dee_model);
  filters_change_connection_ = signals_conn_.Replace(filters_change_connection_, filters_->filter_changed.connect([this](Filter::Ptr const& filter)
  {
    LOG_DEBUG(logger) << "Filters changed for " << scope_data_->id() << "- updating search results.";

    glib::HintsMap hints;
    if (!owner_->form_factor().empty())
    {
      hints["form-factor"] = g_variant_new_string(owner_->form_factor().c_str());
    }
    hints["changed-filter-row"] = filter->VariantValue();
    Search(last_search_, hints, nullptr, cancel_scope_);
  }));

  glib::Object<DeeModel> categories_dee_model(DEE_MODEL(unity_protocol_scope_proxy_get_categories_model(scope_proxy_)), glib::AddRef());
  categories_->SetModel(categories_dee_model);

  if (opened_channel.Str().empty() || error)
  {
    channel = "";
    connected = false;

    LOG_ERROR(logger) << "Failed to open channel for " << scope_data_->id() << " => " << error;
    return;
  }

  channel = opened_channel.Str();
  LOG_DEBUG(logger) << "Opened channel:" << channel() << " for " << scope_data_->id();
  connected = true;
}

void ScopeProxy::Impl::CloseChannel()
{
  if (channel != "" && scope_proxy_connected_)
  {
    LOG_DEBUG(logger) << "Closing channel open for " << scope_data_->id();

    connected = false;
    unity_protocol_scope_proxy_close_channel(scope_proxy_,
                                             channel.Get().c_str(),
                                             nullptr,
                                             OnCloseChannel,
                                             nullptr);
  }
  channel = "";
}

void ScopeProxy::Impl::OnCloseChannel(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  glib::Error err;
  unity_protocol_scope_proxy_close_channel_finish(UNITY_PROTOCOL_SCOPE_PROXY(source_object), res, &err);

  if (err)
  {
    LOG_WARNING(logger) << "Unable to close channel: " << err;
  }
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
        if (callback)
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

      if (callback && !g_cancellable_is_cancelled(canc))
        callback(glib::Error());
      timeout->Remove();
      con->disconnect();
    });
  }
  else
  {
    if (callback)
      callback(glib::Error());
  }
}

void ScopeProxy::Impl::Search(std::string const& search_string, glib::HintsMap const& hints, SearchCallback const& callback, GCancellable* cancellable)
{
  // Activate a guard against performing a "on channel open search" if we're not connected.
  utils::AutoResettingVariable<bool> searching(&searching_, true);

  GCancellable* target_canc = cancellable != NULL ? cancellable : cancel_scope_;

  if (!connected)
  {
    if (!proxy_created_)
      CreateProxy();

    glib::Object<GCancellable> canc(target_canc, glib::AddRef());
    WaitForProxyConnection(canc, PROXY_CONNECT_TIMEOUT, [this, search_string, hints, callback, canc] (glib::Error const& err)
    {
      if (err)
      {
        if (callback)
          callback(search_string, glib::HintsMap(), err);
        LOG_WARNING(logger) << "Could not search '" << search_string
                            << "' on " << scope_data_->id() << " => " << err;
      }
      else
      {
        glib::HintsMap updated_hints(hints);
        if (!owner_->form_factor().empty())
        {
          updated_hints["form-factor"] = g_variant_new_string(owner_->form_factor().c_str());
        }
        Search(search_string, updated_hints, callback, canc);
      }
    });
    return;
  }

  SearchData* data = new SearchData();
  data->search_string = search_string;
  data->callback = callback;

  GHashTable* hints_table = glib::hashtable_from_hintsmap(hints);

  LOG_DEBUG(logger) << "Search '" << search_string << "' on " << scope_data_->id();

  last_search_ = search_string.c_str();
  unity_protocol_scope_proxy_search(scope_proxy_,
    channel().c_str(),
    search_string.c_str(),
    hints_table,
    target_canc,
    Impl::OnScopeSearchCallback,
    data);

  g_hash_table_unref(hints_table);
}

void ScopeProxy::Impl::Activate(LocalResult const& result, uint activate_type, glib::HintsMap const& hints, ScopeProxy::ActivateCallback const& callback, GCancellable* cancellable)
{
  GCancellable* target_canc = cancellable != NULL ? cancellable : cancel_scope_;

  if (!connected)
  {
    if (!proxy_created_)
      CreateProxy();

    glib::Object<GCancellable> canc(target_canc, glib::AddRef());
    WaitForProxyConnection(canc, PROXY_CONNECT_TIMEOUT, [this, result, activate_type, hints, callback, canc] (glib::Error const& err)
    {
      if (err)
      {
        if (callback)
          callback(result, ScopeHandledType::NOT_HANDLED, glib::HintsMap(), err);
        LOG_WARNING(logger) << "Could not activate '" << result.uri
                            << "' on " << scope_data_->id() << " => " << err;
      }
      else
      {
        Activate(result, activate_type, hints, callback, canc);
      }
    });
    return;
  }

  ActivateData* data = new ActivateData();
  data->callback = callback;
  data->result = result;

  GHashTable* hints_table = glib::hashtable_from_hintsmap(hints);

  std::vector<glib::Variant> columns = result.Variants();
  GVariant** variants = new GVariant*[columns.size()];
  
  for (unsigned i = 0; i < columns.size(); i++)
  {
    variants[i] = columns[i];
  }

  LOG_DEBUG(logger) << "Activate '" << result.uri << "' on " << scope_data_->id();

  unity_protocol_scope_proxy_activate(scope_proxy_,
                                      channel().c_str(),
                                      variants,
                                      columns.size(),
                                      (UnityProtocolActionType)activate_type,
                                      hints_table,
                                      target_canc,
                                      Impl::OnScopeActivateCallback,
                                      data);
  delete [] variants;
  g_hash_table_unref(hints_table);
}

void ScopeProxy::Impl::OnScopeConnectedChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param)
{
  bool tmp_scope_proxy_connected = unity_protocol_scope_proxy_get_connected(scope_proxy_);
  if (tmp_scope_proxy_connected != scope_proxy_connected_)
  {
    scope_proxy_connected_ = tmp_scope_proxy_connected;

    LOG_WARN(logger) << "Connection state changed for " << scope_data_->id() << " => " << (scope_proxy_connected_ ? "connected" : "disconnected");

    CloseChannel();
    if (tmp_scope_proxy_connected)
    {
      OpenChannel();
    }
    else
    {
      connected = false;
    }
  }
}

void ScopeProxy::Impl::OnScopeIsMasterChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param)
{
  scope_data_->is_master = unity_protocol_scope_proxy_get_is_master(proxy);
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
  LOG_DEBUG(logger) << scope_data_->id() << " (" << channel() << ") - Filter changed by server";

  // Block the filter change signal so that changes do not cause another search.
  auto filters_change_connection = signals_conn_.Get(filters_change_connection_);
  bool blocked = filters_change_connection.block(true);

  glib::Object<DeeModel> filters_dee_model(DEE_MODEL(unity_protocol_scope_proxy_get_filters_model(scope_proxy_)), glib::AddRef());
  filters_->SetModel(filters_dee_model);

  filters_change_connection.block(blocked);

  owner_->filters.EmitChanged(filters());
}

void ScopeProxy::Impl::OnScopeCategoriesChanged(UnityProtocolScopeProxy* proxy, GParamSpec* param)
{
  LOG_DEBUG(logger) << scope_data_->id() << " (" << channel() << ") - Category model changed";

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

void ScopeProxy::Impl::OnScopeCategoryOrderChanged(UnityProtocolScopeProxy* sender, const gchar* channel_id, guint32* new_order, int new_order_length1)
{
  if (channel() != glib::gchar_to_string(channel_id))
    return;

  LOG_DEBUG(logger) << scope_data_->id() << " (" << channel() << ") - Category order changed";

  std::vector<unsigned int> order;
  for (int i = 0; i < new_order_length1; i++)
  {
    order.push_back(new_order[i]);
  }

  category_order = order;
}

void ScopeProxy::Impl::OnScopeFilterSettingsChanged(UnityProtocolScopeProxy* sender, const gchar* channel_id, GVariant* filter_rows)
{
  if (channel() != glib::gchar_to_string(channel_id))
    return;

  LOG_DEBUG(logger) << scope_data_->id() << " (" << channel() << ") - Filter settings changed";

  // Block the filter change signal so that changes do not cause another search.
  auto filters_change_connection = signals_conn_.Get(filters_change_connection_);
  bool blocked = filters_change_connection.block(true);

  filters_->ApplyStateChanges(filter_rows);

  filters_change_connection.block(blocked);
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

ScopeProxy::ScopeProxy(ScopeData::Ptr const& scope_data)
: pimpl(new Impl(this, scope_data))
{
}

ScopeProxy::~ScopeProxy()
{  
}

void ScopeProxy::ConnectProxy()
{
  pimpl->CreateProxy();
}

void ScopeProxy::DisconnectProxy()
{
  pimpl->DestroyProxy();
}

void ScopeProxy::Search(std::string const& search_string, glib::HintsMap const& user_hints, SearchCallback const& callback, GCancellable* cancellable)
{
  glib::HintsMap hints = user_hints;
  if (!form_factor.Get().empty() && hints.find("form-factor") == hints.end())
  {
    hints["form-factor"] = g_variant_new_string(form_factor().c_str());
  }
  pimpl->Search(search_string, hints, callback, cancellable);
}

void ScopeProxy::Activate(LocalResult const& result, uint activate_type, glib::HintsMap const& hints, ScopeProxy::ActivateCallback const& callback, GCancellable* cancellable)
{
  pimpl->Activate(result, activate_type, hints, callback, cancellable);
}

Results::Ptr ScopeProxy::GetResultsForCategory(unsigned category) const
{
  Results::Ptr all_results = results;

  if (!all_results || !all_results->model())
    return Results::Ptr();

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
