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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#ifndef UNITY_SIMPLELAUNCHERICON_H
#define UNITY_SIMPLELAUNCHERICON_H

#include "SimpleLauncherIcon.h"

#include "unity-shared/ThemeSettings.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/UBusMessages.h"

namespace unity
{
namespace launcher
{
NUX_IMPLEMENT_OBJECT_TYPE(SimpleLauncherIcon);

SimpleLauncherIcon::SimpleLauncherIcon(IconType type)
  : LauncherIcon(type)
  , icon_name("", sigc::mem_fun(this, &SimpleLauncherIcon::SetIconName))
  , icon_pixbuf(glib::Object<GdkPixbuf>(), sigc::mem_fun(this, &SimpleLauncherIcon::SetIconPixbuf))
{
  theme::Settings::Get()->icons_changed.connect(sigc::mem_fun(this, &SimpleLauncherIcon::ReloadIcon));
}

void SimpleLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  UBusManager::SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);
}

nux::BaseTexture* SimpleLauncherIcon::GetTextureForSize(int size)
{
  auto it = texture_map_.find(size);
  if (it != texture_map_.end())
    return it->second.GetPointer();

  BaseTexturePtr texture;

  if (icon_pixbuf())
  {
    texture = TextureFromPixbuf(icon_pixbuf(), size);
  }
  else
  {
    std::string const& icon_string = icon_name();

    if (icon_string.empty())
      return nullptr;

    if (icon_string[0] == '/')
      texture = TextureFromPath(icon_string, size);
    else
      texture = TextureFromGtkTheme(icon_string, size);
  }

  if (!texture)
    return nullptr;

  texture_map_.insert({size, texture});
  return texture.GetPointer();
}

bool SimpleLauncherIcon::SetIconName(std::string& target, std::string const& value)
{
  if (target == value)
    return false;

  target = value;
  ReloadIcon();

  return true;
}

bool SimpleLauncherIcon::SetIconPixbuf(glib::Object<GdkPixbuf>& target, glib::Object<GdkPixbuf> const& value)
{
  if (target == value)
    return false;

  target = value;
  ReloadIcon();

  return true;
}

void SimpleLauncherIcon::ReloadIcon()
{
  texture_map_.clear();
  EmitNeedsRedraw();
}

std::string SimpleLauncherIcon::GetName() const
{
  return "SimpleLauncherIcon";
}

void SimpleLauncherIcon::AddProperties(debug::IntrospectionData& introspection)
{
  LauncherIcon::AddProperties(introspection);
  introspection.add("icon_name", icon_name);
}

} // namespace launcher
} // namespace unity

#endif
