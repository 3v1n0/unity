#include <gtest/gtest.h>
#include <glib-object.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Hud.h>

using namespace std;

namespace
{

GMainLoop* loop_ = NULL;
unity::hud::Hud *hud;

class TestHud : public ::testing::Test
{
public:
  TestHud()
    : suggestion_return_result(false)
    , connected_result(false)
  {
  }
  unity::hud::Hud::Suggestions suggestions;
  bool suggestion_return_result;
  bool connected_result;
};

static gboolean
timeout_cb(TestHud* hud)
{
  g_critical("got timeout");
  hud->suggestion_return_result = false;
  g_main_loop_quit(loop_);
  return FALSE;
}
 
TEST_F(TestHud, TestConstruction)
{
  loop_ = g_main_loop_new(NULL, FALSE);
  hud = new unity::hud::Hud("com.canonical.Unity.Test", "/com/canonical/hud");
  
  // check every one second to see if the hud is connected
  g_timeout_add_seconds(1, [] (gpointer data) -> gboolean
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
  }, this);
  
  // if the hud is not connected in ten seconds, fail.
  g_timeout_add_seconds(10, [] (gpointer data) -> gboolean 
  {
    TestHud* self = static_cast<TestHud*>(data);
    // reached timeout, failed testing
    self->connected_result = false;
    g_main_loop_quit(loop_);
    return FALSE;
  }, this);
  
  g_main_loop_run(loop_);
  
  EXPECT_EQ(connected_result, true);
}

TEST_F(TestHud, TestSuggestionReturn)
{
  suggestion_return_result = false;
  
  // make sure we receive the suggestions
  auto suggestion_connection = [this](unity::hud::Hud::Suggestions suggestions_) 
  { 
    suggestion_return_result = true;
    g_main_loop_quit(loop_);
    suggestions = suggestions_;
  };
  
   
  hud->suggestion_search_finished.connect(suggestion_connection);
 
  g_timeout_add_seconds(10, (GSourceFunc)timeout_cb, (void*)(this));
 
  hud->GetSuggestions("Request30Suggestions");
  
  g_main_loop_run(loop_);
  EXPECT_EQ(suggestion_return_result, true);
  EXPECT_EQ(suggestions.size(), 30);
  
}

}
