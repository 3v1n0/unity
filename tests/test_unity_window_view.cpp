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
#include "UnitySettings.h"
#include "WindowManager.h"
#include "test_utils.h"

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
    nux::Geometry GetBackgroundGeometry() { return nux::Geometry(); }

    MOCK_METHOD0(QueueDraw, void());

    using UnityWindowView::FindKeyFocusArea;
    using UnityWindowView::internal_layout_;
    using UnityWindowView::bg_helper_;
    using UnityWindowView::close_button_;
  };

  Settings settings;
  testing::NiceMock<MockUnityWindowView> view;
};

TEST_F(TestUnityWindowView, Construct)
{
  EXPECT_FALSE(view.live_background());
  EXPECT_NE(view.style(), nullptr);
  EXPECT_FALSE(view.closable());
  EXPECT_EQ(view.internal_layout_, nullptr);
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

  EXPECT_EQ(view.close_button_->texture(), view.style()->GetCloseIcon());
  EXPECT_EQ(view.close_button_->GetParentObject(), &view);

  int padding = view.style()->GetCloseButtonPadding();
  EXPECT_EQ(view.close_button_->GetBaseX(), padding);
  EXPECT_EQ(view.close_button_->GetBaseY(), padding);
}

TEST_F(TestUnityWindowView, CloseButtonStates)
{
  view.closable = true;
  ASSERT_NE(view.close_button_, nullptr);

  view.close_button_->mouse_enter.emit(0, 0, 0, 0);
  EXPECT_EQ(view.close_button_->texture(), view.style()->GetCloseIconHighligted());

  view.close_button_->mouse_leave.emit(0, 0, 0, 0);
  EXPECT_EQ(view.close_button_->texture(), view.style()->GetCloseIcon());

  view.close_button_->mouse_down.emit(0, 0, 0, 0);
  EXPECT_EQ(view.close_button_->texture(), view.style()->GetCloseIconPressed());

  view.close_button_->mouse_up.emit(0, 0, 0, 0);
  EXPECT_EQ(view.close_button_->texture(), view.style()->GetCloseIcon());
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
  ASSERT_NE(view.close_button_, nullptr);

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
  ASSERT_NE(view.close_button_, nullptr);

  auto& close_key = WindowManager::Default().close_window_key;
  close_key = std::make_pair(nux::KEY_MODIFIER_ALT, g_random_int());

  bool close_requested = false;
  view.request_close.connect([&close_requested] { close_requested = true; });

  unsigned sent_modifier = close_key().second|nux::KEY_MODIFIER_CAPS_LOCK;
  view.FindKeyFocusArea(nux::NUX_KEYDOWN, sent_modifier, close_key().first);
  EXPECT_TRUE(close_requested);
}

TEST_F(TestUnityWindowView, QueueDrawsOnCloseTextureUpdate)
{
  view.closable = true;
  ASSERT_NE(view.close_button_, nullptr);

  EXPECT_CALL(view, QueueDraw());
  view.close_button_->texture_updated(nux::ObjectPtr<nux::BaseTexture>());
}


} // ui
} // unity