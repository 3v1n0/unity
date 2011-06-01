#include "IndicatorEntry.h"

#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

#include <gtest/gtest.h>

using namespace std;
using namespace unity;

namespace {


TEST(TestIndicatorEntry, TestConstruction) {

  indicator::Entry entry("id", "label", true, true,
                         0, "some icon", false, true);

  EXPECT_EQ(entry.id(), "id");
  EXPECT_EQ(entry.label(), "label");
  EXPECT_TRUE(entry.label_sensitive());
  EXPECT_TRUE(entry.label_visible());
  EXPECT_FALSE(entry.image_sensitive());
  EXPECT_TRUE(entry.image_visible());
  EXPECT_FALSE(entry.active());
  EXPECT_FALSE(entry.show_now());
  EXPECT_FALSE(entry.IsUnused());
  // TODO: work out a nice test for the GdkPixbuf.
}



}
