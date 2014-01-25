// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <gmock/gmock.h>
#include <UnityCore/Indicators.h>
#include <UnityCore/AppmenuIndicator.h>

using namespace std;
using namespace unity;
using namespace indicator;
using namespace testing;

namespace
{

struct TargetData
{
  TargetData()
  {
    Reset();
  }

  void Reset()
  {
    entry = "";
    geo = nux::Rect();
    delta = 0;
    x = 0;
    y = 0;
    xid = 0;
    button = 0;
  }

  std::string entry;
  nux::Rect geo;
  int delta;
  int x;
  int y;
  unsigned int xid;
  unsigned int button;
};

class MockIndicators : public Indicators
{
public:
  MockIndicators()
  {}

  // Implementing Indicators virtual functions
  virtual void OnEntryScroll(std::string const& entry_id, int delta)
  {
    target.entry = entry_id;
    target.delta = delta;
  }

  virtual void OnEntryShowMenu(std::string const& entry_id, unsigned int xid,
                               int x, int y, unsigned int button)
  {
    on_entry_show_menu.emit(entry_id, xid, x, y, button);

    target.entry = entry_id;
    target.xid = xid;
    target.x = x;
    target.y = y;
    target.button = button;
  }

  virtual void OnEntrySecondaryActivate(std::string const& entry_id)
  {
    target.entry = entry_id;
  }

  virtual void OnShowAppMenu(unsigned int xid, int x, int y)
  {
    on_show_appmenu.emit(xid, x, y);

    target.xid = xid;
    target.x = x;
    target.y = y;
  }

  // Redirecting protected methods
  using Indicators::GetIndicator;
  using Indicators::ActivateEntry;
  using Indicators::AddIndicator;
  using Indicators::RemoveIndicator;
  using Indicators::SetEntryShowNow;

  // Utility function used to fill the class with test indicators with entries
  void SetupTestChildren()
  {
    // Adding an indicator filled with entries into the MockIndicators
    Indicator::Entries sync_data;
    Entry* entry;

    Indicator::Ptr test_indicator_1 = AddIndicator("indicator-test-1");

    entry = new Entry("indicator-test-1|entry-1", "name-hint-1", "label", true, true,
                      0, "icon", true, true, -1);
    sync_data.push_back(Entry::Ptr(entry));

    entry = new Entry("indicator-test-1|entry-2", "name-hint-2", "label", true, true,
                      0, "icon", true, true, -1);
    sync_data.push_back(Entry::Ptr(entry));

    entry = new Entry("indicator-test-1|entry-3", "name-hint-3", "label", true, true,
                      0, "icon", true, true, -1);
    sync_data.push_back(Entry::Ptr(entry));

    // Sync the indicator, adding 3 entries
    test_indicator_1->Sync(sync_data);
    EXPECT_EQ(test_indicator_1->GetEntries().size(), 3);


    // Adding another indicator filled with entries into the MockIndicators
    Indicator::Ptr test_indicator_2 = AddIndicator("indicator-test-2");
    sync_data.clear();

    entry = new Entry("indicator-test-2|entry-1", "name-hint-1", "label", true, true,
                      0, "icon", true, true, -1);
    sync_data.push_back(Entry::Ptr(entry));

    entry = new Entry("indicator-test-2|entry-2", "name-hint-2", "label", true, true,
                      0, "icon", true, true, -1);
    sync_data.push_back(Entry::Ptr(entry));

    // Sync the indicator, adding 2 entries
    test_indicator_2->Sync(sync_data);
    EXPECT_EQ(test_indicator_2->GetEntries().size(), 2);

    ASSERT_THAT(GetIndicators().size(), 2);
  }

  TargetData target;
};

TEST(TestIndicators, Construction)
{
  {
    MockIndicators indicators;

    EXPECT_TRUE(indicators.GetIndicators().empty());
  }
}

TEST(TestIndicators, GetInvalidIndicator)
{
  MockIndicators indicators;

  ASSERT_THAT(indicators.GetIndicator("no-available-indicator"), IsNull());
}

TEST(TestIndicators, IndicatorsFactory)
{
  MockIndicators indicators;

  Indicator::Ptr standard_indicator = indicators.AddIndicator("libapplication.so");
  EXPECT_EQ(standard_indicator->name(), "libapplication.so");
  EXPECT_FALSE(standard_indicator->IsAppmenu());

  Indicator::Ptr appmenu_indicator = indicators.AddIndicator("libappmenu.so");
  EXPECT_EQ(appmenu_indicator->name(), "libappmenu.so");
  EXPECT_TRUE(appmenu_indicator->IsAppmenu());
}

TEST(TestIndicators, IndicatorsHandling)
{
  MockIndicators indicators;
  Indicators::IndicatorsList indicators_list;

  // Connecting to signals
  Indicators::IndicatorsList added_list;
  indicators.on_object_added.connect([&added_list] (Indicator::Ptr const& i) {
    added_list.push_back(i);
  });

  Indicators::IndicatorsList removed_list;
  indicators.on_object_removed.connect([&removed_list] (Indicator::Ptr const& i) {
    removed_list.push_back(i);
  });

  // Adding some indicators...
  Indicator::Ptr test_indicator_1(indicators.AddIndicator("indicator-test-1"));
  EXPECT_EQ(indicators.GetIndicator("indicator-test-1"), test_indicator_1);

  EXPECT_EQ(added_list.size(), 1);
  EXPECT_NE(std::find(added_list.begin(), added_list.end(), test_indicator_1), added_list.end());
  EXPECT_TRUE(removed_list.empty());

  indicators_list = indicators.GetIndicators();
  EXPECT_EQ(indicators_list.size(), 1);
  EXPECT_NE(std::find(indicators_list.begin(), indicators_list.end(), test_indicator_1), indicators_list.end());


  Indicator::Ptr test_indicator_2(indicators.AddIndicator("indicator-test-2"));
  EXPECT_EQ(indicators.GetIndicator("indicator-test-2"), test_indicator_2);

  EXPECT_EQ(added_list.size(), 2);
  EXPECT_NE(std::find(added_list.begin(), added_list.end(), test_indicator_2), added_list.end());
  EXPECT_TRUE(removed_list.empty());

  indicators_list = indicators.GetIndicators();
  EXPECT_EQ(indicators_list.size(), 2);
  EXPECT_NE(std::find(indicators_list.begin(), indicators_list.end(), test_indicator_2), indicators_list.end());


  Indicator::Ptr test_indicator_3(indicators.AddIndicator("indicator-test-3"));
  EXPECT_EQ(indicators.GetIndicator("indicator-test-3"), test_indicator_3);

  EXPECT_EQ(added_list.size(), 3);
  EXPECT_NE(std::find(added_list.begin(), added_list.end(), test_indicator_3), added_list.end());
  EXPECT_TRUE(removed_list.empty());

  indicators_list = indicators.GetIndicators();
  EXPECT_EQ(indicators_list.size(), 3);
  EXPECT_NE(std::find(indicators_list.begin(), indicators_list.end(), test_indicator_3), indicators_list.end());


  ASSERT_THAT(indicators.GetIndicator("invalid-indicator-test-4"), IsNull());
  EXPECT_EQ(added_list.size(), 3);
  EXPECT_TRUE(removed_list.empty());
  EXPECT_EQ(indicators.GetIndicators().size(), 3);

  // Readding an indicator already there should do nothing
  Indicator::Ptr test_indicator_3_duplicate(indicators.AddIndicator("indicator-test-3"));
  EXPECT_EQ(added_list.size(), 3);
  EXPECT_EQ(indicators.GetIndicator("indicator-test-3"), test_indicator_3);
  EXPECT_EQ(indicators.GetIndicators().size(), 3);
  EXPECT_EQ(test_indicator_3, test_indicator_3_duplicate);

  // Removing the indicators...
  added_list.clear();

  indicators.RemoveIndicator("indicator-test-2");
  ASSERT_THAT(indicators.GetIndicator("indicator-test-2"), IsNull());

  EXPECT_TRUE(added_list.empty());
  EXPECT_NE(std::find(removed_list.begin(), removed_list.end(), test_indicator_2), removed_list.end());
  EXPECT_EQ(removed_list.size(), 1);

  indicators_list = indicators.GetIndicators();
  EXPECT_EQ(indicators_list.size(), 2);
  EXPECT_EQ(std::find(indicators_list.begin(), indicators_list.end(), test_indicator_2), indicators_list.end());


  indicators.RemoveIndicator("indicator-test-1");
  ASSERT_THAT(indicators.GetIndicator("indicator-test-1"), IsNull());

  EXPECT_TRUE(added_list.empty());
  EXPECT_NE(std::find(removed_list.begin(), removed_list.end(), test_indicator_1), removed_list.end());
  EXPECT_EQ(removed_list.size(), 2);

  indicators_list = indicators.GetIndicators();
  EXPECT_EQ(indicators_list.size(), 1);
  EXPECT_EQ(std::find(indicators_list.begin(), indicators_list.end(), test_indicator_1), indicators_list.end());


  indicators.RemoveIndicator("indicator-test-3");
  ASSERT_THAT(indicators.GetIndicator("indicator-test-3"), IsNull());

  EXPECT_TRUE(added_list.empty());
  EXPECT_NE(std::find(removed_list.begin(), removed_list.end(), test_indicator_3), removed_list.end());
  EXPECT_EQ(removed_list.size(), 3);

  indicators_list = indicators.GetIndicators();
  EXPECT_TRUE(indicators_list.empty());


  indicators.RemoveIndicator("invalid-indicator-test-4");
  EXPECT_EQ(removed_list.size(), 3);
}

TEST(TestIndicators, ActivateEntry)
{
  MockIndicators indicators;
  indicators.SetupTestChildren();

  // Activating Entries from the Indicators class to see if they get updated
  TargetData target;

  sigc::connection activated_conn =
  indicators.on_entry_activated.connect([&] (std::string const& e, nux::Rect const& g) {
    target.entry = e;
    target.geo = g;
  });

  ASSERT_THAT(indicators.GetIndicator("indicator-test-1"), NotNull());

  Entry::Ptr entry12(indicators.GetIndicator("indicator-test-1")->GetEntry("indicator-test-1|entry-2"));
  ASSERT_THAT(entry12, NotNull());

  ASSERT_THAT(entry12->active(), false);
  ASSERT_THAT(entry12->geometry(), nux::Rect());

  target.Reset();
  indicators.ActivateEntry("indicator-test-1|entry-2", nux::Rect(1, 2, 3, 4));

  EXPECT_EQ(entry12->active(), true);
  EXPECT_EQ(entry12->geometry(), nux::Rect(1, 2, 3, 4));
  EXPECT_EQ(target.entry, entry12->id());
  EXPECT_EQ(target.geo, entry12->geometry());

  activated_conn.disconnect();
}

TEST(TestIndicators, ActivateEntryShouldDisactivatePrevious)
{
  MockIndicators indicators;
  indicators.SetupTestChildren();

  // Activating Entries from the Indicators class to see if they get updated
  TargetData target;

  sigc::connection activated_conn =
  indicators.on_entry_activated.connect([&] (std::string const& e, nux::Rect const& g) {
    target.entry = e;
    target.geo = g;
  });

  ASSERT_THAT(indicators.GetIndicator("indicator-test-2"), NotNull());

  Entry::Ptr entry22(indicators.GetIndicator("indicator-test-2")->GetEntry("indicator-test-2|entry-2"));
  ASSERT_THAT(entry22, NotNull());

  ASSERT_THAT(entry22->active(), false);
  ASSERT_THAT(entry22->geometry(), nux::Rect());

  indicators.ActivateEntry("indicator-test-2|entry-2", nux::Rect(1, 2, 3, 4));

  ASSERT_THAT(entry22->active(), true);
  ASSERT_THAT(entry22->geometry(), nux::Rect(1, 2, 3, 4));


  // Activating another entry, the previously selected one should be disactivate
  Entry::Ptr entry21(indicators.GetIndicator("indicator-test-2")->GetEntry("indicator-test-2|entry-1"));
  ASSERT_THAT(entry21, NotNull());

  ASSERT_THAT(entry21->active(), false);
  ASSERT_THAT(entry21->geometry(), nux::Rect());

  target.Reset();
  indicators.ActivateEntry("indicator-test-2|entry-1", nux::Rect(4, 3, 2, 1));

  EXPECT_EQ(entry22->active(), false);
  EXPECT_EQ(entry22->geometry(), nux::Rect());

  EXPECT_EQ(entry21->active(), true);
  EXPECT_EQ(entry21->geometry(), nux::Rect(4, 3, 2, 1));
  EXPECT_EQ(target.entry, entry21->id());
  EXPECT_EQ(target.geo, entry21->geometry());

  activated_conn.disconnect();
}

TEST(TestIndicators, ActivateEntryInvalidEmitsNullSignal)
{
  MockIndicators indicators;
  indicators.SetupTestChildren();

  TargetData target;
  bool signal_received = false;

  sigc::connection activated_conn =
  indicators.on_entry_activated.connect([&] (std::string const& e, nux::Rect const& g) {
    signal_received = true;
    target.entry = e;
    target.geo = g;
  });

  ASSERT_THAT(indicators.GetIndicator("indicator-test-1"), NotNull());

  Entry::Ptr entry13(indicators.GetIndicator("indicator-test-1")->GetEntry("indicator-test-1|entry-3"));
  ASSERT_THAT(entry13, NotNull());

  ASSERT_THAT(entry13->active(), false);
  ASSERT_THAT(entry13->geometry(), nux::Rect());

  indicators.ActivateEntry("indicator-test-1|entry-3", nux::Rect(4, 2, 3, 4));

  ASSERT_THAT(entry13->active(), true);
  ASSERT_THAT(entry13->geometry(), nux::Rect(4, 2, 3, 4));
  ASSERT_TRUE(signal_received);


  // Activating invalid entry, the previously selected one should be disactivate
  target.Reset();
  signal_received = false;
  indicators.ActivateEntry("indicator-entry-invalid", nux::Rect(5, 5, 5, 5));
  EXPECT_TRUE(target.entry.empty());
  EXPECT_EQ(target.geo, nux::Rect());
  EXPECT_TRUE(signal_received);

  EXPECT_EQ(entry13->active(), false);
  EXPECT_EQ(entry13->geometry(), nux::Rect());

  activated_conn.disconnect();
}

TEST(TestIndicators, SetEntryShowNow)
{
  MockIndicators indicators;
  indicators.SetupTestChildren();

  TargetData target;

  sigc::connection activated_conn =
  indicators.on_entry_activated.connect([&] (std::string const& e, nux::Rect const& g) {
    target.entry = e;
    target.geo = g;
  });

  ASSERT_THAT(indicators.GetIndicator("indicator-test-2"), NotNull());

  Entry::Ptr entry22(indicators.GetIndicator("indicator-test-2")->GetEntry("indicator-test-2|entry-2"));

  ASSERT_THAT(entry22, NotNull());
  ASSERT_THAT(entry22->show_now(), false);

  indicators.SetEntryShowNow("indicator-test-2|entry-2", true);
  EXPECT_EQ(entry22->show_now(), true);

  indicators.SetEntryShowNow("indicator-test-2|entry-2", false);
  EXPECT_EQ(entry22->show_now(), false);
}

TEST(TestIndicators, EntryShowMenu)
{
  MockIndicators indicators;
  indicators.SetupTestChildren();

  // See if the indicators class get notified on entries actions
  ASSERT_THAT(indicators.GetIndicator("indicator-test-1"), NotNull());

  Entry::Ptr entry13(indicators.GetIndicator("indicator-test-1")->GetEntry("indicator-test-1|entry-3"));
  ASSERT_THAT(entry13, NotNull());

  TargetData show_menu_data;
  sigc::connection on_entry_show_menu_conn =
  indicators.on_entry_show_menu.connect([&] (std::string const& e, unsigned int w,
                                             int x, int y, unsigned int b) {
    show_menu_data.entry = e;
    show_menu_data.xid = w;
    show_menu_data.x = x;
    show_menu_data.y = y;
    show_menu_data.button = b;
  });

  entry13->ShowMenu(465789, 35, 53, 2);

  EXPECT_EQ(indicators.target.entry, entry13->id());
  EXPECT_EQ(indicators.target.xid, 465789);
  EXPECT_EQ(indicators.target.x, 35);
  EXPECT_EQ(indicators.target.y, 53);
  EXPECT_EQ(indicators.target.button, 2);

  EXPECT_EQ(show_menu_data.entry, entry13->id());
  EXPECT_EQ(show_menu_data.xid, 465789);
  EXPECT_EQ(show_menu_data.x, 35);
  EXPECT_EQ(show_menu_data.y, 53);
  EXPECT_EQ(show_menu_data.button, 2);

  show_menu_data.Reset();
  indicators.target.Reset();

  entry13->ShowMenu(55, 68, 3);

  EXPECT_EQ(indicators.target.entry, entry13->id());
  EXPECT_EQ(indicators.target.xid, 0);
  EXPECT_EQ(indicators.target.x, 55);
  EXPECT_EQ(indicators.target.y, 68);
  EXPECT_EQ(indicators.target.button, 3);

  EXPECT_EQ(show_menu_data.entry, entry13->id());
  EXPECT_EQ(show_menu_data.xid, 0);
  EXPECT_EQ(show_menu_data.x, 55);
  EXPECT_EQ(show_menu_data.y, 68);
  EXPECT_EQ(show_menu_data.button, 3);

  on_entry_show_menu_conn.disconnect();
}

TEST(TestIndicators, EntryScroll)
{
  MockIndicators indicators;
  indicators.SetupTestChildren();

  // See if the indicators class get notified on entries actions
  ASSERT_THAT(indicators.GetIndicator("indicator-test-1"), NotNull());

  Entry::Ptr entry11(indicators.GetIndicator("indicator-test-1")->GetEntry("indicator-test-1|entry-1"));
  ASSERT_THAT(entry11, NotNull());

  entry11->Scroll(80);
  EXPECT_EQ(indicators.target.entry, entry11->id());
  EXPECT_EQ(indicators.target.delta, 80);

  entry11->SecondaryActivate();
  EXPECT_EQ(indicators.target.entry, entry11->id());
}

TEST(TestIndicators, EntrySecondaryActivate)
{
  MockIndicators indicators;
  indicators.SetupTestChildren();

  // See if the indicators class get notified on entries actions
  ASSERT_THAT(indicators.GetIndicator("indicator-test-2"), NotNull());

  Entry::Ptr entry22(indicators.GetIndicator("indicator-test-2")->GetEntry("indicator-test-2|entry-1"));
  ASSERT_THAT(entry22, NotNull());

  entry22->SecondaryActivate();
  EXPECT_EQ(indicators.target.entry, entry22->id());
}

TEST(TestIndicators, ShowAppMenu)
{
  MockIndicators indicators;

  {
    Indicator::Ptr appmenu_indicator = indicators.AddIndicator("libappmenu.so");
    ASSERT_TRUE(appmenu_indicator->IsAppmenu());
  }

  ASSERT_EQ(indicators.GetIndicators().size(), 1);

  {
    Indicator::Ptr indicator = indicators.GetIndicator("libappmenu.so");
    ASSERT_THAT(indicator, NotNull());
    auto appmenu_indicator = dynamic_cast<AppmenuIndicator*>(indicator.get());
    ASSERT_THAT(appmenu_indicator, NotNull());

    indicators.target.Reset();

    appmenu_indicator->ShowAppmenu(4356789, 54, 13);

    EXPECT_TRUE(indicators.target.entry.empty());
    EXPECT_EQ(indicators.target.xid, 4356789);
    EXPECT_EQ(indicators.target.x, 54);
    EXPECT_EQ(indicators.target.y, 13);
    EXPECT_EQ(indicators.target.button, 0);
  }
}

}
