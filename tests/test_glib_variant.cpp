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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <list>
#include <algorithm>
#include <gmock/gmock.h>
#include <UnityCore/Variant.h>

using namespace std;
using namespace testing;
using namespace unity;
using namespace unity::glib;

namespace
{

bool IsVariant(Variant const& variant)
{
  return g_variant_get_type_string (variant) != NULL;
}

bool IsFloating(Variant const& variant)
{
  return g_variant_is_floating (variant);
}

bool IsSameVariant(Variant const& v1, Variant const& v2)
{
  GVariant *gv1 = v1;
  GVariant *gv2 = v2;

  return gv1 == gv2;
}

bool ValuesEqual(Variant const& v1, Variant const& v2)
{
  return g_variant_equal ((GVariant*)v1, (GVariant*)v2);
}

TEST(TestGLibVariant, Construct)
{
  Variant v (g_variant_new_string ("foo"));

  ASSERT_TRUE(IsVariant(v));
  EXPECT_FALSE(IsFloating(v));
}

TEST(TestGLibVariant, ConstructSteal)
{
  GVariant *gv = g_variant_new_string ("qoo");
  g_variant_ref_sink (gv);
  Variant v (gv, StealRef());

  ASSERT_TRUE(IsVariant(v));
  EXPECT_FALSE(IsFloating(v));
}

TEST(TestGLibVariant, ConstructNullptr)
{
  Variant v(nullptr);
  EXPECT_FALSE(v);
}

TEST(TestGLibVariant, ConstructNull)
{
  GVariant* value = NULL;
  Variant v(value);
  EXPECT_FALSE(v);
}

TEST(TestGLibVariant, ConstructString)
{
  std::string value = "UnityVariant";
  Variant v(value);
  EXPECT_EQ(value, g_variant_get_string(v, nullptr));
}

TEST(TestGLibVariant, ConstructInt16)
{
  int16_t value = g_random_int_range(G_MININT16, G_MAXINT16);
  Variant v(value);
  EXPECT_EQ(value, g_variant_get_int16(v));
}

TEST(TestGLibVariant, ConstructUInt16)
{
  uint16_t value = g_random_int_range(0, G_MAXUINT16);
  Variant v(value);
  EXPECT_EQ(value, g_variant_get_uint16(v));
}

TEST(TestGLibVariant, ConstructInt32)
{
  int32_t value = g_random_int_range(G_MININT32, G_MAXINT32);
  Variant v(value);
  EXPECT_EQ(value, g_variant_get_int32(v));
}

TEST(TestGLibVariant, ConstructUInt32)
{
  uint32_t value = g_random_int();
  Variant v(value);
  EXPECT_EQ(value, g_variant_get_uint32(v));
}

TEST(TestGLibVariant, ConstructInt64)
{
  int64_t value = g_random_int_range(G_MININT, G_MAXINT);
  Variant v(value);
  EXPECT_EQ(value, g_variant_get_int64(v));
}

TEST(TestGLibVariant, ConstructUInt64)
{
  uint64_t value = g_random_int();
  Variant v(value);
  EXPECT_EQ(value, g_variant_get_uint64(v));
}

TEST(TestGLibVariant, ConstructBool)
{
  bool value = g_random_int();
  Variant v(value);
  EXPECT_EQ(value, g_variant_get_boolean(v));
}

TEST(TestGLibVariant, ConstructDouble)
{
  double value = g_random_int();
  Variant v(value);
  EXPECT_DOUBLE_EQ(value, g_variant_get_double(v));
}

TEST(TestGLibVariant, ConstructFloat)
{
  float value = g_random_int();
  Variant v(value);
  EXPECT_FLOAT_EQ(value, static_cast<float>(g_variant_get_double(v)));
}

TEST(TestGLibVariant, ConstructNumericInt)
{
  Variant v0(0);
  EXPECT_EQ(0, v0.GetInt32());

  Variant v1(123456789);
  EXPECT_EQ(123456789, v1.GetInt32());
}

TEST(TestGLibVariant, ConstructNumericDouble)
{
  Variant v0(0.0f);
  EXPECT_EQ(0.0f, v0.GetDouble());

  Variant v1(0.987654321);
  EXPECT_EQ(0.987654321, v1.GetDouble());
}

TEST(TestGLibVariant, Copy)
{
  Variant v1 (g_variant_new_string ("bar"));
  Variant v2 (v1);

  ASSERT_TRUE(IsVariant(v1));
  ASSERT_TRUE(IsVariant(v2));
  EXPECT_FALSE(IsFloating(v1));
  EXPECT_FALSE(IsFloating(v2));
  EXPECT_TRUE(IsSameVariant(v1, v2));
  EXPECT_TRUE(ValuesEqual(v1, v2));
}

TEST(TestGLibVariant, Assign)
{
  Variant v;
  GVariant *raw_variant = g_variant_new_string("bar");
  v = raw_variant;

  ASSERT_TRUE(IsVariant(v));
  EXPECT_FALSE(IsFloating(v));
  EXPECT_TRUE(IsSameVariant(v, raw_variant));
  EXPECT_TRUE(ValuesEqual(v, raw_variant));
}

TEST(TestGLibVariant, AssignSame)
{
  GVariant *raw_variant = g_variant_new_string("bar");
  Variant v(raw_variant);
  v = raw_variant;

  ASSERT_TRUE(IsVariant(v));
  EXPECT_FALSE(IsFloating(v));
  EXPECT_TRUE(IsSameVariant(v, raw_variant));
  EXPECT_TRUE(ValuesEqual(v, raw_variant));
}

TEST(TestGLibVariant, KeepsRef)
{
  GVariant *gv = g_variant_new_int32 (456);
  g_variant_ref_sink (gv);

  Variant v (gv);

  ASSERT_TRUE(IsVariant(v));
  EXPECT_FALSE(IsFloating(v));

  g_variant_unref (gv);

  ASSERT_TRUE(IsVariant(v));
  EXPECT_FALSE(IsFloating(v));
  EXPECT_EQ(v.GetInt32(), 456);
}

TEST(TestGLibVariant, UseGVariantMethod)
{
  Variant v (g_variant_new_int32 (123));

  ASSERT_TRUE(IsVariant(v));
  EXPECT_FALSE(IsFloating(v));
  EXPECT_EQ(v.GetInt32(), 123);

  EXPECT_TRUE(g_variant_is_of_type (v, G_VARIANT_TYPE ("i")));
}

TEST(TestGLibVariant, HintsMap)
{
  GVariantBuilder b;

  g_variant_builder_init (&b, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_add(&b, "{sv}", "charstring-key", g_variant_new_string("charstring-value"));
  g_variant_builder_add(&b, "{sv}", "string-key", g_variant_new_string(std::string("string-value").c_str()));
  g_variant_builder_add(&b, "{sv}", "gint32-key", g_variant_new_int32(-1));
  g_variant_builder_add(&b, "{sv}", "guint32-key", g_variant_new_uint32(-2));
  g_variant_builder_add(&b, "{sv}", "gint64-key", g_variant_new_int64(-3));
  g_variant_builder_add(&b, "{sv}", "guint64-key", g_variant_new_uint64(-4));
  g_variant_builder_add(&b, "{sv}", "float-key", g_variant_new_double((float)1.1));
  g_variant_builder_add(&b, "{sv}", "double-key", g_variant_new_double(2.2));
  g_variant_builder_add(&b, "{sv}", "bool-key", g_variant_new_boolean(true));
  g_variant_builder_add(&b, "{sv}", "variant-key", g_variant_new_int32(123));

  GVariant *dict_variant = g_variant_builder_end (&b);
  Variant dict (g_variant_new_tuple (&dict_variant, 1));

  ASSERT_TRUE(IsVariant(dict));
  EXPECT_FALSE(IsFloating(dict));

  HintsMap hints;
  EXPECT_TRUE(dict.ASVToHints (hints));

  EXPECT_EQ(hints["charstring-key"].GetString(), "charstring-value");
  EXPECT_EQ(hints["string-key"].GetString(), "string-value");
  EXPECT_EQ(hints["gint32-key"].GetInt32(), (gint32)-1);
  EXPECT_EQ(hints["guint32-key"].GetUInt32(), (guint32)-2);
  EXPECT_EQ(hints["gint64-key"].GetInt64(), (gint64)-3);
  EXPECT_EQ(hints["guint64-key"].GetUInt64(), (guint64)-4);
  EXPECT_FLOAT_EQ(hints["float-key"].GetFloat(), 1.1);
  EXPECT_DOUBLE_EQ(hints["double-key"].GetDouble(), 2.2);
  EXPECT_EQ(hints["bool-key"].GetBool(), true);

  // throw away all references to the original variant
  dict = g_variant_new_string ("bar");
  ASSERT_TRUE(IsVariant(dict));
  EXPECT_FALSE(IsFloating(dict));
  EXPECT_EQ(dict.GetString(), "bar");

  // this has to still work
  EXPECT_EQ(hints["charstring-key"].GetString(), "charstring-value");
  EXPECT_EQ(hints["string-key"].GetString(), "string-value");
  EXPECT_EQ(hints["gint32-key"].GetInt32(), (gint32)-1);
  EXPECT_EQ(hints["guint32-key"].GetUInt32(), (guint32)-2);
  EXPECT_EQ(hints["gint64-key"].GetInt64(), (gint64)-3);
  EXPECT_EQ(hints["guint64-key"].GetUInt64(), (guint64)-4);
  EXPECT_FLOAT_EQ(hints["float-key"].GetFloat(), 1.1);
  EXPECT_DOUBLE_EQ(hints["double-key"].GetDouble(), 2.2);
  EXPECT_EQ(hints["bool-key"].GetBool(), true);
}

TEST(TestGLibVariant, GetString)
{
  Variant v1(g_variant_new_string("Unity"));
  EXPECT_EQ(v1.GetString(), "Unity");

  Variant v2(g_variant_new("(s)", "Rocks"));
  EXPECT_EQ(v2.GetString(), "Rocks");

  Variant v3(g_variant_new("(si)", "!!!", G_MININT));
  EXPECT_EQ(v3.GetString(), "");

  Variant v4;
  EXPECT_EQ(v4.GetString(), "");

  Variant v5(g_variant_new_variant(g_variant_new_string("Yeah!!!")));
  EXPECT_EQ(v5.GetString(), "Yeah!!!");
}

TEST(TestGLibVariant, GetInt16)
{
  gint16 value = g_random_int_range(G_MININT16, G_MAXINT16);
  Variant v1(g_variant_new_int16(value));
  EXPECT_EQ(v1.GetInt16(), value);

  value = g_random_int_range(G_MININT16, G_MAXINT16);
  Variant v2(g_variant_new("(n)", value));
  EXPECT_EQ(v2.GetInt16(), value);

  Variant v3(g_variant_new("(ns)", value, "fooostring"));
  EXPECT_EQ(v3.GetInt16(), 0);

  Variant v4;
  EXPECT_EQ(v4.GetInt16(), 0);

  value = g_random_int_range(G_MININT16, G_MAXINT16);
  Variant v5(g_variant_new_variant(g_variant_new_int16(value)));
  EXPECT_EQ(v5.GetInt16(), value);
}

TEST(TestGLibVariant, GetUInt16)
{
  guint16 value = g_random_int();
  Variant v1(g_variant_new_uint16(value));
  EXPECT_EQ(v1.GetUInt16(), value);

  value = g_random_int();
  Variant v2(g_variant_new("(q)", value));
  EXPECT_EQ(v2.GetUInt16(), value);

  Variant v3(g_variant_new("(qi)", value, G_MAXINT16));
  EXPECT_EQ(v3.GetUInt16(), 0);

  Variant v4;
  EXPECT_EQ(v4.GetUInt16(), 0);

  value = g_random_int();
  Variant v5(g_variant_new_variant(g_variant_new_uint16(value)));
  EXPECT_EQ(v5.GetUInt16(), value);
}

TEST(TestGLibVariant, GetInt32)
{
  gint32 value = g_random_int_range(G_MININT32, G_MAXINT32);
  Variant v1(g_variant_new_int32(value));
  EXPECT_EQ(v1.GetInt32(), value);

  value = g_random_int_range(G_MININT32, G_MAXINT32);
  Variant v2(g_variant_new("(i)", value));
  EXPECT_EQ(v2.GetInt32(), value);

  Variant v3(g_variant_new("(is)", value, "fooostring"));
  EXPECT_EQ(v3.GetInt32(), 0);

  Variant v4;
  EXPECT_EQ(v4.GetInt32(), 0);

  value = g_random_int_range(G_MININT32, G_MAXINT32);
  Variant v5(g_variant_new_variant(g_variant_new_int32(value)));
  EXPECT_EQ(v5.GetInt32(), value);
}

TEST(TestGLibVariant, GetUInt32)
{
  guint32 value = g_random_int();
  Variant v1(g_variant_new_uint32(value));
  EXPECT_EQ(v1.GetUInt32(), value);

  value = g_random_int();
  Variant v2(g_variant_new("(u)", value));
  EXPECT_EQ(v2.GetUInt32(), value);

  Variant v3(g_variant_new("(ui)", value, G_MAXUINT32));
  EXPECT_EQ(v3.GetUInt32(), 0);

  Variant v4;
  EXPECT_EQ(v4.GetUInt32(), 0);

  value = g_random_int();
  Variant v5(g_variant_new_variant(g_variant_new_uint32(value)));
  EXPECT_EQ(v5.GetUInt32(), value);
}

TEST(TestGLibVariant, GetInt64)
{
  gint64 value = g_random_int_range(G_MININT, G_MAXINT);
  Variant v1(g_variant_new_int64(value));
  EXPECT_EQ(v1.GetInt64(), value);

  value = g_random_int_range(G_MININT, G_MAXINT);
  Variant v2(g_variant_new("(x)", value));
  EXPECT_EQ(v2.GetInt64(), value);

  Variant v3(g_variant_new("(xs)", value, "fooostring"));
  EXPECT_EQ(v3.GetInt64(), 0);

  Variant v4;
  EXPECT_EQ(v4.GetInt64(), 0);

  value = g_random_int_range(G_MININT, G_MAXINT);
  Variant v5(g_variant_new_variant(g_variant_new_int64(value)));
  EXPECT_EQ(v5.GetInt64(), value);
}

TEST(TestGLibVariant, GetUInt64)
{
  guint64 value = g_random_int();
  Variant v1(g_variant_new_uint64(value));
  EXPECT_EQ(v1.GetUInt64(), value);

  value = g_random_int();
  Variant v2(g_variant_new("(t)", value));
  EXPECT_EQ(v2.GetUInt64(), value);

  Variant v3(g_variant_new("(ti)", value, G_MAXINT64));
  EXPECT_EQ(v3.GetUInt64(), 0);

  Variant v4;
  EXPECT_EQ(v4.GetUInt64(), 0);

  value = g_random_int();
  Variant v5(g_variant_new_variant(g_variant_new_uint64(value)));
  EXPECT_EQ(v5.GetUInt64(), value);
}

TEST(TestGLibVariant, GetBool)
{
  gboolean value = (g_random_int() % 2) ? TRUE : FALSE;
  Variant v1(g_variant_new_boolean(value));
  EXPECT_EQ(v1.GetBool(), (value != FALSE));

  value = (g_random_int() % 2) ? TRUE : FALSE;
  Variant v2(g_variant_new("(b)", value));
  EXPECT_EQ(v2.GetBool(), (value != FALSE));

  Variant v3(g_variant_new("(bs)", value, "fooostring"));
  EXPECT_EQ(v3.GetBool(), false);

  Variant v4;
  EXPECT_EQ(v4.GetBool(), false);

  value = (g_random_int() % 2) ? TRUE : FALSE;
  Variant v5(g_variant_new_variant(g_variant_new_boolean(value)));
  EXPECT_EQ(v5.GetBool(), value);
}

TEST(TestGLibVariant, GetVariant)
{
  Variant value(g_variant_new_uint32(g_random_int()));
  Variant v1(g_variant_new_variant(value));
  EXPECT_TRUE(ValuesEqual(v1.GetVariant(), value));

  value = g_variant_new_boolean((g_random_int() % 2) ? TRUE : FALSE);
  Variant v2(g_variant_new("(v)", static_cast<GVariant*>(value)));
  EXPECT_TRUE(ValuesEqual(v2.GetVariant(), value));

  Variant v3(g_variant_new("(vs)", static_cast<GVariant*>(value), "fooostring"));
  EXPECT_FALSE(v3.GetVariant());

  Variant v4;
  EXPECT_FALSE(v4.GetVariant());
}



} // Namespace
