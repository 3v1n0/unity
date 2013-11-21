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

#ifndef _INTROSPECTABLE_WRAPPERS_H
#define _INTROSPECTABLE_WRAPPERS_H

#include <UnityCore/Result.h>
#include <Nux/Nux.h>

#include "Introspectable.h"

namespace unity
{
namespace debug
{

/** Wrap a result object from UnityCore and present it's properties
 * to the introspectable tree.
 */
class ResultWrapper: public Introspectable
{
public:
  ResultWrapper(const dash::Result& result, nux::Geometry const& geo = nux::Geometry());
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

  void UpdateGeometry(nux::Geometry const& geo);

private:
  std::string uri_;
  std::string name_;
  std::string icon_hint_;
  std::string mime_type_;
  nux::Geometry geo_;
};

}
}

#endif
