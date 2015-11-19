// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <NuxCore/Logger.h>
#include <X11/cursorfont.h>
#include <sigc++/adaptors/hide.h>
#include "glow_texture.h"
#include "DecorationsDataPool.h"
#include "UnitySettings.h"
#include "UScreen.h"

namespace unity
{
namespace decoration
{
namespace
{
DECLARE_LOGGER(logger, "unity.decoration.datapool");
static DataPool::Ptr instance_;
const std::string PLUGIN_NAME = "unityshell";
const int BUTTONS_SIZE = 16;
const int BUTTONS_PADDING = 1;
const cu::SimpleTexture::Ptr EMPTY_BUTTON;

unsigned EdgeTypeToCursorShape(Edge::Type type)
{
  switch (type)
  {
    case Edge::Type::TOP:
      return XC_top_side;
    case Edge::Type::TOP_LEFT:
      return XC_top_left_corner;
    case Edge::Type::TOP_RIGHT:
      return XC_top_right_corner;
    case Edge::Type::LEFT:
      return XC_left_side;
    case Edge::Type::RIGHT:
      return XC_right_side;
    case Edge::Type::BOTTOM:
      return XC_bottom_side;
    case Edge::Type::BOTTOM_LEFT:
      return XC_bottom_left_corner;
    case Edge::Type::BOTTOM_RIGHT:
      return XC_bottom_right_corner;
    default:
      return XC_left_ptr;
  }
}

}

DataPool::DataPool()
{
  SetupTextures();

  CompSize glow_size(texture::GLOW_SIZE, texture::GLOW_SIZE);
  glow_texture_ = std::make_shared<cu::SimpleTexture>(GLTexture::imageDataToTexture(texture::GLOW, glow_size, GL_RGBA, GL_UNSIGNED_BYTE));

  auto cb = sigc::mem_fun(this, &DataPool::SetupTextures);
  Style::Get()->theme.changed.connect(sigc::hide(cb));
  unity::Settings::Instance().dpi_changed.connect(cb);
}

DataPool::Ptr const& DataPool::Get()
{
  if (instance_)
    return instance_;

  instance_.reset(new DataPool);
  return instance_;
}

void DataPool::Reset()
{
  instance_.reset();
}

Cursor DataPool::EdgeCursor(Edge::Type type) const
{
  return screen->cursorCache(EdgeTypeToCursorShape(type));
}

void DataPool::SetupTextures()
{
  auto const& style = Style::Get();
  unsigned monitors = UScreen::GetDefault()->GetPluggedMonitorsNumber();
  auto& settings = Settings::Instance();
  bool found_normal = false;
  nux::Size size;

  scaled_window_buttons_.clear();

  for (unsigned monitor = 0; monitor < monitors; ++monitor)
  {
    double scale = settings.em(monitor)->DPIScale();
    bool scaled = (scale != 1.0f);

    if (!scaled)
    {
      if (found_normal)
        continue;

      found_normal = true;
    }

    auto& destination = scaled ? scaled_window_buttons_[scale] : window_buttons_;

    for (unsigned button = 0; button < window_buttons_.size(); ++button)
    {
      for (unsigned state = 0; state < window_buttons_[button].size(); ++state)
      {
        glib::Error error;
        auto const& file = style->WindowButtonFile(WindowButtonType(button), WidgetState(state));
        gdk_pixbuf_get_file_info(file.c_str(), &size.width, &size.height);

        size.width = std::round(size.width * scale);
        size.height = std::round(size.height * scale);
        glib::Object<GdkPixbuf> pixbuf(gdk_pixbuf_new_from_file_at_size(file.c_str(), size.width, size.height, &error));

        if (pixbuf)
        {
          LOG_DEBUG(logger) << "Loading texture " << file;
          cu::CairoContext ctx(size.width, size.height);
          gdk_cairo_set_source_pixbuf(ctx, pixbuf, 0, 0);
          cairo_paint(ctx);
          destination[button][state] = ctx;
        }
        else
        {
          LOG_WARN(logger) << "Impossible to load local button texture file: "
                           << error << "; falling back to cairo generated one.";

          int button_size = std::round((BUTTONS_SIZE + BUTTONS_PADDING * 2) * scale);
          cu::CairoContext ctx(button_size, button_size, scale);
          cairo_translate(ctx, BUTTONS_PADDING, BUTTONS_PADDING);
          style->DrawWindowButton(WindowButtonType(button), WidgetState(state), ctx, BUTTONS_SIZE, BUTTONS_SIZE);
          destination[button][state] = ctx;
        }
      }
    }
  }
}

cu::SimpleTexture::Ptr const& DataPool::GlowTexture() const
{
  return glow_texture_;
}

cu::SimpleTexture::Ptr const& DataPool::ButtonTexture(WindowButtonType wbt, WidgetState ws) const
{
  if (wbt >= WindowButtonType::Size || ws >= WidgetState::Size)
  {
    LOG_ERROR(logger) << "It has been requested an invalid button texture "
                      << "WindowButtonType: " << unsigned(wbt) << ", WidgetState: "
                      << unsigned(ws);
    return EMPTY_BUTTON;
  }

  return window_buttons_[unsigned(wbt)][unsigned(ws)];
}

cu::SimpleTexture::Ptr const& DataPool::ButtonTexture(double scale, WindowButtonType wbt, WidgetState ws) const
{
  if (wbt >= WindowButtonType::Size || ws >= WidgetState::Size)
  {
    LOG_ERROR(logger) << "It has been requested an invalid button texture "
                      << "WindowButtonType: " << unsigned(wbt) << ", WidgetState: "
                      << unsigned(ws);
    return EMPTY_BUTTON;
  }

  if (scale == 1.0f)
    return window_buttons_[unsigned(wbt)][unsigned(ws)];

  auto it = scaled_window_buttons_.find(scale);

  if (it == scaled_window_buttons_.end())
    return EMPTY_BUTTON;

  return it->second[unsigned(wbt)][unsigned(ws)];
}

} // decoration namespace
} // unity namespace
