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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include <gdk/gdk.h>
#include "config.h"
#include <glib/gi18n-lib.h>
#include <libnotify/notify.h>
#include <sigc++/sigc++.h>
#include <UnityCore/GLibWrapper.h>

#include "DeviceNotificationDisplayImp.h"
#include "unity-shared/IconLoader.h"

namespace unity
{
namespace launcher
{

//
// Start private implementation
//
class DeviceNotificationDisplayImp::Impl
{
public:
  void Show(std::string const& icon_name, std::string const& volume_name)
  {
    int icon_size = 48;
    IconLoader::GetDefault().LoadFromGIconString(icon_name, -1, icon_size,
                                                 sigc::bind(sigc::mem_fun(this, &Impl::ShowNotificationWhenIconIsReady), volume_name));
  }

  void ShowNotificationWhenIconIsReady(std::string const& icon_name,
                                       int max_width,
                                       int max_height,
                                       glib::Object<GdkPixbuf> const& pixbuf,
                                       std::string const& volume_name)
  {
    glib::Object<NotifyNotification> notification(notify_notification_new(volume_name.c_str(),
                                                                         _("The drive has been successfully ejected"),
                                                                          nullptr));

    notify_notification_set_hint(notification, "x-canonical-private-synchronous", g_variant_new_boolean(TRUE));

    if (pixbuf)
      notify_notification_set_image_from_pixbuf(notification, pixbuf);

    notify_notification_show(notification, nullptr);
  }
};

//
// End private implementation
//

DeviceNotificationDisplayImp::DeviceNotificationDisplayImp()
  : pimpl(new Impl)
{}

DeviceNotificationDisplayImp::~DeviceNotificationDisplayImp()
{}

void DeviceNotificationDisplayImp::Display(std::string const& icon_name, std::string const& volume_name)
{
  pimpl->Show(icon_name, volume_name);
}

}
}
