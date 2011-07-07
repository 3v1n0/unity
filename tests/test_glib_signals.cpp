#include <UnityCore/UnityCore.h>

#include <gtest/gtest.h>

using namespace unity;

namespace {

TEST(TestGLibSignals, TestConstruction)
{

}

/*
TEST(TestIndicatorEntry, TestConstruction) {

  indicator::Entry entry("id", "label", true, true,
                         1, "some icon", false, true);

  EXPECT_EQ(entry.id(), "id");
  EXPECT_EQ(entry.label(), "label");
  EXPECT_TRUE(entry.label_sensitive());
  EXPECT_TRUE(entry.label_visible());
  EXPECT_FALSE(entry.image_sensitive());
  EXPECT_TRUE(entry.image_visible());
  EXPECT_FALSE(entry.active());
  EXPECT_FALSE(entry.show_now());
  EXPECT_FALSE(entry.IsUnused());
  EXPECT_EQ(entry.image_type(), 1);
  EXPECT_EQ(entry.image_data(), "some icon");
}
*/

}
