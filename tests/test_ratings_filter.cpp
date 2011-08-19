#include <gtest/gtest.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/RatingsFilter.h>

using namespace std;
using namespace unity::dash;
using namespace unity;

namespace
{

class TestRatingsFilter : public ::testing::Test
{
public:
  TestRatingsFilter()
    : model_(dee_sequence_model_new())
  {
    dee_model_set_schema(model_, "s", "s", "s", "s", "a{sv}", "b", "b", "b", NULL);

    AddIters();
  }

  void AddIters()
  {
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&b, "{sv}", "rating", g_variant_new("d", 0.6));

    iter_ = dee_model_append(model_,
                             "ratings",
                             "Ratings0",
                             "gtk-apply",
                             "ratings",
                             g_variant_builder_end(&b),
                             TRUE,
                             TRUE,
                             FALSE);
  }

  glib::Object<DeeModel> model_;
  DeeModelIter* iter_;
};

TEST_F(TestRatingsFilter, TestConstruction)
{
  RatingsFilter::Ptr ratings(new RatingsFilter(model_, iter_));
  EXPECT_FLOAT_EQ(ratings->rating, 0.6);
}

TEST_F(TestRatingsFilter, TestSetting)
{
  RatingsFilter::Ptr ratings(new RatingsFilter(model_, iter_));
  ratings->rating = 1.0f;

  GVariant* row_value = dee_model_get_value(model_, iter_, FilterColumn::RENDERER_STATE);

  GVariantIter iter;
  g_variant_iter_init(&iter, row_value);

  char* key = NULL;
  GVariant* value = NULL;
  float rating = 0.0f;
  while (g_variant_iter_loop(&iter, "{sv}", &key, &value))
  {
    if (g_strcmp0(key, "rating") == 0)
    {
      rating = g_variant_get_double(value);
      break;
    }
  }

  EXPECT_FLOAT_EQ(ratings->rating, rating);
  EXPECT_TRUE(ratings->filtering);

  g_variant_unref(row_value);
}


}
