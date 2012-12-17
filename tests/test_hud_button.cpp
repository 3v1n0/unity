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

#include <gtest/gtest.h>

#include <Nux/Nux.h>
#include <Nux/Layout.h>
#include <UnityCore/Hud.h>

#include "hud/HudButton.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/UnitySettings.h"

using namespace unity;

namespace
{

TEST(TestHudButton, TestLabelOpacity)
{
  Settings unity_settings;
  dash::Style dash_style;
  nux::ObjectPtr<hud::HudButton> button(new hud::HudButton());
  nux::Layout* layout = button->GetLayout();

  ASSERT_NE(layout, nullptr);
  ASSERT_EQ(layout->GetChildren().size(), 0);

  hud::Query::Ptr query(new hud::Query("<b>Op</b> Fi<b>le</b>", "","", "", "", NULL));
  button->SetQuery(query);

  auto children(layout->GetChildren());
  ASSERT_EQ(children.size(), 3);

  auto it = children.begin();
  StaticCairoText* label = dynamic_cast<StaticCairoText*>(*it);
  ASSERT_NE(label, nullptr);
  EXPECT_EQ(label->GetText(), "Op");
  EXPECT_EQ(label->GetTextColor().alpha, 1.0f);

  it++;
  label = dynamic_cast<StaticCairoText*>(*it);
  ASSERT_NE(label, nullptr);
  EXPECT_EQ(label->GetText(), " Fi");
  EXPECT_EQ(label->GetTextColor().alpha, 0.5f);

  it++;
  label = dynamic_cast<StaticCairoText*>(*it);
  ASSERT_NE(label, nullptr);
  EXPECT_EQ(label->GetText(), "le");
  EXPECT_EQ(label->GetTextColor().alpha, 1.0f);
}

}
