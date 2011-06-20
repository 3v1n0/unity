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

#include "Nux/Nux.h"
#include "Nux/BaseWindow.h"

#include "SimpleLauncherIcon.h"
#include "Launcher.h"
#include "PluginAdapter.h"

SimpleLauncherIcon::SimpleLauncherIcon (Launcher* IconManager)
:   LauncherIcon(IconManager),
  _theme_changed_id (0)
{
  m_Icon = 0;
  m_IconName = 0;

  _on_mouse_down_connection = (sigc::connection) LauncherIcon::MouseDown.connect (sigc::mem_fun (this, &SimpleLauncherIcon::OnMouseDown));
  _on_mouse_up_connection = (sigc::connection) LauncherIcon::MouseUp.connect (sigc::mem_fun (this, &SimpleLauncherIcon::OnMouseUp));
  _on_mouse_click_connection = (sigc::connection) LauncherIcon::MouseClick.connect (sigc::mem_fun (this, &SimpleLauncherIcon::OnMouseClick));
  _on_mouse_enter_connection = (sigc::connection) LauncherIcon::MouseEnter.connect (sigc::mem_fun (this, &SimpleLauncherIcon::OnMouseEnter));
  _on_mouse_leave_connection = (sigc::connection) LauncherIcon::MouseLeave.connect (sigc::mem_fun (this, &SimpleLauncherIcon::OnMouseLeave));

  _theme_changed_id = g_signal_connect (gtk_icon_theme_get_default (), "changed",
                                        G_CALLBACK (SimpleLauncherIcon::OnIconThemeChanged), this);
}

SimpleLauncherIcon::~SimpleLauncherIcon()
{
  _on_mouse_down_connection.disconnect ();
  _on_mouse_up_connection.disconnect ();
  _on_mouse_click_connection.disconnect ();
  _on_mouse_enter_connection.disconnect ();
  _on_mouse_leave_connection.disconnect ();

  if (m_Icon)
    m_Icon->UnReference ();

  if (_theme_changed_id)
    g_signal_handler_disconnect (gtk_icon_theme_get_default (), _theme_changed_id);
}

void
SimpleLauncherIcon::OnMouseDown (int button)
{
}

void
SimpleLauncherIcon::OnMouseUp (int button)
{
}

void
SimpleLauncherIcon::OnMouseClick (int button)
{
}

void
SimpleLauncherIcon::OnMouseEnter ()
{
}

void
SimpleLauncherIcon::OnMouseLeave ()
{
}

void
SimpleLauncherIcon::ActivateLauncherIcon ()
{
  activate.emit ();
}

nux::BaseTexture *
SimpleLauncherIcon::GetTextureForSize (int size)
{
  if (m_Icon && size == m_Icon->GetHeight ())
    return m_Icon;

  if (m_Icon)
    m_Icon->UnReference ();
  m_Icon = 0;

  if (!m_IconName)
    return 0;

  if (g_str_has_prefix (m_IconName, "/"))
    m_Icon = TextureFromPath (m_IconName, size);
  else
    m_Icon = TextureFromGtkTheme (m_IconName, size);
  return m_Icon;
}

void
SimpleLauncherIcon::SetIconName (const char *name)
{
  if (m_IconName)
    g_free (m_IconName);
  m_IconName = g_strdup (name);

  if (m_Icon)
  {
    m_Icon->UnReference ();
    m_Icon = 0;
  }

  needs_redraw.emit (this);
}

void
SimpleLauncherIcon::OnIconThemeChanged (GtkIconTheme* icon_theme, gpointer data)
{
  SimpleLauncherIcon *self;

  if (!data)
    return;

  // invalidate the current cache
  _current_theme_is_mono = -1;

  self = (SimpleLauncherIcon*) data;

  /*
   * Unreference the previous icon and redraw
   * (forcing the new icon to be loaded)
   */
  if (self->m_Icon)
  {
    self->m_Icon->UnReference ();
    self->m_Icon = 0;
    self->needs_redraw.emit (self);
  }
}
