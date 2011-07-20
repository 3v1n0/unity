// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#ifndef UNITY_CATEGORY_H
#define UNITY_CATEGORY_H

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <NuxCore/Property.h>
#include <sigc++/trackable.h>

namespace unity {
namespace dash {

/**
 * CategoryRenderer:
 *
 * @GRID: Standard Grid of tiles with icon above two lines of text
 * @GRID_WIDE: Tiles are wider, with text next to the icon horizontally.
 *   comment also shows
 * @LIST: List with three columns: [icon][name][comment]
 * @FLOW: Flow view with large icons and both name and comment showing
 */
enum CategoryRenderer
{
  GRID=0,
  GRID_WIDE,
  LIST,
  FLOW
}

class Category : boost::noncopyable
{
public:
  typedef boost::shared_ptr<Category> Ptr;

  Category();

  nux::Property<unsigned int> index;
  nux::Property<std::string> icon_hint;
  nux::Property<std::string> name;
  nux::Property<CategoryRenderer> renderer;
};

}
}

#endif
