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

    using UnityWindowView::internal_layout_;
  };

  Settings settings;
  MockUnityWindowView view;
};

TEST_F(TestUnityWindowView, Construct)
{
  EXPECT_FALSE(view.live_background());
  EXPECT_NE(view.style(), nullptr);
  EXPECT_FALSE(view.closable());
  EXPECT_EQ(view.internal_layout_, nullptr);
}

} // ui
} // unity