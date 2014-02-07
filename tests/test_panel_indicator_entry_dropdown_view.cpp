/*
 * Copyright 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 *
 */

#include <gmock/gmock.h>

#include <Nux/Nux.h>
#include "PanelIndicatorEntryDropdownView.h"
#include "PanelStyle.h"
#include "UnitySettings.h"

#include "mock_indicators.h"
#include "test_standalone_wm.h"

namespace unity
{
namespace panel
{
namespace
{
using namespace testing;
using namespace indicator;

struct SigReceiver : sigc::trackable
{
  typedef NiceMock<SigReceiver> Nice;

  SigReceiver(PanelIndicatorEntryDropdownView const& const_dropdown)
  {
    auto& dropdown = const_cast<PanelIndicatorEntryDropdownView&>(const_dropdown);
    dropdown.refreshed.connect(sigc::mem_fun(this, &SigReceiver::Refreshed));
    dropdown.active_changed.connect(sigc::mem_fun(this, &SigReceiver::ActiveChanged));
  }

  MOCK_CONST_METHOD1(Refreshed, void(PanelIndicatorEntryView*));
  MOCK_CONST_METHOD2(ActiveChanged, void(PanelIndicatorEntryView*, bool));
};

struct TestPanelIndicatorEntryDropdownView : Test
{
  TestPanelIndicatorEntryDropdownView()
    : indicators_(std::make_shared<MockIndicators::Nice>())
    , dropdown("ID", indicators_)
    , sig_receiver(dropdown)
  {}

  PanelIndicatorEntryView::Ptr BuildEntry(std::string const& id)
  {
    auto entry = std::make_shared<Entry>(id);
    entry->set_label(id, true, true);
    return PanelIndicatorEntryView::Ptr(new PanelIndicatorEntryView(entry));
  }

  Settings settings_;
  Style style_;
  MockIndicators::Ptr indicators_;
  PanelIndicatorEntryDropdownView dropdown;
  SigReceiver::Nice sig_receiver;
  testwrapper::StandaloneWM WM;
};

TEST_F(TestPanelIndicatorEntryDropdownView, Construction)
{
  EXPECT_EQ(std::numeric_limits<int>::max(), dropdown.GetEntryPriority());
  // EXPECT_GT(dropdown.GetBaseWidth(), 0); <- these are failing in jenkins
  // EXPECT_GT(dropdown.GetBaseHeight(), 0); <- these are failing in jenkins
  EXPECT_FALSE(dropdown.IsVisible());
  EXPECT_TRUE(dropdown.Empty());
  EXPECT_EQ("go-down-symbolic", dropdown.GetEntry()->image_data());
}

TEST_F(TestPanelIndicatorEntryDropdownView, PushNull)
{
  dropdown.Push(PanelIndicatorEntryView::Ptr());
  EXPECT_TRUE(dropdown.Empty());
  EXPECT_FALSE(dropdown.IsVisible());
}

TEST_F(TestPanelIndicatorEntryDropdownView, Push)
{
  auto first_entry = BuildEntry("FirstEntry");
  EXPECT_CALL(sig_receiver, Refreshed(&dropdown));

  dropdown.Push(first_entry);
  EXPECT_FALSE(dropdown.Empty());
  EXPECT_EQ(1, dropdown.Size());
  EXPECT_TRUE(dropdown.IsVisible());
  EXPECT_THAT(dropdown.Children(), Contains(first_entry));
  EXPECT_TRUE(first_entry->in_dropdown());
  EXPECT_EQ(first_entry, dropdown.Top());

  EXPECT_EQ(1, first_entry->GetEntry()->parents().size());
  EXPECT_THAT(first_entry->GetEntry()->parents(), Contains(dropdown.GetEntry()));

  auto second_entry = BuildEntry("SecondEntry");
  EXPECT_CALL(sig_receiver, Refreshed(&dropdown)).Times(0);

  dropdown.Push(second_entry);
  EXPECT_FALSE(dropdown.Empty());
  EXPECT_EQ(2, dropdown.Size());
  EXPECT_TRUE(dropdown.IsVisible());
  EXPECT_THAT(dropdown.Children(), Contains(second_entry));
  EXPECT_TRUE(second_entry->in_dropdown());
  EXPECT_EQ(first_entry, dropdown.Top());

  EXPECT_EQ(1, second_entry->GetEntry()->parents().size());
  EXPECT_THAT(second_entry->GetEntry()->parents(), Contains(dropdown.GetEntry()));
}

TEST_F(TestPanelIndicatorEntryDropdownView, Insert)
{
  auto first_entry = BuildEntry("FirstEntry");
  first_entry->GetEntry()->set_priority(100);
  EXPECT_CALL(sig_receiver, Refreshed(&dropdown));

  dropdown.Insert(first_entry);
  EXPECT_FALSE(dropdown.Empty());
  EXPECT_EQ(1, dropdown.Size());
  EXPECT_TRUE(dropdown.IsVisible());
  EXPECT_THAT(dropdown.Children(), Contains(first_entry));
  EXPECT_TRUE(first_entry->in_dropdown());
  EXPECT_EQ(first_entry, dropdown.Top());

  EXPECT_EQ(1, first_entry->GetEntry()->parents().size());
  EXPECT_THAT(first_entry->GetEntry()->parents(), Contains(dropdown.GetEntry()));

  auto second_entry = BuildEntry("SecondEntry");
  second_entry->GetEntry()->set_priority(50);
  EXPECT_CALL(sig_receiver, Refreshed(&dropdown)).Times(0);

  dropdown.Insert(second_entry);
  EXPECT_FALSE(dropdown.Empty());
  EXPECT_EQ(2, dropdown.Size());
  EXPECT_TRUE(dropdown.IsVisible());
  EXPECT_THAT(dropdown.Children(), Contains(second_entry));
  EXPECT_TRUE(second_entry->in_dropdown());
  EXPECT_EQ(second_entry, dropdown.Top());

  EXPECT_EQ(1, second_entry->GetEntry()->parents().size());
  EXPECT_THAT(second_entry->GetEntry()->parents(), Contains(dropdown.GetEntry()));
}

TEST_F(TestPanelIndicatorEntryDropdownView, Remove)
{
  auto first_entry = BuildEntry("FirstEntry");
  dropdown.Push(first_entry);
  auto second_entry = BuildEntry("SecondEntry");
  dropdown.Push(second_entry);

  ASSERT_TRUE(dropdown.IsVisible());
  EXPECT_CALL(sig_receiver, Refreshed(&dropdown)).Times(0);
  dropdown.Remove(second_entry);

  EXPECT_EQ(1, dropdown.Size());
  EXPECT_TRUE(dropdown.IsVisible());
  EXPECT_THAT(dropdown.Children(), Not(Contains(second_entry)));
  EXPECT_FALSE(second_entry->in_dropdown());
  EXPECT_THAT(second_entry->GetEntry()->parents(), Not(Contains(dropdown.GetEntry())));
  EXPECT_EQ(first_entry, dropdown.Top());

  EXPECT_CALL(sig_receiver, Refreshed(&dropdown));
  dropdown.Remove(first_entry);

  EXPECT_TRUE(dropdown.Empty());
  EXPECT_FALSE(dropdown.IsVisible());
  EXPECT_THAT(dropdown.Children(), Not(Contains(first_entry)));
  EXPECT_FALSE(first_entry->in_dropdown());
  EXPECT_THAT(first_entry->GetEntry()->parents(), Not(Contains(dropdown.GetEntry())));
  EXPECT_EQ(PanelIndicatorEntryView::Ptr(), dropdown.Top());
}

TEST_F(TestPanelIndicatorEntryDropdownView, PopEmpty)
{
  EXPECT_EQ(PanelIndicatorEntryView::Ptr(), dropdown.Pop());
}

TEST_F(TestPanelIndicatorEntryDropdownView, Pop)
{
  auto first_entry = BuildEntry("FirstEntry");
  dropdown.Push(first_entry);
  auto second_entry = BuildEntry("SecondEntry");
  dropdown.Push(second_entry);

  ASSERT_TRUE(dropdown.IsVisible());
  EXPECT_CALL(sig_receiver, Refreshed(&dropdown)).Times(0);
  EXPECT_EQ(first_entry, dropdown.Pop());

  EXPECT_EQ(1, dropdown.Size());
  EXPECT_TRUE(dropdown.IsVisible());
  EXPECT_THAT(dropdown.Children(), Not(Contains(first_entry)));
  EXPECT_FALSE(first_entry->in_dropdown());
  EXPECT_THAT(first_entry->GetEntry()->parents(), Not(Contains(dropdown.GetEntry())));
  EXPECT_EQ(second_entry, dropdown.Top());

  EXPECT_CALL(sig_receiver, Refreshed(&dropdown));
  EXPECT_EQ(second_entry, dropdown.Pop());

  EXPECT_TRUE(dropdown.Empty());
  EXPECT_FALSE(dropdown.IsVisible());
  EXPECT_THAT(dropdown.Children(), Not(Contains(second_entry)));
  EXPECT_FALSE(second_entry->in_dropdown());
  EXPECT_THAT(second_entry->GetEntry()->parents(), Not(Contains(dropdown.GetEntry())));
  EXPECT_EQ(PanelIndicatorEntryView::Ptr(), dropdown.Top());
}

TEST_F(TestPanelIndicatorEntryDropdownView, TopEmpty)
{
  EXPECT_EQ(PanelIndicatorEntryView::Ptr(), dropdown.Top());
}

TEST_F(TestPanelIndicatorEntryDropdownView, Top)
{
  std::vector<PanelIndicatorEntryView::Ptr> entries(10);

  for (auto& entry : entries)
  {
    entry = BuildEntry("Entry "+std::to_string(g_random_int()));
    dropdown.Push(entry);
  }

  ASSERT_EQ(entries.size(), dropdown.Size());

  for (auto const& entry : entries)
  {
    ASSERT_EQ(entry, dropdown.Top());
    ASSERT_EQ(entry, dropdown.Pop());
  }

  EXPECT_EQ(PanelIndicatorEntryView::Ptr(), dropdown.Top());
}

TEST_F(TestPanelIndicatorEntryDropdownView, ShowMenuEmpty)
{
  EXPECT_CALL(*indicators_, ShowEntriesDropdown(_, _, _, _, _)).Times(0);
}

TEST_F(TestPanelIndicatorEntryDropdownView, ShowMenu)
{
  Indicator::Entries entries;
  std::vector<PanelIndicatorEntryView::Ptr> views(10);

  for (auto& view : views)
  {
    view = BuildEntry("Entry "+std::to_string(g_random_int()));
    entries.push_back(view->GetEntry());
    dropdown.Push(view);
  }

  auto const& geo = dropdown.GetGeometry();
  EXPECT_CALL(*indicators_, ShowEntriesDropdown(entries, Entry::Ptr(), 0, geo.x, geo.y + Style::Instance().panel_height));
  dropdown.ShowMenu();
}

TEST_F(TestPanelIndicatorEntryDropdownView, ActivateChild)
{
  Indicator::Entries entries;
  std::vector<PanelIndicatorEntryView::Ptr> views(10);

  for (auto& view : views)
  {
    view = BuildEntry("Entry "+std::to_string(g_random_int()));
    entries.push_back(view->GetEntry());
    dropdown.Push(view);
  }

  auto active = views[g_random_int() % views.size()];
  auto const& geo = dropdown.GetGeometry();

  EXPECT_CALL(*indicators_, ShowEntriesDropdown(entries, active->GetEntry(), 0, geo.x, geo.y + Style::Instance().panel_height));
  dropdown.ActivateChild(active);
}

TEST_F(TestPanelIndicatorEntryDropdownView, ChildActivationRefreshesDropdown)
{
  dropdown.Push(BuildEntry("Entry"));

  EXPECT_CALL(sig_receiver, Refreshed(&dropdown));
  EXPECT_CALL(sig_receiver, ActiveChanged(&dropdown, true));
  dropdown.Top()->GetEntry()->set_active(true);

  EXPECT_CALL(sig_receiver, Refreshed(&dropdown));
  EXPECT_CALL(sig_receiver, ActiveChanged(&dropdown, false));
  dropdown.Top()->GetEntry()->set_active(false);

  EXPECT_CALL(sig_receiver, Refreshed(&dropdown));
  auto old_child = dropdown.Pop();

  EXPECT_CALL(sig_receiver, Refreshed(_)).Times(0);
  EXPECT_CALL(sig_receiver, ActiveChanged(_, _)).Times(0);
  old_child->GetEntry()->set_active(true);
}

TEST_F(TestPanelIndicatorEntryDropdownView, ChildShowNowRefreshesDropdown)
{
  dropdown.Push(BuildEntry("Entry"));

  EXPECT_CALL(sig_receiver, Refreshed(&dropdown));
  dropdown.Top()->GetEntry()->set_show_now(true);

  EXPECT_CALL(sig_receiver, Refreshed(&dropdown));
  dropdown.Top()->GetEntry()->set_show_now(false);

  EXPECT_CALL(sig_receiver, Refreshed(&dropdown));
  auto old_child = dropdown.Pop();

  EXPECT_CALL(sig_receiver, Refreshed(_)).Times(0);
  old_child->GetEntry()->set_show_now(true);
}

TEST_F(TestPanelIndicatorEntryDropdownView, ChildGeometryRefreshesDropdown)
{
  dropdown.Push(BuildEntry("Entry"));

  nux::Geometry geo(1, 2, 4, 5);
  EXPECT_CALL(sig_receiver, Refreshed(&dropdown));
  dropdown.Top()->GetEntry()->set_geometry(geo);

  EXPECT_CALL(sig_receiver, Refreshed(&dropdown));
  dropdown.Top()->GetEntry()->set_geometry(nux::Geometry());

  EXPECT_CALL(sig_receiver, Refreshed(&dropdown));
  auto old_child = dropdown.Pop();

  EXPECT_CALL(sig_receiver, Refreshed(_)).Times(0);
  old_child->GetEntry()->set_geometry(geo);
}

} // anonymous namespace
} // panel namespace
} // unity namespace
