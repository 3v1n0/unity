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
                          "Test Lens", "gtk-apply")),
      n_categories_(0)
  {
    WaitForConnected();

    Categories::Ptr categories = lens_->categories;
    categories->category_added.connect(sigc::mem_fun(this, &TestLens::OnCategoryAdded));
  }

  void OnCategoryAdded(Category const& category)
  {
    n_categories_++;
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

    EXPECT_FALSE(timeout_reached);
  }

  void WaitForCategories()
  {
    bool timeout_reached = false;

    auto timeout_cb = [](gpointer data) -> gboolean
    {
      *(bool*)data = true;
      return FALSE;
    };
    guint32 timeout_id = g_timeout_add_seconds(10, timeout_cb, &timeout_reached);

    Categories::Ptr categories = lens_->categories;
    while (categories->count != (unsigned int)3 && !timeout_reached)
    {
      g_main_context_iteration(g_main_context_get_thread_default(), TRUE);
    }
    if (categories->count == (unsigned int)3)
      g_source_remove(timeout_id);

    EXPECT_FALSE(timeout_reached);
  }

  void WaitForSearchResults()
  {
    bool timeout_reached = false;

    auto timeout_cb = [](gpointer data) -> gboolean
    {
      *(bool*)data = true;
      return FALSE;
    };
    guint32 timeout_id = g_timeout_add_seconds(10, timeout_cb, &timeout_reached);

    Results::Ptr results = lens_->results;
    while (results->count != (unsigned int)5 && !timeout_reached)
    {
      g_main_context_iteration(g_main_context_get_thread_default(), TRUE);
    }
    if (results->count == (unsigned int)5)
      g_source_remove(timeout_id);

    EXPECT_FALSE(timeout_reached);
  }

  void WaitForGlobalSearchResults()
  {
    bool timeout_reached = false;

    auto timeout_cb = [](gpointer data) -> gboolean
    {
      *(bool*)data = true;
      return FALSE;
    };
    guint32 timeout_id = g_timeout_add_seconds(10, timeout_cb, &timeout_reached);

    Results::Ptr results = lens_->global_results;
    while (results->count != (unsigned int)10 && !timeout_reached)
    {
      g_main_context_iteration(g_main_context_get_thread_default(), TRUE);
    }
    if (results->count == (unsigned int)10)
      g_source_remove(timeout_id);

    EXPECT_FALSE(timeout_reached);
  }


  Lens::Ptr lens_;
  unsigned int n_categories_;
};

TEST_F(TestLens, TestConnection)
{
  EXPECT_EQ(lens_->id, "testlens");
  EXPECT_EQ(lens_->dbus_name, lens_name);
  EXPECT_EQ(lens_->dbus_path, lens_path);
  EXPECT_EQ(lens_->name, "Test Lens");
  EXPECT_EQ(lens_->icon_hint, "gtk-apply");
}

TEST_F(TestLens, TestSynchronization)
{
  EXPECT_EQ(lens_->search_hint, "Search Test Lens");
  EXPECT_TRUE(lens_->visible);
  EXPECT_TRUE(lens_->search_in_global);
}

TEST_F(TestLens, TestCategories)
{
  Categories::Ptr categories = lens_->categories;
  WaitForCategories();

  EXPECT_EQ(categories->count, (unsigned int)3);
  EXPECT_EQ(n_categories_, 3);
  
  Category category = categories->RowAtIndex(0);
  EXPECT_EQ(category.name, "Category1");
  EXPECT_EQ(category.icon_hint, "gtk-apply");
  EXPECT_EQ(category.index, (unsigned int)0);
  EXPECT_EQ(category.renderer_name, "grid");

  category = categories->RowAtIndex(1);
  EXPECT_EQ(category.name, "Category2");
  EXPECT_EQ(category.icon_hint, "gtk-cancel");
  EXPECT_EQ(category.index, (unsigned int)1);
  EXPECT_EQ(category.renderer_name, "grid");

  category = categories->RowAtIndex(2);
  EXPECT_EQ(category.name, "Category3");
  EXPECT_EQ(category.icon_hint, "gtk-close");
  EXPECT_EQ(category.index, (unsigned int)2);
  EXPECT_EQ(category.renderer_name, "grid");
}

TEST_F(TestLens, TestSearch)
{
  Results::Ptr results = lens_->results;

  lens_->Search("Test Search String");
  WaitForSearchResults();

  for (unsigned int i = 0; i < 5; i++)
  {
    unity::glib::String name(g_strdup_printf("Test Search String%d", i));

    Result result = results->RowAtIndex(i);
    EXPECT_EQ(result.uri, "file:///test");
    EXPECT_EQ(result.icon_hint, "gtk-apply");
    EXPECT_EQ(result.category_index, i);
    EXPECT_EQ(result.mimetype, "text/html");
    EXPECT_EQ(result.name, name.Str());
    EXPECT_EQ(result.comment, "kamstrup likes ponies");
    EXPECT_EQ(result.dnd_uri, "file:///test");
  }
}

TEST_F(TestLens, TestGlobalSearch)
{
  Results::Ptr results = lens_->global_results;

  lens_->GlobalSearch("Test Global Search String");
  WaitForGlobalSearchResults();

  for (unsigned int i = 0; i < 10; i++)
  {
    unity::glib::String name(g_strdup_printf("Test Global Search String%d", i));

    Result result = results->RowAtIndex(i);
    EXPECT_EQ(result.uri, "file:///test");
    EXPECT_EQ(result.icon_hint, "gtk-apply");
    EXPECT_EQ(result.category_index, i);
    EXPECT_EQ(result.mimetype, "text/html");
    EXPECT_EQ(result.name, name.Str());
    EXPECT_EQ(result.comment, "kamstrup likes ponies");
    EXPECT_EQ(result.dnd_uri, "file:///test");
  }
}

}
