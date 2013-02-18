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
  ::Utils::WaitForModelSynchronize<Category>(model, n_rows);
}

TEST(TestCategories, TestConstruction)
{
  Categories model;
  model.swarm_name = swarm_name;
}

TEST(TestCategories, TestSynchronization)
{
  Categories model;
  model.swarm_name = swarm_name;

  WaitForSynchronize(model);
  EXPECT_EQ(model.count, n_rows);
}

TEST(TestCategories, TestRowsValid)
{
  Categories model;
  model.swarm_name = swarm_name;

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
TEST(TestCategories, TestSetGetRenderer)
{
  Categories model;
  model.swarm_name = swarm_name;

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
TEST(TestCategories, TestOnRowChanged)
{
  Categories model;
  model.swarm_name = swarm_name_changing;
  WaitForSynchronize(model);

  bool changed = false;
  model.category_changed.connect([&changed] (Category const&) { changed = true;});
  Utils::WaitUntilMSec(changed,
                       2000,
                       []() { return g_strdup_printf("Did not detect row change from %s.", swarm_name_changing.c_str()); });
}


// We're testing the model's ability to store and retrieve random pointers
TEST(TestCategories, OnRowAdded)
{
  Categories model;
  model.swarm_name = swarm_name_changing;
  WaitForSynchronize(model);

  bool added = false;
  model.category_added.connect([&added] (Category const&) { added = true;});
  Utils::WaitUntilMSec(added,
                       2000,
                       []() { return g_strdup_printf("Did not detect row add %s.", swarm_name_changing.c_str()); });
}

// We're testing the model's ability to store and retrieve random pointers
TEST(TestCategories, OnRowRemoved)
{
  Categories model;
  model.swarm_name = swarm_name_changing;
  WaitForSynchronize(model);

  bool removed = false;
  model.category_removed.connect([&removed] (Category const&) { removed = true;});
  Utils::WaitUntilMSec(removed,
                       2000,
                       []() { return g_strdup_printf("Did not detect row remove %s.", swarm_name_changing.c_str()); });
}

}
