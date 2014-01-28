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

#include "IntrospectionData.h"
#include <core/rect.h>
#include <NuxCore/Rect.h>
#include <NuxCore/Color.h>
#include <NuxCore/Math/Point3D.h>
#include <UnityCore/Variant.h>

namespace unity
{
namespace debug
{
using namespace glib;

enum class ValueType : uint32_t
{
  // This should match the Autopilot Type IDs
  SIMPLE = 0,
  RECTANGLE = 1,
  POINT = 2,
  SIZE = 3,
  COLOR = 4,
  DATE = 5,
  TIME = 6,
  POINT3D = 7,
};

IntrospectionData::IntrospectionData()
  : builder_(g_variant_builder_new(G_VARIANT_TYPE("a{sv}")))
{}

IntrospectionData::~IntrospectionData()
{
  g_clear_pointer(&builder_, g_variant_builder_unref);
}

GVariant* IntrospectionData::Get() const
{
  GVariant* data = g_variant_builder_end(builder_);
  g_clear_pointer(&builder_, g_variant_builder_unref);
  return data;
}

void add_(GVariantBuilder* builder_, std::string const& name, ValueType type, std::vector<Variant> const& values)
{
  std::vector<Variant> new_values = {g_variant_new_variant(Variant(static_cast<uint32_t>(type)))};
  new_values.reserve(new_values.size() + values.size());
  for (auto const& value : values)
    new_values.push_back(g_variant_new_variant(value));

  g_variant_builder_add(builder_, "{sv}", name.c_str(), static_cast<GVariant*>(Variant::FromVector(new_values)));
}

template <typename TYPE>
void add_simple_value_(GVariantBuilder* builder_, std::string const& name, TYPE value)
{
  add_(builder_, name, ValueType::SIMPLE, {Variant(value)});
}

IntrospectionData& IntrospectionData::add(std::string const& name, bool value)
{
  add_simple_value_(builder_, name, value);
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, char const* value)
{
  add_simple_value_(builder_, name, value);
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, std::string const& value)
{
  add_simple_value_(builder_, name, value);
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, int16_t value)
{
  add_simple_value_(builder_, name, value);
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, int32_t value)
{
  add_simple_value_(builder_, name, value);
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, int64_t value)
{
  add_simple_value_(builder_, name, value);
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, uint16_t value)
{
  add_simple_value_(builder_, name, value);
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, uint32_t value)
{
  add_simple_value_(builder_, name, value);
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, uint64_t value)
{
  add_simple_value_(builder_, name, value);
  return *this;
}

#if __WORDSIZE != 64
IntrospectionData& IntrospectionData::add(std::string const& name, long value)
{
  add_simple_value_(builder_, name, value);
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, unsigned long value)
{
  add_simple_value_(builder_, name, value);
  return *this;
}
#endif

IntrospectionData& IntrospectionData::add(std::string const& name, float value)
{
  add_simple_value_(builder_, name, value);
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, double value)
{
  add_simple_value_(builder_, name, value);
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, Variant const& value)
{
  add_simple_value_(builder_, name, value);
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, GVariant* value)
{
  add_simple_value_(builder_, name, value);
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, nux::Rect const& r)
{
  add_(builder_, name, ValueType::RECTANGLE, {Variant(r.x), Variant(r.y), Variant(r.width), Variant(r.height)});
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, nux::Point const& p)
{
  add_(builder_, name, ValueType::POINT, {Variant(p.x), Variant(p.y)});
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, nux::Point3 const& p)
{
  add_(builder_, name, ValueType::POINT3D, {Variant(p.x), Variant(p.y), Variant(p.z)});
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, nux::Size const& s)
{
  add_(builder_, name, ValueType::SIZE, {Variant(s.width), Variant(s.height)});
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, nux::Color const& c)
{
  int32_t r = c.red * 255.0f, g = c.green * 255.0f, b = c.blue * 255.0f, a = c.alpha * 255.0f;
  add_(builder_, name, ValueType::COLOR, {Variant(r), Variant(g), Variant(b), Variant(a)});
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, CompPoint const& p)
{
  return add(name, nux::Point(p.x(), p.y()));
}

IntrospectionData& IntrospectionData::add(std::string const& name, CompRect const& r)
{
  return add(name, nux::Rect(r.x(), r.y(), r.width(), r.height()));
}

IntrospectionData& IntrospectionData::add(std::string const& name, CompSize const& s)
{
  return add(name, nux::Size(s.width(), s.height()));
}

IntrospectionData& IntrospectionData::add(nux::Rect const& value)
{
  add("globalRect", value);
  // Legacy support
  add("x", value.x);
  add("y", value.y);
  add("width", value.width);
  add("height", value.height);
  return *this;
}

IntrospectionData& IntrospectionData::add(CompRect const& r)
{
  return add(nux::Rect(r.x(), r.y(), r.width(), r.height()));
}

} // debug namespace
} // unity namespace
