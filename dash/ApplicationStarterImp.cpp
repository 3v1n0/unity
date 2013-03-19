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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include "ApplicationStarterImp.h"

#include <gdk/gdk.h>
#include <gio/gdesktopappinfo.h>

#include <NuxCore/Logger.h>
#include <UnityCore/GLibWrapper.h>

namespace unity {

DECLARE_LOGGER(logger, "unity.applicationstarterimp");

bool ApplicationStarterImp::Launch(std::string const& application_name, Time timestamp)
{
  std::string id = application_name;

  LOG_DEBUG(logger) << "Launching " << id;

  GdkDisplay* display = gdk_display_get_default();
  glib::Object<GdkAppLaunchContext> app_launch_context(gdk_display_get_app_launch_context(display));

  if (timestamp > 0)
    gdk_app_launch_context_set_timestamp(app_launch_context, timestamp);

  while (true)
  {
    glib::Object<GDesktopAppInfo> info(g_desktop_app_info_new(id.c_str()));

    if (info)
    {
      glib::Error error;
      g_app_info_launch(glib::object_cast<GAppInfo>(info), nullptr, 
                        glib::object_cast<GAppLaunchContext>(app_launch_context), &error);

      if (error)
        LOG_WARNING(logger) << "Unable to launch " << id << ":" << error;
      else
        return true;

      break;
    }

    // Try to replace the next - with a / and do the lookup again.
    auto pos = id.find_first_of('-');
    if (pos != std::string::npos)
      id.replace(pos, 1, "/");
    else
      break;
  }

  return false;
}

}
