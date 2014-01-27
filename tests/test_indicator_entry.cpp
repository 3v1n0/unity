#include <UnityCore/IndicatorEntry.h>
#include <gmock/gmock.h>

using namespace std;
using namespace unity;
using namespace testing;

namespace
{
struct SigReceiver : sigc::trackable
{
  typedef NiceMock<SigReceiver> Nice;

  SigReceiver(indicator::Entry const& const_entry)
  {
    auto& entry = const_cast<indicator::Entry&>(const_entry);
    entry.updated.connect(sigc::mem_fun(this, &SigReceiver::Updated));
    entry.active_changed.connect(sigc::mem_fun(this, &SigReceiver::ActiveChanged));
    entry.geometry_changed.connect(sigc::mem_fun(this, &SigReceiver::GeometryChanged));
    entry.show_now_changed.connect(sigc::mem_fun(this, &SigReceiver::ShowNowChanged));
    entry.on_show_menu.connect(sigc::mem_fun(this, &SigReceiver::OnShowMenu));
    entry.on_secondary_activate.connect(sigc::mem_fun(this, &SigReceiver::OnSecondaryActivate));
    entry.on_scroll.connect(sigc::mem_fun(this, &SigReceiver::OnScroll));
  }

  MOCK_CONST_METHOD0(Updated, void());
  MOCK_CONST_METHOD1(ActiveChanged, void(bool));
  MOCK_CONST_METHOD1(GeometryChanged, void(nux::Rect const&));
  MOCK_CONST_METHOD1(ShowNowChanged, void(bool));
  MOCK_CONST_METHOD5(OnShowMenu, void(std::string const&, unsigned, int, int, unsigned));
  MOCK_CONST_METHOD1(OnSecondaryActivate, void(std::string const&));
  MOCK_CONST_METHOD2(OnScroll, void(std::string const&, int));
};

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

TEST(TestIndicatorEntry, TestAssignment)
{

  indicator::Entry entry("id", "name_hint", "label", true, true,
                         0, "some icon", false, true, 10);
  indicator::Entry other_entry("other_id", "other_name_hint", "other_label",
                               false, false, 2, "other icon", true, false, 5);

  SigReceiver sig_receiver(entry);
  EXPECT_CALL(sig_receiver, Updated());
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
  EXPECT_EQ(entry.priority(), 5);
}

TEST(TestIndicatorEntry, TestShowNowEvents)
{
  indicator::Entry entry("id", "name_hint", "label", true, true,
                         0, "some icon", false, true, -1);
  SigReceiver sig_receiver(entry);

  // Setting show_now to the same value doesn't emit any events.
  EXPECT_CALL(sig_receiver, Updated()).Times(0);
  EXPECT_CALL(sig_receiver, ShowNowChanged(_)).Times(0);

  entry.set_show_now(false);
  EXPECT_FALSE(entry.show_now());

  // Setting to a different value does emit the events.
  EXPECT_CALL(sig_receiver, Updated());
  EXPECT_CALL(sig_receiver, ShowNowChanged(true));

  entry.set_show_now(true);
  EXPECT_TRUE(entry.show_now());
}

TEST(TestIndicatorEntry, TestActiveEvents)
{
  indicator::Entry entry("id", "name_hint", "label", true, true,
                         0, "some icon", false, true, -1);

  SigReceiver sig_receiver(entry);

  // Setting to the same value doesn't emit any events.
  EXPECT_CALL(sig_receiver, Updated()).Times(0);
  EXPECT_CALL(sig_receiver, ActiveChanged(_)).Times(0);

  entry.set_active(false);
  EXPECT_FALSE(entry.active());

  // Setting to a different value does emit the events.
  EXPECT_CALL(sig_receiver, Updated());
  EXPECT_CALL(sig_receiver, ActiveChanged(true));

  entry.set_active(true);
  EXPECT_TRUE(entry.active());
}

TEST(TestIndicatorEntry, TestOnScroll)
{
  indicator::Entry entry("id", "name_hint", "label", true, true,
                         0, "some icon", false, true, -1);
  SigReceiver sig_receiver(entry);

  EXPECT_CALL(sig_receiver, OnScroll("id", 10));
  entry.Scroll(10);

  EXPECT_CALL(sig_receiver, OnScroll("id", -20));
  entry.Scroll(-20);
}

TEST(TestIndicatorEntry, TestOnShowMenu)
{
  indicator::Entry entry("id", "name_hint", "label", true, true,
                         0, "some icon", false, true, -1);
  SigReceiver sig_receiver(entry);

  EXPECT_CALL(sig_receiver, OnShowMenu("id", 0, 10, 20, 1));
  entry.ShowMenu(10, 20, 1);
}

TEST(TestIndicatorEntry, TestOnShowMenuXid)
{
  indicator::Entry entry("xid", "name_hint", "label", true, true,
                         0, "some icon", false, true, -1);
  SigReceiver sig_receiver(entry);

  EXPECT_CALL(sig_receiver, OnShowMenu("xid", 88492615, 15, 25, 2));
  entry.ShowMenu(88492615, 15, 25, 2);
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
  SigReceiver sig_receiver(entry);

  // Setting to the same value doesn't emit any events.
  EXPECT_CALL(sig_receiver, Updated()).Times(0);
  EXPECT_CALL(sig_receiver, GeometryChanged(_)).Times(0);

  entry.set_geometry(nux::Rect());
  EXPECT_EQ(entry.geometry(), nux::Rect());

  // Setting to a different value does emit the events.
  EXPECT_CALL(sig_receiver, Updated());
  EXPECT_CALL(sig_receiver, GeometryChanged(nux::Rect(1, 2, 3, 4)));

  entry.set_geometry(nux::Rect(1, 2, 3, 4));
  EXPECT_EQ(entry.geometry(), nux::Rect(1, 2, 3, 4));
}

}
