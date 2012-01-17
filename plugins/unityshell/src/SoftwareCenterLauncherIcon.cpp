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
 * Authored by: Bilal Akhtar <bilalakhtar@ubuntu.com>
 */

#include "Nux/Nux.h"
#include "Nux/BaseWindow.h"

#include "BamfLauncherIcon.h"
#include "Launcher.h"
#include "LauncherController.h"
#include "PluginAdapter.h"
#include "FavoriteStore.h"

#include "ubus-server.h"
#include "UBusMessages.h"

#include <glib.h>
#include <glib/gvariant.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <libindicator/indicator-desktop-shortcuts.h>
#include <core/core.h>
#include <core/atoms.h>

#include "SoftwareCenterLauncherIcon.h"

namespace unity
{
namespace launcher
{

SoftwareCenterLauncherIcon::SoftwareCenterLauncherIcon(Launcher* IconManager, BamfApplication* app, char* aptdaemon_trans_id, char* icon_path)
: BamfLauncherIcon(IconManager, app)
{
    _aptdaemon_trans_id = aptdaemon_trans_id; 
    
    g_debug("Aptdaemon transaction ID: %s", _aptdaemon_trans_id);

    _aptdaemon_trans = new unity::glib::DBusProxy("org.debian.apt",
                                    _aptdaemon_trans_id,
                                    "org.debian.apt.transaction",
                                    G_BUS_TYPE_SYSTEM,
                                    G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START);

    _aptdaemon_trans->Connect("Finished", sigc::mem_fun(this, &SoftwareCenterLauncherIcon::OnFinished));
    _aptdaemon_trans->Connect("PropertyChanged", sigc::mem_fun(this, &SoftwareCenterLauncherIcon::OnPropertyChanged));

    icon_name = icon_path;
}

SoftwareCenterLauncherIcon::~SoftwareCenterLauncherIcon() {

}

void
SoftwareCenterLauncherIcon::OnFinished(GVariant* params) {

   tooltip_text = BamfName();

   SetQuirk(LauncherIcon::QUIRK_PROGRESS, FALSE); 
}

void
SoftwareCenterLauncherIcon::OnPropertyChanged(GVariant* params) {

    gint32 progress;
    gchar* property_name;
    GVariant* property_value;

    g_variant_get_child (params, 0, "s", &property_name);
    if (!g_strcmp0 (property_name, "Progress")) {
        g_variant_get_child (params,1,"v",&property_value);
        g_variant_get (property_value, "i", &progress);

        if (progress < 100) {
            SetQuirk(LauncherIcon::QUIRK_PROGRESS, TRUE);
            tooltip_text = _("Waiting to install");
        }
        SetProgress(((float)progress) / ((float)100));
    }
    g_variant_unref(property_value);
    g_free(property_name);

}

}
}
