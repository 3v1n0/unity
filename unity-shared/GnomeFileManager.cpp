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

const std::string TRASH_URI = "trash:";
const std::string FILE_SCHEMA = "file://";
const std::string TRASH_PATH = FILE_SCHEMA + DesktopUtilities::GetUserDataDirectory() + "/Trash/files";
const std::string DEVICES_PREFIX = FILE_SCHEMA + "/media/" + glib::gchar_to_string(g_get_user_name());
const std::vector<std::string> EMPTY_LOCATIONS;

const std::string NAUTILUS_NAME = "org.gnome.Nautilus";
const std::string NAUTILUS_PATH = "/org/gnome/Nautilus";
}

struct GnomeFileManager::Impl
{
  Impl(GnomeFileManager* parent)
    : parent_(parent)
    , filemanager_proxy_("org.freedesktop.FileManager1", "/org/freedesktop/FileManager1", "org.freedesktop.FileManager1")
  {
    auto callback = sigc::mem_fun(this, &Impl::OnOpenLocationsXidsUpdated);
    filemanager_proxy_.GetProperty("XUbuntuOpenLocationsXids", callback);
    filemanager_proxy_.ConnectProperty("XUbuntuOpenLocationsXids", callback);
  }

  std::string GetOpenedPrefix(std::string const& uri, bool allow_equal = true)
  {
    glib::Object<GFile> uri_file(g_file_new_for_uri(uri.c_str()));

    for (auto const& loc : opened_locations_)
    {
      bool equal = false;

      glib::Object<GFile> loc_file(g_file_new_for_uri(loc.c_str()));

      if (allow_equal && g_file_equal(loc_file, uri_file))
        equal = true;

      if (equal || g_file_has_prefix(loc_file, uri_file))
        return loc;
    }

    return "";
  }

  glib::DBusProxy::Ptr NautilusOperationsProxy() const
  {
    return std::make_shared<glib::DBusProxy>(NAUTILUS_NAME, NAUTILUS_PATH,
                                             "org.gnome.Nautilus.FileOperations");
  }

  void OnOpenLocationsXidsUpdated(GVariant* value)
  {
    opened_locations_.clear();
    opened_locations_xids_.clear();

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
    const char *str;
    guint32 xid;

    g_variant_iter_init(&iter, value);

    while (g_variant_iter_loop(&iter, "{uas}", &xid, &str_iter))
    {
      while (g_variant_iter_loop(str_iter, "s", &str))
      {
        LOG_DEBUG(logger) << xid << ": Opened location " << str;
        opened_locations_xids_[xid].push_back(str);
        opened_locations_.push_back(str);
      }
    }

    // We must ensure that we emit the locations_changed signal only when all
    // the parent windows have been registered on the app-manager
    auto app_manager_not_synced = [this]
    {
      auto& app_manager = ApplicationManager::Default();
      bool synced = true;

      for (auto const& pair : opened_locations_xids_)
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
  std::vector<std::string> opened_locations_;
  std::map<Window, std::vector<std::string>> opened_locations_xids_;
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

void GnomeFileManager::OpenActiveChild(std::string const& uri, uint64_t timestamp)
{
  auto const& opened = impl_->GetOpenedPrefix(uri);

  Open(opened.empty() ? uri : opened, timestamp);
}

void GnomeFileManager::OpenTrash(uint64_t timestamp)
{
  Open(TRASH_URI, timestamp);
}

void GnomeFileManager::Activate(uint64_t timestamp)
{
  glib::Cancellable cancellable;
  glib::Object<GFile> file(g_file_new_for_uri(TRASH_URI.c_str()));
  glib::Object<GAppInfo> app_info(g_file_query_default_handler(file, cancellable, nullptr));

  if (app_info)
  {
    GdkDisplay* display = gdk_display_get_default();
    glib::Object<GdkAppLaunchContext> context(gdk_display_get_app_launch_context(display));

    if (timestamp > 0)
      gdk_app_launch_context_set_timestamp(context, timestamp);

    auto const& gcontext = glib::object_cast<GAppLaunchContext>(context);
    auto proxy = std::make_shared<glib::DBusProxy>(NAUTILUS_NAME, NAUTILUS_PATH,
                                                   "org.freedesktop.Application");

    glib::String context_string(g_app_launch_context_get_startup_notify_id(gcontext, app_info, nullptr));

    if (context_string && g_utf8_validate(context_string, -1, nullptr))
    {
      GVariantBuilder builder;
      g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
      g_variant_builder_add(&builder, "{sv}", "desktop-startup-id", g_variant_new("s", context_string.Value()));
      GVariant *param = g_variant_new("(@a{sv})", g_variant_builder_end(&builder));

      // Passing the proxy to the lambda we ensure that it will be destroyed when needed
      proxy->CallBegin("Activate", param, [proxy] (GVariant*, glib::Error const&) {});
    }
  }
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
  Activate(timestamp);
  auto const& proxy = impl_->NautilusOperationsProxy();

  // Passing the proxy to the lambda we ensure that it will be destroyed when needed
  proxy->CallBegin("EmptyTrash", nullptr, [proxy] (GVariant*, glib::Error const&) {});
}

void GnomeFileManager::CopyFiles(std::set<std::string> const& uris, std::string const& dest, uint64_t timestamp)
{
  if (uris.empty() || dest.empty())
    return;

  bool found_valid = false;
  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("(ass)"));
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
  glib::Variant parameters(g_variant_builder_end(&b));

  if (found_valid)
  {
    // Passing the proxy to the lambda we ensure that it will be destroyed when needed
    auto const& proxy = impl_->NautilusOperationsProxy();
    proxy->CallBegin("CopyURIs", parameters, [proxy] (GVariant*, glib::Error const&) {});
    Activate(timestamp);
  }
}

std::vector<std::string> const& GnomeFileManager::OpenedLocations() const
{
  return impl_->opened_locations_;
}

bool GnomeFileManager::IsPrefixOpened(std::string const& uri) const
{
  return !impl_->GetOpenedPrefix(uri).empty();
}

bool GnomeFileManager::IsTrashOpened() const
{
  return (IsPrefixOpened(TRASH_URI) || IsPrefixOpened(TRASH_PATH));
}

bool GnomeFileManager::IsDeviceOpened() const
{
  return !impl_->GetOpenedPrefix(DEVICES_PREFIX, false).empty();
}

WindowList GnomeFileManager::WindowsForLocation(std::string const& location) const
{
  std::vector<ApplicationWindowPtr> windows;
  auto& app_manager = ApplicationManager::Default();

  glib::Object<GFile> location_file(g_file_new_for_uri(location.c_str()));

  for (auto const& pair : impl_->opened_locations_xids_)
  {
    for (auto const& loc : pair.second)
    {
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

        break;
      }
    }
  }

  return windows;
}

std::vector<std::string> const& GnomeFileManager::LocationsForWindow(ApplicationWindowPtr const& win) const
{
  if (win)
  {
    auto it = impl_->opened_locations_xids_.find(win->window_id());

    if (it != end(impl_->opened_locations_xids_))
      return it->second;
  }

  return EMPTY_LOCATIONS;
}

}  // namespace unity
