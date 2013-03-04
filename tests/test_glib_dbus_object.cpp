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
#include <UnityCore/GLibDBusServer.h>

using namespace unity::glib;

namespace
{

namespace introspection
{
const std::string SINGLE_OJBECT =
R"(<node>
  <interface name="com.canonical.Unity.ObjectTest">
    <method name="Foo">
      <arg type="i" name="type" direction="in"/>
    </method>
    <method name="Bar">
      <arg type="i" name="type" direction="out"/>
    </method>
    <signal name="SignalWithNoParameters" />
    <signal name="SignalWithParameter">
      <arg type="i" name="arg" />
    </signal>
    <property name="ReadOnlyProperty" type="i" access="read"/>
    <property name="WriteOnlyProperty" type="i" access="write"/>
    <property name="ReadWriteProperty" type="i" access="readwrite"/>
  </interface>
</node>
)";

const std::string MULTIPLE_OJBECTS =
R"(<node>
  <interface name="com.canonical.Unity.ObjectTest1">
    <method name="Test1" />
  </interface>
  <interface name="com.canonical.Unity.ObjectTest2">
    <method name="Test2" />
  </interface>
  <interface name="com.canonical.Unity.ObjectTest3">
    <method name="Test3" />
  </interface>
</node>)";

const std::string INVALID_SYNTAX =
R"(<node>
  <interface name="com.canonical.Unity.ObjectTest">
    <method name="Foo">
)";
}

TEST(TestGLibDBusObject, InitializeWithOneValidObject)
{
  DBusObject object(introspection::SINGLE_OJBECT, "com.canonical.Unity.ObjectTest");
  EXPECT_EQ(object.InterfaceName(), "com.canonical.Unity.ObjectTest");
}

TEST(TestGLibDBusObject, InitializeWithOneInvalidObject)
{
  DBusObject object(introspection::SINGLE_OJBECT, "foo.invalid");
  EXPECT_TRUE(object.InterfaceName().empty());
}

TEST(TestGLibDBusObject, InitializeWithInvalidSyntax)
{
  DBusObject object(introspection::INVALID_SYNTAX, "com.canonical.Unity.ObjectTest");
  EXPECT_TRUE(object.InterfaceName().empty());
}

TEST(TestGLibDBusObject, InitializeWithMultipleObjects)
{
  DBusObject obj1(introspection::MULTIPLE_OJBECTS, "com.canonical.Unity.ObjectTest1");
  EXPECT_EQ(obj1.InterfaceName(), "com.canonical.Unity.ObjectTest1");

  DBusObject obj2(introspection::MULTIPLE_OJBECTS, "com.canonical.Unity.ObjectTest2");
  EXPECT_EQ(obj2.InterfaceName(), "com.canonical.Unity.ObjectTest2");

  DBusObject obj3(introspection::MULTIPLE_OJBECTS, "com.canonical.Unity.ObjectTest3");
  EXPECT_EQ(obj3.InterfaceName(), "com.canonical.Unity.ObjectTest3");

  DBusObject obj4(introspection::MULTIPLE_OJBECTS, "com.canonical.Unity.ObjectTest4");
  EXPECT_TRUE(obj4.InterfaceName().empty());
}

TEST(TestGLibDBusObject, InitializeWithNoObject)
{
  DBusObject object("", "com.canonical.Unity.ObjectTest");
  EXPECT_TRUE(object.InterfaceName().empty());
}


// Builder

TEST(TestGLibDBusObjectBuilder, GetObjectsForIntrospectionWithOneObject)
{
  auto const& objs = DBusObjectBuilder::GetObjectsForIntrospection(introspection::SINGLE_OJBECT);
  ASSERT_EQ(objs.size(), 1);
  EXPECT_EQ(objs.front()->InterfaceName(), "com.canonical.Unity.ObjectTest");
}

TEST(TestGLibDBusObjectBuilder, GetObjectsForIntrospectionWithMultipleObjects)
{
  auto const& objs = DBusObjectBuilder::GetObjectsForIntrospection(introspection::MULTIPLE_OJBECTS);
  ASSERT_EQ(objs.size(), 3);
  EXPECT_EQ((*std::next(objs.begin(), 0))->InterfaceName(), "com.canonical.Unity.ObjectTest1");
  EXPECT_EQ((*std::next(objs.begin(), 1))->InterfaceName(), "com.canonical.Unity.ObjectTest2");
  EXPECT_EQ((*std::next(objs.begin(), 2))->InterfaceName(), "com.canonical.Unity.ObjectTest3");
}

TEST(TestGLibDBusObjectBuilder, GetObjectsForIntrospectionWithInvalidObject)
{
  auto const& objs = DBusObjectBuilder::GetObjectsForIntrospection(introspection::INVALID_SYNTAX);
  EXPECT_TRUE(objs.empty());
}


} // Namespace
