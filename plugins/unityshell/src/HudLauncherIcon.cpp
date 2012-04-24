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

HudLauncherIcon::HudLauncherIcon(LauncherHideMode hide_mode)
 : SingleMonitorLauncherIcon(0)
 , launcher_hide_mode_(hide_mode)
{
  tooltip_text = _("HUD");
  icon_name = PKGDATADIR"/launcher_bfb.png";
  SetQuirk(QUIRK_VISIBLE, false);
  SetQuirk(QUIRK_RUNNING, false);
  SetQuirk(QUIRK_ACTIVE, true);
  SetIconType(TYPE_HUD);

  background_color_ = nux::color::White;

  ubus_manager_.RegisterInterest(UBUS_HUD_ICON_CHANGED, [&](GVariant *data)
  {
    std::string hud_icon_name;
    const gchar* data_string = g_variant_get_string(data, NULL);
    if (data_string)
      hud_icon_name = data_string;
    LOG_DEBUG(logger) << "Hud icon change: " << hud_icon_name;
    if (hud_icon_name != icon_name)
    {
      if (hud_icon_name.empty())
        icon_name = PKGDATADIR"/launcher_bfb.png";
      else
        icon_name = hud_icon_name;

      EmitNeedsRedraw();
    }
  });

  ubus_manager_.RegisterInterest(UBUS_OVERLAY_SHOWN, sigc::bind(sigc::mem_fun(this, &HudLauncherIcon::OnOverlayShown), true));
  ubus_manager_.RegisterInterest(UBUS_OVERLAY_HIDDEN, sigc::bind(sigc::mem_fun(this, &HudLauncherIcon::OnOverlayShown), false));

  mouse_enter.connect([&](int m) { ubus_manager_.SendMessage(UBUS_DASH_ABOUT_TO_SHOW); });
}

void HudLauncherIcon::SetHideMode(LauncherHideMode hide_mode)
{
  if (launcher_hide_mode_ != hide_mode)
  {
    launcher_hide_mode_ = hide_mode;

    if (launcher_hide_mode_ == LAUNCHER_HIDE_AUTOHIDE)
      SetQuirk(QUIRK_VISIBLE, false);
  }
}

void HudLauncherIcon::OnOverlayShown(GVariant* data, bool visible)
{
  unity::glib::String overlay_identity;
  gboolean can_maximise = FALSE;
  gint32 overlay_monitor = 0;
  g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING,
                &overlay_identity, &can_maximise, &overlay_monitor);

  // If the hud is open, we show the HUD button if we have a locked launcher
  if (overlay_identity.Str() == "hud" &&
      launcher_hide_mode_ == LAUNCHER_HIDE_NEVER)
  {
    SetMonitor(overlay_monitor);
    SetQuirk(QUIRK_VISIBLE, visible);
    SetQuirk(QUIRK_ACTIVE, visible);
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
  if (GetQuirk(QUIRK_VISIBLE))
  {
    ubus_manager_.SendMessage(UBUS_HUD_CLOSE_REQUEST);
  }
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

