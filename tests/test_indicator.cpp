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

#include <UnityCore/Indicator.h>

#include <gtest/gtest.h>

using namespace std;
using namespace unity;
using namespace indicator;

namespace
{

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

  // Connecting to signals
  Indicator::Entries added;
  indicator.on_entry_added.connect([&added] (Entry::Ptr const& e) {
    added.push_back(e);
  });

  std::vector<std::string> removed;
  sigc::connection removed_connection = indicator.on_entry_removed.connect([&removed]
  (std::string const& en_id) {
    removed.push_back(en_id);
  });

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
  indicator.Sync(sync_data);
  EXPECT_EQ(indicator.GetEntries().size(), 3);
  EXPECT_EQ(indicator.GetEntry("test-entry-2"), entry2);
  EXPECT_EQ(indicator.EntryIndex("test-entry-2"), 1);
  EXPECT_EQ(added.size(), 3);
  EXPECT_EQ(added.front()->id(), "test-entry-1");
  EXPECT_EQ(added.back()->id(), "test-entry-3");
  EXPECT_EQ(removed.size(), 0);

  added.clear();
  removed.clear();

  // Sync the indicator removing an entry
  sync_data.remove(entry2);
  EXPECT_EQ(sync_data.size(), 2);

  indicator.Sync(sync_data);
  EXPECT_EQ(indicator.GetEntries().size(), 2);
  EXPECT_EQ(indicator.GetEntry("test-entry-2"), nullptr);
  EXPECT_EQ(indicator.EntryIndex("test-entry-2"), -1);
  EXPECT_EQ(added.size(), 0);
  EXPECT_EQ(removed.size(), 1);
  EXPECT_EQ(removed.front(), entry2->id());

  // Sync the indicator removing an entry and adding a new one
  entry = new Entry("test-entry-4", "name-hint", "label", true, true, 0, "icon",
                    true, true, -1);
  Entry::Ptr entry4(entry);
  sync_data.push_back(entry4);
  sync_data.remove(entry3);
  EXPECT_EQ(sync_data.size(), 2);

  added.clear();
  removed.clear();

  indicator.Sync(sync_data);
  EXPECT_EQ(indicator.GetEntries().size(), 2);
  EXPECT_EQ(added.size(), 1);
  EXPECT_EQ(added.front(), entry4);
  EXPECT_EQ(removed.size(), 1);
  EXPECT_EQ(removed.front(), entry3->id());

  // Remove all the indicators
  added.clear();
  removed.clear();

  sync_data.clear();
  indicator.Sync(sync_data);
  EXPECT_EQ(indicator.GetEntries().size(), 0);
  EXPECT_EQ(added.size(), 0);
  EXPECT_EQ(removed.size(), 2);

  removed_connection.disconnect();
}

TEST(TestIndicator, ChildrenSignals)
{
  Indicator::Entries sync_data;
  Entry* entry;

  Indicator indicator("indicator-test");

  entry = new Entry("test-entry-1", "name-hint", "label", true, true, 0, "icon",
                    true, true, -1);
  Entry::Ptr entry1(entry);
  sync_data.push_back(entry1);

  indicator.Sync(sync_data);

  std::string show_entry;
  int show_x, show_y;
  unsigned int show_xid, show_button;

  // Connecting to signals
  indicator.on_show_menu.connect([&] (std::string const& eid, unsigned int xid,
                                      int x, int y, unsigned int button) {
    show_entry = eid;
    show_xid = xid;
    show_x = x;
    show_y = y;
    show_button = button;
  });

  entry1->ShowMenu(123456789, 50, 100, 2);
  EXPECT_EQ(show_entry, "test-entry-1");
  EXPECT_EQ(show_xid, 123456789);
  EXPECT_EQ(show_x, 50);
  EXPECT_EQ(show_y, 100);
  EXPECT_EQ(show_button, 2);

  // Check if a removed entry still emits a signal to the old indicator
  show_entry = "invalid-entry";
  sync_data.remove(entry1);
  indicator.Sync(sync_data);

  entry1->ShowMenu(123456789, 50, 100, 2);
  EXPECT_EQ(show_entry, "invalid-entry");

  // Checking secondary activate signal
  entry = new Entry("test-entry-2", "name-hint", "label", true, true, 0, "icon",
                      true, true, -1);
  sync_data.push_back(Entry::Ptr(entry));
  indicator.Sync(sync_data);

  std::string secondary_activated;
  indicator.on_secondary_activate.connect([&] (std::string const& eid) {
    secondary_activated = eid;
  });

  entry->SecondaryActivate();
  EXPECT_EQ(secondary_activated, "test-entry-2");

  // Checking scroll signal
  std::string scrolled_entry;
  int scrolled_delta;

  indicator.on_scroll.connect([&] (std::string const& eid, int delta) {
    scrolled_entry = eid;
    scrolled_delta = delta;
  });

  entry->Scroll(-5);
  EXPECT_EQ(scrolled_entry, "test-entry-2");
  EXPECT_EQ(scrolled_delta, -5);
}

}
