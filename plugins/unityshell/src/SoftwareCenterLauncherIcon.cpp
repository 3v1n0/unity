// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
#include "PluginAdapter.h"
#include "FavoriteStore.h"

#include "ubus-server.h"
#include "UBusMessages.h"

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <libindicator/indicator-desktop-shortcuts.h>
#include <core/core.h>
#include <core/atoms.h>

#include "SoftwareCenterLauncherIcon.h"

SoftwareCenterLauncherIcon::SoftwareCenterLauncherIcon(Launcher* IconManager, BamfApplication* app, CompScreen* screen, char* aptdaemon_trans_id)
: BamfLauncherIcon(IconManager, app, screen)
{
    char* object_path;
    GVariant* finished_or_not = NULL;
    GError* error = NULL;
    GError* errorr = NULL;

    _aptdaemon_trans_id = aptdaemon_trans_id; 
    g_strdup_printf(object_path, "/org/debian/apt/transaction/%s", _aptdaemon_trans_id);
    
    g_debug("Aptdaemon transaction ID: %s", _aptdaemon_trans_id);

    _aptdaemon_trans = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                    G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                                    NULL,
                                                    "org.debian.apt",
                                                    object_path,
                                                    "org.freedesktop.DBus.Properties",
                                                    NULL,
                                                    &error);

    if (error != NULL) {
        g_debug("Error: %s", error->message);
        g_error_free(error);
    }

    finished_or_not = g_dbus_proxy_call_sync (_aptdaemon_trans,
                                            "Get",
                                            g_variant_new("(ss)",
                                                        "",
                                                        "Progress"),
                                            G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                            2000,
                                            NULL,
                                            &errorr);

    if (errorr != NULL) {
        g_debug("Errorr: %s", errorr->message);
        g_error_free(errorr);
    }

    if (finished_or_not != NULL)
        g_debug("DBus get call succeeded");
    else
        g_debug("DBus get call failed");

}

SoftwareCenterLauncherIcon::~SoftwareCenterLauncherIcon() {

}
