#include <UnityCore/UnityCore.h>
#include <gtest/gtest.h>

#include "test_glib_signals_utils.h"

using namespace unity;

namespace {

void OnSignal1 (std::string str)
{

}

TEST(TestGLibSignals, TestConstruction)
{
  GObject *object = static_cast<GObject*>(g_object_new(TEST_TYPE_SIGNALS, NULL));

  EXPECT_TRUE(G_IS_OBJECT(object));

  glib::Signal<void, std::string> signal1;
  signal1.Connect (object, "signal-1", sigc::ptr_fun (OnSignal1));
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
