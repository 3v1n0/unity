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

const unsigned CATEGORY_COLUMN = 2;
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
                        string const& filters_model_name,
                        GVariantIter* hints_iter);
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
  std::vector<unsigned> GetCategoriesOrder();
  glib::Object<DeeModel> GetFilterModelForCategory(unsigned category);
  void GetFilterForCategoryIndex(unsigned category, DeeFilter* filter);

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
  bool provides_personal_content() const;

  string const& last_search_string() const { return last_search_string_; }
  string const& last_global_search_string() const { return last_global_search_string_; }

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
  bool provides_personal_content_;

  string private_connection_name_;
  string last_search_string_;
  string last_global_search_string_;

  glib::DBusProxy* proxy_;
  glib::Object<GCancellable> search_cancellable_;
  glib::Object<GCancellable> global_search_cancellable_;
  glib::Object<GCancellable> preview_cancellable_;

  glib::Variant results_variant_;
  glib::Variant global_results_variant_;
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
  , provides_personal_content_(false)
  , proxy_(NULL)
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
  owner_->provides_personal_content.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::provides_personal_content));
  owner_->last_search_string.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::last_search_string));
  owner_->last_global_search_string.SetGetterFunction(sigc::mem_fun(this, &Lens::Impl::last_global_search_string));
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
  if (results_variant_ && end_seqnum >= ExtractModelSeqnum (results_variant_))
  {
    Hints hints;

    results_variant_.ASVToHints(hints);

    owner_->search_finished.emit(hints);

    results_variant_ = NULL;
  }
}

void Lens::Impl::GlobalResultsModelUpdated(unsigned long long begin_seqnum,
                                           unsigned long long end_seqnum)
{
  if (global_results_variant_ &&
      end_seqnum >= ExtractModelSeqnum (global_results_variant_))
  {
    Hints hints;

    global_results_variant_.ASVToHints(hints);

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
    results_variant_ = parameters;

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
    global_results_variant_ = parameters;

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
    std::string categories_model_swarm_name(categories_model_name.Str());
    std::string filters_model_swarm_name(filters_model_name.Str());
    std::string results_model_swarm_name(results_model_name.Str());
    std::string global_results_model_swarm_name(global_results_model_name.Str());
    bool models_changed = 
      categories_model_swarm_name != categories_->swarm_name ||
      filters_model_swarm_name != filters_->swarm_name ||
      results_model_swarm_name != results_->swarm_name ||
      global_results_model_swarm_name != global_results_->swarm_name;

    /* FIXME: We ignore hints for now */
    UpdateProperties(search_in_global,
                     visible,
                     search_hint.Str(),
                     private_connection_name.Str(),
                     results_model_swarm_name,
                     global_results_model_swarm_name,
                     categories_model_swarm_name,
                     filters_model_swarm_name,
                     hints_iter);

    if (models_changed) owner_->models_changed.emit();
  }
  else
  {
    LOG_WARNING(logger) << "Paths do not match " << dbus_path_ << " != " << dbus_path;
  }

  if (!connected_)
  {
    connected_ = true;
    owner_->connected.EmitChanged(connected_);
  }

  g_variant_iter_free(hints_iter);
}

void Lens::Impl::UpdateProperties(bool search_in_global,
                                  bool visible,
                                  string const& search_hint,
                                  string const& private_connection_name,
                                  string const& results_model_name,
                                  string const& global_results_model_name,
                                  string const& categories_model_name,
                                  string const& filters_model_name,
                                  GVariantIter* hints_iter)
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

  bool provides_personal_content = false;
  gchar* key;
  GVariant* value;

  // g_variant_iter_loop manages the memory automatically, as long
  // as the iteration is not stopped in the middle
  while (g_variant_iter_loop(hints_iter, "{sv}", &key, &value))
  {
    if (g_strcmp0(key, "provides-personal-content") == 0)
    {
      provides_personal_content = g_variant_get_boolean(value) != FALSE;
    }
  }

  if (provides_personal_content_ != provides_personal_content)
  {
    provides_personal_content_ = provides_personal_content;
    owner_->provides_personal_content.EmitChanged(provides_personal_content_);
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

  global_results_variant_ = NULL;
  last_global_search_string_ = search_string;

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

  results_variant_ = NULL;
  last_search_string_ = search_string;

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

  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));

  proxy_->Call("Activate",
               g_variant_new("(sua{sv})", uri.c_str(),
                             UNITY_PROTOCOL_ACTION_TYPE_ACTIVATE_RESULT,
                             &b),
               sigc::mem_fun(this, &Lens::Impl::ActivationReply));

  g_variant_builder_clear(&b);
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

  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));

  proxy_->Call("Activate",
               g_variant_new("(sua{sv})", uri.c_str(),
                             UNITY_PROTOCOL_ACTION_TYPE_PREVIEW_RESULT,
                             &b),
               sigc::mem_fun(this, &Lens::Impl::ActivationReply),
               preview_cancellable_);

  g_variant_builder_clear(&b);
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

  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));

  proxy_->Call("Activate",
               g_variant_new("(sua{sv})", activation_uri.c_str(),
                             UNITY_PROTOCOL_ACTION_TYPE_PREVIEW_ACTION,
                             &b),
               sigc::mem_fun(this, &Lens::Impl::ActivationReply));

  g_variant_builder_clear(&b);
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

std::vector<unsigned> Lens::Impl::GetCategoriesOrder()
{
  std::vector<unsigned> result;
  for (std::size_t i=0; i < categories_->count; i++)
  {
    result.push_back(i);
  }

  return result;
}

glib::Object<DeeModel> Lens::Impl::GetFilterModelForCategory(unsigned category)
{
  DeeFilter filter;
  GetFilterForCategoryIndex(category, &filter);
  glib::Object<DeeModel> filter_model(dee_filter_model_new(results_->model(), &filter));

  return filter_model;
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

void Lens::Impl::GetFilterForCategoryIndex(unsigned category, DeeFilter* filter)
{
  filter->map_func = category_filter_map_func;
  filter->map_notify = category_filter_notify_func;
  filter->destroy = nullptr;
  filter->userdata = GUINT_TO_POINTER(category);
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

bool Lens::Impl::provides_personal_content() const
{
  return provides_personal_content_;
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

std::vector<unsigned> Lens::GetCategoriesOrder()
{
  return pimpl->GetCategoriesOrder();
}

glib::Object<DeeModel> Lens::GetFilterModelForCategory(unsigned category)
{
  return pimpl->GetFilterModelForCategory(category);
}


}
}
