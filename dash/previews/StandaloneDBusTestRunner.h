/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

 #include <UnityCore/GLibDBusProxy.h>

#include <UnityCore/Preview.h>
#include <UnityCore/Lens.h>
#include <UnityCore/Results.h>
#include <NuxCore/Logger.h>

namespace unity
{
namespace dash
{
namespace previews
{
namespace
{
nux::logging::Logger logger("unity.dash.StandaloneDBusTestRunner");
}


class StandaloneDBusTestRunner
{
public:
  typedef std::map<std::string, unity::glib::Variant> Hints;

  StandaloneDBusTestRunner(std::string const& dbus_name, std::string const& dbus_path)
  : proxy_(nullptr)
  , results_(new Results(ModelType::REMOTE))
  , results_variant_(NULL)
  , connected_(false)
  , dbus_name_(dbus_name)
  , dbus_path_(dbus_path)
  {
    proxy_ = new glib::DBusProxy(dbus_name, dbus_path, "com.canonical.Unity.Lens");
    proxy_->connected.connect(sigc::mem_fun(this, &StandaloneDBusTestRunner::OnProxyConnectionChanged));
    proxy_->disconnected.connect(sigc::mem_fun(this, &StandaloneDBusTestRunner::OnProxyDisconnected));
    proxy_->Connect("Changed", sigc::mem_fun(this, &StandaloneDBusTestRunner::OnChanged));

    results_->end_transaction.connect(sigc::mem_fun(this, &StandaloneDBusTestRunner::ResultsModelUpdated));
  }

  void OnProxyConnectionChanged()
  {
    LOG_DEBUG(logger) << "Dbus connection changed. connected=" << (proxy_->IsConnected() ? "true" : "false");

    if (proxy_->IsConnected())
    {
      proxy_->Call("InfoRequest");
      proxy_->Call("SetViewType", g_variant_new("(u)", LENS_VIEW));
    }
  }

  void OnProxyDisconnected()
  {
    LOG_DEBUG(logger) << "Dbus disconnected";

    connected_ = false;
    connected.emit(connected_);
  }

  void OnChanged(GVariant* parameters)
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

    LOG_DEBUG(logger) << "Lens info changed for " << dbus_name_ << "\n"
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
      results_->swarm_name = results_model_name;
      LOG_DEBUG(logger) << "Received changes for " << dbus_path;
    }
    else
    {
      LOG_WARNING(logger) << "Paths do not match " << dbus_path_ << " != " << dbus_path;
    }

    connected_ = true;
    connected.emit(connected_);

    g_variant_iter_free(hints_iter);
  }

  void Search(std::string const& search_string)
  {
    LOG_DEBUG(logger) << "Searching '" << dbus_name_ << "' for '" << search_string << "'";

    if (proxy_ == NULL)
    {
      LOG_DEBUG(logger) << "Skipping search. Proxy not initialized. ('" << dbus_name_ << "')";
      return;
    }

    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));

    if (search_cancellable_) g_cancellable_cancel (search_cancellable_);
    search_cancellable_ = g_cancellable_new ();

    proxy_->Call("Search",
                 g_variant_new("(sa{sv})",
                               search_string.c_str(),
                               &b),
                 sigc::mem_fun(this, &StandaloneDBusTestRunner::OnSearchFinished),
                 search_cancellable_);

    g_variant_builder_clear(&b);
  }
  
  void OnSearchFinished(GVariant* parameters)
  {
    Hints hints;
    unsigned long long reply_seqnum;
    reply_seqnum = ExtractModelSeqnum (parameters);
    if (results_->seqnum < reply_seqnum)
    {
      // wait for the end-transaction signal
      if (results_variant_) g_variant_unref (results_variant_);
      results_variant_ = g_variant_ref (parameters);

      return;
    }

    glib::Variant dict (parameters);
    dict.ASVToHints(hints);

    search_finished.emit(hints);
  }

  void ResultsModelUpdated(unsigned long long begin_seqnum,
                                       unsigned long long end_seqnum)
  {
    if (results_variant_ != NULL &&
        end_seqnum >= ExtractModelSeqnum (results_variant_))
    {
      glib::Variant dict(results_variant_, glib::StealRef());
      Hints hints;

      dict.ASVToHints(hints);

      search_finished.emit(hints);

      results_variant_ = NULL;
    }
  }



  void Preview(std::string const& uri)
  {
    LOG_DEBUG(logger) << "Previewing '" << uri << "' on  '" << dbus_name_ << "'";

    if (!proxy_->IsConnected())
    {
      LOG_DEBUG(logger) << "Skipping preview. Proxy not connected. ('" << dbus_name_ << "')";
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
                 sigc::mem_fun(this, &StandaloneDBusTestRunner::ActivationReply),
                 preview_cancellable_);
  }

  void ActivationReply(GVariant* parameters)
  {    
    glib::String uri;
    guint32 handled;
    GVariant* hints_variant;
    Hints hints;
    
    g_variant_get(parameters, "((su@a{sv}))", &uri, &handled, &hints_variant);

    glib::Variant dict (hints_variant, glib::StealRef());
    dict.ASVToHints(hints);

    LOG_WARNING(logger) << "ActivationReply type: " << handled;

    if (handled == UNITY_PROTOCOL_HANDLED_TYPE_SHOW_PREVIEW)
    {
      auto iter = hints.find("preview");
      if (iter != hints.end())
      {
        glib::Object<UnityProtocolPreview> proto_object(unity_protocol_preview_parse(iter->second));
        if (!proto_object)
        {
          LOG_WARN(logger) << "Unable to create Preview object for variant type: " << g_variant_get_type_string(iter->second);
          return;
        }
        else
        {
          // would be nice to make parent_lens a shared_ptr,
          // but that's not really doable from here
          preview_ready.emit(uri.Str(), proto_object);
          return;
        }
      }

      LOG_WARNING(logger) << "Unable to deserialize Preview";
    }
    else
    {
    }
  }


  sigc::signal<void, bool> connected;
  sigc::signal<void, std::string const&, glib::Object<UnityProtocolPreview> const&> preview_ready;
  sigc::signal<void, Hints const&> search_finished;

protected:
  unsigned long long ExtractModelSeqnum(GVariant *parameters)
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


  glib::DBusProxy* proxy_;
  glib::Object<GCancellable> preview_cancellable_;
  glib::Object<GCancellable> search_cancellable_;
  Results::Ptr results_;
  GVariant *results_variant_;

  bool connected_;
  std::string dbus_name_;
  std::string dbus_path_;
};






} // namespace previews
} // namespace dash
} // namespace unity