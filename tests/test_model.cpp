#include <gtest/gtest.h>
#include <glib-object.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Model.h>

#define SWARM_NAME "com.canonical.test.model"

using namespace std;
using namespace unity::dash;

namespace {

class TestAdaptor : public RowAdaptorBase
{
public:
  TestAdaptor(DeeModel* model, DeeModelIter* iter, DeeModelTag* tag)
  {
    model_ = model;
    iter_ = iter;
    tag_ = tag;
  }

  int index()
  {
    return dee_model_get_int32(model_, iter_, 0);
  }

  string name()
  {
    return dee_model_get_string(model_, iter_, 1);
  }
};

static void WaitForSynchronize(Model<TestAdaptor>& model)
{
  bool timeout_reached;

  auto timeout_cb = [](gpointer data) -> gboolean
  {
    *(bool*)data = true;
    return FALSE;
  };

  guint32 timeout_id = g_timeout_add_seconds(10, timeout_cb, &timeout_reached);

  while (!model.synchronized && !timeout_reached)
  {
    g_main_context_iteration(g_main_context_get_thread_default(), TRUE);
  }
  if (model.synchronized)
    g_source_remove(timeout_id);
}

TEST(TestModel, TestConstruction)
{
  Model<TestAdaptor> model;
  model.swarm_name = SWARM_NAME;
}

TEST(TestModel, TestSynchronization)
{
  Model<TestAdaptor> model;
  model.swarm_name = SWARM_NAME;

  WaitForSynchronize(model);
  EXPECT_TRUE(model.synchronized);
  EXPECT_EQ(model.count, 100);
}

TEST(TestModel, TestRowsValid)
{
  Model<TestAdaptor> model;
  model.swarm_name = SWARM_NAME;

  WaitForSynchronize(model);
  EXPECT_TRUE(model.synchronized);

  for(int i = 0; i < 100; i++)
  {
    TestAdaptor adaptor = model.RowAtIndex(i);

    EXPECT_EQ(adaptor.index(), i);
    unity::glib::String tmp(g_strdup_printf("Test%d", i));
    EXPECT_EQ(adaptor.name(), tmp.Str());
  }
}

// We're testing the model's ability to store and retrieve random pointers
TEST(TestModel, TestSetGetRenderer)
{
  Model<TestAdaptor> model;
  model.swarm_name = SWARM_NAME;

  WaitForSynchronize(model);
  EXPECT_TRUE(model.synchronized);

  for(int i = 0; i < 100; i++)
  {
    TestAdaptor adaptor = model.RowAtIndex(i);

    char* value = g_strdup_printf("Renderer%d", i);
    adaptor.set_renderer<char*>(value);
  }

  for(int i = 0; i < 100; i++)
  {
    TestAdaptor adaptor = model.RowAtIndex(i);

    unity::glib::String value(adaptor.renderer<char*>());
    unity::glib::String renderer (g_strdup_printf("Renderer%d", i));
  
    EXPECT_EQ(value.Str(), renderer.Str());
  }
}

}
