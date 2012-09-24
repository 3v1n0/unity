// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#ifndef SCREEN_INTROSPECTION_H
#define SCREEN_INTROSPECTION_H

#include "Introspectable.h"
#include <core/core.h>

namespace unity
{
namespace debug
{

class ScreenIntrospection : public Introspectable
{
public:
  ScreenIntrospection(CompScreen* screen);

protected:
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);
  IntrospectableList GetIntrospectableChildren();

private:
  CompScreen* screen_;
};

}
}

#endif
