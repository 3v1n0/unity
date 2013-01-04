#include <UnityCore/IndicatorEntry.h>

#include <gtest/gtest.h>

using namespace std;
using namespace unity;

namespace
{


TEST(TestIndicatorEntry, TestConstruction)
{

  indicator::Entry entry("id", "name_hint", "label", true, true,
                         1, "some icon", false, true, -1);

  EXPECT_EQ(entry.id(), "id");
  EXPECT_EQ(entry.label(), "label");
  EXPECT_EQ(entry.name_hint(), "name_hint");
  EXPECT_TRUE(entry.label_sensitive());
  EXPECT_TRUE(entry.label_visible());
  EXPECT_FALSE(entry.image_sensitive());
  EXPECT_TRUE(entry.image_visible());
  EXPECT_FALSE(entry.active());
  EXPECT_FALSE(entry.show_now());
  EXPECT_TRUE(entry.visible());
  EXPECT_EQ(entry.image_type(), 1);
  EXPECT_EQ(entry.image_data(), "some icon");
  EXPECT_EQ(entry.priority(), -1);
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

TEST(TestIndicatorEntry, TestAssignment)
{

  indicator::Entry entry("id", "name_hint", "label", true, true,
                         0, "some icon", false, true, 10);
  indicator::Entry other_entry("other_id", "other_name_hint", "other_label",
                               false, false, 2, "other icon", true, false, 5);

  Counter counter;
  entry.updated.connect(sigc::mem_fun(counter, &Counter::increment));

  entry = other_entry;

  EXPECT_EQ(entry.id(), "other_id");
  EXPECT_EQ(entry.name_hint(), "other_name_hint");
  EXPECT_EQ(entry.label(), "other_label");
  EXPECT_FALSE(entry.label_sensitive());
  EXPECT_FALSE(entry.label_visible());
  EXPECT_EQ(entry.image_type(), 2);
  EXPECT_EQ(entry.image_data(), "other icon");
  EXPECT_TRUE(entry.image_sensitive());
  EXPECT_FALSE(entry.image_visible());
  EXPECT_EQ(counter.count, 1);
  EXPECT_EQ(entry.priority(), 5);
}

TEST(TestIndicatorEntry, TestShowNowEvents)
{

  indicator::Entry entry("id", "name_hint", "label", true, true,
                         0, "some icon", false, true, -1);

  ChangeRecorder<bool> recorder;
  Counter counter;
  entry.updated.connect(sigc::mem_fun(counter, &Counter::increment));
  entry.show_now_changed.connect(sigc::mem_fun(recorder, &ChangeRecorder<bool>::value_changed));

  // Setting show_now to the same value doesn't emit any events.
  entry.set_show_now(false);
  EXPECT_FALSE(entry.show_now());
  EXPECT_EQ(counter.count, 0);
  EXPECT_EQ(recorder.changed_values.size(), 0);

  // Setting to a different value does emit the events.
  entry.set_show_now(true);
  EXPECT_TRUE(entry.show_now());
  EXPECT_EQ(counter.count, 1);
  ASSERT_EQ(recorder.changed_values.size(), 1);
  EXPECT_TRUE(recorder.changed_values[0]);
}

TEST(TestIndicatorEntry, TestActiveEvents)
{

  indicator::Entry entry("id", "name_hint", "label", true, true,
                         0, "some icon", false, true, -1);

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

struct ScrollRecorder : public ChangeRecorder<int>
{
  ScrollRecorder(std::string const& name) : entry_name(name) {}

  void OnScroll(std::string const& name, int delta)
  {
    EXPECT_EQ(name, entry_name);
    value_changed(delta);
  }

  std::string entry_name;
};

TEST(TestIndicatorEntry, TestOnScroll)
{

  indicator::Entry entry("id", "name_hint", "label", true, true,
                         0, "some icon", false, true, -1);

  ScrollRecorder recorder("id");
  entry.on_scroll.connect(sigc::mem_fun(recorder, &ScrollRecorder::OnScroll));

  entry.Scroll(10);
  entry.Scroll(-20);

  ASSERT_EQ(recorder.changed_values.size(), 2);
  EXPECT_EQ(recorder.changed_values[0], 10);
  EXPECT_EQ(recorder.changed_values[1], -20);
}

struct ShowMenuRecorder
{
  void OnShowMenu(std::string const& a, unsigned b, int c, int d, unsigned e)
  {
    name = a;
    xid = b;
    x = c;
    y = d;
    button = e;
  }
  std::string name;
  unsigned xid, button;
  int x, y;
};

TEST(TestIndicatorEntry, TestOnShowMenu)
{

  indicator::Entry entry("id", "name_hint", "label", true, true,
                         0, "some icon", false, true, -1);

  ShowMenuRecorder recorder;
  entry.on_show_menu.connect(sigc::mem_fun(recorder, &ShowMenuRecorder::OnShowMenu));

  entry.ShowMenu(10, 20, 1);
  EXPECT_EQ(recorder.name, "id");
  EXPECT_EQ(recorder.xid, 0);
  EXPECT_EQ(recorder.x, 10);
  EXPECT_EQ(recorder.y, 20);
  EXPECT_EQ(recorder.button, 1);
}

TEST(TestIndicatorEntry, TestOnShowMenuXid)
{

  indicator::Entry entry("xid", "name_hint", "label", true, true,
                         0, "some icon", false, true, -1);

  ShowMenuRecorder recorder;
  entry.on_show_menu.connect(sigc::mem_fun(recorder, &ShowMenuRecorder::OnShowMenu));

  entry.ShowMenu(88492615, 15, 25, 2);
  EXPECT_EQ(recorder.name, "xid");
  EXPECT_EQ(recorder.xid, 88492615);
  EXPECT_EQ(recorder.x, 15);
  EXPECT_EQ(recorder.y, 25);
  EXPECT_EQ(recorder.button, 2);
}

TEST(TestIndicatorEntry, TestVisibility)
{

  indicator::Entry entry("id", "name_hint", "label", true, true,
                         0, "some icon", false, false, -1);

  EXPECT_TRUE(entry.visible());

  entry.setLabel("", true, true);
  EXPECT_FALSE(entry.visible());

  entry.setLabel("valid-label", true, true);
  EXPECT_TRUE(entry.visible());

  entry.setLabel("invalid-label", true, false);
  EXPECT_FALSE(entry.visible());

  entry.setImage(1, "valid-image", true, true);
  EXPECT_TRUE(entry.visible());

  entry.setImage(1, "", true, true);
  EXPECT_FALSE(entry.visible());

  entry.setImage(1, "valid-image", true, true);
  EXPECT_TRUE(entry.visible());

  entry.setImage(0, "invalid-image-type", true, true);
  EXPECT_FALSE(entry.visible());

  entry.setLabel("valid-label", true, true);
  EXPECT_TRUE(entry.visible());

  entry.setLabel("invalid-label", true, false);
  EXPECT_FALSE(entry.visible());

  entry.setImage(1, "invalid-image", true, false);
  EXPECT_FALSE(entry.visible());

  entry.setLabel("valid-label", true, true);
  entry.setImage(1, "valid-image", true, true);
  EXPECT_TRUE(entry.visible());
}

TEST(TestIndicatorEntry, TestGeometry)
{

  indicator::Entry entry("id", "name_hint", "label", true, true,
                         0, "some icon", false, true, -1);

  Counter counter;
  entry.updated.connect(sigc::mem_fun(counter, &Counter::increment));
  bool geo_changed = false;
  nux::Rect new_geo;

  entry.geometry_changed.connect([&] (nux::Rect const& geo) {
    geo_changed = true;
    new_geo = geo;
  });

  // Setting to the same value doesn't emit any events.
  entry.set_geometry(nux::Rect());
  EXPECT_EQ(entry.geometry(), nux::Rect());
  EXPECT_EQ(counter.count, 0);

  // Setting to a different value does emit the events.
  entry.set_geometry(nux::Rect(1, 2, 3, 4));
  EXPECT_EQ(entry.geometry(), nux::Rect(1, 2, 3, 4));
  EXPECT_TRUE(geo_changed);
  EXPECT_EQ(new_geo, nux::Rect(1, 2, 3, 4));
  EXPECT_EQ(counter.count, 1);
}

}
