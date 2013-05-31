/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 *
 */

#include <list>

#include <gtest/gtest.h>

#include <Nux/Nux.h>
#include <NuxCore/ObjectPtr.h>

#include "UnityCore/Hud.h"

#include "hud/HudView.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UnitySettings.h"

using namespace unity;

namespace
{

TEST(TestHudView, TestSetQueries)
{
  Settings unity_settings;
  dash::Style dash_style;
  panel::Style panel_style;
  nux::ObjectPtr<hud::View> view(new hud::View());

  hud::Hud::Queries queries;
  queries.push_back(hud::Query::Ptr(new hud::Query("1", "","", "", "", NULL)));
  queries.push_back(hud::Query::Ptr(new hud::Query("2", "","", "", "", NULL)));
  queries.push_back(hud::Query::Ptr(new hud::Query("3", "","", "", "", NULL)));
  queries.push_back(hud::Query::Ptr(new hud::Query("4", "","", "", "", NULL)));
  view->SetQueries(queries);

  ASSERT_EQ(view->buttons().size(), 4);

  auto it = view->buttons().begin();
  EXPECT_EQ((*it)->label, "4");
  EXPECT_TRUE((*it)->is_rounded);
  EXPECT_FALSE((*it)->fake_focused);

  it++;
  EXPECT_EQ((*it)->label, "3");
  EXPECT_FALSE((*it)->is_rounded);
  EXPECT_FALSE((*it)->fake_focused);

  it++;
  EXPECT_EQ((*it)->label, "2");
  EXPECT_FALSE((*it)->is_rounded);
  EXPECT_FALSE((*it)->fake_focused);

  it++;
  EXPECT_EQ((*it)->label, "1");
  EXPECT_FALSE((*it)->is_rounded);
  EXPECT_TRUE((*it)->fake_focused);
}

}
