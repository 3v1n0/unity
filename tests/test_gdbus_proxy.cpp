#include <gtest/gtest.h>
#include <glib-object.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibDBusProxy.h>

using namespace std;
using namespace unity;

namespace
{

GMainLoop* loop_ = g_main_loop_new(NULL, FALSE);
glib::DBusProxy proxy("com.canonical.Unity.Test", 
                      "/com/canonical/gdbus_wrapper", 
                      "com.canonical.gdbus_wrapper");

class TestGDBusProxy: public ::testing::Test
{
public:
  TestGDBusProxy()
    : connected_result(false)
  {
  }
  bool connected_result;
};

TEST_F(TestGDBusProxy, TestConstruction)
{
  
  // performs a check on the proxy, if the proxy is connected, report a sucess
  auto timeout_check = [] (gpointer data) -> gboolean
  {
    TestGDBusProxy* self = static_cast<TestGDBusProxy*>(data);
    if (proxy.IsConnected())
    {
      self->connected_result = true;
      g_main_loop_quit(loop_);
      return FALSE;
    }
    else
    {
      self->connected_result = false;
      return TRUE;
    }
  };
  

  // if the proxy is not connected when this lambda runs, fail.
  auto timeout_bailout = [] (gpointer data) -> gboolean 
  {
    TestGDBusProxy* self = static_cast<TestGDBusProxy*>(data);
    // reached timeout, failed testing
    self->connected_result = false;
    g_main_loop_quit(loop_);
    return FALSE;
  };
  
  g_timeout_add_seconds(1, timeout_check, this); // check once a second
  g_timeout_add_seconds(10, timeout_bailout, this); // bail out after ten

  g_main_loop_run(loop_);
  
  EXPECT_EQ(connected_result, true);
}

TEST_F(TestGDBusProxy, TestMethodReturn)
{
  glib::String expected_return((gchar *)"TestStringTestString☻☻☻"); // cast to get gcc to shut up
  gchar* returned_result = (gchar*)"Not equal"; // cast to get gcc to shut up
  gchar* returned_signal = (gchar*)"Not equal"; // cast to get gcc to shut up

  GVariant* param_value = g_variant_new_string(expected_return);
  GVariant* parameters = g_variant_new_tuple(&param_value, 1);
  // signal callback
  auto signal_connection = [&](GVariant *variant)
  {
    if (variant != nullptr)
    {
      returned_signal = (gchar*)g_variant_get_string(g_variant_get_child_value(variant, 0), NULL);
    }
  };

  // method callback
  auto method_connection = [&](GVariant *variant)
  {
    if (variant != nullptr)
    {
      returned_result = (gchar*)g_variant_get_string(g_variant_get_child_value(variant, 0), NULL);
    }
  };
  
  auto timeout_bailout = [] (gpointer data) -> gboolean // bail out after 10 seconds
  {
    g_main_loop_quit(loop_);
    return FALSE;
  };
   
  g_timeout_add_seconds(10, timeout_bailout, this);

  EXPECT_EQ(proxy.IsConnected(), true); // fail if we are not connected
  proxy.Connect("TestSignal", signal_connection);
  proxy.Call("TestMethod", parameters, method_connection); 

 
  // next check we get 30 entries from this specific known callback
  g_main_loop_run(loop_);

  EXPECT_EQ(g_strcmp0(expected_return, returned_result), 0);
  EXPECT_EQ(g_strcmp0(expected_return, returned_signal), 0);
}


}
