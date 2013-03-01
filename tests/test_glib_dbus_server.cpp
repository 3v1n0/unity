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
#include "test_utils.h"
#include <UnityCore/GLibDBusServer.h>
#include <UnityCore/GLibDBusProxy.h>
#include <UnityCore/Variant.h>

using namespace unity::glib;

namespace
{

const std::string DBUS_TEST_NAME = "com.canonical.Unity.Test.Server.Name";
const std::string OBJECT_INTERFACE = "com.canonical.Unity.ObjectTest";
const std::string TEST_OBJECT_PATH = "/com/canonical/Unity/Test/Object";

namespace introspection
{
const std::string SINGLE_OJBECT =
R"(<node>
  <interface name="com.canonical.Unity.ObjectTest">
    <method name="VoidMethod" />
    <method name="MethodWithParameters">
      <arg type="i" name="arg_int" direction="in"/>
      <arg type="s" name="arg_string" direction="in"/>
      <arg type="u" name="arg_uint" direction="in"/>
    </method>
    <method name="MethodWithReturnValue">
      <arg type="u" name="reply" direction="out"/>
    </method>
    <method name="MethodWithParametersAndReturnValue">
      <arg type="s" name="query" direction="in"/>
      <arg type="s" name="reply" direction="out"/>
    </method>
    <signal name="SignalWithNoParameters" />
    <signal name="SignalWithParameter">
      <arg type="s" name="arg" />
    </signal>
    <property name="ReadOnlyProperty" type="i" access="read"/>
    <property name="WriteOnlyProperty" type="i" access="write"/>
    <property name="ReadWriteProperty" type="i" access="readwrite"/>
  </interface>
</node>)";

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

}

struct TestGLibDBusServer : testing::Test
{
  TestGLibDBusServer()
    : server(DBUS_TEST_NAME)
  {}

  void TearDown()
  {
    // We check that the connection is still active
    Utils::WaitForTimeoutMSec(50);
    EXPECT_TRUE(server.OwnsName());
  }

  DBusServer server;
};

TEST_F(TestGLibDBusServer, OwnsName)
{
  bool name_owned = false;
  EXPECT_EQ(server.Name(), DBUS_TEST_NAME);

  server.name_acquired.connect([&name_owned] { name_owned = true; });
  Utils::WaitUntilMSec(name_owned);

  EXPECT_TRUE(name_owned);
  EXPECT_TRUE(server.OwnsName());
}

TEST_F(TestGLibDBusServer, AddsObjectWhenOwingName)
{
  bool object_registered = false;

  auto object = std::make_shared<DBusObject>(introspection::SINGLE_OJBECT, OBJECT_INTERFACE);

  object->registered.connect([&object_registered] (std::string const& path) {
    EXPECT_EQ(path, TEST_OBJECT_PATH);
    object_registered = true;
  });

  server.AddObject(object, TEST_OBJECT_PATH);
  ASSERT_EQ(server.GetObject(OBJECT_INTERFACE), object);
  ASSERT_EQ(server.GetObjects().front(), object);

  Utils::WaitUntilMSec([this] { return server.OwnsName(); });

  ASSERT_EQ(server.GetObjects().front(), object);
  EXPECT_TRUE(object_registered);
}

TEST_F(TestGLibDBusServer, AddsObjectsWhenOwingName)
{
  unsigned objects_registered = 0;

  server.AddObjects(introspection::MULTIPLE_OJBECTS, TEST_OBJECT_PATH);
  ASSERT_EQ(server.GetObjects().size(), 3);

  for (auto const& obj : server.GetObjects())
  {
    ASSERT_EQ(server.GetObject(obj->InterfaceName()), obj);

    obj->registered.connect([&objects_registered] (std::string const& path) {
      EXPECT_EQ(path, TEST_OBJECT_PATH);
      ++objects_registered;
    });
  }

  Utils::WaitUntilMSec([this] { return server.OwnsName(); });

  EXPECT_EQ(objects_registered, 3);
}

TEST_F(TestGLibDBusServer, RemovingObjectWontRegisterIt)
{
  unsigned objects_registered = 0;

  server.AddObjects(introspection::MULTIPLE_OJBECTS, TEST_OBJECT_PATH);
  ASSERT_EQ(server.GetObjects().size(), 3);

  server.RemoveObject(server.GetObjects().front());
  ASSERT_EQ(server.GetObjects().size(), 2);

  for (auto const& obj : server.GetObjects())
  {
    obj->registered.connect([&objects_registered] (std::string const& path) {
      EXPECT_EQ(path, TEST_OBJECT_PATH);
      ++objects_registered;
    });
  }

  Utils::WaitUntilMSec([this] { return server.OwnsName(); });

  EXPECT_EQ(objects_registered, 2);
}

TEST_F(TestGLibDBusServer, RemovingObjectsUnregistersThem)
{
  server.AddObjects(introspection::MULTIPLE_OJBECTS, TEST_OBJECT_PATH);
  ASSERT_EQ(server.GetObjects().size(), 3);

  Utils::WaitUntilMSec([this] { return server.OwnsName(); });

  unsigned objects_unregistered = 0;
  for (auto const& obj : server.GetObjects())
  {
    obj->unregistered.connect([&objects_unregistered] (std::string const& path) {
      EXPECT_EQ(path, TEST_OBJECT_PATH);
      ++objects_unregistered;
    });
  }

  server.RemoveObject(server.GetObjects().front());
  ASSERT_EQ(server.GetObjects().size(), 2);
  EXPECT_EQ(objects_unregistered, 1);

  server.RemoveObject(server.GetObjects().front());
  ASSERT_EQ(server.GetObjects().size(), 1);
  EXPECT_EQ(objects_unregistered, 2);

  server.RemoveObject(server.GetObjects().front());
  ASSERT_EQ(server.GetObjects().size(), 0);
  EXPECT_EQ(objects_unregistered, 3);
}

///

struct TestGLibDBusServerInteractions : testing::Test
{
  TestGLibDBusServerInteractions()
  {}

  static void SetUpTestCase()
  {
    server = std::make_shared<DBusServer>(DBUS_TEST_NAME);
    server->AddObjects(introspection::SINGLE_OJBECT, TEST_OBJECT_PATH);
    proxy = std::make_shared<DBusProxy>(server->Name(), TEST_OBJECT_PATH, OBJECT_INTERFACE);
  }

  void SetUp()
  {
    Utils::WaitUntilMSec([this] { return server->OwnsName(); });
    Utils::WaitUntil([this] { return proxy->IsConnected();}, true, 3);
    ASSERT_TRUE(proxy->IsConnected());

    auto const& objects = server->GetObjects();
    ASSERT_EQ(objects.size(), 1);
    object = objects.front();
    ASSERT_NE(object, nullptr);
  }

  void TearDown()
  {
    object->SetMethodsCallsHandler(nullptr);
    object->SetPropertyGetter(nullptr);
    object->SetPropertySetter(nullptr);

    called_method_.clear();
  }

  GVariant* OnMethodCall(std::string const& method, GVariant* parameters)
  {
    called_method_ = method;

    EXPECT_EQ(called_method_, expected_method_);
    EXPECT_TRUE(g_variant_equal(parameters, expected_parameters_) != FALSE);

    return returned_value_;
  }

  void TestMethodCall(std::string const& method_name, GVariant* parameters = nullptr, GVariant* returns = nullptr)
  {
    object->SetMethodsCallsHandler(sigc::mem_fun(this, &TestGLibDBusServerInteractions::OnMethodCall));

    expected_method_ = method_name;
    expected_parameters_ = parameters ? parameters : g_variant_new("()");
    returned_value_ = returns ? returns : g_variant_new("()");

    bool returned = false;
    proxy->Call(expected_method_, expected_parameters_, [this, &returned] (GVariant* ret) {
      returned = true;
      EXPECT_TRUE(g_variant_equal(ret, returned_value_) != FALSE);
    });

    Utils::WaitUntilMSec([this] { return called_method_ == expected_method_; });
    Utils::WaitUntilMSec(returned);

    EXPECT_TRUE(returned);
  }

  static DBusServer::Ptr server;
  static DBusProxy::Ptr proxy;
  DBusObject::Ptr object;

  std::string called_method_;
  std::string expected_method_;
  Variant expected_parameters_;
  Variant returned_value_;
};

DBusServer::Ptr TestGLibDBusServerInteractions::server;
DBusProxy::Ptr TestGLibDBusServerInteractions::proxy;

TEST_F(TestGLibDBusServerInteractions, VoidMethodCall)
{
  TestMethodCall("VoidMethod");
}

TEST_F(TestGLibDBusServerInteractions, MethodWithParametersCall)
{
  auto parameters = g_variant_new("(isu)", 1, "unity", g_random_int());
  TestMethodCall("MethodWithParameters", parameters);
}

TEST_F(TestGLibDBusServerInteractions, MethodWithReturnValueCall)
{
  auto return_value = g_variant_new("(u)", g_random_int());
  TestMethodCall("MethodWithReturnValue", nullptr, return_value);
}

TEST_F(TestGLibDBusServerInteractions, MethodWithParametersAndReturnValueCall)
{
  auto parameters = g_variant_new("(s)", "unity?");
  auto return_value = g_variant_new("(s)", "It's Awesome!");
  TestMethodCall("MethodWithParametersAndReturnValue", parameters, return_value);
}

} // Namespace
