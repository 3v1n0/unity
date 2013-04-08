#include <gtest/gtest.h>
#include <glib-object.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibDBusProxy.h>

#include "test_utils.h"

using namespace std;
using namespace unity;

namespace
{

class TestGDBusProxy: public ::testing::Test
{
public:
  TestGDBusProxy()
    : got_signal_return(false)
    , got_result_return(false)
    , proxy("com.canonical.Unity.Test",
            "/com/canonical/gdbus_wrapper",
            "com.canonical.gdbus_wrapper")
  {
  }

  bool got_signal_return;
  bool got_result_return;
  glib::DBusProxy proxy;
};

class TestGDBusProxyInvalidService: public ::testing::Test
{
public:
  TestGDBusProxyInvalidService()
    : got_result_return(false)
    , proxy("com.canonical.Unity.Test.NonExistant",
            "/com/canonical/gdbus_wrapper",
            "com.canonical.gdbus_wrapper")
  {
  }

  bool got_result_return;
  glib::DBusProxy proxy;
};

TEST_F(TestGDBusProxy, TestConstruction)
{
  EXPECT_FALSE(proxy.IsConnected());
  Utils::WaitUntilMSec(sigc::mem_fun(proxy, &glib::DBusProxy::IsConnected));
  EXPECT_TRUE(proxy.IsConnected());
}

TEST_F(TestGDBusProxy, TestMethodReturn)
{
  // Our service is setup so that if you call the TestMethod method, it will emit the TestSignal method
  // with whatever string you pass in
  gchar* expected_return = (gchar *)"TestStringTestString☻☻☻"; // cast to get gcc to shut up
  std::string returned_signal("Not equal");
  std::string returned_result("Not equal");

  GVariant* parameters = g_variant_new("(s)", expected_return);
  // signal callback
  auto signal_connection = [&](GVariant *variant)
  {
    if (variant != nullptr)
    {
      returned_signal = g_variant_get_string(g_variant_get_child_value(variant, 0), NULL);
    }

    got_signal_return = true;
  };

  // method callback
  auto method_connection = [&](GVariant *variant)
  {
    if (variant != nullptr)
    {
      returned_result = g_variant_get_string(g_variant_get_child_value(variant, 0), NULL);
    }

    got_result_return = true;
  };

  Utils::WaitUntilMSec(sigc::mem_fun(proxy, &glib::DBusProxy::IsConnected));
  EXPECT_TRUE(proxy.IsConnected()); // fail if we are not connected
  proxy.Connect("TestSignal", signal_connection);
  proxy.Call("TestMethod", parameters, method_connection); 

  Utils::WaitUntilMSec(got_result_return);
  Utils::WaitUntilMSec(got_signal_return);
 
  EXPECT_EQ(returned_result, expected_return);
  EXPECT_EQ(returned_signal, expected_return);
}

TEST_F(TestGDBusProxy, TestCancelling)
{
  std::string call_return;
  // method callback
  auto method_connection = [&](GVariant *variant)
  {
    if (variant != nullptr)
    {
      call_return = g_variant_get_string(g_variant_get_child_value(variant, 0), NULL);
    }

    got_result_return = true;
  };

  EXPECT_FALSE(proxy.IsConnected()); // we shouldn't be connected yet
  glib::Cancellable cancellable;
  // but this has to work eitherway
  proxy.Call("TestMethod", g_variant_new("(s)", "TestStringTestString"),
             method_connection, cancellable);

  // this could mostly cause the next test to fail
  cancellable.Cancel();
  EXPECT_FALSE(got_result_return);
}

TEST_F(TestGDBusProxy, TestAcquiring)
{
  const int NUM_REQUESTS = 10;
  int completed = 0;
  std::string call_return;
  // method callback
  auto method_connection = [&](GVariant* variant, glib::Error const& err)
  {
    if (variant != nullptr)
    {
      call_return = g_variant_get_string(g_variant_get_child_value(variant, 0), NULL);
    }

    if (++completed >= NUM_REQUESTS) got_result_return = true;
  };

  EXPECT_FALSE(proxy.IsConnected()); // we shouldn't be connected yet
  for (int i = 0; i < NUM_REQUESTS; i++)
  {
    proxy.CallBegin("TestMethod", g_variant_new("(s)", "TestStringTestString"),
                    method_connection, nullptr);
    Utils::WaitForTimeoutMSec(150);
  }
  Utils::WaitUntilMSec(got_result_return, 2);
}

TEST_F(TestGDBusProxyInvalidService, TestTimeouting)
{
  std::string call_return;
  // method callback
  auto method_connection = [&](GVariant* variant, glib::Error const& err)
  {
    if (variant != nullptr)
    {
      call_return = g_variant_get_string(g_variant_get_child_value(variant, 0), NULL);
    }

    got_result_return = true;
  };

  EXPECT_FALSE(proxy.IsConnected()); // we shouldn't be connected yet

  proxy.CallBegin("TestMethod", g_variant_new("(s)", "TestStringTestString"),
                  method_connection, nullptr, (GDBusCallFlags) 0, 100);
  // want to test timeout, if non-blocking sleep was used, the proxy would
  // be acquired (with a dbus error)
  g_usleep(110000);

  Utils::WaitUntilMSec(got_result_return);
  EXPECT_EQ(call_return, "");
}

TEST_F(TestGDBusProxy, TestMethodCall)
{
  std::string call_return;
  // method callback
  auto method_connection = [&](GVariant *variant)
  {
    if (variant != nullptr)
    {
      call_return = g_variant_get_string(g_variant_get_child_value(variant, 0), NULL);
    }

    got_result_return = true;
  };

  EXPECT_FALSE(proxy.IsConnected()); // we shouldn't be connected yet
  // but this has to work eitherway
  proxy.Call("TestMethod", g_variant_new("(s)", "TestStringTestString"),
             method_connection);

  Utils::WaitUntilMSec(got_result_return);
 
  EXPECT_TRUE(proxy.IsConnected());
  EXPECT_EQ("TestStringTestString", call_return);
}

TEST_F(TestGDBusProxy, TestDisconnectSignal)
{
  bool got_signal = false;
  proxy.Connect("TestSignal", [&got_signal] (GVariant*) { got_signal = true; });
  proxy.Call("TestMethod", g_variant_new("(s)", "Signal!"));
  Utils::WaitUntilMSec(got_signal);
  ASSERT_TRUE(got_signal);

  got_signal = false;
  proxy.DisconnectSignal("TestSignal");
  proxy.Call("TestMethod", g_variant_new("(s)", "NewSignal!"));

  Utils::WaitForTimeoutMSec(50);
  EXPECT_FALSE(got_signal);
}

TEST_F(TestGDBusProxy, TestDisconnectSignalAll)
{
  bool got_signal = false;
  proxy.Connect("TestSignal", [&got_signal] (GVariant*) { got_signal = true; });
  proxy.Call("TestMethod", g_variant_new("(s)", "Signal!"));
  Utils::WaitUntilMSec(got_signal);
  ASSERT_TRUE(got_signal);

  got_signal = false;
  proxy.DisconnectSignal();
  proxy.Call("TestMethod", g_variant_new("(s)", "NewSignal!"));

  Utils::WaitForTimeoutMSec(50);
  EXPECT_FALSE(got_signal);
}

TEST_F(TestGDBusProxy, TestSignalBeforeConnection)
{
  bool got_signal = false;
  proxy.Connect("TestSignal", [&got_signal] (GVariant*) { got_signal = true; });
  proxy.Call("TestMethod", g_variant_new("(s)", "Signal!"));
  Utils::WaitUntilMSec(got_signal);
  EXPECT_TRUE(got_signal);
}

TEST_F(TestGDBusProxy, TestSignalAfterConnection)
{
  Utils::WaitUntilMSec([this] { return proxy.IsConnected(); });

  bool got_signal = false;
  proxy.Connect("TestSignal", [&got_signal] (GVariant*) { got_signal = true; });
  proxy.Call("TestMethod", g_variant_new("(s)", "Signal!"));
  Utils::WaitUntilMSec(got_signal);
  EXPECT_TRUE(got_signal);
}

TEST_F(TestGDBusProxy, TestSignalAfterConnectionAndDisconnect)
{
  Utils::WaitUntilMSec([this] { return proxy.IsConnected(); });

  bool got_signal1 = false;
  proxy.Connect("TestSignal", [&got_signal1] (GVariant*) { got_signal1 = true; });
  proxy.DisconnectSignal();

  bool got_signal2 = false;
  proxy.Connect("TestSignal", [&got_signal2] (GVariant*) { got_signal2 = true; });
  proxy.Call("TestMethod", g_variant_new("(s)", "Signal!"));

  Utils::WaitUntilMSec(got_signal2);
  ASSERT_FALSE(got_signal1);
  EXPECT_TRUE(got_signal2);
}

TEST_F(TestGDBusProxy, GetROProperty)
{
  auto ROPropertyValue = [this] { return glib::Variant(proxy.GetProperty("ReadOnlyProperty")).GetInt32(); };
  EXPECT_EQ(ROPropertyValue(), 0);

  int value = g_random_int();
  proxy.Call("SetReadOnlyProperty", g_variant_new("(i)", value));

  Utils::WaitUntilMSec([this, value, ROPropertyValue] { return ROPropertyValue() == value; });
  EXPECT_EQ(ROPropertyValue(), value);
}

TEST_F(TestGDBusProxy, SetGetRWPropertyBeforeConnection)
{
  int value = g_random_int();
  proxy.SetProperty("ReadWriteProperty", g_variant_new_int32(value));

  auto RWPropertyValue = [this] { return glib::Variant(proxy.GetProperty("ReadWriteProperty")).GetInt32(); };
  Utils::WaitUntilMSec([this, value, RWPropertyValue] { return RWPropertyValue() == value; });
  EXPECT_EQ(RWPropertyValue(), value);
}

TEST_F(TestGDBusProxy, SetGetRWPropertyAfterConnection)
{
  Utils::WaitUntilMSec([this] { return proxy.IsConnected(); });

  auto RWPropertyValue = [this] { return glib::Variant(proxy.GetProperty("ReadWriteProperty")).GetInt32(); };

  int value = g_random_int();
  proxy.SetProperty("ReadWriteProperty", g_variant_new_int32(value));

  Utils::WaitUntilMSec([this, value, RWPropertyValue] { return RWPropertyValue() == value; });
  EXPECT_EQ(RWPropertyValue(), value);
}

TEST_F(TestGDBusProxy, SetGetAsyncRWPropertyBeforeConnection)
{
  int value = g_random_int();
  proxy.SetProperty("ReadWriteProperty", g_variant_new_int32(value));

  int got_value = 0;
  proxy.GetProperty("ReadWriteProperty", [&got_value] (GVariant* value) { got_value = g_variant_get_int32(value); });

  Utils::WaitUntilMSec([this, value, &got_value] { return got_value == value; });
  ASSERT_EQ(got_value, value);
  EXPECT_EQ(got_value, glib::Variant(proxy.GetProperty("ReadWriteProperty")).GetInt32());
}

TEST_F(TestGDBusProxy, SetGetAsyncRWPropertyAfterConnection)
{
  Utils::WaitUntilMSec([this] { return proxy.IsConnected(); });

  int value = g_random_int();
  proxy.SetProperty("ReadWriteProperty", g_variant_new_int32(value));

  int got_value = 0;
  proxy.GetProperty("ReadWriteProperty", [&got_value] (GVariant* value) { got_value = g_variant_get_int32(value); });

  Utils::WaitUntilMSec([this, value, &got_value] { return got_value == value; });
  ASSERT_EQ(got_value, value);
  EXPECT_EQ(got_value, glib::Variant(proxy.GetProperty("ReadWriteProperty")).GetInt32());
}

TEST_F(TestGDBusProxy, SetWOPropertyBeforeConnection)
{
  int value = g_random_int();
  proxy.SetProperty("WriteOnlyProperty", g_variant_new_int32(value));

  int wo_value = 0;
  proxy.Call("GetWriteOnlyProperty", nullptr, [&wo_value] (GVariant* value) {
    wo_value = glib::Variant(value).GetInt32();
  });

  Utils::WaitUntilMSec([this, value, &wo_value] { return wo_value == value; });
  EXPECT_EQ(wo_value, value);
}

TEST_F(TestGDBusProxy, SetWOPropertyAfterConnection)
{
  Utils::WaitUntilMSec([this] { return proxy.IsConnected(); });

  int value = g_random_int();
  proxy.SetProperty("WriteOnlyProperty", g_variant_new_int32(value));

  int wo_value = 0;
  proxy.Call("GetWriteOnlyProperty", nullptr, [&wo_value] (GVariant* value) {
    wo_value = glib::Variant(value).GetInt32();
  });

  Utils::WaitUntilMSec([this, value, &wo_value] { return wo_value == value; });
  EXPECT_EQ(wo_value, value);
}

TEST_F(TestGDBusProxy, PropertyChangedSignalBeforeConnection)
{
  int value = g_random_int();
  bool got_signal = false;
  proxy.ConnectProperty("ReadWriteProperty", [&got_signal, value] (GVariant* new_value) {
    got_signal = true;
    EXPECT_EQ(g_variant_get_int32(new_value), value);
  });

  proxy.SetProperty("ReadWriteProperty", g_variant_new_int32(value));

  Utils::WaitUntilMSec(got_signal);
  EXPECT_TRUE(got_signal);
}

TEST_F(TestGDBusProxy, PropertyChangedSignalAfterConnection)
{
  Utils::WaitUntilMSec([this] { return proxy.IsConnected(); });

  int value = g_random_int();
  bool got_signal = false;
  proxy.ConnectProperty("ReadOnlyProperty", [&got_signal, value] (GVariant* new_value) {
    got_signal = true;
    EXPECT_EQ(g_variant_get_int32(new_value), value);
  });

  proxy.Call("SetReadOnlyProperty", g_variant_new("(i)", value));

  Utils::WaitUntilMSec(got_signal);
  EXPECT_TRUE(got_signal);
}

TEST_F(TestGDBusProxy, PropertyChangedSignalAfterConnectionAndDisconnect)
{
  Utils::WaitUntilMSec([this] { return proxy.IsConnected(); });

  bool got_signal1 = false;
  proxy.ConnectProperty("ReadWriteProperty", [&got_signal1] (GVariant*) { got_signal1 = true; });
  proxy.DisconnectProperty();

  bool got_signal2 = false;
  proxy.ConnectProperty("ReadWriteProperty", [&got_signal2] (GVariant*) { got_signal2 = true; });
  proxy.SetProperty("ReadWriteProperty", g_variant_new_int32(g_random_int()));

  Utils::WaitUntilMSec(got_signal2);
  ASSERT_FALSE(got_signal1);
  EXPECT_TRUE(got_signal2);
}

}
