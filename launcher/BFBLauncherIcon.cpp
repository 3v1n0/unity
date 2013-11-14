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
 *              Andrea Azzarone <azzaronea@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include "unity-shared/UBusMessages.h"

#include "BFBLauncherIcon.h"

namespace unity
{
namespace launcher
{

BFBLauncherIcon::BFBLauncherIcon(LauncherHideMode hide_mode)
 : SimpleLauncherIcon(IconType::HOME)
 , reader_(dash::GSettingsScopesReader::GetDefault())
 , launcher_hide_mode_(hide_mode)
{
  tooltip_text = _("Search your computer and online sources");
  icon_name = PKGDATADIR"/launcher_bfb.png";
  position = Position::BEGIN;
  SetQuirk(Quirk::VISIBLE, true);
  SkipQuirkAnimation(Quirk::VISIBLE);

  background_color_ = nux::color::White;

  mouse_enter.connect([&](int m) { ubus_manager_.SendMessage(UBUS_DASH_ABOUT_TO_SHOW, NULL); });
  ubus_manager_.RegisterInterest(UBUS_OVERLAY_SHOWN, sigc::bind(sigc::mem_fun(this, &BFBLauncherIcon::OnOverlayShown), true));
  ubus_manager_.RegisterInterest(UBUS_OVERLAY_HIDDEN, sigc::bind(sigc::mem_fun(this, &BFBLauncherIcon::OnOverlayShown), false));
}

void BFBLauncherIcon::SetHideMode(LauncherHideMode hide_mode)
{
  launcher_hide_mode_ = hide_mode;
}

void BFBLauncherIcon::OnOverlayShown(GVariant *data, bool visible)
{
  unity::glib::String overlay_identity;
  gboolean can_maximise = FALSE;
  gint32 overlay_monitor = 0;
  int width, height;
  g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING,
                &overlay_identity, &can_maximise, &overlay_monitor, &width, &height);

  if (overlay_identity.Str() == "dash" && IsVisibleOnMonitor(overlay_monitor))
  {
    tooltip_enabled = !visible;
    SetQuirk(Quirk::ACTIVE, visible, overlay_monitor);
  }
  // If the hud is open, we hide the BFB if we have a locked launcher
  else if (overlay_identity.Str() == "hud")
  {
    if (launcher_hide_mode_ == LAUNCHER_HIDE_NEVER)
    {
      SetVisibleOnMonitor(overlay_monitor, !visible);
      SkipQuirkAnimation(Quirk::VISIBLE, overlay_monitor);
    }
  }
}

nux::Color BFBLauncherIcon::BackgroundColor() const
{
  return background_color_;
}

nux::Color BFBLauncherIcon::GlowColor()
{
  return background_color_;
}

void BFBLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  ubus_manager_.SendMessage(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST, g_variant_new("(sus)", "home.scope", dash::NOT_HANDLED, ""));

  // dont chain down to avoid random dash close events
}

void BFBLauncherIcon::OnMenuitemActivated(DbusmenuMenuitem* item, int time, std::string const& scope_id)
{
  if (scope_id.empty())
    return;

  ubus_manager_.SendMessage(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST, g_variant_new("(sus)", scope_id.c_str(), dash::GOTO_DASH_URI, ""));
}

AbstractLauncherIcon::MenuItemsVector BFBLauncherIcon::GetMenus()
{
  MenuItemsVector result;
  glib::Object<DbusmenuMenuitem> menu_item;

  typedef glib::Signal<void, DbusmenuMenuitem*, int> ItemSignal;
  auto callback = sigc::mem_fun(this, &BFBLauncherIcon::OnMenuitemActivated);

  // Other scopes..
  for (auto scope_data : reader_->GetScopesData())
  {
    if (!scope_data->visible)
      continue;

    menu_item = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, scope_data->name().c_str());
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
    dbusmenu_menuitem_property_set_bool(menu_item, QuicklistMenuItem::OVERLAY_MENU_ITEM_PROPERTY, true);
    signals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, sigc::bind(callback, scope_data->id())));
    result.push_back(menu_item);
  }

  return result;
}

std::string BFBLauncherIcon::GetName() const
{
  return "BFBLauncherIcon";
}

} // namespace launcher
} // namespace unity

