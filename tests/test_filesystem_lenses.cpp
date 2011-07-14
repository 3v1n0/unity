#include <UnityCore/UnityCore.h>

#include <gtest/gtest.h>

using namespace std;
using namespace unity::dash;

namespace {

TEST(TestFilesystemLenses, TestConstruction)
{
  FilesystemLenses lenses1(TESTDATADIR"/lenses");

  while (1)
    g_main_context_iteration(g_main_context_get_thread_default(), TRUE);
}

}
