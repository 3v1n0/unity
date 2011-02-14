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

#include "PanelTray.h"

PanelTray::PanelTray ()
{
  _window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_type_hint (GTK_WINDOW (_window), GDK_WINDOW_TYPE_HINT_DOCK);
  gtk_window_set_keep_above (GTK_WINDOW (_window), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (_window), TRUE);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (_window), TRUE);
  gtk_window_resize (GTK_WINDOW (_window), 24, 24);
  gtk_window_move (GTK_WINDOW (_window), 200, 12);
  gtk_widget_set_name (_window, "UnityPanelApplet");
  gtk_widget_set_colormap (_window, gdk_screen_get_rgba_colormap (gdk_screen_get_default ())); 
  gtk_widget_realize (_window);
  gdk_window_set_back_pixmap (_window->window, NULL, FALSE);
  gtk_widget_set_app_paintable (_window, TRUE);

  _tray = na_tray_new_for_screen (gdk_screen_get_default (),
                                  GTK_ORIENTATION_HORIZONTAL,
                                  (NaTrayFilterCallback)FilterTrayCallback,
                                  this);

  gtk_container_add (GTK_CONTAINER (_window), GTK_WIDGET (_tray));
  gtk_widget_show (GTK_WIDGET (_tray));

  gtk_widget_show_all (_window);
}

PanelTray::~PanelTray ()
{

}

gboolean
PanelTray::FilterTrayCallback (NaTray *tray, NaTrayChild *child, PanelTray *self)
{
  return TRUE;
}

Window
PanelTray::GetTrayWindow ()
{
  return GDK_WINDOW_XWINDOW (_window->window);
}

//
// We don't use these
//
void
PanelTray::OnEntryAdded (IndicatorObjectEntryProxy *proxy)
{

}

void
PanelTray::OnEntryMoved (IndicatorObjectEntryProxy *proxy)
{

}

void
PanelTray::OnEntryRemoved (IndicatorObjectEntryProxy *proxy)
{

}

const gchar *
PanelTray::GetName ()
{
  return "PanelTray";
}

const gchar *
PanelTray::GetChildsName ()
{
  return "";
}

void
PanelTray::AddProperties (GVariantBuilder *builder)
{

}

