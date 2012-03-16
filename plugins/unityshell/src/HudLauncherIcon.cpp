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
 * Authored by: Gordon Allott <gord.allott@canonical.com> 
 */

#include "HudLauncherIcon.h"
#include "Launcher.h"
#include "UnityCore/GLibWrapper.h"
#include <NuxCore/Logger.h>

#include "UBusMessages.h"

#include <glib/gi18n-lib.h>

namespace unity
{
namespace launcher
{

namespace
{
nux::logging::Logger logger("unity.launcher.hudlaunchericon");
}

UBusManager HudLauncherIcon::ubus_manager_;

HudLauncherIcon::HudLauncherIcon()
 : SimpleLauncherIcon()
{
  tooltip_text = _("HUD");
  icon_name = PKGDATADIR"/launcher_bfb.png";
  SetQuirk(QUIRK_VISIBLE, false);
  SetQuirk(QUIRK_RUNNING, false);
  SetQuirk(QUIRK_ACTIVE, true);
  SetIconType(TYPE_HOME);

  background_color_ = nux::color::White;

  ubus_manager_.RegisterInterest(UBUS_HUD_ICON_CHANGED, [&](GVariant *data)
  {
    LOG_DEBUG(logger) << "Hud icon change: " << g_variant_get_string(data, NULL);
    glib::String hud_icon_name(g_strdup(g_variant_get_string(data, NULL)));
    if (!hud_icon_name.Str().empty()
        && hud_icon_name.Str() != icon_name())
    {
      icon_name = hud_icon_name.Str();
      EmitNeedsRedraw();
    }
  });

  ubus_manager_.RegisterInterest(UBUS_OVERLAY_SHOWN, sigc::bind(sigc::mem_fun(this, &HudLauncherIcon::OnOverlayShown), true));
  ubus_manager_.RegisterInterest(UBUS_OVERLAY_HIDDEN, sigc::bind(sigc::mem_fun(this, &HudLauncherIcon::OnOverlayShown), false));

  mouse_enter.connect([&](int m) { ubus_manager_.SendMessage(UBUS_DASH_ABOUT_TO_SHOW, NULL); });
}

void HudLauncherIcon::OnOverlayShown(GVariant *data, bool visible)
{
  unity::glib::String overlay_identity;
  gboolean can_maximise = FALSE;
  gint32 overlay_monitor = 0;
  g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING,
                &overlay_identity, &can_maximise, &overlay_monitor);


  if (!g_strcmp0(overlay_identity, "hud"))
  {
    SetQuirk(QUIRK_VISIBLE, visible);
    EmitNeedsRedraw();
  }
}

nux::Color HudLauncherIcon::BackgroundColor()
{
  return background_color_;
}

nux::Color HudLauncherIcon::GlowColor()
{
  return background_color_;
}

void HudLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  // wut? activate? noo we don't do that.
}

std::list<DbusmenuMenuitem*> HudLauncherIcon::GetMenus()
{
  std::list<DbusmenuMenuitem*> result;
  return result;
}

std::string HudLauncherIcon::GetName() const
{
  return "HudLauncherIcon";
}

} // namespace launcher
} // namespace unity

