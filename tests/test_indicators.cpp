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

struct SigReceiver : sigc::trackable
{
  typedef NiceMock<SigReceiver> Nice;

  SigReceiver(Indicators const& const_indicators)
  {
    auto& indicators = const_cast<Indicators&>(const_indicators);
    indicators.on_object_added.connect(sigc::mem_fun(this, &SigReceiver::OnObjectAdded));
    indicators.on_object_removed.connect(sigc::mem_fun(this, &SigReceiver::OnObjectRemoved));
    indicators.on_entry_activate_request.connect(sigc::mem_fun(this, &SigReceiver::OnEntryActivateRequest));
    indicators.on_entry_activated.connect(sigc::mem_fun(this, &SigReceiver::OnEntryActivated));
    indicators.on_entry_show_menu.connect(sigc::mem_fun(this, &SigReceiver::OnEntryShowMenu));
  }

  MOCK_CONST_METHOD1(OnObjectAdded, void(Indicator::Ptr const&));
  MOCK_CONST_METHOD1(OnObjectRemoved, void(Indicator::Ptr const&));
  MOCK_CONST_METHOD1(OnEntryActivateRequest, void(std::string const&));
  MOCK_CONST_METHOD2(OnEntryActivated, void(std::string const&, nux::Rect const&));
  MOCK_CONST_METHOD5(OnEntryShowMenu, void(std::string const&, unsigned, int, int, unsigned));
};

struct MockIndicators : Indicators
{
  typedef NiceMock<MockIndicators> Nice;

  MockIndicators()
  {}

  // Implementing Indicators virtual functions
  MOCK_METHOD5(ShowEntriesDropdown, void(Indicator::Entries const&, Entry::Ptr const&, unsigned xid, int x, int y));
  MOCK_METHOD2(OnEntryScroll, void(std::string const&, int delta));
  MOCK_METHOD5(OnEntryShowMenu, void(std::string const&, unsigned xid, int x, int y, unsigned button));
  MOCK_METHOD1(OnEntrySecondaryActivate, void(std::string const&));
  MOCK_METHOD3(OnShowAppMenu, void(unsigned xid, int x, int y));

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
  SigReceiver::Nice sig_receiver(indicators);
  Indicators::IndicatorsList indicators_list;

  // Adding some indicators...
  EXPECT_CALL(sig_receiver, OnObjectAdded(_));
  EXPECT_CALL(sig_receiver, OnObjectRemoved(_)).Times(0);

  Indicator::Ptr test_indicator_1(indicators.AddIndicator("indicator-test-1"));
  EXPECT_EQ(indicators.GetIndicator("indicator-test-1"), test_indicator_1);

  indicators_list = indicators.GetIndicators();
  EXPECT_EQ(indicators_list.size(), 1);
  EXPECT_NE(std::find(indicators_list.begin(), indicators_list.end(), test_indicator_1), indicators_list.end());

  EXPECT_CALL(sig_receiver, OnObjectAdded(_));
  EXPECT_CALL(sig_receiver, OnObjectRemoved(_)).Times(0);

  Indicator::Ptr test_indicator_2(indicators.AddIndicator("indicator-test-2"));
  EXPECT_EQ(indicators.GetIndicator("indicator-test-2"), test_indicator_2);

  indicators_list = indicators.GetIndicators();
  EXPECT_EQ(indicators_list.size(), 2);
  EXPECT_NE(std::find(indicators_list.begin(), indicators_list.end(), test_indicator_2), indicators_list.end());

  EXPECT_CALL(sig_receiver, OnObjectAdded(_));
  EXPECT_CALL(sig_receiver, OnObjectRemoved(_)).Times(0);

  Indicator::Ptr test_indicator_3(indicators.AddIndicator("indicator-test-3"));
  EXPECT_EQ(indicators.GetIndicator("indicator-test-3"), test_indicator_3);

  indicators_list = indicators.GetIndicators();
  EXPECT_EQ(indicators_list.size(), 3);
  EXPECT_NE(std::find(indicators_list.begin(), indicators_list.end(), test_indicator_3), indicators_list.end());

  EXPECT_CALL(sig_receiver, OnObjectAdded(_)).Times(0);
  EXPECT_CALL(sig_receiver, OnObjectRemoved(_)).Times(0);

  ASSERT_THAT(indicators.GetIndicator("invalid-indicator-test-4"), IsNull());
  EXPECT_EQ(indicators.GetIndicators().size(), 3);

  // Readding an indicator already there should do nothing
  EXPECT_CALL(sig_receiver, OnObjectAdded(_)).Times(0);
  EXPECT_CALL(sig_receiver, OnObjectRemoved(_)).Times(0);

  Indicator::Ptr test_indicator_3_duplicate(indicators.AddIndicator("indicator-test-3"));
  EXPECT_EQ(indicators.GetIndicator("indicator-test-3"), test_indicator_3);
  EXPECT_EQ(indicators.GetIndicators().size(), 3);
  EXPECT_EQ(test_indicator_3, test_indicator_3_duplicate);

  // Removing the indicators...
  EXPECT_CALL(sig_receiver, OnObjectAdded(_)).Times(0);
  EXPECT_CALL(sig_receiver, OnObjectRemoved(test_indicator_2));

  indicators.RemoveIndicator("indicator-test-2");
  ASSERT_THAT(indicators.GetIndicator("indicator-test-2"), IsNull());

  indicators_list = indicators.GetIndicators();
  EXPECT_EQ(indicators_list.size(), 2);
  EXPECT_EQ(std::find(indicators_list.begin(), indicators_list.end(), test_indicator_2), indicators_list.end());

  EXPECT_CALL(sig_receiver, OnObjectAdded(_)).Times(0);
  EXPECT_CALL(sig_receiver, OnObjectRemoved(test_indicator_1));

  indicators.RemoveIndicator("indicator-test-1");
  ASSERT_THAT(indicators.GetIndicator("indicator-test-1"), IsNull());

  indicators_list = indicators.GetIndicators();
  EXPECT_EQ(indicators_list.size(), 1);
  EXPECT_EQ(std::find(indicators_list.begin(), indicators_list.end(), test_indicator_1), indicators_list.end());

  EXPECT_CALL(sig_receiver, OnObjectAdded(_)).Times(0);
  EXPECT_CALL(sig_receiver, OnObjectRemoved(test_indicator_3));

  indicators.RemoveIndicator("indicator-test-3");
  ASSERT_THAT(indicators.GetIndicator("indicator-test-3"), IsNull());

  indicators_list = indicators.GetIndicators();
  EXPECT_TRUE(indicators_list.empty());

  EXPECT_CALL(sig_receiver, OnObjectAdded(_)).Times(0);
  EXPECT_CALL(sig_receiver, OnObjectRemoved(_)).Times(0);

  indicators.RemoveIndicator("invalid-indicator-test-4");
}

TEST(TestIndicators, ActivateEntry)
{
  MockIndicators indicators;
  indicators.SetupTestChildren();
  SigReceiver::Nice sig_receiver(indicators);

  // Activating Entries from the Indicators class to see if they get updated
  ASSERT_THAT(indicators.GetIndicator("indicator-test-1"), NotNull());

  Entry::Ptr entry12(indicators.GetIndicator("indicator-test-1")->GetEntry("indicator-test-1|entry-2"));
  ASSERT_THAT(entry12, NotNull());
  ASSERT_THAT(entry12->active(), false);
  ASSERT_THAT(entry12->geometry(), nux::Rect());

  EXPECT_CALL(sig_receiver, OnEntryActivated(entry12->id(), nux::Rect(1, 2, 3, 4)));
  indicators.ActivateEntry("indicator-test-1|entry-2", nux::Rect(1, 2, 3, 4));

  EXPECT_EQ(entry12->active(), true);
  EXPECT_EQ(entry12->geometry(), nux::Rect(1, 2, 3, 4));
}

TEST(TestIndicators, ActivateEntryShouldDisactivatePrevious)
{
  MockIndicators indicators;
  indicators.SetupTestChildren();
  SigReceiver::Nice sig_receiver(indicators);

  ASSERT_THAT(indicators.GetIndicator("indicator-test-2"), NotNull());

  Entry::Ptr entry22(indicators.GetIndicator("indicator-test-2")->GetEntry("indicator-test-2|entry-2"));
  ASSERT_THAT(entry22, NotNull());

  indicators.ActivateEntry("indicator-test-2|entry-2", nux::Rect(1, 2, 3, 4));
  ASSERT_THAT(entry22->active(), true);
  ASSERT_THAT(entry22->geometry(), nux::Rect(1, 2, 3, 4));

  // Activating another entry, the previously selected one should be disactivate
  Entry::Ptr entry21(indicators.GetIndicator("indicator-test-2")->GetEntry("indicator-test-2|entry-1"));
  ASSERT_THAT(entry21, NotNull());

  ASSERT_THAT(entry21->active(), false);
  ASSERT_THAT(entry21->geometry(), nux::Rect());

  EXPECT_CALL(sig_receiver, OnEntryActivated(entry21->id(), nux::Rect(4, 3, 2, 1)));
  indicators.ActivateEntry("indicator-test-2|entry-1", nux::Rect(4, 3, 2, 1));

  EXPECT_EQ(entry22->active(), false);
  EXPECT_EQ(entry22->geometry(), nux::Rect());

  EXPECT_EQ(entry21->active(), true);
  EXPECT_EQ(entry21->geometry(), nux::Rect(4, 3, 2, 1));
}

TEST(TestIndicators, ActivateEntryInvalidEmitsNullSignal)
{
  MockIndicators indicators;
  indicators.SetupTestChildren();
  SigReceiver::Nice sig_receiver(indicators);

  ASSERT_THAT(indicators.GetIndicator("indicator-test-1"), NotNull());

  Entry::Ptr entry13(indicators.GetIndicator("indicator-test-1")->GetEntry("indicator-test-1|entry-3"));
  ASSERT_THAT(entry13, NotNull());

  EXPECT_CALL(sig_receiver, OnEntryActivated(entry13->id(), nux::Rect(4, 2, 3, 4)));
  indicators.ActivateEntry("indicator-test-1|entry-3", nux::Rect(4, 2, 3, 4));

  // Activating invalid entry, the previously selected one should be disactivate

  EXPECT_CALL(sig_receiver, OnEntryActivated("", nux::Rect()));
  indicators.ActivateEntry("indicator-entry-invalid", nux::Rect(5, 5, 5, 5));

  EXPECT_EQ(entry13->active(), false);
  EXPECT_EQ(entry13->geometry(), nux::Rect());
}

TEST(TestIndicators, SetEntryShowNow)
{
  MockIndicators indicators;
  indicators.SetupTestChildren();

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

  EXPECT_CALL(indicators, OnEntryShowMenu(entry13->id(), 465789, 35, 53, 2));
  entry13->ShowMenu(465789, 35, 53, 2);

  EXPECT_CALL(indicators, OnEntryShowMenu(entry13->id(), 0, 55, 68, 3));
  entry13->ShowMenu(55, 68, 3);
}

TEST(TestIndicators, EntryScroll)
{
  MockIndicators indicators;
  indicators.SetupTestChildren();

  // See if the indicators class get notified on entries actions
  ASSERT_THAT(indicators.GetIndicator("indicator-test-1"), NotNull());

  Entry::Ptr entry11(indicators.GetIndicator("indicator-test-1")->GetEntry("indicator-test-1|entry-1"));
  ASSERT_THAT(entry11, NotNull());

  EXPECT_CALL(indicators, OnEntryScroll(entry11->id(), 80));
  entry11->Scroll(80);
}

TEST(TestIndicators, EntrySecondaryActivate)
{
  MockIndicators indicators;
  indicators.SetupTestChildren();

  // See if the indicators class get notified on entries actions
  ASSERT_THAT(indicators.GetIndicator("indicator-test-2"), NotNull());

  Entry::Ptr entry22(indicators.GetIndicator("indicator-test-2")->GetEntry("indicator-test-2|entry-1"));
  ASSERT_THAT(entry22, NotNull());

  EXPECT_CALL(indicators, OnEntrySecondaryActivate(entry22->id()));
  entry22->SecondaryActivate();
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

    EXPECT_CALL(indicators, OnShowAppMenu(4356789, 54, 13));
    appmenu_indicator->ShowAppmenu(4356789, 54, 13);
  }
}

}
