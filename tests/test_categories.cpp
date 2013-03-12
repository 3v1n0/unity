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
static const unsigned int n_rows = 5;

static void WaitForSynchronize(Categories& model)
{
  Utils::WaitUntil([&model] { return model.count == n_rows; });
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

    unity::glib::String tmp(g_strdup_printf("Category%d", i));
    string value = tmp.Str();
    EXPECT_EQ(adaptor.name, value);
    EXPECT_EQ(adaptor.icon_hint, "gtk-apply");
    EXPECT_EQ(adaptor.index, i);
    EXPECT_EQ(adaptor.renderer_name, "grid");
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

}
