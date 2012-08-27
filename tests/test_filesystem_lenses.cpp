#include <gtest/gtest.h>
#include <glib-object.h>
#include <UnityCore/FilesystemLenses.h>

using namespace std;
using namespace unity::dash;

namespace
{

void WaitForResult(bool& result)
{
  bool timeout_reached;

  auto timeout_cb = [](gpointer data) -> gboolean
  {
    *(bool*)data = true;
    return FALSE;
  };

  guint32 timeout_id = g_timeout_add(10000, timeout_cb, &timeout_reached);

  while (!result && !timeout_reached)
  {
    g_main_context_iteration(g_main_context_get_thread_default(), TRUE);
  }
  if (result)
    g_source_remove(timeout_id);
}

void WaitForLensesToLoad(FilesystemLenses& lenses)
{
  bool result = false;
  lenses.lenses_loaded.connect([&result] {result = true;});

  WaitForResult(result);
  EXPECT_TRUE(result);
}

TEST(TestFilesystemLenses, TestConstruction)
{
  FilesystemLenses lenses0;
  LensDirectoryReader::Ptr test_reader(new LensDirectoryReader(TESTDATADIR"/lenses"));
  FilesystemLenses lenses1(test_reader);
}

TEST(TestFilesystemLenses, TestFileLoading)
{
  LensDirectoryReader::Ptr test_reader(new LensDirectoryReader(TESTDATADIR"/lenses"));
  FilesystemLenses lenses(test_reader);
  WaitForLensesToLoad(lenses);
}

TEST(TestFilesystemLenses, TestLensesAdded)
{
  LensDirectoryReader::Ptr test_reader(new LensDirectoryReader(TESTDATADIR"/lenses"));
  FilesystemLenses lenses(test_reader);
  unsigned int n_lenses = 0;
  lenses.lens_added.connect([&n_lenses](Lens::Ptr & p) { ++n_lenses; });

  WaitForLensesToLoad(lenses);
  EXPECT_EQ(n_lenses, (unsigned int)3);
  EXPECT_EQ(lenses.count, (unsigned int)3);
}

TEST(TestFilesystemLenses, TestLensContent)
{
  LensDirectoryReader::Ptr test_reader(new LensDirectoryReader(TESTDATADIR"/lenses"));
  FilesystemLenses lenses(test_reader);
  WaitForLensesToLoad(lenses);

  // Test that the lenses have loaded correctly
  Lens::Ptr lens = lenses.GetLens("applications.lens");
  EXPECT_TRUE(lens.get());
  EXPECT_EQ(lens->dbus_name, "com.canonical.tests.Lens.Applications");
  EXPECT_EQ(lens->dbus_path, "/com/canonical/tests/lens/applications");
  EXPECT_EQ(lens->name, "Applications");
  EXPECT_EQ(lens->icon_hint, "/usr/share/unity-lens-applications/applications.png");
  EXPECT_EQ(lens->description, "Search for applications");
  EXPECT_EQ(lens->search_hint, "Search Applications");
  EXPECT_EQ(lens->visible, true);
  EXPECT_EQ(lens->shortcut, "a");

  lens = lenses.GetLens("files.lens");
  EXPECT_TRUE(lens.get());
  EXPECT_EQ(lens->dbus_name, "com.canonical.tests.Lens.Files");
  EXPECT_EQ(lens->dbus_path, "/com/canonical/tests/lens/files");
  EXPECT_EQ(lens->name, "Files");
  EXPECT_EQ(lens->icon_hint, "/usr/share/unity-lens-files/files.png");
  EXPECT_EQ(lens->description, "Search for Files & Folders");
  EXPECT_EQ(lens->search_hint, "Search Files & Folders");
  EXPECT_EQ(lens->visible, false);
  EXPECT_EQ(lens->shortcut, "f");

  lens = lenses.GetLens("social.lens");
  EXPECT_TRUE(lens.get());
  EXPECT_EQ(lens->dbus_name, "com.canonical.tests.Lens.Social");
  EXPECT_EQ(lens->dbus_path, "/com/canonical/tests/lens/social");
  EXPECT_EQ(lens->name, "Social");
  EXPECT_EQ(lens->icon_hint, "/usr/share/unity-lens-social/social.png");
  EXPECT_EQ(lens->description, "");
  EXPECT_EQ(lens->search_hint, "");
  EXPECT_EQ(lens->visible, true);
  EXPECT_EQ(lens->shortcut, "");
}

}
