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
#include <unordered_map>
#include <gmock/gmock.h>
#include <UnityCore/GLibWrapper.h>

#include "test_glib_object_utils.h"

using namespace std;
using namespace testing;
using namespace unity;
using namespace unity::glib;

namespace
{
typedef glib::Object<TestGObject> TestObjectWrapper;

bool IsGOBject(TestGObject* t_obj)
{
  return G_IS_OBJECT(t_obj);
}

bool IsGOBject(TestObjectWrapper const& g_obj)
{
  return IsGOBject(g_obj.RawPtr());
}

bool IsNotGOBject(TestGObject* t_obj)
{
  return !IsGOBject(t_obj);
}

bool IsNotGOBject(TestObjectWrapper const& g_obj)
{
  return !IsGOBject(g_obj);
}

unsigned int RefCount(TestGObject* t_obj)
{
  return G_OBJECT(t_obj)->ref_count;
}

unsigned int RefCount(TestObjectWrapper const& g_obj)
{
  return RefCount(g_obj.RawPtr());
}

bool RefCountIs(TestObjectWrapper const& g_obj, unsigned int expected_ref)
{
  return (RefCount(g_obj) == expected_ref);
}

bool RefCountIs(TestGObject* t_obj, unsigned int expected_ref)
{
  return (RefCount(t_obj) == expected_ref);
}


std::unordered_map<void*, bool> weak_destroyed;

void ResetWeakObjectDestruction() { weak_destroyed.clear(); }

void AddWeakObjectDestruction(GObject* obj)
{
  g_object_weak_ref(obj, [] (gpointer data, GObject*) { weak_destroyed[data] = true; }, obj);
  weak_destroyed[static_cast<void*>(obj)] = false;
}

bool IsObjectDestroyed(void* obj)
{
  auto iter = weak_destroyed.find(obj);
  if (iter != weak_destroyed.end())
    return iter->second;
  return false;
}

TEST(TestGLibObject, ConstructEmpty)
{
  TestObjectWrapper empty_obj;
  ASSERT_THAT(empty_obj.RawPtr(), IsNull());
  EXPECT_TRUE(IsNotGOBject(empty_obj));
}

TEST(TestGLibObject, ConstructValidObject)
{
  TestGObject *t_obj = test_gobject_new();
  TestObjectWrapper g_obj(t_obj);
  ASSERT_THAT(t_obj, NotNull());

  EXPECT_EQ(g_obj.RawPtr(), t_obj);
}

TEST(TestGLibObject, ConstructDoubleRef)
{
  TestGObject *t_obj = test_gobject_new();
  {
    TestObjectWrapper g_obj_double_ref(t_obj, AddRef());
    EXPECT_EQ(g_obj_double_ref.RawPtr(), t_obj);
    EXPECT_TRUE(RefCountIs(g_obj_double_ref, 2));
  }
  EXPECT_TRUE(RefCountIs(t_obj, 1));
  g_object_unref(G_OBJECT(t_obj));
}

TEST(TestGLibObject, ConstructInitialize)
{
  TestGObject *t_obj = test_gobject_new();
  ASSERT_THAT(t_obj, NotNull());

  TestObjectWrapper g_obj1(t_obj);
  EXPECT_EQ(g_obj1.RawPtr(), t_obj);
}

TEST(TestGLibObject, ConstructCopy)
{
  TestGObject *t_obj = test_gobject_new();
  TestObjectWrapper g_obj1(t_obj);
  EXPECT_TRUE(RefCountIs(t_obj, 1));

  TestObjectWrapper g_obj2(g_obj1);
  EXPECT_TRUE(RefCountIs(t_obj, 2));

  EXPECT_EQ(g_obj1.RawPtr(), g_obj2.RawPtr());
}

TEST(TestGLibObject, AssignmentOperators)
{
  TestGObject *t_obj = test_gobject_new();
  AddWeakObjectDestruction(G_OBJECT(t_obj));

  {
    TestObjectWrapper g_obj1;
    TestObjectWrapper g_obj2;

    g_obj1 = t_obj;
    EXPECT_EQ(g_obj1.RawPtr(), t_obj);
    EXPECT_TRUE(RefCountIs(t_obj, 1));

    g_obj1 = g_obj1;
    EXPECT_TRUE(RefCountIs(t_obj, 1));

    g_obj2 = g_obj1;
    EXPECT_EQ(g_obj1.RawPtr(), g_obj2.RawPtr());
    EXPECT_TRUE(RefCountIs(t_obj, 2));
  }

  EXPECT_TRUE(IsObjectDestroyed(t_obj));

  ResetWeakObjectDestruction();
}

TEST(TestGLibObject, AssignmentOperatorOnEqualObject)
{
  TestGObject *t_obj = test_gobject_new();
  TestObjectWrapper g_obj1(t_obj);
  TestObjectWrapper g_obj2(t_obj, AddRef());
  EXPECT_EQ(g_obj1.RawPtr(), g_obj2.RawPtr());

  g_obj1 = g_obj2;
  EXPECT_EQ(g_obj1.RawPtr(), g_obj2.RawPtr());
  EXPECT_TRUE(RefCountIs(g_obj1, 2));
}

TEST(TestGLibObject, EqualityOperators)
{
  TestGObject *t_obj = test_gobject_new();
  TestObjectWrapper g_obj1;
  TestObjectWrapper g_obj2;

  // self equality checks
  EXPECT_TRUE(g_obj1 == g_obj1);
  EXPECT_FALSE(g_obj1 != g_obj1);
  EXPECT_TRUE(g_obj1 == g_obj2);
  EXPECT_FALSE(g_obj1 != g_obj2);

  g_obj1 = t_obj;
  EXPECT_TRUE(RefCountIs(g_obj1, 1));

  // Ref is needed here, since the t_obj reference is already owned by g_obj1
  g_obj2 = TestObjectWrapper(t_obj, AddRef());

  // other object equality checks
  EXPECT_TRUE(g_obj1 == t_obj);
  EXPECT_TRUE(t_obj == g_obj1);
  EXPECT_TRUE(g_obj1 == g_obj1);
  EXPECT_TRUE(g_obj1 == g_obj2);
  EXPECT_FALSE(g_obj1 != t_obj);
  EXPECT_FALSE(t_obj != g_obj1);
  EXPECT_FALSE(g_obj1 != g_obj1);
  EXPECT_FALSE(g_obj1 != g_obj2);

  g_obj2 = test_gobject_new();
  EXPECT_FALSE(g_obj1 == g_obj2);
  EXPECT_TRUE(g_obj1 != g_obj2);
}

TEST(TestGLibObject, CastOperator)
{
  TestGObject *t_obj = test_gobject_new();
  EXPECT_TRUE(t_obj == (TestGObject*) TestObjectWrapper(t_obj));
}

TEST(TestGLibObject, CastObject)
{
  TestObjectWrapper gt_obj(test_gobject_new());

  TestGObject* cast_copy = glib::object_cast<TestGObject>(gt_obj);
  EXPECT_EQ(cast_copy, gt_obj.RawPtr());

  Object<GObject> g_obj = glib::object_cast<GObject>(gt_obj);
  EXPECT_EQ(g_obj->ref_count, 2);

  g_object_set_data(g_obj, "TestData", GINT_TO_POINTER(55));
  EXPECT_EQ(GPOINTER_TO_INT(g_object_get_data(g_obj, "TestData")), 55);
}

TEST(TestGLibObject, TypeCheck)
{
  TestObjectWrapper g_obj;

  EXPECT_FALSE(g_obj.IsType(TEST_TYPE_GOBJECT));
  EXPECT_FALSE(g_obj.IsType(G_TYPE_OBJECT));
  EXPECT_FALSE(g_obj.IsType(G_TYPE_INITIALLY_UNOWNED));

  g_obj = test_gobject_new();

  EXPECT_TRUE(g_obj.IsType(TEST_TYPE_GOBJECT));
  EXPECT_TRUE(g_obj.IsType(G_TYPE_OBJECT));
  EXPECT_FALSE(g_obj.IsType(G_TYPE_INITIALLY_UNOWNED));
}

TEST(TestGLibObject, BoolOperator)
{
  TestObjectWrapper g_obj;
  EXPECT_FALSE(g_obj);

  g_obj = test_gobject_new();
  EXPECT_TRUE(g_obj);

  g_obj = nullptr;
  EXPECT_FALSE(g_obj);
}

TEST(TestGLibObject, AccessToMembers)
{
  TestObjectWrapper g_obj(test_gobject_new());
  g_obj->public_value = 1234567890;
  EXPECT_EQ(g_obj->public_value, 1234567890);
  EXPECT_EQ(test_gobject_get_public_value(g_obj), 1234567890);

  test_gobject_set_public_value(g_obj, 987654321);
  EXPECT_EQ(test_gobject_get_public_value(g_obj), 987654321);
}

TEST(TestGLibObject, UseFunction)
{
  TestObjectWrapper g_obj(test_gobject_new());

  EXPECT_NE(test_gobject_get_private_value(g_obj), 0);

  test_gobject_set_private_value(g_obj, 1234567890);
  EXPECT_EQ(test_gobject_get_private_value(g_obj), 1234567890);
}

TEST(TestGLibObject, ReleaseObject)
{
  TestGObject *t_obj = test_gobject_new();
  TestObjectWrapper g_obj(t_obj);
  ASSERT_THAT(t_obj, NotNull());

  // Release() doesn't unref the object.
  g_obj.Release();
  EXPECT_EQ(g_obj, 0);
  EXPECT_EQ(RefCount(t_obj), 1);

  g_object_unref(t_obj);
}

TEST(TestGLibObject, SwapObjects)
{
  TestGObject *t_obj1 = test_gobject_new();
  AddWeakObjectDestruction(G_OBJECT(t_obj1));
  
  TestGObject *t_obj2 = test_gobject_new();
  AddWeakObjectDestruction(G_OBJECT(t_obj2));

  {
    TestObjectWrapper g_obj1(t_obj1);
    EXPECT_EQ(g_obj1, t_obj1);

    TestObjectWrapper g_obj2(t_obj2);
    EXPECT_EQ(g_obj2, t_obj2);

    std::swap(g_obj1, g_obj2);

    EXPECT_EQ(g_obj1, t_obj2);
    EXPECT_EQ(g_obj2, t_obj1);

    g_obj1.swap(g_obj2);

    EXPECT_EQ(g_obj1, t_obj1);
    EXPECT_EQ(g_obj2, t_obj2);

    EXPECT_EQ(RefCount(g_obj1), 1);
    EXPECT_EQ(RefCount(g_obj2), 1);
  }

  EXPECT_TRUE(IsObjectDestroyed(t_obj1));
  EXPECT_TRUE(IsObjectDestroyed(t_obj2));

  ResetWeakObjectDestruction();
}

TEST(TestGLibObject, ListOperations)
{
  list<TestObjectWrapper> obj_list;
  TestGObject *t_obj1 = test_gobject_new();

  TestObjectWrapper g_obj1(t_obj1);
  TestObjectWrapper g_obj2(test_gobject_new());
  TestObjectWrapper g_obj3(test_gobject_new());
  TestObjectWrapper g_obj4;
  TestObjectWrapper g_obj5;

  EXPECT_EQ(RefCount(g_obj1), 1);

  obj_list.push_back(g_obj1);
  obj_list.push_back(g_obj2);
  obj_list.push_back(g_obj3);
  obj_list.push_back(g_obj4);
  obj_list.push_back(g_obj5);
  EXPECT_EQ(obj_list.size(), 5);

  EXPECT_EQ(RefCount(g_obj1), 2);

  obj_list.remove(g_obj2);
  EXPECT_EQ(obj_list.size(), 4);

  EXPECT_TRUE(std::find(obj_list.begin(), obj_list.end(), g_obj3) != obj_list.end());
  EXPECT_TRUE(std::find(obj_list.begin(), obj_list.end(), g_obj3.RawPtr()) != obj_list.end());

  obj_list.remove(TestObjectWrapper(t_obj1, AddRef()));
  EXPECT_EQ(obj_list.size(), 3);

  EXPECT_TRUE(std::find(obj_list.begin(), obj_list.end(), g_obj4) != obj_list.end());
  obj_list.remove(g_obj5);
  EXPECT_EQ(obj_list.size(), 1);
}

} // Namespace
