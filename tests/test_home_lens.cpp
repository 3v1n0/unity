#include <gtest/gtest.h>
#include <glib-object.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Categories.h>

#include "test_utils.h"

using namespace std;
using namespace unity::dash;

namespace
{

TEST(TestHomeLens, TestConstruction)
{
  HomeLens home_lens_;

  EXPECT_EQ(home_lens.id(), "home.lens");
}

}
