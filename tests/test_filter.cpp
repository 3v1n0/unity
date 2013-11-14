#include <gtest/gtest.h>
#include <glib-object.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Filter.h>
#include <UnityCore/Filters.h>

using namespace std;
using namespace unity::dash;
using namespace unity;

namespace
{

class TestFilter : public ::testing::Test
{
public:
  TestFilter()
    : model_(CreateTestModel())
  {
    iter0_ = dee_model_get_first_iter(model_);
    iter1_ = dee_model_next(model_, iter0_);
    iter2_ = dee_model_next(model_, iter1_);
  }

  static glib::Object<DeeModel> CreateTestModel()
  {
    glib::Object<DeeModel> model(dee_sequence_model_new());
    dee_model_set_schema(model, "s", "s", "s", "s", "a{sv}", "b", "b", "b", NULL);
    // add data
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
    GVariant* hints = g_variant_builder_end(&b);

    dee_model_append(model, "ratings0", "Ratings0", "gtk-apply", "ratings",
                     hints, TRUE, TRUE, FALSE);

    dee_model_append(model, "ratings1", "Ratings1", "gtk-apply", "ratings",
                     hints, TRUE, TRUE, FALSE);

    dee_model_append(model, "ratings2", "Ratings2", "gtk-apply", "ratings",
                     hints, TRUE, TRUE, FALSE);

    return model;
  }

  glib::Object<DeeModel> model_;
  DeeModelIter* iter0_;
  DeeModelIter* iter1_;
  DeeModelIter* iter2_;
};

class TestFilters : public ::testing::Test
{
public:
  TestFilters()
    : model_(TestFilter::CreateTestModel())
    , num_added_(0)
    , num_removed_(0)
  {
    filters_.filter_added.connect(sigc::mem_fun(this, &TestFilters::OnAdded));
    filters_.filter_removed.connect(sigc::mem_fun(this, &TestFilters::OnRemoved));
  }

  void OnAdded(Filter::Ptr)
  {
    num_added_++;
  }

  void OnRemoved(Filter::Ptr)
  {
    num_removed_++;
  }

  glib::Object<DeeModel> model_;
  Filters filters_;
  int num_added_;
  int num_removed_;
};

class FilterRecorder : public Filter
{
public:
  FilterRecorder(DeeModel* model, DeeModelIter* iter)
    : Filter(model, iter)
    , removed_(false)
    , updated_(false)
  {
    removed.connect(sigc::mem_fun(this, &FilterRecorder::OnRemoved));
  }

  void Update(Filter::Hints& hints)
  {
    updated_ = true;
  }

  void OnRemoved()
  {
    removed_ = true;
  }

  void Clear()
  {}

  bool removed_;
  bool updated_;
};

TEST_F(TestFilter, TestConstruction)
{
  FilterRecorder filter(model_, iter0_);
}

TEST_F(TestFilter, TestConnect)
{
  g_debug("a");
  FilterRecorder recorder(model_, iter0_);
  g_debug("b");

  EXPECT_EQ(recorder.id, "ratings0");
  EXPECT_EQ(recorder.name, "Ratings0");
  EXPECT_EQ(recorder.icon_hint, "gtk-apply");
  EXPECT_EQ(recorder.renderer_name, "ratings");
  EXPECT_TRUE(recorder.visible);
  EXPECT_TRUE(recorder.collapsed);
  EXPECT_FALSE(recorder.filtering);
}

TEST_F(TestFilter, TestChanged)
{
  FilterRecorder recorder(model_, iter1_);

  dee_model_set_value(model_, iter1_, 0, g_variant_new("s", "checkbox"));
  dee_model_set_value(model_, iter1_, 7, g_variant_new("b", TRUE));

  EXPECT_EQ(recorder.updated_, true);
  EXPECT_EQ(recorder.id, "checkbox");
  EXPECT_EQ(recorder.name, "Ratings1");
  EXPECT_EQ(recorder.icon_hint, "gtk-apply");
  EXPECT_EQ(recorder.renderer_name, "ratings");
  EXPECT_TRUE(recorder.visible);
  EXPECT_TRUE(recorder.collapsed);
  EXPECT_TRUE(recorder.filtering);
}

TEST_F(TestFilter, TestRemoved)
{
  FilterRecorder recorder(model_, iter1_);

  dee_model_remove(model_, iter2_);
  EXPECT_EQ(recorder.removed_, false);

  dee_model_remove(model_, iter0_);
  EXPECT_EQ(recorder.removed_, false);

  dee_model_remove(model_, iter1_);
  EXPECT_EQ(recorder.removed_, true);
}

TEST_F(TestFilters, TestSetModel)
{
  filters_.SetModel(model_);

  EXPECT_EQ(num_added_, 3);
  EXPECT_EQ(num_removed_, 0);
}

TEST_F(TestFilters, TestSetModelTwice)
{
  filters_.SetModel(model_);
  EXPECT_EQ(num_added_, 3);
  EXPECT_EQ(num_removed_, 0);

  filters_.SetModel(TestFilter::CreateTestModel());
  EXPECT_EQ(num_added_, 6);
  EXPECT_EQ(num_removed_, 3);
}

}
