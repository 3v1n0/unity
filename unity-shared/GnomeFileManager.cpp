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
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibDBusProxy.h>
#include <gdk/gdk.h>
#include <gio/gio.h>

namespace unity
{

namespace
{
DECLARE_LOGGER(logger, "unity.filemanager.gnome");

const std::string TRASH_URI = "trash:";
}

void GnomeFileManager::Open(std::string const& uri, unsigned long long timestamp)
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

void GnomeFileManager::Activate(unsigned long long timestamp)
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
    auto proxy = std::make_shared<glib::DBusProxy>("org.gnome.NautilusApplication",
                                                   "/org/gnome/NautilusApplication",
                                                   "org.gtk.Application");

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

void GnomeFileManager::EmptyTrash(unsigned long long timestamp)
{
  Activate(timestamp);

  auto proxy = std::make_shared<glib::DBusProxy>("org.gnome.Nautilus", "/org/gnome/Nautilus",
                                                 "org.gnome.Nautilus.FileOperations");

  // Passing the proxy to the lambda we ensure that it will be destroyed when needed
  proxy->CallBegin("EmptyTrash", nullptr, [proxy] (GVariant*, glib::Error const&) {});
}

}
