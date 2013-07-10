#include <gtest/gtest.h>
#include <glib-object.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Results.h>

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
  Utils::WaitUntil([&model] { return model.count == n_rows; });
}

struct TestResults : testing::Test
{
  TestResults()
  {
    model.swarm_name = swarm_name;
  }

  Results model;
};

TEST_F(TestResults, TestConstruction)
{
  EXPECT_EQ(model.swarm_name(), swarm_name);
}

TEST_F(TestResults, TestSynchronization)
{
  WaitForSynchronize(model);
  EXPECT_EQ(model.count, n_rows);
}

TEST_F(TestResults, TestFilterValid)
{
  DeeFilter filter;

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

TEST_F(TestResults, TestRowsValid)
{
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
    EXPECT_EQ(result.result_type(), 0);
    EXPECT_EQ(result.mimetype(), value);
    EXPECT_EQ(result.name(), value);
    EXPECT_EQ(result.comment(), value);
    EXPECT_EQ(result.dnd_uri(), value);

    glib::HintsMap hints = result.hints();
    auto iter = hints.find("key");
    EXPECT_TRUE(iter != hints.end());
    if (iter != hints.end())
    {
      std::string value = glib::gchar_to_string(g_variant_get_string(iter->second, NULL));
      EXPECT_EQ(value, "value");
    }

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
TEST_F(TestResults, TestSetGetRenderer)
{
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

// We're testing the model's ability to store and retrieve random pointers
TEST_F(TestResults, TestResultEqual)
{
  WaitForSynchronize(model);

  Result result_1(*model.begin());
  Result result_2(NULL, NULL, NULL);
  result_2 = result_1;

  EXPECT_EQ(result_2.uri(), result_1.uri());
  EXPECT_EQ(result_2.icon_hint(), result_1.icon_hint());
  EXPECT_EQ(result_2.category_index(), result_1.category_index());
  EXPECT_EQ(result_2.result_type(), result_1.result_type());
  EXPECT_EQ(result_2.mimetype(), result_1.mimetype());
  EXPECT_EQ(result_2.name(), result_1.name());
  EXPECT_EQ(result_2.comment(), result_1.comment());
  EXPECT_EQ(result_2.dnd_uri(), result_1.dnd_uri());
}

// We're testing the model's ability to store and retrieve random pointers
TEST_F(TestResults, LocalResult_Construct)
{
  WaitForSynchronize(model);

  ResultIterator iter(model.model);
  for (Result const& result : model)
  {
    LocalResult local_result_1(result);
    LocalResult local_result_2(local_result_1);

    EXPECT_EQ(local_result_1.uri, result.uri());
    EXPECT_EQ(local_result_1.icon_hint, result.icon_hint());
    EXPECT_EQ(local_result_1.category_index, result.category_index());
    EXPECT_EQ(local_result_1.result_type, result.result_type());
    EXPECT_EQ(local_result_1.mimetype, result.mimetype());
    EXPECT_EQ(local_result_1.name, result.name());
    EXPECT_EQ(local_result_1.comment, result.comment());
    EXPECT_EQ(local_result_1.dnd_uri, result.dnd_uri());

    EXPECT_EQ(local_result_2.uri, result.uri());
    EXPECT_EQ(local_result_2.icon_hint, result.icon_hint());
    EXPECT_EQ(local_result_2.category_index, result.category_index());
    EXPECT_EQ(local_result_2.result_type, result.result_type());
    EXPECT_EQ(local_result_2.mimetype, result.mimetype());
    EXPECT_EQ(local_result_2.name, result.name());
    EXPECT_EQ(local_result_2.comment, result.comment());
    EXPECT_EQ(local_result_2.dnd_uri, result.dnd_uri());
  }
}


// We're testing the model's ability to store and retrieve random pointers
TEST_F(TestResults, LocalResult_OperatorEqual)
{
  WaitForSynchronize(model);

  ResultIterator iter(model.model);
  for (Result const& result : model)
  {
    LocalResult local_result_1(result);
    LocalResult local_result_2(local_result_1);

    EXPECT_TRUE(local_result_1 == local_result_2);
    EXPECT_FALSE(local_result_1 != local_result_2);
  }
}


// We're testing the model's ability to store and retrieve random pointers
TEST_F(TestResults, LocalResult_FromToVariant)
{
  LocalResult local_result_1;
  local_result_1.uri = "uri";
  local_result_1.icon_hint = "icon_hint";
  local_result_1.category_index = 1;
  local_result_1.result_type = 2;
  local_result_1.mimetype = "mimetype";
  local_result_1.name = "name";
  local_result_1.comment = "comment";
  local_result_1.dnd_uri = "dnd_uri";
  
  local_result_1.hints["key1"] = g_variant_new_string("value1");
  local_result_1.hints["key2"] = g_variant_new_string("value2");

  glib::Variant variant_value = local_result_1.Variant();
  LocalResult local_result_2 = LocalResult::FromVariant(variant_value);

  EXPECT_EQ(local_result_2.uri, "uri");
  EXPECT_EQ(local_result_2.icon_hint, "icon_hint");
  EXPECT_EQ(local_result_2.category_index, 1);
  EXPECT_EQ(local_result_2.result_type, 2);
  EXPECT_EQ(local_result_2.mimetype, "mimetype");
  EXPECT_EQ(local_result_2.name, "name");
  EXPECT_EQ(local_result_2.comment, "comment");
  EXPECT_EQ(local_result_2.dnd_uri, "dnd_uri");

  auto iter = local_result_2.hints.find("key1");
  EXPECT_TRUE(iter != local_result_2.hints.end());
  if (iter != local_result_2.hints.end())
  {
    std::string value = glib::gchar_to_string(g_variant_get_string(iter->second, NULL));
    EXPECT_EQ(value, "value1");
  }
  iter = local_result_2.hints.find("key2");
  EXPECT_TRUE(iter != local_result_2.hints.end());
  if (iter != local_result_2.hints.end())
  {
    std::string value = glib::gchar_to_string(g_variant_get_string(iter->second, NULL));
    EXPECT_EQ(value, "value2");
  }
}


// We're testing the model's ability to store and retrieve random pointers
TEST_F(TestResults, LocalResult_Variants)
{
  LocalResult local_result;
  EXPECT_EQ(local_result.Variants().size(), 9);
}

}
