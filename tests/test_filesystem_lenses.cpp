#include <gtest/gtest.h>
#include <glib-object.h>
#include <UnityCore/FilesystemLenses.h>

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

static void WaitForLensesToLoad(FilesystemLenses& lenses)
{
  bool result = false;

  auto lenses_loaded_cb = [&result]() {result = true;};
  lenses.lenses_loaded.connect(sigc::slot<void>(lenses_loaded_cb));
  
  WaitForResult (result);
  EXPECT_TRUE(result);
}

TEST(TestFilesystemLenses, TestConstruction)
{
  FilesystemLenses lenses0;
  FilesystemLenses lenses1(TESTDATADIR"/lenses");
}

TEST(TestFilesystemLenses, TestFileLoading)
{
  FilesystemLenses lenses(TESTDATADIR"/lenses");
  WaitForLensesToLoad(lenses);
}

TEST(TestFilesystemLenses, TestLensCreation)
{
  FilesystemLenses lenses(TESTDATADIR"/lenses");
  int  n_lenses = 0;

  auto lens_added_cb = [&n_lenses](Lens::Ptr& p) { n_lenses++; };
  lenses.lens_added.connect(sigc::slot<void, Lens::Ptr&>(lens_added_cb));

  WaitForLensesToLoad(lenses);
  EXPECT_EQ(n_lenses, 3);
  EXPECT_EQ(lenses.count, 3);
}

TEST(TestFilesystemLenses, TestLensContent)
{
  FilesystemLenses lenses(TESTDATADIR"/lenses");
  WaitForLensesToLoad(lenses);

  // We need to assign the std::string properties before using them
  std::string s;
  
  // Test that the lenses have loaded correctly
  Lens::Ptr lens = lenses.GetLens("applications.lens");
  EXPECT_TRUE(lens);
  EXPECT_EQ(s = lens->dbus_name, "com.canonical.tests.Lens.Applications");
  EXPECT_EQ(s = lens->dbus_path, "/com/canonical/tests/lens/applications");
  EXPECT_EQ(s = lens->name, "Applications");
  EXPECT_EQ(s = lens->icon, "/usr/share/unity-lens-applications/applications.png");
  EXPECT_EQ(s = lens->description, "Search for applications");
  EXPECT_EQ(s = lens->search_hint, "Search Applications");
  EXPECT_EQ(lens->visible, true);
  EXPECT_EQ(s = lens->shortcut, "a");

  lens = lenses.GetLens("files.lens");
  EXPECT_TRUE(lens);
  EXPECT_EQ(s = lens->dbus_name, "com.canonical.tests.Lens.Files");
  EXPECT_EQ(s = lens->dbus_path, "/com/canonical/tests/lens/files");
  EXPECT_EQ(s = lens->name, "Files");
  EXPECT_EQ(s = lens->icon, "/usr/share/unity-lens-files/files.png");
  EXPECT_EQ(s = lens->description, "Search for Files & Folders");
  EXPECT_EQ(s = lens->search_hint, "Search Files & Folders");
  EXPECT_EQ(lens->visible, true);
  EXPECT_EQ(s = lens->shortcut, "f");

  lens = lenses.GetLens("social.lens");
  EXPECT_TRUE(lens);
  EXPECT_EQ(s = lens->dbus_name, "com.canonical.tests.Lens.Social");
  EXPECT_EQ(s = lens->dbus_path, "/com/canonical/tests/lens/social");
  EXPECT_EQ(s = lens->name, "Social");
  EXPECT_EQ(s = lens->icon, "/usr/share/unity-lens-social/social.png");
  EXPECT_EQ(s = lens->description, "");
  EXPECT_EQ(s = lens->search_hint, "");
  EXPECT_EQ(lens->visible, false);
  EXPECT_EQ(s = lens->shortcut, "");
}

}
