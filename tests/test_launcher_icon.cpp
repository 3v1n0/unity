/*
 * Copyright 2012-2013 Canonical Ltd.
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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>

#include "LauncherIcon.h"
#include "MultiMonitor.h"
#include "UnitySettings.h"

using namespace unity;
using namespace unity::launcher;
using namespace testing;

namespace
{

struct MockLauncherIcon : LauncherIcon
{
  MockLauncherIcon(IconType type = AbstractLauncherIcon::IconType::APPLICATION)
    : LauncherIcon(type)
  {}

  virtual nux::BaseTexture* GetTextureForSize(int size) { return nullptr; }

  using LauncherIcon::UpdateQuirkTime;
  using LauncherIcon::ResetQuirkTime;
};

struct TestLauncherIcon : Test
{
  unity::Settings settings;
  MockLauncherIcon icon;
};

struct SigReceiver : sigc::trackable
{
  typedef NiceMock<SigReceiver> Nice;

  SigReceiver(AbstractLauncherIcon::Ptr const& icon)
  {
    icon->needs_redraw.connect(sigc::mem_fun(this, &SigReceiver::Redraw));
    icon->visibility_changed.connect(sigc::mem_fun(this, &SigReceiver::Visible));
  }

  MOCK_METHOD1(Redraw, void(AbstractLauncherIcon::Ptr const&));
  MOCK_METHOD1(Visible, void(int monitor));
};

std::vector<AbstractLauncherIcon::Quirk> GetQuirks()
{
  std::vector<AbstractLauncherIcon::Quirk> quirks;
  for (unsigned i = 0; i < unsigned(AbstractLauncherIcon::Quirk::LAST); ++i)
    quirks.push_back(static_cast<AbstractLauncherIcon::Quirk>(i));

  return quirks;
}

struct Quirks : TestLauncherIcon, WithParamInterface<AbstractLauncherIcon::Quirk> {};
INSTANTIATE_TEST_CASE_P(TestLauncherIcon, Quirks, ValuesIn(GetQuirks()));

TEST_F(TestLauncherIcon, Construction)
{
  EXPECT_EQ(icon.GetIconType(), AbstractLauncherIcon::IconType::APPLICATION);
  EXPECT_EQ(icon.position(), AbstractLauncherIcon::Position::FLOATING);
  EXPECT_EQ(icon.SortPriority(), AbstractLauncherIcon::DefaultPriority(AbstractLauncherIcon::IconType::APPLICATION));
  EXPECT_FALSE(icon.IsSticky());
  EXPECT_FALSE(icon.IsVisible());

  for (auto quirk : GetQuirks())
  {
    ASSERT_FALSE(icon.GetQuirk(quirk));

    for (unsigned i = 0; i < monitors::MAX; ++i)
    {
      ASSERT_FALSE(icon.GetQuirk(quirk, i));
      ASSERT_EQ(0, icon.GetQuirkTime(quirk, i).tv_sec);
      ASSERT_EQ(0, icon.GetQuirkTime(quirk, i).tv_nsec);
    }
  }
}

TEST_P(/*TestLauncherIcon*/Quirks, SetQuirkNewSingleMonitor)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    SigReceiver::Nice receiver(icon_ptr);
    EXPECT_CALL(receiver, Redraw(_)).Times(AtLeast(1));
    EXPECT_CALL(receiver, Visible(i)).Times((GetParam() == AbstractLauncherIcon::Quirk::VISIBLE) ? 1 : 0);

    icon_ptr->SetQuirk(GetParam(), true, i);
    time::Spec now;
    now.SetToNow();

    ASSERT_TRUE(icon_ptr->GetQuirk(GetParam(), i));
    ASSERT_GE(now->tv_sec, icon_ptr->GetQuirkTime(GetParam(), i).tv_sec);
    ASSERT_GE(now->tv_nsec, icon_ptr->GetQuirkTime(GetParam(), i).tv_nsec);
  }
}

TEST_P(/*TestLauncherIcon*/Quirks, SetQuirkNewAllMonitors)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());

  SigReceiver::Nice receiver(icon_ptr);
  EXPECT_CALL(receiver, Redraw(_)).Times(AtLeast(1));
  EXPECT_CALL(receiver, Visible(-1)).Times((GetParam() == AbstractLauncherIcon::Quirk::VISIBLE) ? 1 : 0);

  icon_ptr->SetQuirk(GetParam(), true);
  time::Spec now;
  now.SetToNow();

  ASSERT_TRUE(icon_ptr->GetQuirk(GetParam()));

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    ASSERT_TRUE(icon_ptr->GetQuirk(GetParam(), i));
    ASSERT_GE(now->tv_sec, icon_ptr->GetQuirkTime(GetParam(), i).tv_sec);
    ASSERT_GE(now->tv_nsec, icon_ptr->GetQuirkTime(GetParam(), i).tv_nsec);
  }
}

TEST_P(/*TestLauncherIcon*/Quirks, SetQuirkOldSingleMonitor)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    SigReceiver::Nice receiver(icon_ptr);
    EXPECT_CALL(receiver, Redraw(_)).Times(0);
    EXPECT_CALL(receiver, Visible(_)).Times(0);

    icon_ptr->SetQuirk(GetParam(), false, i);

    ASSERT_FALSE(icon_ptr->GetQuirk(GetParam(), i));
    ASSERT_EQ(0, icon_ptr->GetQuirkTime(GetParam(), i).tv_sec);
    ASSERT_EQ(0, icon_ptr->GetQuirkTime(GetParam(), i).tv_nsec);
  }
}

TEST_P(/*TestLauncherIcon*/Quirks, SetQuirkOldAllMonitors)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());

  SigReceiver::Nice receiver(icon_ptr);
  EXPECT_CALL(receiver, Redraw(_)).Times(0);
  EXPECT_CALL(receiver, Visible(_)).Times(0);

  icon_ptr->SetQuirk(GetParam(), false);
  ASSERT_FALSE(icon_ptr->GetQuirk(GetParam()));

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    ASSERT_FALSE(icon_ptr->GetQuirk(GetParam(), i));
    ASSERT_EQ(0, icon_ptr->GetQuirkTime(GetParam(), i).tv_sec);
    ASSERT_EQ(0, icon_ptr->GetQuirkTime(GetParam(), i).tv_nsec);
  }
}

TEST_P(/*TestLauncherIcon*/Quirks, UpdateQuirkTimeSingleMonitor)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    SigReceiver::Nice receiver(icon_ptr);
    EXPECT_CALL(receiver, Redraw(_));

    static_cast<MockLauncherIcon*>(icon_ptr.GetPointer())->UpdateQuirkTime(GetParam(), i);
    time::Spec now;
    now.SetToNow();

    ASSERT_GE(now->tv_sec, icon_ptr->GetQuirkTime(GetParam(), i).tv_sec);
    ASSERT_GE(now->tv_nsec, icon_ptr->GetQuirkTime(GetParam(), i).tv_nsec);
  }
}

TEST_P(/*TestLauncherIcon*/Quirks, UpdateQuirkTimeAllMonitors)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());

  SigReceiver::Nice receiver(icon_ptr);
  EXPECT_CALL(receiver, Redraw(_));

  static_cast<MockLauncherIcon*>(icon_ptr.GetPointer())->UpdateQuirkTime(GetParam());
  time::Spec now;
  now.SetToNow();

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    ASSERT_GE(now->tv_sec, icon_ptr->GetQuirkTime(GetParam(), i).tv_sec);
    ASSERT_GE(now->tv_nsec, icon_ptr->GetQuirkTime(GetParam(), i).tv_nsec);
  }
}

TEST_P(/*TestLauncherIcon*/Quirks, ResetQuirkTimeSingleMonitor)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    static_cast<MockLauncherIcon*>(icon_ptr.GetPointer())->UpdateQuirkTime(GetParam(), i);

    SigReceiver::Nice receiver(icon_ptr);
    EXPECT_CALL(receiver, Redraw(_)).Times(0);
    static_cast<MockLauncherIcon*>(icon_ptr.GetPointer())->ResetQuirkTime(GetParam(), i);

    ASSERT_EQ(0, icon_ptr->GetQuirkTime(GetParam(), i).tv_sec);
    ASSERT_EQ(0, icon_ptr->GetQuirkTime(GetParam(), i).tv_nsec);
  }
}

TEST_P(/*TestLauncherIcon*/Quirks, ResetQuirkTimeAllMonitors)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());
  static_cast<MockLauncherIcon*>(icon_ptr.GetPointer())->UpdateQuirkTime(GetParam());

  SigReceiver::Nice receiver(icon_ptr);
  EXPECT_CALL(receiver, Redraw(_)).Times(0);
  static_cast<MockLauncherIcon*>(icon_ptr.GetPointer())->ResetQuirkTime(GetParam());

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    ASSERT_EQ(0, icon_ptr->GetQuirkTime(GetParam(), i).tv_sec);
    ASSERT_EQ(0, icon_ptr->GetQuirkTime(GetParam(), i).tv_nsec);
  }
}

TEST_F(TestLauncherIcon, Visibility)
{
  ASSERT_FALSE(icon.GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
  ASSERT_FALSE(icon.IsVisible());

  icon.SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, true);
  ASSERT_TRUE(icon.GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
  EXPECT_TRUE(icon.IsVisible());

  icon.SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, false);
  ASSERT_FALSE(icon.GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
  EXPECT_FALSE(icon.IsVisible());
}

TEST_F(TestLauncherIcon, Stick)
{
  bool saved = false;
  icon.position_saved.connect([&saved] {saved = true;});

  icon.Stick(false);
  EXPECT_TRUE(icon.IsSticky());
  EXPECT_TRUE(icon.IsVisible());
  EXPECT_FALSE(saved);

  icon.Stick(true);
  EXPECT_TRUE(saved);
}

TEST_F(TestLauncherIcon, StickAndSave)
{
  bool saved = false;
  icon.position_saved.connect([&saved] {saved = true;});

  icon.Stick(true);
  EXPECT_TRUE(icon.IsSticky());
  EXPECT_TRUE(icon.IsVisible());
  EXPECT_TRUE(saved);
}

TEST_F(TestLauncherIcon, Unstick)
{
  bool forgot = false;
  icon.position_forgot.connect([&forgot] {forgot = true;});

  icon.Stick(false);
  ASSERT_TRUE(icon.IsSticky());
  ASSERT_TRUE(icon.IsVisible());

  icon.UnStick();
  EXPECT_FALSE(icon.IsSticky());
  EXPECT_FALSE(icon.IsVisible());
  EXPECT_TRUE(forgot);
}

TEST_F(TestLauncherIcon, TooltipVisibilityConstruction)
{
  EXPECT_TRUE(icon.tooltip_text().empty());
  EXPECT_TRUE(icon.tooltip_enabled());
}

TEST_F(TestLauncherIcon, TooltipVisibilityValid)
{
  bool tooltip_shown = false;
  icon.tooltip_text = "Unity";
  icon.tooltip_visible.connect([&tooltip_shown] (nux::ObjectPtr<nux::View>) {tooltip_shown = true;});
  icon.ShowTooltip();
  EXPECT_TRUE(tooltip_shown);
}

TEST_F(TestLauncherIcon, TooltipVisibilityEmpty)
{
  bool tooltip_shown = false;
  icon.tooltip_text = "";
  icon.tooltip_visible.connect([&tooltip_shown] (nux::ObjectPtr<nux::View>) {tooltip_shown = true;});
  icon.ShowTooltip();
  EXPECT_FALSE(tooltip_shown);
}

TEST_F(TestLauncherIcon, TooltipVisibilityDisabled)
{
  bool tooltip_shown = false;
  icon.tooltip_text = "Unity";
  icon.tooltip_enabled = false;
  icon.tooltip_visible.connect([&tooltip_shown] (nux::ObjectPtr<nux::View>) {tooltip_shown = true;});
  icon.ShowTooltip();
  EXPECT_FALSE(tooltip_shown);
}

}
