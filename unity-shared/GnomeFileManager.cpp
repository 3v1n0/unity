// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012-2013 Canonical Ltd
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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "GnomeFileManager.h"
#include <NuxCore/Logger.h>
#include <UnityCore/DesktopUtilities.h>
#include <UnityCore/GLibSource.h>
#include <UnityCore/GLibDBusProxy.h>
#include <UnityCore/GLibWrapper.h>
#include <gdk/gdk.h>
#include <gio/gio.h>

namespace unity
{

namespace
{
DECLARE_LOGGER(logger, "unity.filemanager.gnome");

const std::string TRASH_URI = "trash:///";
const std::string FILE_SCHEMA = "file://";

const std::string NAUTILUS_NAME = "org.gnome.Nautilus";
const std::string NAUTILUS_PATH = "/org/gnome/Nautilus";
}

struct GnomeFileManager::Impl
{
  Impl(GnomeFileManager* parent)
    : parent_(parent)
    , filemanager_proxy_("org.freedesktop.FileManager1", "/org/freedesktop/FileManager1", "org.freedesktop.FileManager1", G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS)
  {
    auto callback = sigc::mem_fun(this, &Impl::OnOpenLocationsXidsUpdated);
    filemanager_proxy_.GetProperty("XUbuntuOpenLocationsXids", callback);
    filemanager_proxy_.ConnectProperty("XUbuntuOpenLocationsXids", callback);
  }

  glib::DBusProxy::Ptr NautilusOperationsProxy() const
  {
    auto flags = static_cast<GDBusProxyFlags>(G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES|G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS);
    return std::make_shared<glib::DBusProxy>(NAUTILUS_NAME, NAUTILUS_PATH,
                                             "org.gnome.Nautilus.FileOperations",
                                             G_BUS_TYPE_SESSION, flags);
  }

  void OnOpenLocationsXidsUpdated(GVariant* value)
  {
    opened_location_for_xid_.clear();

    if (!value)
    {
      LOG_WARN(logger) << "Locations have been invalidated, maybe there's no filemanager around...";
      parent_->locations_changed.emit();
      return;
    }

    if (!g_variant_is_of_type(value, G_VARIANT_TYPE("a{uas}")))
    {
      LOG_ERROR(logger) << "Locations value type is not matching the expected one!";
      parent_->locations_changed.emit();
      return;
    }

    GVariantIter iter;
    GVariantIter *str_iter;
    const char *loc;
    guint32 xid;

    g_variant_iter_init(&iter, value);

    while (g_variant_iter_loop(&iter, "{uas}", &xid, &str_iter))
    {
      while (g_variant_iter_loop(str_iter, "s", &loc))
      {
        /* We only care about the first mentioned location as per our "standard"
         * it's the active one */
        LOG_DEBUG(logger) << xid << ": Opened location " << loc;
        opened_location_for_xid_[xid] = loc;
        break;
      }
    }

    // We must ensure that we emit the locations_changed signal only when all
    // the parent windows have been registered on the app-manager
    auto app_manager_not_synced = [this]
    {
      auto& app_manager = ApplicationManager::Default();
      bool synced = true;

      for (auto const& pair : opened_location_for_xid_)
      {
        synced = (app_manager.GetWindowForId(pair.first) != nullptr);

        if (!synced)
          break;
      }

      if (synced)
        parent_->locations_changed.emit();

      return !synced;
    };

    if (app_manager_not_synced())
      idle_.reset(new glib::Idle(app_manager_not_synced));
  }

  GnomeFileManager* parent_;
  glib::DBusProxy filemanager_proxy_;
  glib::Source::UniquePtr idle_;
  std::map<Window, std::string> opened_location_for_xid_;
};


FileManager::Ptr GnomeFileManager::Get()
{
  static FileManager::Ptr instance(new GnomeFileManager());
  return instance;
}

GnomeFileManager::GnomeFileManager()
  : impl_(new Impl(this))
{}

GnomeFileManager::~GnomeFileManager()
{}

void GnomeFileManager::Open(std::string const& uri, uint64_t timestamp)
{
  if (uri.empty())
  {
    LOG_ERROR(logger) << "Impossible to open an empty location";
    return;
  }

  glib::Error error;
  GdkDisplay* display = gdk_display_get_default();
  glib::Object<GdkAppLaunchContext> context(gdk_display_get_app_launch_context(display));

  if (timestamp > 0)
    gdk_app_launch_context_set_timestamp(context, timestamp);

  auto const& gcontext = glib::object_cast<GAppLaunchContext>(context);
  g_app_info_launch_default_for_uri(uri.c_str(), gcontext, &error);

  if (error)
  {
    LOG_ERROR(logger) << "Impossible to open the location: " << error.Message();
  }
}

void GnomeFileManager::OpenTrash(uint64_t timestamp)
{
  Open(TRASH_URI, timestamp);
}

bool GnomeFileManager::TrashFile(std::string const& uri)
{
  glib::Cancellable cancellable;
  glib::Object<GFile> file(g_file_new_for_uri(uri.c_str()));
  glib::Error error;

  if (g_file_trash(file, cancellable, &error))
    return true;

  LOG_ERROR(logger) << "Impossible to trash file '" << uri << "': " << error;
  return false;
}

void GnomeFileManager::EmptyTrash(uint64_t timestamp)
{
  auto const& proxy = impl_->NautilusOperationsProxy();

  // Passing the proxy to the lambda we ensure that it will be destroyed when needed
  proxy->CallBegin("EmptyTrashWithTimestamp", g_variant_new("(u)", timestamp), [proxy] (GVariant*, glib::Error const&) {});
}

void GnomeFileManager::CopyFiles(std::set<std::string> const& uris, std::string const& dest, uint64_t timestamp)
{
  if (uris.empty() || dest.empty())
    return;

  bool found_valid = false;
  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("(assu)"));
  g_variant_builder_open(&b, G_VARIANT_TYPE("as"));

  for (auto const& uri : uris)
  {
    if (uri.find(FILE_SCHEMA) == 0)
    {
      found_valid = true;
      g_variant_builder_add(&b, "s", uri.c_str());
    }
  }

  g_variant_builder_close(&b);
  g_variant_builder_add(&b, "s", dest.c_str());
  g_variant_builder_add(&b, "u", timestamp);
  glib::Variant parameters(g_variant_builder_end(&b));

  if (found_valid)
  {
    // Passing the proxy to the lambda we ensure that it will be destroyed when needed
    auto const& proxy = impl_->NautilusOperationsProxy();
    proxy->CallBegin("CopyURIsWithTimestamp", parameters, [proxy] (GVariant*, glib::Error const&) {});
  }
}

WindowList GnomeFileManager::WindowsForLocation(std::string const& location) const
{
  std::vector<ApplicationWindowPtr> windows;
  auto& app_manager = ApplicationManager::Default();

  glib::Object<GFile> location_file(g_file_new_for_uri(location.c_str()));

  for (auto const& pair : impl_->opened_location_for_xid_)
  {
    auto const& loc = pair.second;
    bool matches = (loc == location);

    if (!matches)
    {
      glib::Object<GFile> loc_file(g_file_new_for_uri(loc.c_str()));
      glib::String relative(g_file_get_relative_path(location_file, loc_file));
      matches = static_cast<bool>(relative);
    }

    if (matches)
    {
      auto const& win = app_manager.GetWindowForId(pair.first);

      if (win && std::find(windows.rbegin(), windows.rend(), win) == windows.rend())
        windows.push_back(win);
    }
  }

  return windows;
}

std::string GnomeFileManager::LocationForWindow(ApplicationWindowPtr const& win) const
{
  if (win)
  {
    auto it = impl_->opened_location_for_xid_.find(win->window_id());

    if (it != end(impl_->opened_location_for_xid_))
      return it->second;
  }

  return std::string();
}

}  // namespace unity
