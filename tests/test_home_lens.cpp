#include <gtest/gtest.h>
#include <glib-object.h>
#include <dee.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/HomeLens.h>
#include <UnityCore/Lens.h>
#include <UnityCore/Lenses.h>

#include "test_utils.h"

using namespace std;
using namespace unity::dash;

namespace
{

TEST(TestHomeLens, TestConstruction)
{
  HomeLens home_lens_("name", "description", "searchhint");

  EXPECT_EQ(home_lens_.id(), "home.lens");
  EXPECT_EQ(home_lens_.connected, false);
  EXPECT_EQ(home_lens_.search_in_global, false);
  EXPECT_EQ(home_lens_.name, "name");
  EXPECT_EQ(home_lens_.description, "description");
  EXPECT_EQ(home_lens_.search_hint, "searchhint");
}

TEST(TestHomeLens, TestInitiallyEmpty)
{
  HomeLens home_lens_("name", "description", "searchhint");
  DeeModel* results = home_lens_.results()->model();
  DeeModel* categories = home_lens_.categories()->model();;
  DeeModel* filters = home_lens_.filters()->model();;

  EXPECT_EQ(dee_model_get_n_rows(results), 0);
  EXPECT_EQ(dee_model_get_n_rows(categories), 0);
  EXPECT_EQ(dee_model_get_n_rows(filters), 0);

  EXPECT_EQ(home_lens_.count(), 0);
}

}
