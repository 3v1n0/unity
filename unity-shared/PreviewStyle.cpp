// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */
#include "PreviewStyle.h"
#include <NuxCore/Logger.h>

namespace unity
{
namespace dash
{
namespace previews
{
namespace
{
Style* style_instance = nullptr;

nux::logging::Logger logger("unity.dash.previews.style");

} // namespace

Style::Style()
{
  if (style_instance)
  {
    LOG_ERROR(logger) << "More than one previews::Style created.";
  }
  else
  {
    style_instance = this;
  }
}

Style::~Style()
{
  if (style_instance == this)
    style_instance = nullptr;
}

Style& Style::Instance()
{
  if (!style_instance)
  {
    LOG_ERROR(logger) << "No previews::Style created yet.";
  }

  return *style_instance;
}

int Style::NavigatorMinimumWidth() const
{
  return 42;
}

int Style::NavigatorMaximumWidth() const
{
  return 42;
}

std::string Style::app_name_font() const
{
  return "Ubuntu 20";
}
std::string Style::version_size_font() const
{
  return "Ubuntu 12";
}
std::string Style::app_license_font() const
{
  return "Ubuntu Light 9.5";
}
std::string Style::app_last_update_font() const
{
  return "Ubuntu Light 10";
}
std::string Style::app_copywrite_font() const
{
  return "Ubuntu Light 10";
}
std::string Style::app_description_font() const
{
  return "Ubuntu Light 10";
}

std::string Style::info_hint_font() const
{
  return "Ubuntu Light 10";
}
std::string Style::user_rating_font() const
{
  return "Ubuntu Light 10";
}
std::string Style::no_preview_image_font() const
{
  return "Ubuntu Light 16";
}

}
}
}
