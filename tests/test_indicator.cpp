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
#include <UnityCore/Indicator.h>

using namespace std;
using namespace unity;
using namespace indicator;
using namespace testing;

namespace
{
struct SigReceiver : sigc::trackable
{
  typedef NiceMock<SigReceiver> Nice;

  SigReceiver(Indicator const& const_indicator)
  {
    auto& indicator = const_cast<Indicator&>(const_indicator);
    indicator.updated.connect(sigc::mem_fun(this, &SigReceiver::Updated));
    indicator.on_entry_added.connect(sigc::mem_fun(this, &SigReceiver::EntryAdded));
    indicator.on_entry_removed.connect(sigc::mem_fun(this, &SigReceiver::EntryRemoved));
    indicator.on_show_menu.connect(sigc::mem_fun(this, &SigReceiver::ShowMenu));
    indicator.on_show_dropdown_menu.connect(sigc::mem_fun(this, &SigReceiver::ShowDropdownMenu));
    indicator.on_secondary_activate.connect(sigc::mem_fun(this, &SigReceiver::SecondaryActivate));
    indicator.on_scroll.connect(sigc::mem_fun(this, &SigReceiver::Scroll));
  }

  MOCK_CONST_METHOD0(Updated, void());
  MOCK_CONST_METHOD1(EntryAdded, void(Entry::Ptr const&));
  MOCK_CONST_METHOD1(EntryRemoved, void(std::string const&));
  MOCK_CONST_METHOD5(ShowMenu, void(std::string const&, unsigned, int, int, unsigned));
  MOCK_CONST_METHOD4(ShowDropdownMenu, void(std::string const&, unsigned, int, int));
  MOCK_CONST_METHOD1(SecondaryActivate, void(std::string const&));
  MOCK_CONST_METHOD2(Scroll, void(std::string const&, int));
};

TEST(TestIndicator, Construction)
{
  Indicator indicator("indicator-test");

  EXPECT_EQ(indicator.name(), "indicator-test");
  EXPECT_FALSE(indicator.IsAppmenu());
  EXPECT_EQ(indicator.GetEntry("test-entry"), nullptr);
  EXPECT_EQ(indicator.EntryIndex("test-entry"), -1);
  EXPECT_TRUE(indicator.GetEntries().empty());
}

TEST(TestIndicator, Syncing)
{
  Indicator::Entries sync_data;
  Entry* entry;

  Indicator indicator("indicator-test");
  SigReceiver::Nice sig_receiver(indicator);

  entry = new Entry("test-entry-1", "name-hint", "label", true, true, 0, "icon",
                    true, true, -1);
  Entry::Ptr entry1(entry);
  sync_data.push_back(entry1);

  entry = new Entry("test-entry-2", "name-hint", "label", true, true, 0, "icon",
                    true, true, -1);
  Entry::Ptr entry2(entry);
  sync_data.push_back(entry2);

  entry = new Entry("test-entry-3", "name-hint", "label", true, true, 0, "icon",
                    true, true, -1);
  Entry::Ptr entry3(entry);
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

  indicator.Sync(sync_data);
  EXPECT_EQ(indicator.GetEntries().size(), 3);
  EXPECT_EQ(indicator.GetEntry("test-entry-2"), entry2);
  EXPECT_EQ(indicator.EntryIndex("test-entry-2"), 1);
  // Mock::VerifyAndClearExpectations(&sig_receiver);

  // Sync the indicator removing an entry
  sync_data.remove(entry2);
  EXPECT_EQ(sync_data.size(), 2);
  EXPECT_CALL(sig_receiver, Updated());
  EXPECT_CALL(sig_receiver, EntryAdded(_)).Times(0);
  EXPECT_CALL(sig_receiver, EntryRemoved(entry2->id()));

  indicator.Sync(sync_data);
  EXPECT_EQ(indicator.GetEntries().size(), 2);
  EXPECT_EQ(indicator.GetEntry("test-entry-2"), nullptr);
  EXPECT_EQ(indicator.EntryIndex("test-entry-2"), -1);

  // Sync the indicator removing an entry and adding a new one
  entry = new Entry("test-entry-4", "name-hint", "label", true, true, 0, "icon",
                    true, true, -1);
  Entry::Ptr entry4(entry);
  sync_data.push_back(entry4);
  sync_data.remove(entry3);
  EXPECT_EQ(sync_data.size(), 2);

  EXPECT_CALL(sig_receiver, EntryAdded(entry4));
  EXPECT_CALL(sig_receiver, EntryRemoved(entry3->id()));
  EXPECT_CALL(sig_receiver, Updated());
  indicator.Sync(sync_data);
  EXPECT_EQ(indicator.GetEntries().size(), 2);

  // Remove all the indicators

  EXPECT_CALL(sig_receiver, EntryAdded(_)).Times(0);
  EXPECT_CALL(sig_receiver, EntryRemoved(entry1->id()));
  EXPECT_CALL(sig_receiver, EntryRemoved(entry4->id()));
  EXPECT_CALL(sig_receiver, Updated());

  sync_data.clear();
  indicator.Sync(sync_data);
  EXPECT_EQ(indicator.GetEntries().size(), 0);
}

TEST(TestIndicator, Updated)
{
  Indicator indicator("indicator-test");
  SigReceiver::Nice sig_receiver(indicator);
  Indicator::Entries sync_data;

  auto entry1 = std::make_shared<Entry>("test-entry-1", "name-hint", "label", true, true, 0, "icon", true, true, -1);
  sync_data.push_back(entry1);

  auto entry2 = std::make_shared<Entry>("test-entry-2", "name-hint", "label", true, true, 0, "icon", true, true, -1);
  sync_data.push_back(entry2);

  auto entry3 = std::make_shared<Entry>("test-entry-3", "name-hint", "label", true, true, 0, "icon", true, true, -1);
  sync_data.push_back(entry3);

  EXPECT_CALL(sig_receiver, Updated());

  // Sync the indicator, adding 3 entries
  indicator.Sync(sync_data);

  // Readding the same entries, nothing is emitted
  EXPECT_CALL(sig_receiver, Updated()).Times(0);
  indicator.Sync(sync_data);

  sync_data.remove(entry3);
  EXPECT_CALL(sig_receiver, Updated());
  indicator.Sync(sync_data);

  sync_data.push_back(entry3);
  EXPECT_CALL(sig_receiver, Updated());
  indicator.Sync(sync_data);
}

TEST(TestIndicator, ChildrenSignalShowMenu)
{
  Indicator indicator("indicator-test");
  SigReceiver::Nice sig_receiver(indicator);

  auto entry = std::make_shared<Entry>("test-entry-1", "name-hint", "label", true, true, 0, "icon", true, true, -1);
  indicator.Sync({entry});

  EXPECT_CALL(sig_receiver, ShowMenu(entry->id(), 123456789, 50, 100, 2));
  entry->ShowMenu(123456789, 50, 100, 2);

  // Check if a removed entry still emits a signal to the old indicator
  indicator.Sync({});

  entry->ShowMenu(123456789, 50, 100, 2);
  EXPECT_CALL(sig_receiver, ShowMenu(_, _, _, _, _)).Times(0);
}

TEST(TestIndicator, ChildrenSignalShowDropdownMenu)
{
  Indicator indicator("indicator-test");
  SigReceiver::Nice sig_receiver(indicator);

  auto entry = std::make_shared<Entry>("test-entry-1", "name-hint", "label", true, true, 0, "icon", true, true, -1);
  indicator.Sync({entry});

  EXPECT_CALL(sig_receiver, ShowDropdownMenu(entry->id(), 123456789, 50, 101));
  entry->ShowDropdownMenu(123456789, 50, 101);

  // Check if a removed entry still emits a signal to the old indicator
  indicator.Sync({});

  entry->ShowDropdownMenu(123456789, 50, 100);
  EXPECT_CALL(sig_receiver, ShowDropdownMenu(_, _, _, _)).Times(0);
}

TEST(TestIndicator, ChildrenSignalSecondaryActivate)
{
  Indicator indicator("indicator-test");
  SigReceiver::Nice sig_receiver(indicator);

  auto entry = std::make_shared<Entry>("test-entry-2", "name-hint", "label", true, true, 0, "icon", true, true, -1);
  indicator.Sync({entry});

  EXPECT_CALL(sig_receiver, SecondaryActivate(entry->id()));
  entry->SecondaryActivate();
}

TEST(TestIndicator, ChildrenSignalScroll)
{
  Indicator indicator("indicator-test");
  SigReceiver::Nice sig_receiver(indicator);

  auto entry = std::make_shared<Entry>("test-entry-2", "name-hint", "label", true, true, 0, "icon", true, true, -1);
  indicator.Sync({entry});

  EXPECT_CALL(sig_receiver, Scroll(entry->id(), -5));
  entry->Scroll(-5);
}

}
