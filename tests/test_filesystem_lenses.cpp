#include <UnityCore/UnityCore.h>

#include <gtest/gtest.h>

using namespace std;
using namespace unity::dash;

namespace {

static void WaitForResult(bool& result)
{
  bool timeout_reached;

  auto timeout_cb = [](gpointer data) -> gboolean
  {
    *(bool*)data = true;
    return FALSE;
  };

  guint32 timeout_id = g_timeout_add_seconds(10, timeout_cb, &timeout_reached);

  while (!result && !timeout_reached)
  {
    g_main_context_iteration(g_main_context_get_thread_default(), TRUE);
  }
  if (result)
    g_source_remove(timeout_id);
}

TEST(TestFilesystemLenses, TestConstruction)
{
  FilesystemLenses lenses(TESTDATADIR"/lenses");
}

TEST(TestFilesystemLenses, TestFileLoading)
{
  FilesystemLenses lenses(TESTDATADIR"/lenses");
  bool result = false;

  auto lenses_loaded_cb = [&result]() {result = true;};
  lenses.lenses_loaded.connect(sigc::slot<void>(lenses_loaded_cb));

  WaitForResult (result);
  EXPECT_TRUE(result);
}

TEST(TestFilesystemLenses, TestLensCreation)
{
  FilesystemLenses lenses(TESTDATADIR"/lenses");
  bool result = false;
  int  n_lenses = 0;

  auto lenses_loaded_cb = [&result]() { result = true; };
  lenses.lenses_loaded.connect(sigc::slot<void>(lenses_loaded_cb));

  auto lens_added_cb = [&n_lenses](Lens::Ptr& p) { n_lenses++; };
  lenses.lens_added.connect(sigc::slot<void, Lens::Ptr&>(lens_added_cb));

  WaitForResult (result);
  EXPECT_TRUE(result);
  EXPECT_EQ(n_lenses, 3);
  EXPECT_EQ(lenses.Count(), 3);
}

}
