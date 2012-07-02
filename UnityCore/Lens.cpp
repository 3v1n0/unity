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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "Lens.h"

#include <gio/gio.h>
#include <glib.h>
#include <NuxCore/Logger.h>
#include <unity-protocol.h>

#include "config.h"
#include "GLibDBusProxy.h"
#include "GLibWrapper.h"

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.lens");
}

using std::string;

class Lens::Impl
{
public:
  Impl(Lens* owner,
       string const& id,
       string const& dbus_name,
       string const& dbus_path,
       string const& name,
       string const& icon_hint,
       string const& description,
       string const& search_hint,
       bool visible,
       string const& shortcut,
       ModelType model_type);

  ~Impl();

  void OnProxyConnectionChanged();
  void OnProxyDisconnected();

  void ResultsModelUpdated(unsigned long long begin_seqnum, 
                           unsigned long long end_seqnum);
  void GlobalResultsModelUpdated(unsigned long long begin_seqnum,
                                 unsigned long long end_seqnum);

  unsigned long long ExtractModelSeqnum(GVariant *parameters);

  void OnSearchFinished(GVariant* parameters);
  void OnGlobalSearchFinished(GVariant* parameters);
  void OnChanged(GVariant* parameters);
  void UpdateProperties(bool search_in_global,
                        bool visible,
                        string const& search_hint,
                        string const& private_connection_name,
                        string const& results_model_name,
                        string const& global_results_model_name,
                        string const& categories_model_name,
                        string const& filters_model_name);
  void OnViewTypeChanged(ViewType view_type);

  void GlobalSearch(std::string const& search_string);
  void Search(std::string const& search_string);
  void Activate(std::string const& uri);
  void ActivationReply(GVariant* parameters);
  void Preview(std::string const& uri);
  void ActivatePreviewAction(std::string const& action_id,
                             std::string const& uri);
  void SignalPreview(std::string const& preview_uri,
                     glib::Variant const& preview_update,
                     glib::DBusProxy::ReplyCallback reply_cb);

  string const& id() const;
  string const& dbus_name() const;
  string const& dbus_path() const;
  string const& name() const;
  string const& icon_hint() const;
  string const& description() const;
  string const& search_hint() const;
  bool visible() const;
  bool search_in_global() const;
  bool set_search_in_global(bool val);
  string const& shortcut() const;
  Results::Ptr const& results() const;
  Results::Ptr const& global_results() const;
  Categories::Ptr const& categories() const;
  Filters::Ptr const& filters() const;
  bool connected() const;

  Lens* owner_;

  string id_;
  string dbus_name_;
  string dbus_path_;
  string name_;
  string icon_hint_;
  string description_;
  string search_hint_;
  bool visible_;
  bool search_in_global_;
  string shortcut_;
  Results::Ptr results_;
  Results::Ptr global_results_;
  Categories::Ptr categories_;
  Filters::Ptr filters_;
  bool connected_;

  string private_connection_name_;

  glib::DBusProxy* proxy_;
  glib::Object<GCancellable> search_cancellable_;
  glib::Object<GCancellable> global_search_cancellable_;
  glib::Object<GCancellable> preview_cancellable_;

  GVariant *results_variant_;
  GVariant *global_results_variant_;
};

Lens::Impl::Impl(Lens* owner,
                 string const& id,
                 string const& dbus_name,
                 string const& dbus_path,
                 string const& name,
                 string const& icon_hint,
                 string const& description,
                 string const& search_hint,
                 bool visible,
                 string const& shortcut,
                 ModelType model_type)
  : owner_(owner)
  , id_(id)
  , dbus_name_(dbus_name)
  , dbus_path_(dbus_path)
  , name_(name)
  , icon_hint_(icon_hint)
  , description_(description)
  , search_hint_(search_hint)
  , visible_(visible)
  , search_in_global_(false)
  , shortcut_(shortcut)
  , results_(new Results(model_type))
  , global_results_(new Results(model_type))
  , categories_(new Categories(model_type))
  , filters_(new Filters(model_type))
  , connected_(false)
  , proxy_(NULL)
  , results_variant_(NULL)
  , global_results_variant_(NULL)
{
  if (model_type == ModelType::REMOTE)
  {
    proxy_ = new glib::DBusProxy(dbus_name, dbus_path, "com.canonical.Unity.Lens");
    proxy_->connected.connect(sigc::mem_fun(this, &Lens::Impl::OnProxyConnectionChanged));
    proxy_->disconnected.connect(sigc::mem_fun(this, &Lens::Impl::OnProxyDisconnected));
    proxy_->Connect("Changed", sigc::mem_fun(this, &Lens::Impl::OnChanged));
  }

  /* Technically these signals will only be fired by remote models, but we
   * connect them no matter the ModelType. Dee may grow support in the future.
   */
  results_->end_transaction.connect(sigc::mem_fun(this, &Lens::Impl::ResultsModelUpdated));
  global_results_->end_transaction.connect(sigc::mem_fun(this, &Lens::Impl::GlobalResultsModelUpdated));

  owner_->id.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::id));
  owner_->dbus_name.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::dbus_name));
  owner_->dbus_path.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::dbus_path));
  owner_->name.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::name));
  owner_->icon_hint.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::icon_hint));
  owner_->description.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::description));
  owner_->search_hint.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::search_hint));
  owner_->visible.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::visible));
  owner_->search_in_global.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::search_in_global));
  owner_->search_in_global.SetSetterFunction(sigc::mem_fun(this, &Lens::Impl::set_search_in_global));
  owner_->shortcut.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::shortcut));
  owner_->results.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::results));
  owner_->global_results.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::global_results));
  owner_->categories.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::categories));
  owner_->filters.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::filters));
  owner_->connected.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::connected));
  owner_->view_type.changed.connect(sigc::mem_fun(this, &Lens::Impl::OnViewTypeChanged));
}

Lens::Impl::~Impl()
{
  if (search_cancellable_)
  {
    g_cancellable_cancel (search_cancellable_);
  }
  if (global_search_cancellable_)
  {
    g_cancellable_cancel (global_search_cancellable_);
  }

  if (proxy_)
    delete proxy_;
}

void Lens::Impl::OnProxyConnectionChanged()
{
  if (proxy_->IsConnected())
  {
    proxy_->Call("InfoRequest");
    ViewType current_view_type = owner_->view_type;
    proxy_->Call("SetViewType", g_variant_new("(u)", current_view_type));
  }
}

void Lens::Impl::OnProxyDisconnected()
{
  connected_ = false;
  owner_->connected.EmitChanged(connected_);
}

void Lens::Impl::ResultsModelUpdated(unsigned long long begin_seqnum,
                                     unsigned long long end_seqnum)
{
  if (results_variant_ != NULL &&
      end_seqnum >= ExtractModelSeqnum (results_variant_))
  {
    glib::Variant dict(results_variant_, glib::StealRef());
    Hints hints;

    dict.ASVToHints(hints);

    owner_->search_finished.emit(hints);

    results_variant_ = NULL;
  }
}

void Lens::Impl::GlobalResultsModelUpdated(unsigned long long begin_seqnum,
                                           unsigned long long end_seqnum)
{
  if (global_results_variant_ != NULL &&
      end_seqnum >= ExtractModelSeqnum (global_results_variant_))
  {
    glib::Variant dict(global_results_variant_, glib::StealRef());
    Hints hints;

    dict.ASVToHints(hints);

    owner_->global_search_finished.emit(hints);

    global_results_variant_ = NULL;
  }
}

unsigned long long Lens::Impl::ExtractModelSeqnum(GVariant *parameters)
{
  GVariant *dict;
  guint64 seqnum64;
  unsigned long long seqnum = 0;

  dict = g_variant_get_child_value (parameters, 0);
  if (dict)
  {
    if (g_variant_lookup (dict, "model-seqnum", "t", &seqnum64))
    {
      seqnum = static_cast<unsigned long long> (seqnum64);
    }

    g_variant_unref (dict);
  }

  return seqnum;
}

void Lens::Impl::OnSearchFinished(GVariant* parameters)
{
  Hints hints;
  unsigned long long reply_seqnum;

  reply_seqnum = ExtractModelSeqnum (parameters);
  if (results_->seqnum < reply_seqnum)
  {
    // wait for the end-transaction signal
    if (results_variant_) g_variant_unref (results_variant_);
    results_variant_ = g_variant_ref (parameters);

    // ResultsModelUpdated will emit OnSearchFinished
    return;
  }

  glib::Variant dict (parameters);
  dict.ASVToHints(hints);

  owner_->search_finished.emit(hints);
}

void Lens::Impl::OnGlobalSearchFinished(GVariant* parameters)
{
  Hints hints;
  unsigned long long reply_seqnum;
  
  reply_seqnum = ExtractModelSeqnum (parameters);
  if (global_results_->seqnum < reply_seqnum)
  {
    // wait for the end-transaction signal
    if (global_results_variant_) g_variant_unref (global_results_variant_);
    global_results_variant_ = g_variant_ref (parameters);

    // GlobalResultsModelUpdated will emit OnGlobalSearchFinished
    return;
  }

  glib::Variant dict (parameters);
  dict.ASVToHints(hints);

  owner_->global_search_finished.emit(hints);
}

void Lens::Impl::OnChanged(GVariant* parameters)
{
  glib::String dbus_path;
  gboolean search_in_global = FALSE;
  gboolean visible = FALSE;
  glib::String search_hint;
  glib::String private_connection_name;
  glib::String results_model_name;
  glib::String global_results_model_name;
  glib::String categories_model_name;
  glib::String filters_model_name;
  GVariantIter* hints_iter = NULL;

  g_variant_get(parameters, "((sbbssssssa{sv}))",
                &dbus_path,
                &search_in_global,
                &visible,
                &search_hint,
                &private_connection_name,
                &results_model_name,
                &global_results_model_name,
                &categories_model_name,
                &filters_model_name,
                &hints_iter);

  LOG_DEBUG(logger) << "Lens info changed for " << name_ << "\n"
                    << "  Path: " << dbus_path << "\n"
                    << "  SearchInGlobal: " << search_in_global << "\n"
                    << "  Visible: " << visible << "\n"
                    << "  PrivateConnName: " << private_connection_name << "\n"
                    << "  Results: " << results_model_name << "\n"
                    << "  GlobalModel: " << global_results_model_name << "\n"
                    << "  Categories: " << categories_model_name << "\n"
                    << "  Filters: " << filters_model_name << "\n";
  if (dbus_path.Str() == dbus_path_)
  {
    /* FIXME: We ignore hints for now */
    UpdateProperties(search_in_global,
                     visible,
                     search_hint.Str(),
                     private_connection_name.Str(),
                     results_model_name.Str(),
                     global_results_model_name.Str(),
                     categories_model_name.Str(),
                     filters_model_name.Str());
  }
  else
  {
    LOG_WARNING(logger) << "Paths do not match " << dbus_path_ << " != " << dbus_path;
  }

  connected_ = true;
  owner_->connected.EmitChanged(connected_);

  g_variant_iter_free(hints_iter);
}

void Lens::Impl::UpdateProperties(bool search_in_global,
                                  bool visible,
                                  string const& search_hint,
                                  string const& private_connection_name,
                                  string const& results_model_name,
                                  string const& global_results_model_name,
                                  string const& categories_model_name,
                                  string const& filters_model_name)
{
  // Diff the properties received from those we have
  if (search_hint_ != search_hint)
  {
    search_hint_ = search_hint;
    owner_->search_hint.EmitChanged(search_hint_);
  }

  if (search_in_global_ != search_in_global)
  {
    search_in_global_ = search_in_global;
    owner_->search_in_global.EmitChanged(search_in_global_);
  }

  if (visible_ != visible)
  {
    visible_ = visible;
    owner_->visible.EmitChanged(visible_);
  }

  if (private_connection_name_ != private_connection_name)
  {
    // FIXME: Update all the models as they are no longer valid when we use this
    private_connection_name_ = private_connection_name;
  }
  categories_->swarm_name = categories_model_name;
  filters_->swarm_name = filters_model_name;
  global_results_->swarm_name = global_results_model_name;
  results_->swarm_name = results_model_name;
}

void Lens::Impl::OnViewTypeChanged(ViewType view_type)
{
  if (proxy_ && proxy_->IsConnected())
    proxy_->Call("SetViewType", g_variant_new("(u)", view_type));
}

void Lens::Impl::GlobalSearch(std::string const& search_string)
{
  LOG_DEBUG(logger) << "Global Searching '" << id_ << "' for '" << search_string << "'";

  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));

  if (global_search_cancellable_)
    g_cancellable_cancel (global_search_cancellable_);
  global_search_cancellable_ = g_cancellable_new ();

  if (global_results_variant_)
  {
    g_variant_unref (global_results_variant_);
    global_results_variant_ = NULL;
  }

  proxy_->Call("GlobalSearch",
              g_variant_new("(sa{sv})",
                            search_string.c_str(),
                            &b),
              sigc::mem_fun(this, &Lens::Impl::OnGlobalSearchFinished),
              global_search_cancellable_);
  g_variant_builder_clear(&b);
}

void Lens::Impl::Search(std::string const& search_string)
{
  LOG_DEBUG(logger) << "Searching '" << id_ << "' for '" << search_string << "'";

  if (proxy_ == NULL)
  {
    LOG_DEBUG(logger) << "Skipping search. Proxy not initialized. ('" << id_ << "')";
    return;
  }

  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));

  if (search_cancellable_) g_cancellable_cancel (search_cancellable_);
  search_cancellable_ = g_cancellable_new ();

  if (results_variant_)
  {
    g_variant_unref (results_variant_);
    results_variant_ = NULL;
  }

  proxy_->Call("Search",
               g_variant_new("(sa{sv})",
                             search_string.c_str(),
                             &b),
               sigc::mem_fun(this, &Lens::Impl::OnSearchFinished),
               search_cancellable_);

  g_variant_builder_clear(&b);
}

void Lens::Impl::Activate(std::string const& uri)
{
  LOG_DEBUG(logger) << "Activating '" << uri << "' on  '" << id_ << "'";

  if (!proxy_->IsConnected())
    {
      LOG_DEBUG(logger) << "Skipping activation. Proxy not connected. ('" << id_ << "')";
      return;
    }

  proxy_->Call("Activate",
               g_variant_new("(su)", uri.c_str(),
                             UNITY_PROTOCOL_ACTION_TYPE_ACTIVATE_RESULT),
               sigc::mem_fun(this, &Lens::Impl::ActivationReply));
}

void Lens::Impl::ActivationReply(GVariant* parameters)
{
  glib::String uri;
  guint32 handled;
  GVariant* hints_variant;
  Hints hints;
  
  g_variant_get(parameters, "((su@a{sv}))", &uri, &handled, &hints_variant);

  glib::Variant dict (hints_variant, glib::StealRef());
  dict.ASVToHints(hints);

  if (handled == UNITY_PROTOCOL_HANDLED_TYPE_SHOW_PREVIEW)
  {
    auto iter = hints.find("preview");
    if (iter != hints.end())
    {
      Preview::Ptr preview(Preview::PreviewForVariant(iter->second));
      if (preview)
      {
        // would be nice to make parent_lens a shared_ptr,
        // but that's not really doable from here
        preview->parent_lens = owner_;
        preview->preview_uri = uri.Str();
        owner_->preview_ready.emit(uri.Str(), preview);
        return;
      }
    }

    LOG_WARNING(logger) << "Unable to deserialize Preview";
  }
  else
  {
    owner_->activated.emit(uri.Str(), static_cast<HandledType>(handled), hints);
  }
}

void Lens::Impl::Preview(std::string const& uri)
{
  LOG_DEBUG(logger) << "Previewing '" << uri << "' on  '" << id_ << "'";

  if (!proxy_->IsConnected())
    {
      LOG_DEBUG(logger) << "Skipping preview. Proxy not connected. ('" << id_ << "')";
      return;
    }

  if (preview_cancellable_)
  {
    g_cancellable_cancel(preview_cancellable_);
  }
  preview_cancellable_ = g_cancellable_new ();

  proxy_->Call("Activate",
               g_variant_new("(su)", uri.c_str(),
                             UNITY_PROTOCOL_ACTION_TYPE_PREVIEW_RESULT),
               sigc::mem_fun(this, &Lens::Impl::ActivationReply),
               preview_cancellable_);
}

void Lens::Impl::ActivatePreviewAction(std::string const& action_id,
                                       std::string const& uri)
{
  LOG_DEBUG(logger) << "Activating action '" << action_id << "' on  '" << id_ << "'";

  if (!proxy_->IsConnected())
    {
      LOG_DEBUG(logger) << "Skipping activation. Proxy not connected. ('" << id_ << "')";
      return;
    }

  std::string activation_uri(action_id);
  activation_uri += ":";
  activation_uri += uri;

  proxy_->Call("Activate",
               g_variant_new("(su)", activation_uri.c_str(),
                             UNITY_PROTOCOL_ACTION_TYPE_PREVIEW_ACTION),
               sigc::mem_fun(this, &Lens::Impl::ActivationReply));
}

void Lens::Impl::SignalPreview(std::string const& preview_uri,
                               glib::Variant const& preview_update,
                               glib::DBusProxy::ReplyCallback reply_cb)
{
  LOG_DEBUG(logger) << "Signalling preview '" << preview_uri << "' on  '" << id_ << "'";

  if (!proxy_->IsConnected())
    {
      LOG_DEBUG(logger) << "Can't signal preview. Proxy not connected. ('" << id_ << "')";
      return;
    }

  GVariant *preview_update_variant = preview_update;
  proxy_->Call("UpdatePreviewProperty",
               g_variant_new("(s@a{sv})", preview_uri.c_str(),
                             preview_update_variant),
               reply_cb);
}

string const& Lens::Impl::id() const
{
  return id_;
}

string const& Lens::Impl::dbus_name() const
{
  return dbus_name_;
}

string const& Lens::Impl::dbus_path() const
{
  return dbus_path_;
}

string const& Lens::Impl::name() const
{
  return name_;
}

string const& Lens::Impl::icon_hint() const
{
  return icon_hint_;
}

string const& Lens::Impl::description() const
{
  return description_;
}

string const& Lens::Impl::search_hint() const
{
  return search_hint_;
}

bool Lens::Impl::visible() const
{
  return visible_;
}

bool Lens::Impl::search_in_global() const
{
  return search_in_global_;
}

bool Lens::Impl::set_search_in_global(bool val)
{
  if (search_in_global_ != val)
  {
    search_in_global_ = val;
    owner_->search_in_global.EmitChanged(val);
  }

  return search_in_global_;
}

string const& Lens::Impl::shortcut() const
{
  return shortcut_;
}

Results::Ptr const& Lens::Impl::results() const
{
  return results_;
}

Results::Ptr const& Lens::Impl::global_results() const
{
  return global_results_;
}

Categories::Ptr const& Lens::Impl::categories() const
{
  return categories_;
}

Filters::Ptr const& Lens::Impl::filters() const
{
  return filters_;
}

bool Lens::Impl::connected() const
{
  return connected_;
}

Lens::Lens(string const& id_,
           string const& dbus_name_,
           string const& dbus_path_,
           string const& name_,
           string const& icon_hint_,
           string const& description_,
           string const& search_hint_,
           bool visible_,
           string const& shortcut_)

  : pimpl(new Impl(this,
                   id_,
                   dbus_name_,
                   dbus_path_,
                   name_,
                   icon_hint_,
                   description_,
                   search_hint_,
                   visible_,
                   shortcut_,
                   ModelType::REMOTE))
{}

Lens::Lens(string const& id_,
            string const& dbus_name_,
            string const& dbus_path_,
            string const& name_,
            string const& icon_hint_,
            string const& description_,
            string const& search_hint_,
            bool visible_,
            string const& shortcut_,
            ModelType model_type)

  : pimpl(new Impl(this,
                   id_,
                   dbus_name_,
                   dbus_path_,
                   name_,
                   icon_hint_,
                   description_,
                   search_hint_,
                   visible_,
                   shortcut_,
                   model_type))
{}

Lens::~Lens()
{
  delete pimpl;
}

void Lens::GlobalSearch(std::string const& search_string)
{
  pimpl->GlobalSearch(search_string);
}

void Lens::Search(std::string const& search_string)
{
  pimpl->Search(search_string);
}

void Lens::Activate(std::string const& uri)
{
  pimpl->Activate(uri);
}

void Lens::Preview(std::string const& uri)
{
  pimpl->Preview(uri);
}

void Lens::ActivatePreviewAction(std::string const& action_id,
                                 std::string const& uri)
{
  pimpl->ActivatePreviewAction(action_id, uri);
}

void Lens::SignalPreview(std::string const& uri,
                         glib::Variant const& preview_update,
                         glib::DBusProxy::ReplyCallback reply_cb)
{
  pimpl->SignalPreview(uri, preview_update, reply_cb);
}


}
}
