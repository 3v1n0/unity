#include <gtest/gtest.h>
#include <glib-object.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Hud.h>
#include <sigc++/connection.h>

using namespace std;

namespace
{

GMainLoop* loop_ = NULL;
unity::hud::Hud *hud;

class TestHud : public ::testing::Test
{
public:
  TestHud()
    : query_return_result(false)
    , connected_result(false)
  {
  }
  unity::hud::Hud::Queries queries;
  bool query_return_result;
  bool connected_result;
  int number_signals_found;
};

TEST_F(TestHud, TestConstruction)
{
  loop_ = g_main_loop_new(NULL, FALSE);
  hud = new unity::hud::Hud("com.canonical.Unity.Test", "/com/canonical/hud");
  
  // performs a check on the hud, if the hud is connected, report a sucess
  auto timeout_check = [] (gpointer data) -> gboolean
  {
    TestHud* self = static_cast<TestHud*>(data);
    if (hud->connected)
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
  

  // if the hud is not connected when this lambda runs, fail.
  auto timeout_bailout = [] (gpointer data) -> gboolean 
  {
    TestHud* self = static_cast<TestHud*>(data);
    // reached timeout, failed testing
    self->connected_result = false;
    g_main_loop_quit(loop_);
    return FALSE;
  };
  
  g_timeout_add(1000, timeout_check, this);
  g_timeout_add(10000, timeout_bailout, this);

  g_main_loop_run(loop_);
  
  EXPECT_EQ(connected_result, true);
}

TEST_F(TestHud, TestQueryReturn)
{
  query_return_result = false;
  
  // make sure we receive the queries
  auto query_connection = [this](unity::hud::Hud::Queries queries_) 
  { 
    query_return_result = true;
    g_main_loop_quit(loop_);
    queries = queries_;
  };

  auto timeout_bailout = [] (gpointer data) -> gboolean
  {
    TestHud* self = static_cast<TestHud*>(data);
    self->query_return_result = false;
    g_main_loop_quit(loop_);
    return FALSE;
  };
   
  sigc::connection connection = hud->queries_updated.connect(query_connection);
 
  guint source_id = g_timeout_add(10000, timeout_bailout, this);
 
  // next check we get 30 entries from this specific known callback
  hud->RequestQuery("Request30Queries");
  g_main_loop_run(loop_);
  EXPECT_EQ(query_return_result, true);
  EXPECT_NE(queries.size(), 0);
  g_source_remove(source_id);

  // finally close the connection - Nothing to check for here
  hud->CloseQuery();
  connection.disconnect();
}

TEST_F(TestHud, TestSigEmission)
{
  // checks that the signal emission from Hud is working correctly
  // the service is setup to emit the same signal every 1000ms
  // using the same query key as its StarQuery method
  // so calling StartQuery and listening we expect > 1 
  // signal emission
  number_signals_found = 0;

  // make sure we receive the queries
  auto query_connection = [this](unity::hud::Hud::Queries queries_) 
  { 
    number_signals_found += 1;
  };

  auto timeout_bailout = [] (gpointer data) -> gboolean
  {
    g_main_loop_quit(loop_);
    return FALSE;
  };
   
  sigc::connection connection = hud->queries_updated.connect(query_connection);
 
  hud->RequestQuery("Request30Queries");
  guint source_id = g_timeout_add(10000, timeout_bailout, this);
 
  g_main_loop_run(loop_);
  EXPECT_GT(number_signals_found, 1);
  g_source_remove(source_id);

  // finally close the connection - Nothing to check for here
  hud->CloseQuery();
  connection.disconnect();
  
}

}
