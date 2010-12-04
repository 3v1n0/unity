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

#include "TrashLauncherIcon.h"

TrashLauncherIcon::TrashLauncherIcon (Launcher* IconManager)
:   SimpleLauncherIcon(IconManager)
{
  SetTooltipText ("Trash");
  SetIconName ("user-trash");
  SetQuirk (LAUNCHER_ICON_QUIRK_VISIBLE, true);
  SetQuirk (LAUNCHER_ICON_QUIRK_RUNNING, false);
  SetIconType (LAUNCHER_ICON_TYPE_TRASH); 
}

TrashLauncherIcon::~TrashLauncherIcon()
{
}

void
TrashLauncherIcon::OnMouseClick (int button)
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
