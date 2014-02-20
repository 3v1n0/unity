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
#include "mock_indicators.h"

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
  MOCK_CONST_METHOD3(OnEntryActivated, void(std::string const&, std::string const&, nux::Rect const&));
  MOCK_CONST_METHOD5(OnEntryShowMenu, void(std::string const&, unsigned, int, int, unsigned));
};

struct TestIndicators : Test
{
  // Utility function used to fill the class with test indicators with entries
  void SetupTestChildren()
  {
    // Adding an indicator filled with entries into the TestMockIndicators
    Indicator::Entries sync_data;
    Entry* entry;

    Indicator::Ptr test_indicator_1 = indicators.AddIndicator("indicator-test-1");

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


    // Adding another indicator filled with entries into the TestMockIndicators
    Indicator::Ptr test_indicator_2 = indicators.AddIndicator("indicator-test-2");
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

    ASSERT_THAT(indicators.GetIndicators().size(), 2);
  }

  MockIndicators::Nice indicators;
};

TEST_F(TestIndicators, Construction)
{
  EXPECT_TRUE(indicators.GetIndicators().empty());
}

TEST_F(TestIndicators, GetInvalidIndicator)
{
  ASSERT_THAT(indicators.GetIndicator("no-available-indicator"), IsNull());
}

TEST_F(TestIndicators, IndicatorsFactory)
{
  Indicator::Ptr standard_indicator = indicators.AddIndicator("libapplication.so");
  EXPECT_EQ(standard_indicator->name(), "libapplication.so");
  EXPECT_FALSE(standard_indicator->IsAppmenu());

  Indicator::Ptr appmenu_indicator = indicators.AddIndicator("libappmenu.so");
  EXPECT_EQ(appmenu_indicator->name(), "libappmenu.so");
  EXPECT_TRUE(appmenu_indicator->IsAppmenu());
}

TEST_F(TestIndicators, IndicatorsHandling)
{
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

TEST_F(TestIndicators, ActivateEntry)
{
  SetupTestChildren();
  SigReceiver::Nice sig_receiver(indicators);

  // Activating Entries from the Indicators class to see if they get updated
  ASSERT_THAT(indicators.GetIndicator("indicator-test-1"), NotNull());

  Entry::Ptr entry12(indicators.GetIndicator("indicator-test-1")->GetEntry("indicator-test-1|entry-2"));
  ASSERT_THAT(entry12, NotNull());
  ASSERT_THAT(entry12->active(), false);
  ASSERT_THAT(entry12->geometry(), nux::Rect());

  EXPECT_CALL(sig_receiver, OnEntryActivated("panel1", entry12->id(), nux::Rect(1, 2, 3, 4)));
  indicators.ActivateEntry("panel1", "indicator-test-1|entry-2", nux::Rect(1, 2, 3, 4));

  EXPECT_EQ(entry12->active(), true);
  EXPECT_EQ(entry12->geometry(), nux::Rect(1, 2, 3, 4));
}

TEST_F(TestIndicators, ActivateEntryShouldDisactivatePrevious)
{
  SetupTestChildren();
  SigReceiver::Nice sig_receiver(indicators);

  ASSERT_THAT(indicators.GetIndicator("indicator-test-2"), NotNull());

  Entry::Ptr entry22(indicators.GetIndicator("indicator-test-2")->GetEntry("indicator-test-2|entry-2"));
  ASSERT_THAT(entry22, NotNull());

  indicators.ActivateEntry("panel0", "indicator-test-2|entry-2", nux::Rect(1, 2, 3, 4));
  ASSERT_THAT(entry22->active(), true);
  ASSERT_THAT(entry22->geometry(), nux::Rect(1, 2, 3, 4));

  // Activating another entry, the previously selected one should be disactivate
  Entry::Ptr entry21(indicators.GetIndicator("indicator-test-2")->GetEntry("indicator-test-2|entry-1"));
  ASSERT_THAT(entry21, NotNull());

  ASSERT_THAT(entry21->active(), false);
  ASSERT_THAT(entry21->geometry(), nux::Rect());

  EXPECT_CALL(sig_receiver, OnEntryActivated("panel1", entry21->id(), nux::Rect(4, 3, 2, 1)));
  indicators.ActivateEntry("panel1", "indicator-test-2|entry-1", nux::Rect(4, 3, 2, 1));

  EXPECT_EQ(entry22->active(), false);
  EXPECT_EQ(entry22->geometry(), nux::Rect());

  EXPECT_EQ(entry21->active(), true);
  EXPECT_EQ(entry21->geometry(), nux::Rect(4, 3, 2, 1));
}

TEST_F(TestIndicators, ActivateEntryInvalidEmitsNullSignal)
{
  SetupTestChildren();
  SigReceiver::Nice sig_receiver(indicators);

  ASSERT_THAT(indicators.GetIndicator("indicator-test-1"), NotNull());

  Entry::Ptr entry13(indicators.GetIndicator("indicator-test-1")->GetEntry("indicator-test-1|entry-3"));
  ASSERT_THAT(entry13, NotNull());

  EXPECT_CALL(sig_receiver, OnEntryActivated("panel0", entry13->id(), nux::Rect(4, 2, 3, 4)));
  indicators.ActivateEntry("panel0", "indicator-test-1|entry-3", nux::Rect(4, 2, 3, 4));

  // Activating invalid entry, the previously selected one should be disactivate

  EXPECT_CALL(sig_receiver, OnEntryActivated("", "", nux::Rect()));
  indicators.ActivateEntry("panel1", "indicator-entry-invalid", nux::Rect(5, 5, 5, 5));

  EXPECT_EQ(entry13->active(), false);
  EXPECT_EQ(entry13->geometry(), nux::Rect());
}

TEST_F(TestIndicators, SetEntryShowNow)
{
  SetupTestChildren();

  ASSERT_THAT(indicators.GetIndicator("indicator-test-2"), NotNull());
  Entry::Ptr entry22(indicators.GetIndicator("indicator-test-2")->GetEntry("indicator-test-2|entry-2"));

  ASSERT_THAT(entry22, NotNull());
  ASSERT_THAT(entry22->show_now(), false);

  indicators.SetEntryShowNow("indicator-test-2|entry-2", true);
  EXPECT_EQ(entry22->show_now(), true);

  indicators.SetEntryShowNow("indicator-test-2|entry-2", false);
  EXPECT_EQ(entry22->show_now(), false);
}

TEST_F(TestIndicators, EntryShowMenu)
{
  SetupTestChildren();

  // See if the indicators class get notified on entries actions
  ASSERT_THAT(indicators.GetIndicator("indicator-test-1"), NotNull());

  Entry::Ptr entry13(indicators.GetIndicator("indicator-test-1")->GetEntry("indicator-test-1|entry-3"));
  ASSERT_THAT(entry13, NotNull());

  EXPECT_CALL(indicators, OnEntryShowMenu(entry13->id(), 465789, 35, 53, 2));
  entry13->ShowMenu(465789, 35, 53, 2);

  EXPECT_CALL(indicators, OnEntryShowMenu(entry13->id(), 0, 55, 68, 3));
  entry13->ShowMenu(55, 68, 3);
}

TEST_F(TestIndicators, EntryScroll)
{
  SetupTestChildren();

  // See if the indicators class get notified on entries actions
  ASSERT_THAT(indicators.GetIndicator("indicator-test-1"), NotNull());

  Entry::Ptr entry11(indicators.GetIndicator("indicator-test-1")->GetEntry("indicator-test-1|entry-1"));
  ASSERT_THAT(entry11, NotNull());

  EXPECT_CALL(indicators, OnEntryScroll(entry11->id(), 80));
  entry11->Scroll(80);
}

TEST_F(TestIndicators, EntrySecondaryActivate)
{
  SetupTestChildren();

  // See if the indicators class get notified on entries actions
  ASSERT_THAT(indicators.GetIndicator("indicator-test-2"), NotNull());

  Entry::Ptr entry22(indicators.GetIndicator("indicator-test-2")->GetEntry("indicator-test-2|entry-1"));
  ASSERT_THAT(entry22, NotNull());

  EXPECT_CALL(indicators, OnEntrySecondaryActivate(entry22->id()));
  entry22->SecondaryActivate();
}

TEST_F(TestIndicators, ShowAppMenu)
{
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
