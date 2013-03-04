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

struct TestModel : public ::testing::Test
{
  void SetUp()
  {
    model.swarm_name = swarm_name;
    Utils::WaitUntil([this] { return model.count == n_rows; }, true, 2);
    ASSERT_EQ(model.count, n_rows);
  }

  Model<TestAdaptor> model;
};

TEST_F(TestModel, TestConstruction)
{
  EXPECT_EQ(model.swarm_name(), swarm_name);
}

TEST_F(TestModel, TestRowsValid)
{
  for (unsigned int i = 0; i < n_rows; i++)
  {
    TestAdaptor adaptor = model.RowAtIndex(i);

    EXPECT_EQ(adaptor.index(), i);
    unity::glib::String tmp(g_strdup_printf("Test%u", i));
    EXPECT_EQ(adaptor.name(), tmp.Str());
  }
}

// We're testing the model's ability to store and retrieve random pointers
TEST_F(TestModel, TestSetGetRenderer)
{
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



void discard_g_log_calls(const gchar* log_domain,
                         GLogLevelFlags log_level,
                         const gchar* message,
                         gpointer user_data)
{
  // do nothing for the messages.
}

class TestRowAdapterBase : public Test
{
public:
  void SetUp() {
    logger_ = g_log_set_default_handler(discard_g_log_calls, NULL);
  }
  void TearDown() {
    g_log_set_default_handler(logger_, NULL);
  }
private:
  GLogFunc logger_;
};

TEST_F(TestRowAdapterBase, TestGetStringNull)
{
  DeeModel* model = dee_sequence_model_new();
  dee_model_set_schema(model, "i", NULL);
  // Add in zero to an int field
  DeeModelIter* iter = dee_model_append(model, 0);

  RowAdaptorBase row(model, iter);

  // Check that the method we call is null.
  const gchar* value = dee_model_get_string(model, iter, 0);
  ASSERT_THAT(value, IsNull());
  ASSERT_THAT(model, NotNull());
  ASSERT_THAT(iter, NotNull());
  ASSERT_THAT(row.GetStringAt(0), Eq(""));
}

TEST_F(TestRowAdapterBase, TestGetStringEmpty)
{
  DeeModel* model = dee_sequence_model_new();
  dee_model_set_schema(model, "s", NULL);
  // Add on a empty string.
  DeeModelIter* iter = dee_model_append(model, "");

  RowAdaptorBase row(model, iter);

  ASSERT_THAT(model, NotNull());
  ASSERT_THAT(iter, NotNull());
  ASSERT_THAT(row.GetStringAt(0), Eq(""));
}

TEST_F(TestRowAdapterBase, TestGetStringValue)
{
  DeeModel* model = dee_sequence_model_new();
  dee_model_set_schema(model, "s", NULL);
  // Add on a real string.
  DeeModelIter* iter = dee_model_append(model, "Hello");

  RowAdaptorBase row(model, iter);

  ASSERT_THAT(model, NotNull());
  ASSERT_THAT(iter, NotNull());
  ASSERT_THAT(row.GetStringAt(0), Eq("Hello"));
}

}
