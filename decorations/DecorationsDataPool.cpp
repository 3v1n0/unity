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
#include "DecorationsDataPool.h"

namespace unity
{
namespace decoration
{
namespace
{
const std::string PLUGIN_NAME = "unityshell";
DECLARE_LOGGER(logger, "unity.decoration.datapool");
}

DataPool::DataPool()
{
  SetupButtonsTextures();
}

DataPool::Ptr const& DataPool::Get()
{
  static DataPool::Ptr data_pool(new DataPool());
  return data_pool;
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
        LOG_WARN(logger) << "Impossible to load button texture " << file;
        // Generate cairo texture!
      }
    }
  }
}

cu::SimpleTexture::Ptr const& DataPool::GetButtonTexture(WindowButtonType wbt, WidgetState ws) const
{
  return window_buttons_[unsigned(wbt)][unsigned(ws)];
}

} // decoration namespace
} // unity namespace
