#include <gtest/gtest.h>
#include <glib-object.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Results.h>
#include <UnityCore/ResultIterator.h>

#include "test_utils.h"

using namespace std;
using namespace unity::dash;
using namespace unity;

namespace
{

static const string swarm_name = "com.canonical.test.resultsmodel";
static const unsigned int n_rows = 200;

static void WaitForSynchronize(Results& model)
{
  ::Utils::WaitForModelSynchronize<Result>(model, n_rows);
}

TEST(TestResults, TestConstruction)
{
  Results model;
  model.swarm_name = swarm_name;
}

TEST(TestResults, TestSynchronization)
{
  Results model;
  model.swarm_name = swarm_name;

  WaitForSynchronize(model);
  EXPECT_EQ(model.count, n_rows);
}

TEST(TestResults, TestFilterValid)
{
  Results model;
  DeeFilter filter;

  model.swarm_name = swarm_name;
  WaitForSynchronize(model);

  dee_filter_new_for_any_column(2, g_variant_new_uint32(1), &filter);
  glib::Object<DeeModel> filter_model(dee_filter_model_new(model.model(), &filter));
  
  unsigned int i = 0;
  for (ResultIterator iter(filter_model);  !iter.IsLast(); ++iter)
  {
    EXPECT_EQ((*iter).category_index(), 1);
    i++;
  }

  EXPECT_EQ(i, 50);
}

TEST(TestResults, TestRowsValid)
{
  Results model;
  model.swarm_name = swarm_name;

  WaitForSynchronize(model);
 
  ResultIterator iter(model.model);
  unsigned int i = 0;
  for (Result result : model)
  {
    //Result adaptor = *iter;
    unity::glib::String tmp(g_strdup_printf("Result%d", i));
    string value = tmp.Str();
    EXPECT_EQ(result.uri(), value);
    EXPECT_EQ(result.icon_hint(), value);
    EXPECT_EQ(result.category_index(), (i / 50));
    EXPECT_EQ(result.mimetype(), value);
    EXPECT_EQ(result.name(), value);
    EXPECT_EQ(result.comment(), value);
    EXPECT_EQ(result.dnd_uri(), value);
    i++;
  }

  //test reading a subset
  i = 20;
  for (auto iter = model.begin() + i; iter != model.end(); ++iter)
  {
    Result result = (*iter);
    unity::glib::String tmp(g_strdup_printf("Result%d", i));
    string value = tmp.Str();
    EXPECT_EQ(result.uri(), value);
    i++;
  }

  // test post incrementor
  i = 20;
  for (auto iter = model.begin() + i; iter != model.end(); iter++)
  {
    Result result = (*iter);
    unity::glib::String tmp(g_strdup_printf("Result%d", i));
    string value = tmp.Str();
    EXPECT_EQ(result.uri(), value);
    i++;
  }

  // test equality
  EXPECT_TRUE(model.begin() == model.begin());
  EXPECT_TRUE(model.begin() != model.end());

  EXPECT_FALSE(model.begin().IsLast());
  EXPECT_FALSE(model.end().IsFirst());
  EXPECT_TRUE(model.begin().IsFirst());
  EXPECT_TRUE(model.end().IsLast());

  EXPECT_TRUE(model.begin() < model.end());
  EXPECT_FALSE(model.end() < model.begin());
  EXPECT_TRUE(model.end() > model.begin());
  EXPECT_FALSE(model.begin() > model.end());
  EXPECT_TRUE(model.begin() <= model.end());
  EXPECT_FALSE(model.end() <= model.begin());
  EXPECT_TRUE(model.end() >= model.begin());
  EXPECT_FALSE(model.begin() >= model.end());


  EXPECT_TRUE(model.begin() + 20 > model.begin());
  EXPECT_TRUE(model.begin() + 20 > model.begin() + 19);
  EXPECT_TRUE(model.end() - 20 < model.end());
  EXPECT_TRUE(model.end() - 20 < model.end() - 19);

}

// We're testing the model's ability to store and retrieve random pointers
TEST(TestResults, TestSetGetRenderer)
{
  Results model;
  model.swarm_name = swarm_name;

  WaitForSynchronize(model);

  for (unsigned int i = 0; i < n_rows; i++)
  {
    Result adaptor = model.RowAtIndex(i);

    char* value = g_strdup_printf("Renderer%d", i);
    adaptor.set_renderer<char*>(value);
  }

  for (unsigned int i = 0; i < n_rows; i++)
  {
    Result adaptor = model.RowAtIndex(i);

    unity::glib::String value(adaptor.renderer<char*>());
    unity::glib::String renderer(g_strdup_printf("Renderer%d", i));

    EXPECT_EQ(value.Str(), renderer.Str());
  }
}

}
