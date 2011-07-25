#include <gtest/gtest.h>
#include <glib-object.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Lens.h>

#include "test_utils.h"

using namespace std;
using namespace unity::dash;

namespace
{

static const string lens_name = "com.canonical.Unity.Test";
static const string lens_path = "/com/canonical/unity/testlens";

class TestLens : public ::testing::Test
{
public:
  TestLens()
    : lens_(new Lens("testlens", lens_name, lens_path,
                          "Test Lens", "gtk-apply"))
  {
    WaitForConnected();
  }

  void WaitForConnected()
  {
    bool timeout_reached = false;

    auto timeout_cb = [](gpointer data) -> gboolean
    {
      *(bool*)data = true;
      return FALSE;
    };
    guint32 timeout_id = g_timeout_add_seconds(10, timeout_cb, &timeout_reached);

    while (!lens_->connected && !timeout_reached)
    {
      g_main_context_iteration(g_main_context_get_thread_default(), TRUE);
    }
    if (lens_->connected)
      g_source_remove(timeout_id);

    g_debug("BOO: %s", lens_->connected ? "TRUE" : "FALSE");
    EXPECT_FALSE(timeout_reached);
  }


  Lens::Ptr lens_;
};

TEST_F(TestLens, TestConnection)
{
  ;
}

TEST_F(TestLens, TestSynchronization)
{
  EXPECT_EQ(lens_->search_hint, "Search Test Lens");
  EXPECT_TRUE(lens_->visible);
  EXPECT_TRUE(lens_->search_in_global);
}

}
