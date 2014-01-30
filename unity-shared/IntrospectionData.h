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

#ifndef UNITY_DEBUG_INTROSPECTION_DATA
#define UNITY_DEBUG_INTROSPECTION_DATA

#include <string>
#include <glib.h>

namespace nux
{
template <class T> class Point3D;
template <class T> class Point2D;
class Rect;
class Size;
namespace color { class Color; }

using Color = color::Color;
using Point = Point2D<int>;
using Point3 = Point3D<float>;
}

class CompRect;
class CompPoint;
class CompSize;

namespace unity
{
namespace glib
{
class Variant;
}

namespace debug
{

class IntrospectionData
{
public:
  IntrospectionData();
  ~IntrospectionData();

  IntrospectionData& add(std::string const& name, bool);
  IntrospectionData& add(std::string const& name, const char*);
  IntrospectionData& add(std::string const& name, std::string const&);
  IntrospectionData& add(std::string const& name, int16_t);
  IntrospectionData& add(std::string const& name, int32_t);
  IntrospectionData& add(std::string const& name, int64_t);
  IntrospectionData& add(std::string const& name, uint16_t);
  IntrospectionData& add(std::string const& name, uint32_t);
  IntrospectionData& add(std::string const& name, uint64_t);
#if __WORDSIZE != 64
  IntrospectionData& add(std::string const& name, long);
  IntrospectionData& add(std::string const& name, unsigned long);
#endif
  IntrospectionData& add(std::string const& name, float);
  IntrospectionData& add(std::string const& name, double);
  IntrospectionData& add(std::string const& name, GVariant*);
  IntrospectionData& add(std::string const& name, glib::Variant const&);
  IntrospectionData& add(std::string const& name, nux::Rect const&);
  IntrospectionData& add(std::string const& name, nux::Point const&);
  IntrospectionData& add(std::string const& name, nux::Point3 const&);
  IntrospectionData& add(std::string const& name, nux::Size const&);
  IntrospectionData& add(std::string const& name, nux::Color const&);
  IntrospectionData& add(std::string const& name, CompPoint const&);
  IntrospectionData& add(std::string const& name, CompRect const&);
  IntrospectionData& add(std::string const& name, CompSize const&);
  IntrospectionData& add(nux::Rect const&);
  IntrospectionData& add(CompRect const&);

  GVariant* Get() const;

private:
  GVariantBuilder* builder_;
};

} // debug namespace
} // unity namespace

#endif // UNITY_DEBUG_INTROSPECTION