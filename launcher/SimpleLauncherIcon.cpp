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

#include <NuxCore/Logger.h>
#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <UnityCore/Variant.h>

#include "SimpleLauncherIcon.h"

#include "unity-shared/UBusWrapper.h"
#include "unity-shared/UBusMessages.h"

namespace unity
{
namespace launcher
{
DECLARE_LOGGER(logger, "unity.launcher.icon");

NUX_IMPLEMENT_OBJECT_TYPE(SimpleLauncherIcon);

SimpleLauncherIcon::SimpleLauncherIcon(IconType type)
  : LauncherIcon(type)
  , icon_name("", sigc::mem_fun(this, &SimpleLauncherIcon::SetIconName))
  , theme_changed_id_(0)
{
  LauncherIcon::mouse_down.connect(sigc::mem_fun(this, &SimpleLauncherIcon::OnMouseDown));
  LauncherIcon::mouse_up.connect(sigc::mem_fun(this, &SimpleLauncherIcon::OnMouseUp));
  LauncherIcon::mouse_click.connect(sigc::mem_fun(this, &SimpleLauncherIcon::OnMouseClick));
  LauncherIcon::mouse_enter.connect(sigc::mem_fun(this, &SimpleLauncherIcon::OnMouseEnter));
  LauncherIcon::mouse_leave.connect(sigc::mem_fun(this, &SimpleLauncherIcon::OnMouseLeave));

  theme_changed_id_ = g_signal_connect(gtk_icon_theme_get_default(), "changed",
                                       G_CALLBACK(SimpleLauncherIcon::OnIconThemeChanged), this);
}

SimpleLauncherIcon::~SimpleLauncherIcon()
{
  for (auto element : texture_map)
    if (element.second)
      element.second->UnReference();

  texture_map.clear ();

  if (theme_changed_id_)
    g_signal_handler_disconnect(gtk_icon_theme_get_default(), theme_changed_id_);
}

void SimpleLauncherIcon::OnMouseDown(int button, int monitor, unsigned long key_flags)
{
}

void SimpleLauncherIcon::OnMouseUp(int button, int monitor, unsigned long key_flags)
{
}

void SimpleLauncherIcon::OnMouseClick(int button, int monitor, unsigned long key_flags)
{
}

void SimpleLauncherIcon::OnMouseEnter(int monitor)
{
}

void SimpleLauncherIcon::OnMouseLeave(int monitor)
{
}

void SimpleLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  activate.emit();
  UBusManager::SendMessage(UBUS_OVERLAY_CLOSE_REQUEST,
                           g_variant_new_boolean(FALSE));
}

nux::BaseTexture* SimpleLauncherIcon::GetTextureForSize(int size)
{
  if (texture_map[size] != 0)
    return texture_map[size];

  std::string icon_string(icon_name());

  if (icon_string.empty())
    return 0;

  if (icon_string[0] == '/')
    texture_map[size] = TextureFromPath(icon_string, size);
  else
    texture_map[size] = TextureFromGtkTheme(icon_string, size);
  return texture_map[size];
}

bool SimpleLauncherIcon::SetIconName(std::string& target, std::string const& value)
{
  if (target == value)
    return false;

  target = value;
  ReloadIcon();

  return true;
}

void SimpleLauncherIcon::ReloadIcon()
{
  for (auto element : texture_map)
    if (element.second)
      element.second->UnReference();

  texture_map.clear ();
  EmitNeedsRedraw();
}

void SimpleLauncherIcon::OnIconThemeChanged(GtkIconTheme* icon_theme, gpointer data)
{
  SimpleLauncherIcon* self = static_cast<SimpleLauncherIcon*>(data);

  // invalidate the current cache
  self->_current_theme_is_mono = -1;
  self->ReloadIcon();
}

std::string SimpleLauncherIcon::GetName() const
{
  return "SimpleLauncherIcon";
}

void SimpleLauncherIcon::AddProperties(GVariantBuilder* builder)
{
  LauncherIcon::AddProperties(builder);
  variant::BuilderWrapper(builder).add("icon_name", icon_name);
}

} // namespace launcher
} // namespace unity

#endif
