// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>
#include "UnityWindowView.h"
#include "WindowManager.h"
#include <Nux/VLayout.h>

using namespace unity;
using namespace unity::ui;

namespace unity
{
namespace ui
{

struct TestUnityWindowView : testing::Test
{
  struct MockUnityWindowView : UnityWindowView
  {
    void DrawOverlay(nux::GraphicsEngine& ge, bool force, const nux::Geometry& geo) {}
    nux::Geometry GetBackgroundGeometry() { return background_geo_; }

    MOCK_METHOD0(QueueDraw, void());

    using UnityWindowView::GetInternalBackground;
    using UnityWindowView::FindAreaUnderMouse;
    using UnityWindowView::FindKeyFocusArea;
    using UnityWindowView::internal_layout_;
    using UnityWindowView::bg_helper_;
    using UnityWindowView::close_button_;
    using UnityWindowView::bounding_area_;

    nux::Geometry background_geo_;
  };

  testing::NiceMock<MockUnityWindowView> view;
};

TEST_F(TestUnityWindowView, Construct)
{
  EXPECT_FALSE(view.live_background());
  EXPECT_NE(view.style(), nullptr);
  EXPECT_FALSE(view.closable());
  EXPECT_EQ(view.internal_layout_, nullptr);
  EXPECT_EQ(view.bounding_area_, nullptr);
}

TEST_F(TestUnityWindowView, LiveBackgroundChange)
{
  EXPECT_EQ(view.bg_helper_.enabled(), view.live_background());

  view.live_background = true;
  EXPECT_TRUE(view.bg_helper_.enabled());
  EXPECT_TRUE(view.live_background());

  view.live_background = false;
  EXPECT_FALSE(view.bg_helper_.enabled());
  EXPECT_FALSE(view.live_background());
}

TEST_F(TestUnityWindowView, Closable)
{
  EXPECT_EQ(view.close_button_, nullptr);

  view.closable = true;
  ASSERT_NE(view.close_button_, nullptr);

  auto weak_ptr = nux::ObjectWeakPtr<IconTexture>(view.close_button_);
  ASSERT_TRUE(weak_ptr.IsValid());

  EXPECT_EQ(view.close_button_->texture(), view.style()->GetTexture(view.scale, WindowTextureType::CLOSE_ICON));
  EXPECT_EQ(view.close_button_->GetParentObject(), &view);

  int padding = view.style()->GetCloseButtonPadding().CP(view.scale);
  EXPECT_EQ(view.close_button_->GetBaseX(), padding);
  EXPECT_EQ(view.close_button_->GetBaseY(), padding);

  view.closable = false;
  ASSERT_FALSE(weak_ptr.IsValid());
}

TEST_F(TestUnityWindowView, CloseButtonStates)
{
  view.closable = true;
  ASSERT_NE(view.close_button_, nullptr);

  view.close_button_->mouse_enter.emit(0, 0, 0, 0);
  EXPECT_EQ(view.close_button_->texture(), view.style()->GetTexture(view.scale, WindowTextureType::CLOSE_ICON_HIGHLIGHTED));

  view.close_button_->mouse_leave.emit(0, 0, 0, 0);
  EXPECT_EQ(view.close_button_->texture(), view.style()->GetTexture(view.scale, WindowTextureType::CLOSE_ICON));

  view.close_button_->mouse_down.emit(0, 0, 0, 0);
  EXPECT_EQ(view.close_button_->texture(), view.style()->GetTexture(view.scale, WindowTextureType::CLOSE_ICON_PRESSED));

  view.close_button_->mouse_up.emit(0, 0, 0, 0);
  EXPECT_EQ(view.close_button_->texture(), view.style()->GetTexture(view.scale, WindowTextureType::CLOSE_ICON));
}

TEST_F(TestUnityWindowView, CloseButtonClicksRequestsClose)
{
  view.closable = true;
  ASSERT_NE(view.close_button_, nullptr);

  bool close_requested = false;
  view.request_close.connect([&close_requested] { close_requested = true; });

  view.close_button_->mouse_click.emit(0, 0, 0, 0);
  EXPECT_TRUE(close_requested);
}

TEST_F(TestUnityWindowView, WindowManagerCloseKeyRequestsClose)
{
  view.closable = true;

  auto& close_key = WindowManager::Default().close_window_key;
  close_key = std::make_pair(nux::KEY_MODIFIER_ALT, g_random_int());

  bool close_requested = false;
  view.request_close.connect([&close_requested] { close_requested = true; });

  view.FindKeyFocusArea(nux::NUX_KEYDOWN, close_key().second, close_key().first);
  EXPECT_TRUE(close_requested);
}

TEST_F(TestUnityWindowView, WindowManagerCloseKeyRequestsCloseWithCaps)
{
  view.closable = true;

  auto& close_key = WindowManager::Default().close_window_key;
  close_key = std::make_pair(nux::KEY_MODIFIER_ALT, g_random_int());

  bool close_requested = false;
  view.request_close.connect([&close_requested] { close_requested = true; });

  unsigned long sent_modifier = close_key().first|nux::KEY_MODIFIER_CAPS_LOCK;
  view.FindKeyFocusArea(nux::NUX_KEYDOWN, close_key().second, sent_modifier);
  EXPECT_TRUE(close_requested);
}

TEST_F(TestUnityWindowView, EscapeKeyRequestsClose)
{
  view.closable = true;

  bool close_requested = false;
  view.request_close.connect([&close_requested] { close_requested = true; });

  view.FindKeyFocusArea(nux::NUX_KEYDOWN, NUX_VK_ESCAPE, 0);
  EXPECT_TRUE(close_requested);

  close_requested = false;
  view.closable = false;
  EXPECT_FALSE(close_requested);
}

TEST_F(TestUnityWindowView, QueueDrawsOnCloseTextureUpdate)
{
  view.closable = true;
  ASSERT_NE(view.close_button_, nullptr);

  EXPECT_CALL(view, QueueDraw());
  view.close_button_->texture_updated(nux::ObjectPtr<nux::BaseTexture>());
}

TEST_F(TestUnityWindowView, QueueDrawsOnBgUpdated)
{
  EXPECT_CALL(view, QueueDraw());
  view.background_color = nux::color::RandomColor();
}

TEST_F(TestUnityWindowView, SetLayoutWrapsOriginalLayout)
{
  auto* layout = new nux::VLayout();
  view.SetLayout(layout);
  view.ComputeContentSize();

  int offset = view.style()->GetInternalOffset().CP(view.scale);
  EXPECT_EQ(layout->GetBaseX(), offset);
  EXPECT_EQ(layout->GetBaseY(), offset);
}

TEST_F(TestUnityWindowView, GetLayout)
{
  auto* layout = new nux::VLayout();
  view.SetLayout(layout);
  EXPECT_EQ(view.GetLayout(), layout);
}

TEST_F(TestUnityWindowView, GetInternalBackground)
{
  int offset = view.style()->GetInternalOffset().CP(view.scale);
  view.background_geo_.Set(g_random_int(), g_random_int(), g_random_int(), g_random_int());
  EXPECT_EQ(view.GetInternalBackground(), view.background_geo_.GetExpand(-offset, -offset));
}

TEST_F(TestUnityWindowView, GetBoundingArea)
{
  auto const& input_area = view.GetBoundingArea();
  ASSERT_NE(input_area, nullptr);
  ASSERT_EQ(input_area, view.bounding_area_);
  EXPECT_EQ(input_area->GetGeometry(), view.GetGeometry());
}

TEST_F(TestUnityWindowView, BoundingAreaMatchesGeometry)
{
  auto const& input_area = view.GetBoundingArea();
  ASSERT_NE(input_area, nullptr);

  view.SetGeometry(nux::Geometry(g_random_int(), g_random_int(), g_random_int(), g_random_int()));
  EXPECT_EQ(input_area->GetGeometry(), view.GetGeometry());

  view.SetGeometry(nux::Geometry(g_random_int(), g_random_int(), g_random_int(), g_random_int()));
  EXPECT_EQ(input_area->GetGeometry(), view.GetGeometry());
}

TEST_F(TestUnityWindowView, FindAreaUnderMouse)
{
  auto* layout = new nux::VLayout();
  layout->SetSize(30, 40);
  view.SetLayout(layout);
  view.ComputeContentSize();
  auto const& input_area = view.GetBoundingArea();

  nux::Point event_pos(layout->GetAbsoluteX(), layout->GetAbsoluteY());
  EXPECT_EQ(view.FindAreaUnderMouse(event_pos, nux::NUX_MOUSE_MOVE), &view);

  event_pos = nux::Point(input_area->GetAbsoluteX(), input_area->GetAbsoluteY());
  EXPECT_EQ(view.FindAreaUnderMouse(event_pos, nux::NUX_MOUSE_MOVE), input_area.GetPointer());
}

} // ui
} // unity
