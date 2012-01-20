#include <gtest/gtest.h>
#include <glib-object.h>

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
  EXPECT_EQ(home_lens_.name(), "name");
  EXPECT_EQ(home_lens_.description(), "description");
  EXPECT_EQ(home_lens_.search_hint(), "searchhint");
}

}
