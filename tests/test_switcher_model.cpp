/*
 * Copyright 2011 Canonical Ltd.
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 *
 */

#include <gtest/gtest.h>

#include "SwitcherModel.h"
#include "MockLauncherIcon.h"
#include "AbstractLauncherIcon.h"

#include <vector>


using namespace unity::switcher;
using unity::launcher::AbstractLauncherIcon;
using unity::launcher::MockLauncherIcon;

namespace
{

TEST(TestSwitcherModel, TestConstructor)
{
  AbstractLauncherIcon::Ptr first(new MockLauncherIcon());
  AbstractLauncherIcon::Ptr second(new MockLauncherIcon());
  std::vector<AbstractLauncherIcon::Ptr> icons;
  icons.push_back(first);
  icons.push_back(second);

  SwitcherModel::Ptr model(new SwitcherModel(icons));

  EXPECT_EQ(model->Size(), 2);
  EXPECT_EQ(model->Selection(), first);
  EXPECT_EQ(model->LastSelection(), first);
  EXPECT_EQ(model->SelectionIndex(), 0);
  EXPECT_EQ(model->LastSelectionIndex(), 0);
}

TEST(TestSwitcherModel, TestSelection)
{
  std::vector<AbstractLauncherIcon::Ptr> icons;
  AbstractLauncherIcon::Ptr first(new MockLauncherIcon());
  AbstractLauncherIcon::Ptr second(new MockLauncherIcon());
  AbstractLauncherIcon::Ptr third(new MockLauncherIcon());
  AbstractLauncherIcon::Ptr fourth(new MockLauncherIcon());

  icons.push_back(first);
  icons.push_back(second);
  icons.push_back(third);
  icons.push_back(fourth);

  SwitcherModel::Ptr model(new SwitcherModel(icons));

  EXPECT_EQ(model->Size(), 4);
  EXPECT_EQ(model->Selection(), first);

  model->Next();
  EXPECT_EQ(model->Selection(), second);
  EXPECT_EQ(model->LastSelection(), first);
  model->Next();
  EXPECT_EQ(model->Selection(), third);
  EXPECT_EQ(model->LastSelection(), second);
  model->Next();
  EXPECT_EQ(model->Selection(), fourth);
  EXPECT_EQ(model->LastSelection(), third);
  model->Next();
  EXPECT_EQ(model->Selection(), first);
  EXPECT_EQ(model->LastSelection(), fourth);
  model->Next();
  EXPECT_EQ(model->Selection(), second);
  EXPECT_EQ(model->LastSelection(), first);
  model->Prev();
  EXPECT_EQ(model->Selection(), first);
  EXPECT_EQ(model->LastSelection(), second);
  model->Prev();
  EXPECT_EQ(model->Selection(), fourth);
  EXPECT_EQ(model->LastSelection(), first);

  model->Select(2);
  EXPECT_EQ(model->Selection(), third);
  EXPECT_EQ(model->LastSelection(), fourth);

  model->Select(first);
  EXPECT_EQ(model->Selection(), first);
  EXPECT_EQ(model->LastSelection(), third);
}

}
