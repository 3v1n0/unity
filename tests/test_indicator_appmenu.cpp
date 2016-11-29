// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012-2015 Canonical Ltd
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

#include <UnityCore/AppmenuIndicator.h>
#include <gmock/gmock.h>

using namespace std;
using namespace unity;
using namespace indicator;
using namespace testing;

namespace
{
struct SigReceiver : sigc::trackable
{
  typedef NiceMock<SigReceiver> Nice;

  SigReceiver(AppmenuIndicator const& const_indicator)
  {
    auto& indicator = const_cast<AppmenuIndicator&>(const_indicator);
    indicator.updated.connect(sigc::mem_fun(this, &SigReceiver::Updated));
    indicator.updated_win.connect(sigc::mem_fun(this, &SigReceiver::UpdatedWin));
    indicator.on_entry_added.connect(sigc::mem_fun(this, &SigReceiver::EntryAdded));
    indicator.on_entry_removed.connect(sigc::mem_fun(this, &SigReceiver::EntryRemoved));
    indicator.on_show_menu.connect(sigc::mem_fun(this, &SigReceiver::ShowMenu));
    indicator.on_show_appmenu.connect(sigc::mem_fun(this, &SigReceiver::ShowAppmenu));
    indicator.on_secondary_activate.connect(sigc::mem_fun(this, &SigReceiver::SecondaryActivate));
    indicator.on_scroll.connect(sigc::mem_fun(this, &SigReceiver::Scroll));
  }

  MOCK_CONST_METHOD0(Updated, void());
  MOCK_CONST_METHOD1(UpdatedWin, void(uint32_t));
  MOCK_CONST_METHOD1(EntryAdded, void(Entry::Ptr const&));
  MOCK_CONST_METHOD1(EntryRemoved, void(Entry::Ptr const&));
  MOCK_CONST_METHOD5(ShowMenu, void(std::string const&, unsigned, int, int, unsigned));
  MOCK_CONST_METHOD3(ShowAppmenu, void(unsigned, int, int));
  MOCK_CONST_METHOD1(SecondaryActivate, void(std::string const&));
  MOCK_CONST_METHOD2(Scroll, void(std::string const&, int));
};

TEST(TestAppmenuIndicator, Construction)
{
  AppmenuIndicator indicator("indicator-appmenu");

  EXPECT_EQ(indicator.name(), "indicator-appmenu");
  EXPECT_TRUE(indicator.IsAppmenu());
}

TEST(TestAppmenuIndicator, ShowAppmenu)
{
  AppmenuIndicator indicator("indicator-appmenu");
  SigReceiver::Nice sig_receiver(indicator);

  EXPECT_CALL(sig_receiver, ShowAppmenu(123456789, 50, 100));
  indicator.ShowAppmenu(123456789, 50, 100);
}

TEST(TestAppmenuIndicator, Syncing)
{
  Indicator::Entries sync_data;

  AppmenuIndicator indicator("indicator-appmenu");
  SigReceiver::Nice sig_receiver(indicator);
  const uint32_t parent_window1 = 12345;
  const uint32_t parent_window2 = 54321;

  auto entry1 = std::make_shared<Entry>("test-entry-1", "name-hint", parent_window1, "label", true, true, 0, "icon", true, true, -1);
  sync_data.push_back(entry1);

  auto entry2 = std::make_shared<Entry>("test-entry-2", "name-hint", parent_window1, "label", true, true, 0, "icon", true, true, -1);
  sync_data.push_back(entry2);

  auto entry3 = std::make_shared<Entry>("test-entry-3", "name-hint", parent_window2, "label", true, true, 0, "icon", true, true, -1);
  sync_data.push_back(entry3);

  // Sync the indicator, adding 3 entries
  {
    testing::InSequence s;
    EXPECT_CALL(sig_receiver, EntryAdded(entry1));
    EXPECT_CALL(sig_receiver, EntryAdded(entry2));
    EXPECT_CALL(sig_receiver, EntryAdded(entry3));
    EXPECT_CALL(sig_receiver, EntryRemoved(_)).Times(0);
    EXPECT_CALL(sig_receiver, Updated());
  }

  EXPECT_CALL(sig_receiver, UpdatedWin(parent_window1));
  EXPECT_CALL(sig_receiver, UpdatedWin(parent_window2));

  indicator.Sync(sync_data);
  EXPECT_EQ(indicator.GetEntriesForWindow(parent_window1), Indicator::Entries({entry1, entry2}));
  EXPECT_EQ(indicator.GetEntriesForWindow(parent_window2), Indicator::Entries({entry3}));

  // Sync the indicator removing an entry
  sync_data.erase(std::remove(sync_data.begin(), sync_data.end(), entry2), sync_data.end());
  ASSERT_EQ(sync_data.size(), 2u);
  EXPECT_CALL(sig_receiver, Updated());
  EXPECT_CALL(sig_receiver, UpdatedWin(parent_window1));
  EXPECT_CALL(sig_receiver, UpdatedWin(parent_window2)).Times(0);
  EXPECT_CALL(sig_receiver, EntryAdded(_)).Times(0);
  EXPECT_CALL(sig_receiver, EntryRemoved(entry2));

  indicator.Sync(sync_data);
  EXPECT_EQ(indicator.GetEntries().size(), 2u);
  EXPECT_EQ(indicator.GetEntry("test-entry-2"), nullptr);
  EXPECT_EQ(indicator.GetEntriesForWindow(parent_window1), Indicator::Entries({entry1}));

  // Sync the indicator removing an entry and adding a new one
  auto entry4 = std::make_shared<Entry>("test-entry-4", "name-hint", parent_window2, "label", true, true, 0, "icon", true, true, -1);
  sync_data.push_back(entry4);
  sync_data.erase(std::remove(sync_data.begin(), sync_data.end(), entry3), sync_data.end());
  EXPECT_EQ(sync_data.size(), 2u);

  EXPECT_CALL(sig_receiver, EntryAdded(entry4));
  EXPECT_CALL(sig_receiver, EntryRemoved(entry3));
  EXPECT_CALL(sig_receiver, Updated());
  EXPECT_CALL(sig_receiver, UpdatedWin(parent_window1)).Times(0);
  EXPECT_CALL(sig_receiver, UpdatedWin(parent_window2));
  indicator.Sync(sync_data);
  EXPECT_EQ(indicator.GetEntriesForWindow(parent_window1), Indicator::Entries({entry1}));
  EXPECT_EQ(indicator.GetEntriesForWindow(parent_window2), Indicator::Entries({entry4}));

  // Remove all the indicators
  EXPECT_CALL(sig_receiver, EntryAdded(_)).Times(0);
  EXPECT_CALL(sig_receiver, EntryRemoved(entry1));
  EXPECT_CALL(sig_receiver, EntryRemoved(entry4));
  EXPECT_CALL(sig_receiver, Updated());
  EXPECT_CALL(sig_receiver, UpdatedWin(parent_window1));
  EXPECT_CALL(sig_receiver, UpdatedWin(parent_window2));

  sync_data.clear();
  indicator.Sync(sync_data);

  EXPECT_TRUE(indicator.GetEntriesForWindow(parent_window1).empty());
  EXPECT_TRUE(indicator.GetEntriesForWindow(parent_window2).empty());
}

TEST(TestAppmenuIndicator, Updated)
{
  AppmenuIndicator indicator("indicator-test");
  SigReceiver::Nice sig_receiver(indicator);
  Indicator::Entries sync_data;
  const uint32_t parent_window1 = 12345;
  const uint32_t parent_window2 = 54321;

  auto entry1 = std::make_shared<Entry>("test-entry-1", "name-hint", parent_window1, "label", true, true, 0, "icon", true, true, -1);
  sync_data.push_back(entry1);

  auto entry2 = std::make_shared<Entry>("test-entry-2", "name-hint", parent_window2, "label", true, true, 0, "icon", true, true, -1);
  sync_data.push_back(entry2);

  auto entry3 = std::make_shared<Entry>("test-entry-3", "name-hint", parent_window1, "label", true, true, 0, "icon", true, true, -1);
  sync_data.push_back(entry3);

  EXPECT_CALL(sig_receiver, Updated());

  // Sync the indicator, adding 3 entries
  indicator.Sync(sync_data);

  // Readding the same entries, nothing is emitted
  EXPECT_CALL(sig_receiver, Updated()).Times(0);
  indicator.Sync(sync_data);

  sync_data.erase(std::remove(sync_data.begin(), sync_data.end(), entry3), sync_data.end());
  EXPECT_CALL(sig_receiver, Updated());
  indicator.Sync(sync_data);

  sync_data.push_back(entry3);
  EXPECT_CALL(sig_receiver, Updated());
  indicator.Sync(sync_data);
}

TEST(TestAppmenuIndicator, UpdatedWin)
{
  AppmenuIndicator indicator("indicator-test");
  SigReceiver::Nice sig_receiver(indicator);
  Indicator::Entries sync_data;
  const uint32_t parent_window1 = 12345;
  const uint32_t parent_window2 = 54321;

  auto entry1 = std::make_shared<Entry>("test-entry-1", "name-hint", parent_window1, "label", true, true, 0, "icon", true, true, -1);
  sync_data.push_back(entry1);

  auto entry2 = std::make_shared<Entry>("test-entry-2", "name-hint", parent_window2, "label", true, true, 0, "icon", true, true, -1);
  sync_data.push_back(entry2);

  auto entry3 = std::make_shared<Entry>("test-entry-3", "name-hint", parent_window1, "label", true, true, 0, "icon", true, true, -1);
  sync_data.push_back(entry3);

  EXPECT_CALL(sig_receiver, UpdatedWin(parent_window1));
  EXPECT_CALL(sig_receiver, UpdatedWin(parent_window2));

  // Sync the indicator, adding 3 entries
  indicator.Sync(sync_data);

  // Readding the same entries, nothing is emitted
  EXPECT_CALL(sig_receiver, UpdatedWin(_)).Times(0);
  indicator.Sync(sync_data);

  sync_data.erase(std::remove(sync_data.begin(), sync_data.end(), entry3), sync_data.end());
  EXPECT_CALL(sig_receiver, UpdatedWin(entry3->parent_window()));
  indicator.Sync(sync_data);

  sync_data.push_back(entry3);
  EXPECT_CALL(sig_receiver, UpdatedWin(entry3->parent_window()));
  indicator.Sync(sync_data);
}

}
