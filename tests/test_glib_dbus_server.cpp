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

TEST(TestGLibDBusServerUnnamed, Connects)
{
  bool connected = false;
  DBusServer server;
  server.connected.connect([&connected] { connected = true; });
  Utils::WaitUntilMSec(connected);

  EXPECT_TRUE(connected);
  EXPECT_TRUE(server.IsConnected());
  EXPECT_TRUE(server.Name().empty());
  EXPECT_FALSE(server.OwnsName());

  Utils::WaitForTimeoutMSec(50);
  EXPECT_TRUE(server.IsConnected());
}

TEST(TestGLibDBusServerUnnamed, AddsObjectWhenConnected)
{
  bool object_registered = false;

  DBusServer server;
  auto object = std::make_shared<DBusObject>(introspection::SINGLE_OJBECT, OBJECT_INTERFACE);

  object->registered.connect([&object_registered] (std::string const& path) {
    EXPECT_EQ(path, TEST_OBJECT_PATH);
    object_registered = true;
  });

  server.AddObject(object, TEST_OBJECT_PATH);
  ASSERT_EQ(server.GetObject(OBJECT_INTERFACE), object);
  ASSERT_EQ(server.GetObjects().front(), object);

  Utils::WaitUntilMSec([&server] { return server.IsConnected(); });

  ASSERT_EQ(server.GetObjects().front(), object);
  EXPECT_TRUE(object_registered);
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

TEST_F(TestGLibDBusServer, Connects)
{
  bool connected = false;
  server.connected.connect([&connected] { connected = true; });
  Utils::WaitUntilMSec(connected);

  EXPECT_TRUE(connected);
  EXPECT_TRUE(server.OwnsName());
  EXPECT_TRUE(server.IsConnected());
}

TEST_F(TestGLibDBusServer, OwnsName)
{
  bool name_owned = false;
  EXPECT_EQ(server.Name(), DBUS_TEST_NAME);

  server.name_acquired.connect([&name_owned] { name_owned = true; });
  Utils::WaitUntilMSec(name_owned);

  EXPECT_TRUE(name_owned);
  EXPECT_TRUE(server.OwnsName());
  EXPECT_TRUE(server.IsConnected());
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
    Utils::WaitUntilMSec([this] { return proxy->IsConnected();});
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
    proxy->DisconnectSignal();
    proxy->DisconnectProperty();
  }

  static void TearDownTestCase()
  {
    proxy.reset();
    server.reset();
  }

  void TestMethodCall(std::string const& method_name, GVariant* parameters = nullptr, GVariant* returns = nullptr)
  {
    std::string const& expected_method = method_name;
    Variant expected_parameters = parameters ? parameters : g_variant_new("()");
    Variant returned_value = returns ? returns : g_variant_new("()");

    bool called = false;
    object->SetMethodsCallsHandler([&] (std::string const& called_method, GVariant* called_parameters) {
      called = true;
      EXPECT_EQ(called_method, expected_method);
      EXPECT_TRUE(g_variant_equal(called_parameters, expected_parameters) != FALSE);

      return returned_value;
    });

    bool returned = false;
    proxy->CallBegin(expected_method, expected_parameters, [&returned, &returned_value] (GVariant* ret, Error const& error) {
      returned = true;
      EXPECT_TRUE(g_variant_equal(ret, returned_value) != FALSE);
      EXPECT_FALSE(error);
    });

    Utils::WaitUntilMSec(called);
    EXPECT_TRUE(called);

    Utils::WaitUntilMSec(returned);
    EXPECT_TRUE(returned);
  }

  static DBusServer::Ptr server;
  static DBusProxy::Ptr proxy;
  DBusObject::Ptr object;
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

TEST_F(TestGLibDBusServerInteractions, NotHandledMethod)
{
  bool got_return = false;
  proxy->CallBegin("VoidMethod", nullptr, [&got_return] (GVariant* ret, Error const& error) {
    got_return = TRUE;
    EXPECT_TRUE(error);
  });

  Utils::WaitUntilMSec(got_return);
  EXPECT_TRUE(got_return);
}

TEST_F(TestGLibDBusServerInteractions, NullReturnOnMethodWithReturn)
{
  object->SetMethodsCallsHandler([&] (std::string const& called_method, GVariant* called_parameters) {
    return static_cast<GVariant*>(nullptr);
  });

  bool returned = false;
  proxy->CallBegin("MethodWithReturnValue", nullptr, [&returned] (GVariant*, Error const& error) {
    returned = true;
    EXPECT_TRUE(error);
  });

  Utils::WaitUntilMSec(returned);
}

TEST_F(TestGLibDBusServerInteractions, EmptyReturnOnMethodWithReturn)
{
  object->SetMethodsCallsHandler([&] (std::string const& called_method, GVariant* called_parameters) {
    return g_variant_new("()");
  });

  bool returned = false;
  proxy->CallBegin("MethodWithReturnValue", nullptr, [&returned] (GVariant*, Error const& error) {
    returned = true;
    EXPECT_TRUE(error);
  });

  Utils::WaitUntilMSec(returned);
}

TEST_F(TestGLibDBusServerInteractions, SignalWithNoParametersEmission)
{
  auto const& signal_name = "SignalWithNoParameters";

  bool signal_got = false;
  proxy->Connect(signal_name, [&signal_got] (GVariant* parameters) {
    signal_got = true;
    EXPECT_TRUE(g_variant_equal(g_variant_new("()"), parameters) != FALSE);
  });

  object->EmitSignal(signal_name);
  Utils::WaitUntilMSec(signal_got);
  EXPECT_TRUE(signal_got);

  signal_got = false;
  server->EmitSignal(object->InterfaceName(), signal_name);
  Utils::WaitUntilMSec(signal_got);
  EXPECT_TRUE(signal_got);
}

TEST_F(TestGLibDBusServerInteractions, SignalWithParameterEmission)
{
  auto const& signal_name = "SignalWithParameter";
  Variant sent_parameters = g_variant_new("(s)", "Unity is Awesome!");

  bool signal_got = false;
  proxy->Connect(signal_name, [&signal_got, &sent_parameters] (GVariant* parameters) {
    signal_got = true;
    EXPECT_TRUE(g_variant_equal(sent_parameters, parameters) != FALSE);
  });

  object->EmitSignal(signal_name, sent_parameters);
  Utils::WaitUntilMSec(signal_got);
  EXPECT_TRUE(signal_got);

  signal_got = false;
  server->EmitSignal(object->InterfaceName(), signal_name, sent_parameters);
  Utils::WaitUntilMSec(signal_got);
  EXPECT_TRUE(signal_got);
}

struct ReadableProperties : TestGLibDBusServerInteractions, testing::WithParamInterface<std::string> {};
INSTANTIATE_TEST_CASE_P(TestGLibDBusServerInteractions, ReadableProperties, testing::Values("ReadOnlyProperty", "ReadWriteProperty"));

TEST_P(/*TestGLibDBusServerInteractions*/ReadableProperties, PropertyGetter)
{
  int value = g_random_int();
  bool called = false;
  object->SetPropertyGetter([this, &called, value] (std::string const& property) -> GVariant* {
    EXPECT_EQ(property, GetParam());
    if (property == GetParam())
    {
      called = true;
      return g_variant_new_int32(value);
    }

    return nullptr;
  });

  int got_value = 0;
  proxy->GetProperty(GetParam(), [&got_value] (GVariant* value) { got_value = g_variant_get_int32(value); });

  Utils::WaitUntilMSec(called);
  ASSERT_TRUE(called);

  Utils::WaitUntilMSec([&got_value, value] { return got_value == value; });
  EXPECT_EQ(got_value, value);
}

TEST_P(/*TestGLibDBusServerInteractions*/ReadableProperties, EmitPropertyChanged)
{
  int value = g_random_int();
  object->SetPropertyGetter([this, value] (std::string const& property) -> GVariant* {
    if (property == GetParam())
      return g_variant_new_int32(value);

    return nullptr;
  });

  bool got_signal = false;
  proxy->ConnectProperty(GetParam(), [&got_signal, value] (GVariant* new_value) {
    EXPECT_EQ(g_variant_get_int32(new_value), value);
    got_signal = true;
  });

  object->EmitPropertyChanged(GetParam());

  Utils::WaitUntilMSec(got_signal);
  ASSERT_TRUE(got_signal);
}

struct WritableProperties : TestGLibDBusServerInteractions, testing::WithParamInterface<std::string> {};
INSTANTIATE_TEST_CASE_P(TestGLibDBusServerInteractions, WritableProperties, testing::Values("WriteOnlyProperty", "ReadWriteProperty"));

TEST_P(/*TestGLibDBusServerInteractions*/WritableProperties, PropertySetter)
{
  int value = 0;
  bool called = false;
  object->SetPropertySetter([this, &called, &value] (std::string const& property, GVariant* new_value) {
    EXPECT_EQ(property, GetParam());
    if (property == GetParam())
    {
      value = g_variant_get_int32(new_value);
      called = true;
      return true;
    }

    return false;
  });

  int new_value = g_random_int();
  proxy->SetProperty(GetParam(), g_variant_new_int32(new_value));

  Utils::WaitUntilMSec(called);
  ASSERT_TRUE(called);
  EXPECT_EQ(value, new_value);
}

} // Namespace
