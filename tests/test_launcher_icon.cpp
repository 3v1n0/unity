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
#include <Nux/NuxTimerTickSource.h>

#include "LauncherIcon.h"
#include "UnitySettings.h"

using namespace unity;
using namespace unity::launcher;
using namespace testing;

namespace
{
typedef nux::animation::Animation::State AnimationState;

struct MockLauncherIcon : LauncherIcon
{
  MockLauncherIcon(IconType type = AbstractLauncherIcon::IconType::APPLICATION)
    : LauncherIcon(type)
  {}

  virtual nux::BaseTexture* GetTextureForSize(int size) { return nullptr; }

  using LauncherIcon::FullyAnimateQuirk;
  using LauncherIcon::SkipQuirkAnimation;
  using LauncherIcon::GetQuirkAnimation;
  using LauncherIcon::EmitNeedsRedraw;
};

struct TestLauncherIcon : Test
{
  TestLauncherIcon()
    : animation_controller(tick_source_)
    , animations_tick_(0)
  {}

  void AnimateIconQuirk(AbstractLauncherIcon* icon, AbstractLauncherIcon::Quirk quirk)
  {
    const int duration = 1;
    icon->SetQuirkDuration(quirk, 1);
    animations_tick_ += duration * 1000;
    tick_source_.tick(animations_tick_);
  }

  void SetIconFullyVisible(AbstractLauncherIcon* icon)
  {
    icon->SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, true);
    AnimateIconQuirk(icon, AbstractLauncherIcon::Quirk::VISIBLE);
    ASSERT_TRUE(icon->IsVisible());
  }

  nux::NuxTimerTickSource tick_source_;
  nux::animation::AnimationController animation_controller;
  unsigned animations_tick_;
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

  MOCK_METHOD2(Redraw, void(AbstractLauncherIcon::Ptr const&, int monitor));
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

      float expected_progress = (quirk == AbstractLauncherIcon::Quirk::CENTER_SAVED) ? 1.0f : 0.0f;

      ASSERT_FLOAT_EQ(expected_progress, icon.GetQuirkProgress(quirk, i));
      ASSERT_EQ(AnimationState::Stopped, icon.GetQuirkAnimation(quirk, i).CurrentState());
    }
  }
}

TEST_P(/*TestLauncherIcon*/Quirks, SetQuirkNewSingleMonitor)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());
  auto* mock_icon = static_cast<MockLauncherIcon*>(icon_ptr.GetPointer());

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    SigReceiver::Nice receiver(icon_ptr);
    EXPECT_CALL(receiver, Redraw(_, i)).Times((GetParam() == AbstractLauncherIcon::Quirk::VISIBLE) ? AtLeast(1) : Exactly(0));
    EXPECT_CALL(receiver, Visible(i)).Times((GetParam() == AbstractLauncherIcon::Quirk::VISIBLE) ? 1 : 0);

    icon_ptr->SetQuirk(GetParam(), true, i);
    ASSERT_TRUE(icon_ptr->GetQuirk(GetParam(), i));
    ASSERT_FLOAT_EQ(1.0f, mock_icon->GetQuirkAnimation(GetParam(), i).GetFinishValue());
    ASSERT_EQ(AnimationState::Running, mock_icon->GetQuirkAnimation(GetParam(), i).CurrentState());
  }
}

TEST_P(/*TestLauncherIcon*/Quirks, SetQuirkNewAllMonitors)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());
  auto* mock_icon = static_cast<MockLauncherIcon*>(icon_ptr.GetPointer());

  SigReceiver::Nice receiver(icon_ptr);

  for (unsigned i = 0; i < monitors::MAX; ++i)
    EXPECT_CALL(receiver, Redraw(_, i)).Times((GetParam() == AbstractLauncherIcon::Quirk::VISIBLE) ? AtLeast(1) : Exactly(0));

  EXPECT_CALL(receiver, Visible(-1)).Times((GetParam() == AbstractLauncherIcon::Quirk::VISIBLE) ? 1 : 0);

  icon_ptr->SetQuirk(GetParam(), true);
  ASSERT_TRUE(icon_ptr->GetQuirk(GetParam()));

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    ASSERT_TRUE(icon_ptr->GetQuirk(GetParam(), i));
    ASSERT_EQ(AnimationState::Running, mock_icon->GetQuirkAnimation(GetParam(), i).CurrentState());
  }
}

TEST_P(/*TestLauncherIcon*/Quirks, SetQuirkOldSingleMonitor)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());
  auto* mock_icon = static_cast<MockLauncherIcon*>(icon_ptr.GetPointer());

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    SigReceiver::Nice receiver(icon_ptr);
    EXPECT_CALL(receiver, Redraw(_, _)).Times(0);
    EXPECT_CALL(receiver, Visible(_)).Times(0);

    icon_ptr->SetQuirk(GetParam(), false, i);

    ASSERT_FALSE(icon_ptr->GetQuirk(GetParam(), i));
    ASSERT_EQ(AnimationState::Stopped, mock_icon->GetQuirkAnimation(GetParam(), i).CurrentState());
  }
}

TEST_P(/*TestLauncherIcon*/Quirks, SetQuirkOldAllMonitors)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());
  auto* mock_icon = static_cast<MockLauncherIcon*>(icon_ptr.GetPointer());

  SigReceiver::Nice receiver(icon_ptr);
  EXPECT_CALL(receiver, Redraw(_, _)).Times(0);
  EXPECT_CALL(receiver, Visible(_)).Times(0);

  icon_ptr->SetQuirk(GetParam(), false);
  ASSERT_FALSE(icon_ptr->GetQuirk(GetParam()));

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    ASSERT_FALSE(icon_ptr->GetQuirk(GetParam(), i));
    ASSERT_EQ(AnimationState::Stopped, mock_icon->GetQuirkAnimation(GetParam(), i).CurrentState());
  }
}

TEST_P(/*TestLauncherIcon*/Quirks, FullyAnimateQuirkSingleMonitor)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());
  auto* mock_icon = static_cast<MockLauncherIcon*>(icon_ptr.GetPointer());
  SetIconFullyVisible(mock_icon);

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    SigReceiver::Nice receiver(icon_ptr);
    EXPECT_CALL(receiver, Redraw(_, i)).Times(AtLeast(1));
    mock_icon->FullyAnimateQuirk(GetParam(), i);

    AnimateIconQuirk(mock_icon, GetParam());
    ASSERT_FLOAT_EQ(1.0f, mock_icon->GetQuirkProgress(GetParam(), i));
  }
}

TEST_P(/*TestLauncherIcon*/Quirks, FullyAnimateQuirkAllMonitors)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());
  auto* mock_icon = static_cast<MockLauncherIcon*>(icon_ptr.GetPointer());
  SetIconFullyVisible(mock_icon);

  SigReceiver::Nice receiver(icon_ptr);

  for (unsigned i = 0; i < monitors::MAX; ++i)
    EXPECT_CALL(receiver, Redraw(_, i)).Times(AtLeast(1));

  mock_icon->FullyAnimateQuirk(GetParam());
  AnimateIconQuirk(mock_icon, GetParam());

  for (unsigned i = 0; i < monitors::MAX; ++i)
    ASSERT_FLOAT_EQ(1.0f, mock_icon->GetQuirkProgress(GetParam(), i));
}

TEST_P(/*TestLauncherIcon*/Quirks, SkipQuirkAnimationSingleMonitor)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());
  auto* mock_icon = static_cast<MockLauncherIcon*>(icon_ptr.GetPointer());

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    mock_icon->SetQuirk(GetParam(), true, i);
    mock_icon->SkipQuirkAnimation(GetParam(), i);
    ASSERT_FLOAT_EQ(1.0f, mock_icon->GetQuirkProgress(GetParam(), i));
  }
}

TEST_P(/*TestLauncherIcon*/Quirks, SkipQuirkAnimationAllMonitors)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());
  auto* mock_icon = static_cast<MockLauncherIcon*>(icon_ptr.GetPointer());

  mock_icon->SetQuirk(GetParam(), true);
  mock_icon->SkipQuirkAnimation(GetParam());

  for (unsigned i = 0; i < monitors::MAX; ++i)
    ASSERT_FLOAT_EQ(1.0f, mock_icon->GetQuirkProgress(GetParam(), i));
}

TEST_P(/*TestLauncherIcon*/Quirks, SetQuirkDurationSingleMonitor)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());
  auto* mock_icon = static_cast<MockLauncherIcon*>(icon_ptr.GetPointer());

  int duration = g_random_int();
  icon_ptr->SetQuirkDuration(GetParam(), duration);

  for (unsigned i = 0; i < monitors::MAX; ++i)
    ASSERT_EQ(duration, mock_icon->GetQuirkAnimation(GetParam(), i).Duration());
}

TEST_P(/*TestLauncherIcon*/Quirks, SetQuirkDurationAllMonitors)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());
  auto* mock_icon = static_cast<MockLauncherIcon*>(icon_ptr.GetPointer());

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    int duration = g_random_int();
    icon_ptr->SetQuirkDuration(GetParam(), duration, i);
    ASSERT_EQ(duration, mock_icon->GetQuirkAnimation(GetParam(), i).Duration());
  }
}

TEST_F(TestLauncherIcon, NeedRedrawInvisibleAllMonitors)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());
  auto* mock_icon = static_cast<MockLauncherIcon*>(icon_ptr.GetPointer());

  SigReceiver::Nice receiver(icon_ptr);
  EXPECT_CALL(receiver, Redraw(icon_ptr, -1));

  mock_icon->EmitNeedsRedraw();
}

TEST_F(TestLauncherIcon, NeedRedrawVisibleAllMonitors)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());
  auto* mock_icon = static_cast<MockLauncherIcon*>(icon_ptr.GetPointer());
  icon_ptr->SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, true);

  SigReceiver::Nice receiver(icon_ptr);
  EXPECT_CALL(receiver, Redraw(icon_ptr, -1));

  mock_icon->EmitNeedsRedraw();
}

TEST_F(TestLauncherIcon, NeedRedrawInvisibleSingleMonitor)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());
  auto* mock_icon = static_cast<MockLauncherIcon*>(icon_ptr.GetPointer());

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    SigReceiver::Nice receiver(icon_ptr);
    EXPECT_CALL(receiver, Redraw(icon_ptr, i)).Times(0);
    mock_icon->EmitNeedsRedraw(i);
  }
}

TEST_F(TestLauncherIcon, NeedRedrawVisibleSingleMonitor)
{
  AbstractLauncherIcon::Ptr icon_ptr(new NiceMock<MockLauncherIcon>());
  auto* mock_icon = static_cast<MockLauncherIcon*>(icon_ptr.GetPointer());
  icon_ptr->SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, true);

  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    SigReceiver::Nice receiver(icon_ptr);
    EXPECT_CALL(receiver, Redraw(icon_ptr, i));
    mock_icon->EmitNeedsRedraw(i);
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

TEST_F(TestLauncherIcon, SetVisiblePresentsAllMonitors)
{
  icon.SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, true);
  EXPECT_TRUE(icon.GetQuirk(AbstractLauncherIcon::Quirk::PRESENTED));
}

TEST_F(TestLauncherIcon, SetVisiblePresentsOneMonitor)
{
  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    icon.SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, true, i);
    ASSERT_TRUE(icon.GetQuirk(AbstractLauncherIcon::Quirk::PRESENTED, i));
    ASSERT_EQ(i == monitors::MAX-1, icon.GetQuirk(AbstractLauncherIcon::Quirk::PRESENTED));
  }
}

TEST_F(TestLauncherIcon, SetUrgentPresentsAllMonitors)
{
  icon.SetQuirk(AbstractLauncherIcon::Quirk::URGENT, true);
  EXPECT_TRUE(icon.GetQuirk(AbstractLauncherIcon::Quirk::PRESENTED));
}

TEST_F(TestLauncherIcon, SetUrgentPresentsOneMonitor)
{
  for (unsigned i = 0; i < monitors::MAX; ++i)
  {
    icon.SetQuirk(AbstractLauncherIcon::Quirk::URGENT, true, i);
    ASSERT_TRUE(icon.GetQuirk(AbstractLauncherIcon::Quirk::PRESENTED, i));
    ASSERT_EQ(i == monitors::MAX-1, icon.GetQuirk(AbstractLauncherIcon::Quirk::PRESENTED));
  }
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
