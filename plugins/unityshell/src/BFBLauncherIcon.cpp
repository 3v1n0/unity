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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#include "BFBLauncherIcon.h"
#include "Launcher.h"

#include <glib/gi18n-lib.h>

#include "ubus-server.h"
#include "UBusMessages.h"

BFBLauncherIcon::BFBLauncherIcon(Launcher* IconManager)
 : SimpleLauncherIcon(IconManager)
{
  tooltip_text = _("Ubuntu Start");
  SetIconName("distributor-logo");
  SetQuirk(QUIRK_VISIBLE, true);
  SetQuirk(QUIRK_RUNNING, false);
  SetIconType(TYPE_HOME);
}

nux::Color BFBLauncherIcon::BackgroundColor()
{
  return nux::Color(0xFF333333);
}

nux::Color BFBLauncherIcon::GlowColor()
{
  return nux::Color(0xFF333333);
}

void BFBLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  if (arg.button == 1)
  {
    UBusServer* ubus = ubus_server_get_default();
    ubus_server_send_message(ubus, UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                             g_variant_new("(sus)", "home.lens", 0, ""));
  }
}
