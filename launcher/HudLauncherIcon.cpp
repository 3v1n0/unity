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
#include "UnityCore/GLibWrapper.h"
#include <NuxCore/Logger.h>

#include "unity-shared/UBusMessages.h"

#include "config.h"
#include <glib/gi18n-lib.h>

namespace unity
{
namespace launcher
{
DECLARE_LOGGER(logger, "unity.launcher.icon.hud");

HudLauncherIcon::HudLauncherIcon(LauncherHideMode hide_mode)
 : SingleMonitorLauncherIcon(IconType::HUD)
 , launcher_hide_mode_(hide_mode)
 , overlay_monitor_(0)
{
  tooltip_text = _("HUD");
  tooltip_enabled = false;
  icon_name = PKGDATADIR"/launcher_bfb.png";
  position = Position::BEGIN;
  SetQuirk(Quirk::ACTIVE, true);

  background_color_ = nux::color::White;

  ubus_manager_.RegisterInterest(UBUS_HUD_ICON_CHANGED,
                                 sigc::mem_fun(this, &HudLauncherIcon::OnHudIconChanged));
  ubus_manager_.RegisterInterest(UBUS_OVERLAY_SHOWN,
                                 sigc::bind(sigc::mem_fun(this, &HudLauncherIcon::OnOverlayShown),
                                            true));
  ubus_manager_.RegisterInterest(UBUS_OVERLAY_HIDDEN,
                                 sigc::bind(sigc::mem_fun(this, &HudLauncherIcon::OnOverlayShown),
                                            false));

  mouse_enter.connect([this](int m) { ubus_manager_.SendMessage(UBUS_DASH_ABOUT_TO_SHOW); });
}

void HudLauncherIcon::OnHudIconChanged(GVariant *data)
{
  std::string hud_icon_name = glib::Variant(data).GetString();
  LOG_DEBUG(logger) << "Hud icon change: " << hud_icon_name;
  if (hud_icon_name != icon_name)
  {
    if (hud_icon_name.empty())
      icon_name = PKGDATADIR"/launcher_bfb.png";
    else
      icon_name = hud_icon_name;
  }
}

void HudLauncherIcon::SetHideMode(LauncherHideMode hide_mode)
{
  if (launcher_hide_mode_ != hide_mode)
  {
    launcher_hide_mode_ = hide_mode;

    if (launcher_hide_mode_ == LAUNCHER_HIDE_AUTOHIDE)
      SetQuirk(Quirk::VISIBLE, false);
  }
}

void HudLauncherIcon::OnOverlayShown(GVariant* data, bool visible)
{
  unity::glib::String overlay_identity;
  gboolean can_maximise = FALSE;
  int width, height;
  g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING,
                &overlay_identity, &can_maximise, &overlay_monitor_, &width, &height);

  // If the hud is open, we show the HUD button if we have a locked launcher
  if (overlay_identity.Str() == "hud" &&
      launcher_hide_mode_ == LAUNCHER_HIDE_NEVER)
  {
    SetMonitor(visible ? overlay_monitor_ : -1);
    SkipQuirkAnimation(Quirk::VISIBLE, overlay_monitor_);
  }
}

nux::Color HudLauncherIcon::BackgroundColor() const
{
  return background_color_;
}

nux::Color HudLauncherIcon::GlowColor()
{
  return background_color_;
}

void HudLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  if (IsVisibleOnMonitor(overlay_monitor_))
  {
    ubus_manager_.SendMessage(UBUS_HUD_CLOSE_REQUEST);
  }
}

std::string HudLauncherIcon::GetName() const
{
  return "HudLauncherIcon";
}

} // namespace launcher
} // namespace unity

