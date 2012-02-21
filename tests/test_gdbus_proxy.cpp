#include <gtest/gtest.h>
#include <glib-object.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibDBusProxy.h>

using namespace std;
using namespace unity;

namespace
{

GMainLoop* loop_ = NULL;
glib::DBusProxy* proxy = NULL;

class TestGDBusProxy: public ::testing::Test
{
public:
  TestGDBusProxy()
    : connected_result(false)
    , got_signal_return(false)
    , got_result_return(false)
  {
  }
  bool connected_result;
  bool got_signal_return;
  bool got_result_return;
};

TEST_F(TestGDBusProxy, TestConstruction)
{
  loop_ = g_main_loop_new(NULL, FALSE);
  proxy = new glib::DBusProxy("com.canonical.Unity.Test", 
                              "/com/canonical/gdbus_wrapper", 
                              "com.canonical.gdbus_wrapper");
  // performs a check on the proxy, if the proxy is connected, report a sucess
  auto timeout_check = [] (gpointer data) -> gboolean
  {
    TestGDBusProxy* self = static_cast<TestGDBusProxy*>(data);
    if (proxy->IsConnected())
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
  // Our service is setup so that if you call the TestMethod method, it will emit the TestSignal method
  // with whatever string you pass in
  gchar* expected_return = (gchar *)"TestStringTestString☻☻☻"; // cast to get gcc to shut up
  gchar* returned_result = g_strdup("Not equal"); 
  gchar* returned_signal = g_strdup("Not equal"); 

  GVariant* param_value = g_variant_new_string(expected_return);
  GVariant* parameters = g_variant_new_tuple(&param_value, 1);
  // signal callback
  auto signal_connection = [&](GVariant *variant)
  {
    if (variant != nullptr)
    {
      g_free(returned_signal);
      returned_signal = g_strdup(g_variant_get_string(g_variant_get_child_value(variant, 0), NULL));
    }

    got_signal_return = true;
    if (got_signal_return && got_result_return)
      g_main_loop_quit(loop_);
  };

  // method callback
  auto method_connection = [&](GVariant *variant)
  {
    if (variant != nullptr)
    {
      g_free(returned_result);
      returned_result = g_strdup(g_variant_get_string(g_variant_get_child_value(variant, 0), NULL));
    }

    got_result_return = true;
    if (got_signal_return && got_result_return)
      g_main_loop_quit(loop_);
  };
  
  auto timeout_bailout = [] (gpointer data) -> gboolean // bail out after 10 seconds
  {
    g_main_loop_quit(loop_);
    return FALSE;
  };
   
  guint bailout_source = g_timeout_add_seconds(10, timeout_bailout, this);

  EXPECT_EQ(proxy->IsConnected(), true); // fail if we are not connected
  proxy->Connect("TestSignal", signal_connection);
  proxy->Call("TestMethod", parameters, method_connection); 

 
  // next check we get 30 entries from this specific known callback
  g_main_loop_run(loop_);

  EXPECT_EQ(g_strcmp0(expected_return, returned_result), 0);
  EXPECT_EQ(g_strcmp0(expected_return, returned_signal), 0);

  g_free(returned_result);
  g_free(returned_signal);
  g_source_remove(bailout_source);
}


}
