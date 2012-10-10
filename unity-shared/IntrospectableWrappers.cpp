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
 * Authored by: Thomi Richards <thomi.richards@canonical.com>
 */

#include <UnityCore/Variant.h>

#include "IntrospectableWrappers.h"

namespace unity
{
namespace debug
{
  
ResultWrapper::ResultWrapper(dash::Result const& result, nux::Geometry const& geo)
: uri_(result.uri),
  name_(result.name),
  icon_hint_(result.icon_hint),
  mime_type_(result.mimetype),
  geo_(geo)
{
}

std::string ResultWrapper::GetName() const
{
  return "Result";
}

void ResultWrapper::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder)
    .add("uri", uri_)
    .add("name", name_)
    .add("icon_hint", icon_hint_)
    .add("mimetype", mime_type_)
    .add(geo_);
}

void ResultWrapper::UpdateGeometry(nux::Geometry const& geo)
{
  geo_ = geo;
}

}
}
