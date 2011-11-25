//#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <glib-object.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Model.h>

#include "test_utils.h"

using namespace std;
using namespace unity::dash;
using namespace testing;

namespace
{

static const string swarm_name = "com.canonical.test.model";
static const unsigned int n_rows = 100;

class TestAdaptor : public RowAdaptorBase
{
public:
  TestAdaptor(DeeModel* model, DeeModelIter* iter, DeeModelTag* tag)
    : RowAdaptorBase(model, iter, tag)
  {
  }

  unsigned int index()
  {
    return GetUIntAt(0);
  }

  string name()
  {
    return GetStringAt(1);
  }
};

static void WaitForSynchronize(Model<TestAdaptor>& model)
{
  ::Utils::WaitForModelSynchronize<TestAdaptor>(model, n_rows);
}

TEST(TestModel, TestConstruction)
{
  Model<TestAdaptor> model;
  model.swarm_name = swarm_name;
  ::Utils::WaitForModelSynchronize<TestAdaptor>(model, n_rows);
}

TEST(TestModel, TestSynchronization)
{
  Model<TestAdaptor> model;
  model.swarm_name = swarm_name;

  WaitForSynchronize(model);
  EXPECT_EQ(model.count, n_rows);
}

TEST(TestModel, TestRowsValid)
{
  Model<TestAdaptor> model;
  model.swarm_name = swarm_name;

  WaitForSynchronize(model);

  for (unsigned int i = 0; i < n_rows; i++)
  {
    TestAdaptor adaptor = model.RowAtIndex(i);

    EXPECT_EQ(adaptor.index(), i);
    unity::glib::String tmp(g_strdup_printf("Test%u", i));
    EXPECT_EQ(adaptor.name(), tmp.Str());
  }
}

// We're testing the model's ability to store and retrieve random pointers
TEST(TestModel, TestSetGetRenderer)
{
  Model<TestAdaptor> model;
  model.swarm_name = swarm_name;

  WaitForSynchronize(model);

  for (unsigned int i = 0; i < n_rows; i++)
  {
    TestAdaptor adaptor = model.RowAtIndex(i);

    char* value = g_strdup_printf("Renderer%d", i);
    adaptor.set_renderer<char*>(value);
  }

  for (unsigned int i = 0; i < n_rows; i++)
  {
    TestAdaptor adaptor = model.RowAtIndex(i);

    unity::glib::String value(adaptor.renderer<char*>());
    unity::glib::String renderer(g_strdup_printf("Renderer%d", i));

    EXPECT_EQ(value.Str(), renderer.Str());
  }
}

TEST(TestRowAdapterBase, TestGetStringNull)
{
  DeeModel* model = dee_sequence_model_new();
  dee_model_set_schema(model, "s", NULL);
  // Add on a null string.
  DeeModelIter* iter = dee_model_append(model, NULL);

  RowAdaptorBase row(model, iter);
  EXPECT_THAT(row.GetStringAt(0), Eq(""));
}

TEST(TestRowAdapterBase, TestGetStringEmpty)
{
  DeeModel* model = dee_sequence_model_new();
  dee_model_set_schema(model, "s", NULL);
  // Add on a null string.
  DeeModelIter* iter = dee_model_append(model, "");

  RowAdaptorBase row(model, iter);
  EXPECT_THAT(row.GetStringAt(0), Eq(""));
}

TEST(TestRowAdapterBase, TestGetStringValue)
{
  DeeModel* model = dee_sequence_model_new();
  dee_model_set_schema(model, "s", NULL);
  // Add on a null string.
  DeeModelIter* iter = dee_model_append(model, "Hello");

  RowAdaptorBase row(model, iter);
  EXPECT_THAT(row.GetStringAt(0), Eq("Hello"));
}

}
