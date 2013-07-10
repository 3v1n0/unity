#include <gtest/gtest.h>
#include <glib-object.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Categories.h>

#include "test_utils.h"

using namespace std;
using namespace unity::dash;

namespace
{

static const string swarm_name = "com.canonical.test.categoriesmodel";
static const string swarm_name_changing = "com.canonical.test.categoriesmodel_changing";
static const unsigned int n_rows = 5;

static void WaitForSynchronize(Categories& model)
{
  Utils::WaitUntil([&model] { return model.count == n_rows; });
}

struct TestCategories : testing::Test
{
  TestCategories()
  {
    model.swarm_name = swarm_name;
  }

  Categories model;
};

TEST_F(TestCategories, TestConstruction)
{
  EXPECT_EQ(model.swarm_name(), swarm_name);
}

TEST_F(TestCategories, TestSignalProxyAdded)
{
  Category cat(nullptr, nullptr, nullptr);

  bool added = false;
  ASSERT_EQ(model.row_added.size(), 1);
  model.category_added.connect([&added] (Category const&) { added = true; });
  model.row_added.emit(cat);
  EXPECT_TRUE(added);
}

TEST_F(TestCategories, TestSignalProxyChanged)
{
  Category cat(nullptr, nullptr, nullptr);

  bool changed = false;
  ASSERT_EQ(model.row_changed.size(), 1);
  model.category_changed.connect([&changed] (Category const&) { changed = true; });
  model.row_changed.emit(cat);
  EXPECT_TRUE(changed);
}

TEST_F(TestCategories, TestSignalProxyRemoved)
{
  Category cat(nullptr, nullptr, nullptr);

  bool removed = false;
  ASSERT_EQ(model.row_removed.size(), 1);
  model.category_removed.connect([&removed] (Category const&) { removed = true; });
  model.row_removed.emit(cat);
  EXPECT_TRUE(removed);
}

TEST_F(TestCategories, TestSynchronization)
{
  WaitForSynchronize(model);
  EXPECT_EQ(model.count, n_rows);
}

TEST_F(TestCategories, TestRowsValid)
{
  WaitForSynchronize(model);

  for (unsigned int i = 0; i < n_rows; i++)
  {
    Category adaptor = model.RowAtIndex(i);

    unity::glib::String tmp(g_strdup_printf("cat%d", i));
    string id = tmp.Str();

    unity::glib::String tmp2(g_strdup_printf("Category %d", i));
    string name = tmp2.Str();

    EXPECT_EQ(adaptor.id(), id);
    EXPECT_EQ(adaptor.name(), name);
    EXPECT_EQ(adaptor.icon_hint(), "gtk-apply");
    EXPECT_EQ(adaptor.renderer_name(), "grid");
    EXPECT_EQ(adaptor.index(), i);
  }
}

// We're testing the model's ability to store and retrieve random pointers
TEST_F(TestCategories, TestSetGetRenderer)
{
  WaitForSynchronize(model);

  for (unsigned int i = 0; i < n_rows; i++)
  {
    Category adaptor = model.RowAtIndex(i);

    char* value = g_strdup_printf("Renderer%d", i);
    adaptor.set_renderer<char*>(value);
  }

  for (unsigned int i = 0; i < n_rows; i++)
  {
    Category adaptor = model.RowAtIndex(i);

    unity::glib::String value(adaptor.renderer<char*>());
    unity::glib::String renderer(g_strdup_printf("Renderer%d", i));

    EXPECT_EQ(value.Str(), renderer.Str());
  }
}

// We're testing the model's ability to store and retrieve random pointers
TEST_F(TestCategories, TestOnRowChanged)
{
  model.swarm_name = swarm_name_changing;
  WaitForSynchronize(model);

  unsigned changed = 0;
  model.category_changed.connect([&changed] (Category const&) { ++changed; });
  Utils::WaitUntil([&changed] { return changed == 1; }, true, 2, "Did not detect row change from "+swarm_name_changing+".");
}


// We're testing the model's ability to store and retrieve random pointers
TEST_F(TestCategories, TestOnRowAdded)
{
  model.swarm_name = swarm_name_changing;
  WaitForSynchronize(model);

  unsigned added = 0;
  model.category_added.connect([&added] (Category const&) { ++added;});
  Utils::WaitUntil([&added] { return added == 1; }, true, 2, "Did not detect row add "+swarm_name_changing+".");
}

// We're testing the model's ability to store and retrieve random pointers
TEST_F(TestCategories, TestOnRowRemoved)
{
  model.swarm_name = swarm_name_changing;
  WaitForSynchronize(model);

  unsigned removed = 0;
  model.category_removed.connect([&removed] (Category const&) { ++removed; });
  Utils::WaitUntil([&removed] { return removed == 1; }, true, 2, "Did not detect row removal "+swarm_name_changing+".");
}

TEST_F(TestCategories, TestCategoryCopy)
{
  model.swarm_name = swarm_name_changing;
  WaitForSynchronize(model);
  
  Category category = model.RowAtIndex(0);
  Category category_2(category);

  EXPECT_EQ(category.id(), category_2.id());
  EXPECT_EQ(category.name(), category_2.name());
  EXPECT_EQ(category.icon_hint(), category_2.icon_hint());
  EXPECT_EQ(category.renderer_name(), category_2.renderer_name());
  EXPECT_EQ(category.index(), category_2.index());
}

TEST_F(TestCategories, TestCategoryEqual)
{
  model.swarm_name = swarm_name_changing;
  WaitForSynchronize(model);

  Category category = model.RowAtIndex(0);
  Category category_2(NULL, NULL, NULL);
  category_2 = category;

  EXPECT_EQ(category.id(), category_2.id());
  EXPECT_EQ(category.name(), category_2.name());
  EXPECT_EQ(category.icon_hint(), category_2.icon_hint());
  EXPECT_EQ(category.renderer_name(), category_2.renderer_name());
  EXPECT_EQ(category.index(), category_2.index());
}

}
