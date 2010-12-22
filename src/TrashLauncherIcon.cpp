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

#include <gio/gio.h>

TrashLauncherIcon::TrashLauncherIcon (Launcher* IconManager)
:   SimpleLauncherIcon(IconManager)
{
  SetTooltipText ("Trash");
  SetIconName ("user-trash");
  SetQuirk (QUIRK_VISIBLE, true);
  SetQuirk (QUIRK_RUNNING, false);
  SetIconType (TYPE_TRASH); 

  m_TrashMonitor = g_file_monitor_directory (g_file_new_for_uri("trash:///"),
					     G_FILE_MONITOR_NONE,
					     NULL,
					     NULL);

  g_signal_connect(m_TrashMonitor,
                   "changed",
                   G_CALLBACK (&TrashLauncherIcon::OnTrashChanged),
                   this);

  UpdateTrashIcon ();
}

TrashLauncherIcon::~TrashLauncherIcon()
{
  g_object_unref (m_TrashMonitor);
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

void
TrashLauncherIcon::UpdateTrashIcon ()
{
  GFile *location;
  location = g_file_new_for_uri ("trash:///");

  g_file_query_info_async (location,
                           G_FILE_ATTRIBUTE_STANDARD_ICON, 
                           G_FILE_QUERY_INFO_NONE, 
                           0, 
                           NULL, 
                           &TrashLauncherIcon::UpdateTrashIconCb, 
                           this);

  g_object_unref(location);
}

void
TrashLauncherIcon::UpdateTrashIconCb (GObject      *source,
                                      GAsyncResult *res,
                                      gpointer      data)
{
  TrashLauncherIcon *self = (TrashLauncherIcon*) data;
  GFileInfo *info;
  GIcon *icon;

  info = g_file_query_info_finish (G_FILE (source), res, NULL);

  if (info != NULL) {
    icon = g_file_info_get_icon (info);
    self->SetIconName (g_icon_to_string (icon));

    g_object_unref(info);
  }
}

void
TrashLauncherIcon::OnTrashChanged (GFileMonitor        *monitor,
                                   GFile               *file,
                                   GFile               *other_file,
                                   GFileMonitorEvent    event_type,
                                   gpointer             data)
{
    TrashLauncherIcon *self = (TrashLauncherIcon*) data;
    self->UpdateTrashIcon ();
}

