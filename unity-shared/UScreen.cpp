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
#include <NuxCore/NuxCore.h>
#include <NuxGraphics/GraphicsDisplay.h>

namespace unity
{
DECLARE_LOGGER(logger, "unity.screen");

UScreen* UScreen::default_screen_ = nullptr;

UScreen::UScreen()
  : primary_(0)
  , screen_(gdk_screen_get_default(), glib::AddRef())
  , proxy_("org.freedesktop.login1",
           "/org/freedesktop/login1",
           "org.freedesktop.login1.Manager",
           G_BUS_TYPE_SYSTEM)
{
  size_changed_signal_.Connect(screen_, "size-changed", sigc::mem_fun(this, &UScreen::Changed));
  monitors_changed_signal_.Connect(screen_, "monitors-changed", sigc::mem_fun(this, &UScreen::Changed));
  proxy_.Connect("PrepareForSleep", [this] (GVariant* data) {
    gboolean val;
    g_variant_get(data, "(b)", &val);
    if (!val)
      resuming.emit();
  });

  Refresh();
}

UScreen::~UScreen()
{
  if (default_screen_ == this)
    default_screen_ = nullptr;
}

UScreen* UScreen::GetDefault()
{
  if (G_UNLIKELY(!default_screen_))
    default_screen_ = new UScreen();

  return default_screen_;
}

int UScreen::GetMonitorWithMouse() const
{
  auto const& mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
  return GetMonitorAtPosition(mouse.x, mouse.y);
}

int UScreen::GetPrimaryMonitor() const
{
  return primary_;
}

int UScreen::GetMonitorAtPosition(int x, int y) const
{
  for (unsigned i = 0; i < monitors_.size(); ++i)
  {
    if (monitors_[i].IsPointInside(x, y))
      return i;
  }

  return gdk_screen_get_monitor_at_point(screen_, x, y);
}

nux::Geometry const& UScreen::GetMonitorGeometry(int monitor) const
{
  return monitors_[monitor];
}

nux::Size const& UScreen::GetMonitorPhysicalSize(int monitor) const
{
  return physical_monitors_[monitor];
}

std::vector<nux::Geometry> const& UScreen::GetMonitors() const
{
  return monitors_;
}

nux::Geometry UScreen::GetScreenGeometry() const
{
  if (monitors_.empty())
    return {};

  auto rightmost_geo = max_element(monitors_.begin(), monitors_.end(), [](nux::Geometry const& a, nux::Geometry const& b) {
    return a.x + a.width < b.x + b.width;
  });

  auto lower_geo = max_element(monitors_.begin(), monitors_.end(), [](nux::Geometry const& a, nux::Geometry const& b) {
    return a.y + a.height < b.y + b.height;
  });

  auto width = rightmost_geo->x + rightmost_geo->width;
  auto height = lower_geo->y + lower_geo->height;

  return {0, 0, width, height};
}

const std::string UScreen::GetMonitorName(int output_number = 0) const
{
  if (output_number < 0 || output_number >= gdk_screen_get_n_monitors(screen_))
  {
    LOG_WARN(logger) << "UScreen::GetMonitorName: Invalid monitor number" << output_number;
    return "";
  }

  glib::String output_name(gdk_screen_get_monitor_plug_name(screen_, output_number));
  if (!output_name)
  {
    LOG_WARN(logger) << "UScreen::GetMonitorName: Failed to get monitor name for monitor" << output_number;
    return "";
  }

  return output_name.Str();
}

int UScreen::GetPluggedMonitorsNumber() const
{
  return monitors_.size();
}

void UScreen::Changed(GdkScreen* screen)
{
  if (refresh_idle_)
    return;

  refresh_idle_.reset(new glib::Idle([this] () {
    Refresh();
    refresh_idle_.reset();

    return false;
  }));
}

void UScreen::Refresh()
{
  LOG_DEBUG(logger) << "Screen geometry changed";

  nux::Geometry last_geo;
  monitors_.clear();
  physical_monitors_.clear();
  primary_ = gdk_screen_get_primary_monitor(screen_);
  int monitors = gdk_screen_get_n_monitors(screen_);

  for (int i = 0; i < monitors; ++i)
  {
    GdkRectangle rect = { 0 };
    gdk_screen_get_monitor_geometry(screen_, i, &rect);

    float scale = gdk_screen_get_monitor_scale_factor(screen_, i);
    nux::Geometry geo(rect.x, rect.y, rect.width, rect.height);
    
    nux::Size physical_size(gdk_screen_get_monitor_width_mm(screen_, i),
      gdk_screen_get_monitor_height_mm(screen_, i));

    if (scale != 1.0)
      geo = geo * scale;

    // Check for mirrored displays
    if (geo == last_geo)
      continue;

    last_geo = geo;
    monitors_.push_back(geo);
    physical_monitors_.push_back(physical_size);

    LOG_DEBUG(logger) << "Monitor " << i << " has geometry " << geo.x << "x"
                      << geo.y << "x" << geo.width << "x" << geo.height;
  }

  changed.emit(primary_, monitors_);
}

} // Namespace
