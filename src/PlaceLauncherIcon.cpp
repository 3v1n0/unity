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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "PlaceLauncherIcon.h"

#include <gio/gio.h>

PlaceLauncherIcon::PlaceLauncherIcon (Launcher *launcher, PlaceEntry *entry)
: SimpleLauncherIcon(launcher),
  _entry (entry)
{
  SetTooltipText (entry->GetName ());
  SetIconName ("user-trash");
  SetQuirk (QUIRK_VISIBLE, true);
  SetQuirk (QUIRK_RUNNING, false);
  SetIconType (TYPE_PLACE); 
}

PlaceLauncherIcon::~PlaceLauncherIcon()
{

}

nux::Color 
PlaceLauncherIcon::BackgroundColor ()
{
  return nux::Color (0xFF333333);
}

nux::Color 
PlaceLauncherIcon::GlowColor ()
{
  return nux::Color (0xFF333333);
}

void
PlaceLauncherIcon::OnMouseClick (int button)
{
  SimpleLauncherIcon::OnMouseClick (button);

  if (button == 1)
  {
    GError *error = NULL;

    g_spawn_command_line_async ("xdg-open trash://", &error);

    if (error)
      g_error_free (error);
  }
}

void
PlaceLauncherIcon::UpdatePlaceIcon ()
{

}
