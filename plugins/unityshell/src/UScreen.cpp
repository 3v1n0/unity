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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "UScreen.h"

static UScreen* _default_screen = NULL;

UScreen::UScreen()
  : _refresh_id(0)
{
  GdkScreen* screen;

  screen = gdk_screen_get_default();
  g_signal_connect(screen, "size-changed",
                   (GCallback)UScreen::Changed, this);
  g_signal_connect(screen, "monitors-changed",
                   (GCallback)UScreen::Changed, this);

  Refresh();
}

UScreen::~UScreen()
{
  if (_default_screen == this)
    _default_screen = NULL;

  g_signal_handlers_disconnect_by_func((gpointer)gdk_screen_get_default(),
                                       (gpointer)UScreen::Changed,
                                       (gpointer)this);
}

UScreen*
UScreen::GetDefault()
{
  if (G_UNLIKELY(!_default_screen))
    _default_screen = new UScreen();

  return _default_screen;
}

int
UScreen::GetMonitorWithMouse()
{
  GdkScreen* screen;
  GdkDevice* device;
  GdkDisplay *display;
  int x;
  int y;

  screen = gdk_screen_get_default();
  display = gdk_display_get_default();
  device = gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(display));

  gdk_device_get_position(device, NULL, &x, &y);

  return gdk_screen_get_monitor_at_point(screen, x, y);
}

int
UScreen::GetPrimaryMonitor()
{
  return primary_;
}

nux::Geometry&
UScreen::GetMonitorGeometry(int monitor)
{
  return _monitors[monitor];
}

std::vector<nux::Geometry>&
UScreen::GetMonitors()
{
  return _monitors;
}

void
UScreen::Changed(GdkScreen* screen, UScreen* self)
{
  if (self->_refresh_id)
    return;

  self->_refresh_id = g_idle_add((GSourceFunc)UScreen::OnIdleChanged, self);
}

gboolean
UScreen::OnIdleChanged(UScreen* self)
{
  self->_refresh_id = 0;
  self->Refresh();

  return FALSE;
}

void
UScreen::Refresh()
{
  GdkScreen*    screen;
  nux::Geometry last_geo(0, 0, 1, 1);

  screen = gdk_screen_get_default();

  _monitors.erase(_monitors.begin(), _monitors.end());

  g_print("\nScreen geometry changed:\n");

  int lowest_x = std::numeric_limits<int>::max();
  int highest_y = std::numeric_limits<int>::min();
  for (int i = 0; i < gdk_screen_get_n_monitors(screen); i++)
  {
    GdkRectangle rect = { 0 };

    gdk_screen_get_monitor_geometry(screen, i, &rect);

    nux::Geometry geo(rect.x, rect.y, rect.width, rect.height);

    // Check for mirrored displays
    if (geo == last_geo)
      continue;
    last_geo = geo;

    _monitors.push_back(geo);

    if (geo.x < lowest_x || (geo.x == lowest_x && geo.y > highest_y))
    {
      lowest_x = geo.x;
      highest_y = geo.y;
      primary_ = i;
    }

    g_print("   %dx%dx%dx%d\n", geo.x, geo.y, geo.width, geo.height);
  }

  g_print("\n");

  changed.emit(primary_, _monitors);
}
