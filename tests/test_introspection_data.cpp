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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>
#include <NuxCore/Rect.h>
#include <NuxCore/Color.h>
#include <NuxCore/Math/Point3D.h>
#include "IntrospectionData.h"

namespace
{

using namespace testing;
using namespace unity::debug;

GVariant* get_variant_child(GVariant *container, std::size_t index)
{
  return g_variant_get_variant(g_variant_get_child_value(container, index));
}

TEST(TestIntrospectionData, Construction)
{
  IntrospectionData data;
}

TEST(TestIntrospectionData, Get)
{
  IntrospectionData data;
  EXPECT_TRUE(g_variant_is_of_type(data.Get(), G_VARIANT_TYPE("a{sv}")));
}

TEST(TestIntrospectionData, AddBool)
{
  IntrospectionData data;
  bool value = g_random_int();
  data.add("Bool", value);
  GVariant* variant = g_variant_lookup_value(data.Get(), "Bool", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(2, g_variant_n_children(variant));

  EXPECT_EQ(0, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(value, g_variant_get_boolean(get_variant_child(variant, 1)));
}

TEST(TestIntrospectionData, AddConstChar)
{
  IntrospectionData data;
  const char* value = "ConstCharString";
  data.add("ConstChar", value);
  auto tmp = data.Get();
  GVariant* variant = g_variant_lookup_value(tmp, "ConstChar", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(2, g_variant_n_children(variant));

  EXPECT_EQ(0, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_STREQ(value, g_variant_get_string(get_variant_child(variant, 1), nullptr));
}

TEST(TestIntrospectionData, AddString)
{
  IntrospectionData data;
  std::string const& value = "StringString";
  data.add("String", value);
  GVariant* variant = g_variant_lookup_value(data.Get(), "String", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(2, g_variant_n_children(variant));

  EXPECT_EQ(0, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(value, g_variant_get_string(get_variant_child(variant, 1), nullptr));
}

TEST(TestIntrospectionData, AddInt16)
{
  IntrospectionData data;
  int16_t value = g_random_int();
  data.add("Int16", value);
  GVariant* variant = g_variant_lookup_value(data.Get(), "Int16", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(2, g_variant_n_children(variant));

  EXPECT_EQ(0, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(value, g_variant_get_int16(get_variant_child(variant, 1)));
}

TEST(TestIntrospectionData, AddInt32)
{
  IntrospectionData data;
  int32_t value = g_random_int();
  data.add("Int32", value);
  GVariant* variant = g_variant_lookup_value(data.Get(), "Int32", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(2, g_variant_n_children(variant));

  EXPECT_EQ(0, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(value, g_variant_get_int32(get_variant_child(variant, 1)));
}

TEST(TestIntrospectionData, AddInt64)
{
  IntrospectionData data;
  int64_t value = g_random_int();
  data.add("Int64", value);
  GVariant* variant = g_variant_lookup_value(data.Get(), "Int64", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(2, g_variant_n_children(variant));

  EXPECT_EQ(0, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(value, g_variant_get_int64(get_variant_child(variant, 1)));
}

TEST(TestIntrospectionData, AddUInt16)
{
  IntrospectionData data;
  uint16_t value = g_random_int();
  data.add("Uint16", value);
  GVariant* variant = g_variant_lookup_value(data.Get(), "Uint16", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(2, g_variant_n_children(variant));

  EXPECT_EQ(0, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(value, g_variant_get_uint16(get_variant_child(variant, 1)));
}

TEST(TestIntrospectionData, AddUInt32)
{
  IntrospectionData data;
  uint32_t value = g_random_int();
  data.add("Uint32", value);
  GVariant* variant = g_variant_lookup_value(data.Get(), "Uint32", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(2, g_variant_n_children(variant));

  EXPECT_EQ(0, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(value, g_variant_get_uint32(get_variant_child(variant, 1)));
}

TEST(TestIntrospectionData, AddUInt64)
{
  IntrospectionData data;
  uint64_t value = g_random_int();
  data.add("Uint64", value);
  GVariant* variant = g_variant_lookup_value(data.Get(), "Uint64", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(2, g_variant_n_children(variant));

  EXPECT_EQ(0, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(value, g_variant_get_uint64(get_variant_child(variant, 1)));
}

TEST(TestIntrospectionData, AddFloat)
{
  IntrospectionData data;
  float value = g_random_double();
  data.add("Float", value);
  GVariant* variant = g_variant_lookup_value(data.Get(), "Float", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(2, g_variant_n_children(variant));

  EXPECT_EQ(0, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_FLOAT_EQ(value, g_variant_get_double(get_variant_child(variant, 1)));
}

TEST(TestIntrospectionData, AddDouble)
{
  IntrospectionData data;
  double value = g_random_double();
  data.add("Double", value);
  GVariant* variant = g_variant_lookup_value(data.Get(), "Double", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(2, g_variant_n_children(variant));

  EXPECT_EQ(0, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_DOUBLE_EQ(value, g_variant_get_double(get_variant_child(variant, 1)));
}

TEST(TestIntrospectionData, AddVariant)
{
  IntrospectionData data;
  GVariant* value = g_variant_new_int64(g_random_int());
  data.add("Variant", value);
  GVariant* variant = g_variant_lookup_value(data.Get(), "Variant", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(2, g_variant_n_children(variant));

  EXPECT_EQ(0, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_TRUE(g_variant_equal(value, get_variant_child(variant, 1)));
}

TEST(TestIntrospectionData, AddRect)
{
  IntrospectionData data;
  nux::Rect value(g_random_int(), g_random_int(), g_random_int(), g_random_int());
  data.add("Rect", value);
  GVariant* variant = g_variant_lookup_value(data.Get(), "Rect", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(5, g_variant_n_children(variant));

  EXPECT_EQ(1, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(value.x, g_variant_get_int32(get_variant_child(variant, 1)));
  EXPECT_EQ(value.y, g_variant_get_int32(get_variant_child(variant, 2)));
  EXPECT_EQ(value.width, g_variant_get_int32(get_variant_child(variant, 3)));
  EXPECT_EQ(value.height, g_variant_get_int32(get_variant_child(variant, 4)));
}

TEST(TestIntrospectionData, AddRectDefault)
{
  IntrospectionData data;
  nux::Rect value(g_random_int(), g_random_int(), g_random_int(), g_random_int());
  data.add(value);
  GVariant* data_variant = data.Get();
  GVariant* variant = g_variant_lookup_value(data_variant, "globalRect", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(5, g_variant_n_children(variant));

  EXPECT_EQ(1, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(value.x, g_variant_get_int32(get_variant_child(variant, 1)));
  EXPECT_EQ(value.y, g_variant_get_int32(get_variant_child(variant, 2)));
  EXPECT_EQ(value.width, g_variant_get_int32(get_variant_child(variant, 3)));
  EXPECT_EQ(value.height, g_variant_get_int32(get_variant_child(variant, 4)));

  variant = g_variant_lookup_value(data_variant, "x", nullptr);
  ASSERT_THAT(variant, NotNull());
  EXPECT_EQ(0, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(value.x, g_variant_get_int32(get_variant_child(variant, 1)));

  variant = g_variant_lookup_value(data_variant, "y", nullptr);
  ASSERT_THAT(variant, NotNull());
  EXPECT_EQ(0, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(value.y, g_variant_get_int32(get_variant_child(variant, 1)));

  variant = g_variant_lookup_value(data_variant, "width", nullptr);
  ASSERT_THAT(variant, NotNull());
  EXPECT_EQ(0, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(value.width, g_variant_get_int32(get_variant_child(variant, 1)));

  variant = g_variant_lookup_value(data_variant, "height", nullptr);
  ASSERT_THAT(variant, NotNull());
  EXPECT_EQ(0, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(value.height, g_variant_get_int32(get_variant_child(variant, 1)));
}

TEST(TestIntrospectionData, AddPoint)
{
  IntrospectionData data;
  nux::Point value(g_random_int(), g_random_int());
  data.add("Point", value);
  GVariant* variant = g_variant_lookup_value(data.Get(), "Point", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(3, g_variant_n_children(variant));

  EXPECT_EQ(2, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(value.x, g_variant_get_int32(get_variant_child(variant, 1)));
  EXPECT_EQ(value.y, g_variant_get_int32(get_variant_child(variant, 2)));
}

TEST(TestIntrospectionData, AddSize)
{
  IntrospectionData data;
  nux::Size value(g_random_int(), g_random_int());
  data.add("Size", value);
  GVariant* variant = g_variant_lookup_value(data.Get(), "Size", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(3, g_variant_n_children(variant));

  EXPECT_EQ(3, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(value.width, g_variant_get_int32(get_variant_child(variant, 1)));
  EXPECT_EQ(value.height, g_variant_get_int32(get_variant_child(variant, 2)));
}

TEST(TestIntrospectionData, AddColor)
{
  IntrospectionData data;
  nux::Color value(g_random_double(), g_random_double(), g_random_double(), g_random_double());
  data.add("Color", value);
  GVariant* variant = g_variant_lookup_value(data.Get(), "Color", nullptr);
  ASSERT_THAT(variant, NotNull());
  ASSERT_EQ(5, g_variant_n_children(variant));

  EXPECT_EQ(4, g_variant_get_uint32(get_variant_child(variant, 0)));
  EXPECT_EQ(static_cast<int32_t>(value.red * 255.), g_variant_get_int32(get_variant_child(variant, 1)));
  EXPECT_EQ(static_cast<int32_t>(value.green * 255.), g_variant_get_int32(get_variant_child(variant, 2)));
  EXPECT_EQ(static_cast<int32_t>(value.blue * 255.), g_variant_get_int32(get_variant_child(variant, 3)));
  EXPECT_EQ(static_cast<int32_t>(value.alpha * 255.), g_variant_get_int32(get_variant_child(variant, 4)));
}


} // Namespace
