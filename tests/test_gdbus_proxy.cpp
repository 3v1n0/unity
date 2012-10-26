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

TEST_F(TestGDBusProxy, TestConstruction)
{
  EXPECT_FALSE(proxy.IsConnected());
  Utils::WaitUntil(sigc::mem_fun(proxy, &glib::DBusProxy::IsConnected));
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

  Utils::WaitUntil(sigc::mem_fun(proxy, &glib::DBusProxy::IsConnected));
  EXPECT_TRUE(proxy.IsConnected()); // fail if we are not connected
  proxy.Connect("TestSignal", signal_connection);
  proxy.Call("TestMethod", parameters, method_connection); 

  Utils::WaitUntil(got_result_return);
  Utils::WaitUntil(got_signal_return);
 
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
  glib::Object<GCancellable> cancellable(g_cancellable_new());
  // but this has to work eitherway
  proxy.Call("TestMethod", g_variant_new("(s)", "TestStringTestString"),
               method_connection, cancellable);

  // this could mostly cause the next test to fail
  g_cancellable_cancel(cancellable);
  EXPECT_FALSE(got_result_return);
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

  Utils::WaitUntil(got_result_return);
 
  EXPECT_TRUE(proxy.IsConnected());
  EXPECT_EQ("TestStringTestString", call_return);
}


}
