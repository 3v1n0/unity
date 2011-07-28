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
  typedef std::vector<string> URIPatterns;
  typedef std::vector<string> MIMEPatterns;

  Impl(Lens* owner,
       string const& id,
       string const& dbus_name,
       string const& dbus_path,
       string const& name,
       string const& icon,
       string const& description,
       string const& search_hint,
       bool visible,
       string const& shortcut);

  ~Impl();

  void OnProxyConnected();
  void OnProxyDisconnected();

  void OnSearchFinished(GVariant* parameters);
  void OnGlobalSearchFinished(GVariant* parameters);
  void OnChanged(GVariant* parameters);
  URIPatterns URIPatternsFromIter(GVariantIter* iter);
  MIMEPatterns MIMEPatternsFromIter(GVariantIter* iter);
  void UpdateProperties(bool search_in_global,
                        bool visible,
                        string const& search_hint,
                        string const& private_connection_name,
                        string const& results_model_name,
                        string const& global_results_model_name,
                        string const& categories_model_name,
                        string const& filters_model_name,
                        URIPatterns uri_patterns,
                        MIMEPatterns mime_patterns);
  void OnActiveChanged(bool is_active);
  void GlobalSearch(std::string const& search_string);
  void Search(std::string const& search_string);

  string const& id() const;
  string const& dbus_name() const;
  string const& dbus_path() const;
  string const& name() const;
  string const& icon() const;
  string const& description() const;
  string const& search_hint() const;
  bool visible() const;
  bool search_in_global() const;
  string const& shortcut() const;
  Results::Ptr const& results() const;
  Results::Ptr const& global_results() const;
  Categories::Ptr const& categories() const;

  Lens* owner_;

  string id_;
  string dbus_name_;
  string dbus_path_;
  string name_;
  string icon_;
  string description_;
  string search_hint_;
  bool visible_;
  bool search_in_global_;
  string shortcut_;
  Results::Ptr results_;
  Results::Ptr global_results_;
  Categories::Ptr categories_;

  string private_connection_name_;

  glib::DBusProxy proxy_;
};

Lens::Impl::Impl(Lens* owner,
                 string const& id,
                 string const& dbus_name,
                 string const& dbus_path,
                 string const& name,
                 string const& icon,
                 string const& description,
                 string const& search_hint,
                 bool visible,
                 string const& shortcut)
  : owner_(owner)
  , id_(id)
  , dbus_name_(dbus_name)
  , dbus_path_(dbus_path)
  , name_(name)
  , icon_(icon)
  , description_(description)
  , search_hint_(search_hint)
  , visible_(visible)
  , search_in_global_(false)
  , shortcut_(shortcut)
  , results_(new Results())
  , global_results_(new Results())
  , categories_(new Categories())
  , proxy_(dbus_name, dbus_path, "com.canonical.Unity.Lens")
{
  proxy_.connected.connect(sigc::mem_fun(this, &Lens::Impl::OnProxyConnected));
  proxy_.disconnected.connect(sigc::mem_fun(this, &Lens::Impl::OnProxyDisconnected));
  proxy_.Connect("Changed", sigc::mem_fun(this, &Lens::Impl::OnChanged));
  proxy_.Connect("SearchFinished", sigc::mem_fun(this, &Lens::Impl::OnSearchFinished));
  proxy_.Connect("GlobalSearchFinished", sigc::mem_fun(this, &Lens::Impl::OnGlobalSearchFinished));
}

Lens::Impl::~Impl()
{}

void Lens::Impl::OnProxyConnected()
{
  proxy_.Call("InfoRequest");
  proxy_.Call("SetActive", g_variant_new("b", owner_->active ? TRUE : FALSE));
}

void Lens::Impl::OnProxyDisconnected()
{
  //FIXME: When we're ready, we need to reset the entire model/category/filters state
}

void Lens::Impl::OnSearchFinished(GVariant* parameters)
{
  char* search_string;

  g_variant_get(parameters, "(sa{sv})", &search_string, NULL);
  owner_->search_finished(search_string);

  g_free(search_string);
}

void Lens::Impl::OnGlobalSearchFinished(GVariant* parameters)
{
  char* search_string;

  g_variant_get(parameters, "(sa{sv})", &search_string, NULL);
  owner_->global_search_finished(search_string);

  g_free(search_string);
}

void Lens::Impl::OnChanged(GVariant* parameters)
{
  char* dbus_path = NULL;
  gboolean search_in_global;
  gboolean visible;
  char* search_hint = NULL;
  char* private_connection_name = NULL;
  char* results_model_name = NULL;
  char* global_results_model_name = NULL;
  char* categories_model_name = NULL;
  char* filters_model_name = NULL;
  GVariantIter* uri_patterns_iter = NULL;
  GVariantIter* mime_patterns_iter = NULL;
  GVariantIter* hints_iter = NULL;

  g_variant_get(parameters, "((sbbssssssasasa{sv}))",
                &dbus_path,
                &search_in_global,
                &visible,
                &search_hint,
                &private_connection_name,
                &results_model_name,
                &global_results_model_name,
                &categories_model_name,
                &filters_model_name,
                &uri_patterns_iter,
                &mime_patterns_iter,
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
  if (dbus_path == dbus_path_)
  {
    URIPatterns uri_patterns = URIPatternsFromIter(uri_patterns_iter);
    MIMEPatterns mime_patterns = MIMEPatternsFromIter(mime_patterns_iter);
    /* FIXME: We ignore hints for now */
    UpdateProperties(search_in_global,
                     visible,
                     search_hint,
                     private_connection_name,
                     results_model_name,
                     global_results_model_name,
                     categories_model_name,
                     filters_model_name,
                     uri_patterns,
                     mime_patterns);
  }
  else
  {
    LOG_WARNING(logger) << "Paths do not match " << dbus_path_ << " != " << dbus_path;
  }

  g_free(dbus_path);
  g_free(search_hint);
  g_free(private_connection_name);
  g_free(results_model_name);
  g_free(global_results_model_name);
  g_free(categories_model_name);
  g_free(filters_model_name);
  g_variant_iter_free(uri_patterns_iter);
  g_variant_iter_free(mime_patterns_iter);
  g_variant_iter_free(hints_iter);
}

Lens::Impl::URIPatterns Lens::Impl::URIPatternsFromIter(GVariantIter* iter)
{
  URIPatterns pats;
  return pats;
}

Lens::Impl::MIMEPatterns Lens::Impl::MIMEPatternsFromIter(GVariantIter* iter)
{
  MIMEPatterns pats;
  return pats;
}

void Lens::Impl::UpdateProperties(bool search_in_global,
                                  bool visible,
                                  string const& search_hint,
                                  string const& private_connection_name,
                                  string const& results_model_name,
                                  string const& global_results_model_name,
                                  string const& categories_model_name,
                                  string const& filters_model_name,
                                  URIPatterns uri_patterns,
                                  MIMEPatterns mime_patterns)
{
  // Diff the properties received from those we have
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

  results_->swarm_name = results_model_name;
  global_results_->swarm_name = global_results_model_name;
  categories_->swarm_name = categories_model_name;
}

void Lens::Impl::OnActiveChanged(bool is_active)
{
  proxy_.Call("SetActive", g_variant_new("b", is_active ? TRUE : FALSE));
}

void Lens::Impl::GlobalSearch(std::string const& search_string)
{
  LOG_DEBUG(logger) << "Global Searching " << id_ << " for " << search_string;

  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));

  proxy_.Call("GlobalSearch",
              g_variant_new("(sa{sv})",
                            search_string.c_str(),
                            g_variant_builder_end(&b)));
}

void Lens::Impl::Search(std::string const& search_string)
{
  LOG_DEBUG(logger) << "Searching " << id_ << " for " << search_string;

  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));

  proxy_.Call("Search",
              g_variant_new("(sa{sv})",
                            search_string.c_str(),
                            g_variant_builder_end(&b)));
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

string const& Lens::Impl::icon() const
{
  return icon_;
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

Lens::Lens(string const& id_,
           string const& dbus_name_,
           string const& dbus_path_,
           string const& name_,
           string const& icon_,
           string const& description_,
           string const& search_hint_,
           bool visible_,
           string const& shortcut_)

  : pimpl(new Impl(this,
                   id_,
                   dbus_name_,
                   dbus_path_,
                   name_,
                   icon_,
                   description_,
                   search_hint_,
                   visible_,
                   shortcut_))
{
  id.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::id));
  dbus_name.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::dbus_name));
  dbus_path.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::dbus_path));
  name.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::name));
  icon.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::icon));
  description.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::description));
  search_hint.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::search_hint));
  visible.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::visible));
  search_in_global.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::search_in_global));
  shortcut.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::shortcut));
  results.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::results));
  global_results.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::global_results));
  categories.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::categories));
  active.changed.connect(sigc::mem_fun(pimpl, &Lens::Impl::OnActiveChanged));
}

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

}
}
