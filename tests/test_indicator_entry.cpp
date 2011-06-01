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

struct Counter : sigc::trackable
{
  Counter() : count(0) {}
  void increment()
    {
      ++count;
    }
  int count;
};

template <typename T>
struct ChangeRecorder : sigc::trackable
{
  void value_changed(T const& value)
    {
      changed_values.push_back(value);
    }
  typedef std::vector<T> ChangedValues;
  ChangedValues changed_values;
};


TEST(TestIndicatorEntry, TestActiveEvents) {

  indicator::Entry entry("id", "label", true, true,
                         0, "some icon", false, true);

  ChangeRecorder<bool> recorder;
  Counter counter;
  entry.updated.connect(sigc::mem_fun(counter, &Counter::increment));
  entry.active_changed.connect(sigc::mem_fun(recorder, &ChangeRecorder<bool>::value_changed));

  // Setting to the same value doesn't emit any events.
  entry.set_active(false);
  EXPECT_FALSE(entry.active());
  EXPECT_EQ(counter.count, 0);
  EXPECT_EQ(recorder.changed_values.size(), 0);

  // Setting to a different value does emit the events.
  entry.set_active(true);
  EXPECT_TRUE(entry.active());
  EXPECT_EQ(counter.count, 1);
  ASSERT_EQ(recorder.changed_values.size(), 1);
  EXPECT_TRUE(recorder.changed_values[0]);


}

}
