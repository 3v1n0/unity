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
#include <NuxCore/Logger.h>

namespace unity
{

namespace
{
static UScreen* default_screen_ = nullptr;
nux::logging::Logger logger("unity.screen");
} 

UScreen::UScreen()
  : screen_(gdk_screen_get_default(), glib::AddRef())
  , proxy_("org.freedesktop.UPower",
           "/org/freedesktop/UPower",
           "org.freedesktop.UPower",
           G_BUS_TYPE_SYSTEM)
  , refresh_id_(0)
  , primary_(0)
{
  size_changed_signal_.Connect(screen_, "size-changed", sigc::mem_fun(this, &UScreen::Changed));
  monitors_changed_signal_.Connect(screen_, "monitors-changed", sigc::mem_fun(this, &UScreen::Changed));
  proxy_.Connect("Resuming", [&] (GVariant* data) { resuming.emit(); });

  Refresh();
}

UScreen::~UScreen()
{
  if (default_screen_ == this)
    default_screen_ = nullptr;

  if (refresh_id_ != 0)
    g_source_remove(refresh_id_);
}

UScreen* UScreen::GetDefault()
{
  if (G_UNLIKELY(!default_screen_))
    default_screen_ = new UScreen();

  return default_screen_;
}

int UScreen::GetMonitorWithMouse()
{
  GdkDevice* device;
  GdkDisplay *display;
  int x;
  int y;

  display = gdk_display_get_default();
  device = gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(display));

  gdk_device_get_position(device, nullptr, &x, &y);

  return GetMonitorAtPosition(x, y);
}

int UScreen::GetPrimaryMonitor()
{
  return primary_;
}

int UScreen::GetMonitorAtPosition(int x, int y)
{
  return gdk_screen_get_monitor_at_point(screen_, x, y);
}

nux::Geometry& UScreen::GetMonitorGeometry(int monitor)
{
  return monitors_[monitor];
}

std::vector<nux::Geometry>& UScreen::GetMonitors()
{
  return monitors_;
}

void UScreen::Changed(GdkScreen* screen)
{
  if (refresh_id_)
    return;

  refresh_id_ = g_idle_add([] (gpointer data) -> gboolean {
    auto self = static_cast<UScreen*>(data);
    self->refresh_id_ = 0;
    self->Refresh();

    return FALSE;
  }, this);
}

void UScreen::Refresh()
{
  LOG_DEBUG(logger) << "Screen geometry changed";

  nux::Geometry last_geo;
  monitors_.clear();
  primary_ = gdk_screen_get_primary_monitor(screen_);

  for (int i = 0; i < gdk_screen_get_n_monitors(screen_); i++)
  {
    GdkRectangle rect = { 0 };
    gdk_screen_get_monitor_geometry(screen_, i, &rect);
    nux::Geometry geo(rect.x, rect.y, rect.width, rect.height);

    // Check for mirrored displays
    if (geo == last_geo)
      continue;

    last_geo = geo;
    monitors_.push_back(geo);

    LOG_DEBUG(logger) << "Monitor " << i << " has geometry " << geo.x << "x"
                      << geo.y << "x" << geo.width << "x" << geo.height;
  }

  changed.emit(primary_, monitors_);
}

} // Namespace
