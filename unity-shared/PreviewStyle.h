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

#ifndef PREVIEWSTYLE_H
#define PREVIEWSTYLE_H

namespace unity
{
namespace dash
{
namespace previews
{

enum class Orientation {
  UP,
  DOWN,
  LEFT,
  RIGHT
};

class Style
{
public:
  static Style& Instance();

  int NavigatorMinimumWidth() const;
  int NavigatorMaximumWidth() const;
  
protected:
  Style();
  ~Style();


};

}
}
}

#endif //PREVIEWSTYLE_H
