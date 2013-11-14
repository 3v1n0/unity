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
  GVariantBuilder array;
  g_variant_builder_init(&array, G_VARIANT_TYPE("av"));
  g_variant_builder_add(&array, "v", g_variant_new_uint32(static_cast<uint32_t>(type)));

  for (auto const& value : values)
    g_variant_builder_add(&array, "v", static_cast<GVariant*>(value));

  g_variant_builder_add(builder_, "{sv}", name.c_str(), g_variant_builder_end(&array));
}

IntrospectionData& IntrospectionData::add(std::string const& name, bool value)
{
  add_(builder_, name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, char const* value)
{
  add_(builder_, name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, std::string const& value)
{
  add_(builder_, name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, int16_t value)
{
  add_(builder_, name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, int32_t value)
{
  add_(builder_, name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, int64_t value)
{
  add_(builder_, name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, uint16_t value)
{
  add_(builder_, name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, uint32_t value)
{
  add_(builder_, name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, uint64_t value)
{
  add_(builder_, name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, float value)
{
  add_(builder_, name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, double value)
{
  add_(builder_, name, ValueType::SIMPLE, {Variant(value)});
  return *this;
}

IntrospectionData& IntrospectionData::add(std::string const& name, GVariant* value)
{
  add_(builder_, name, ValueType::SIMPLE, {Variant(value)});
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

} // debug namespace
} // unity namespace
