// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2016 Canonical Ltd
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
* Authored by: Marco Trevisan <marco@ubuntu.com>
*/

#ifndef UNITYSHELL_SPREAD_WIDGETS_H
#define UNITYSHELL_SPREAD_WIDGETS_H

#include "SpreadFilter.h"

namespace unity
{
namespace spread
{
class Decorations;

class Widgets : public sigc::trackable
{
public:
  typedef std::shared_ptr<Widgets> Ptr;

  Widgets();

  Filter::Ptr GetFilter() const;

private:
  Filter::Ptr filter_;
  std::vector<std::shared_ptr<Decorations>> decos_;
};

} // namespace spread
} // namespace unity

#endif
