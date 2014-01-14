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
#include "DecorationsDataPool.h"

namespace unity
{
namespace decoration
{
namespace
{
DECLARE_LOGGER(logger, "unity.decoration.datapool");
const std::string PLUGIN_NAME = "unityshell";
const int BUTTONS_SIZE = 16;
const int BUTTONS_PADDING = 1;

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
  SetupCursors();
  SetupButtonsTextures();

  auto cb = sigc::hide(sigc::mem_fun(this, &DataPool::SetupButtonsTextures));
  Style::Get()->theme.changed.connect(cb);
}

DataPool::~DataPool()
{
  for (auto cursor : edge_cursors_)
    XFreeCursor(screen->dpy(), cursor);
}

DataPool::Ptr const& DataPool::Get()
{
  static DataPool::Ptr data_pool(new DataPool());
  return data_pool;
}

void DataPool::SetupCursors()
{
  for (unsigned c = 0; c < edge_cursors_.size(); ++c)
    edge_cursors_[c] = XCreateFontCursor(screen->dpy(), EdgeTypeToCursorShape(Edge::Type(c)));
}

Cursor DataPool::EdgeCursor(Edge::Type type) const
{
  return edge_cursors_[unsigned(type)];
}

void DataPool::SetupButtonsTextures()
{
  CompSize size;
  CompString plugin_name(PLUGIN_NAME);
  auto const& style = Style::Get();

  for (unsigned button = 0; button < window_buttons_.size(); ++button)
  {
    for (unsigned state = 0; state < window_buttons_[button].size(); ++state)
    {
      auto file = style->WindowButtonFile(WindowButtonType(button), WidgetState(state));
      auto const& tex_list = GLTexture::readImageToTexture(file, plugin_name, size);

      if (!tex_list.empty())
      {
        LOG_DEBUG(logger) << "Loading texture " << file;
        window_buttons_[button][state] = std::make_shared<cu::SimpleTexture>(tex_list);
      }
      else
      {
        LOG_WARN(logger) << "Impossible to load button texture " << file << "; "
                         << "falling back to cairo generated one";

        cu::CairoContext ctx(BUTTONS_SIZE + BUTTONS_PADDING*2, BUTTONS_SIZE + BUTTONS_PADDING*2);
        cairo_translate(ctx, BUTTONS_PADDING, BUTTONS_PADDING);
        style->DrawWindowButton(WindowButtonType(button), WidgetState(state), ctx, BUTTONS_SIZE, BUTTONS_SIZE);
        window_buttons_[button][state] = ctx;
      }
    }
  }
}

cu::SimpleTexture::Ptr const& DataPool::ButtonTexture(WindowButtonType wbt, WidgetState ws) const
{
  return window_buttons_[unsigned(wbt)][unsigned(ws)];
}

} // decoration namespace
} // unity namespace
