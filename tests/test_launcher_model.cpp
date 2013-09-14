/*
 * Copyright 2011-2012 Canonical Ltd.
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
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <gtest/gtest.h>

#include "LauncherModel.h"
#include "MockLauncherIcon.h"
#include "AbstractLauncherIcon.h"

#include <vector>


using namespace unity::launcher;

namespace
{

class TestLauncherModel : public testing::Test
{
public:
  TestLauncherModel()
    : icon1(new MockLauncherIcon())
    , icon2(new MockLauncherIcon())
    , icon3(new MockLauncherIcon())
    , icon4(new MockLauncherIcon())
  {}

  AbstractLauncherIcon::Ptr icon1;
  AbstractLauncherIcon::Ptr icon2;
  AbstractLauncherIcon::Ptr icon3;
  AbstractLauncherIcon::Ptr icon4;

  LauncherModel model;
};

TEST_F(TestLauncherModel, Constructor)
{
  EXPECT_EQ(model.Size(), 0);
}

TEST_F(TestLauncherModel, Add)
{
  model.AddIcon(icon1);
  EXPECT_EQ(model.Size(), 1);

  model.AddIcon(icon1);
  EXPECT_EQ(model.Size(), 1);

  model.AddIcon(AbstractLauncherIcon::Ptr());
  EXPECT_EQ(model.Size(), 1);
}

TEST_F(TestLauncherModel, Remove)
{
  model.AddIcon(icon1);
  EXPECT_EQ(model.Size(), 1);

  model.RemoveIcon(icon1);
  EXPECT_EQ(model.Size(), 0);
}

TEST_F(TestLauncherModel, AddSignal)
{
  bool icon_added = false;
  model.icon_added.connect([&icon_added] (AbstractLauncherIcon::Ptr const&) { icon_added = true; });

  model.AddIcon(icon1);
  EXPECT_TRUE(icon_added);

  icon_added = false;
  model.AddIcon(icon1);
  EXPECT_FALSE(icon_added);

  icon_added = false;
  model.AddIcon(AbstractLauncherIcon::Ptr());
  EXPECT_FALSE(icon_added);
}

TEST_F(TestLauncherModel, RemoveSignal)
{
  bool icon_removed = false;
  model.icon_removed.connect([&icon_removed] (AbstractLauncherIcon::Ptr const&) { icon_removed = true; });

  model.AddIcon(icon1);
  EXPECT_FALSE(icon_removed);
  model.RemoveIcon(icon1);
  EXPECT_TRUE(icon_removed);
}

TEST_F(TestLauncherModel, Sort)
{
  icon2->SetSortPriority(0);
  model.AddIcon(icon2);

  icon1->SetSortPriority(-1);
  model.AddIcon(icon1);

  icon4->SetSortPriority(2);
  model.AddIcon(icon4);

  icon3->SetSortPriority(0);
  model.AddIcon(icon3);

  LauncherModel::iterator it;
  it = model.begin();

  EXPECT_EQ(icon1, *it);
  it++;
  EXPECT_EQ(icon2, *it);
  it++;
  EXPECT_EQ(icon3, *it);
  it++;
  EXPECT_EQ(icon4, *it);
}

TEST_F(TestLauncherModel, ModelKeepsPriorityDeltas)
{
  icon2->SetSortPriority(0);
  icon1->SetSortPriority(-1);
  icon4->SetSortPriority(2);
  icon3->SetSortPriority(0);

  model.AddIcon(icon2);
  model.AddIcon(icon1);
  model.AddIcon(icon4);
  model.AddIcon(icon3);

  auto it = model.begin();
  EXPECT_EQ(icon1, *it);
  it++;
  EXPECT_EQ(icon2, *it);
  it++;
  EXPECT_EQ(icon3, *it);
  it++;
  EXPECT_EQ(icon4, *it);

  EXPECT_GT(icon2->SortPriority(), icon1->SortPriority());
  EXPECT_EQ(icon2->SortPriority(), icon3->SortPriority());
  EXPECT_LT(icon3->SortPriority(), icon4->SortPriority());
}

TEST_F(TestLauncherModel, ReorderBefore)
{
  model.AddIcon(icon1);
  model.AddIcon(icon2);
  model.AddIcon(icon3);
  model.AddIcon(icon4);

  model.ReorderBefore(icon3, icon2, false);

  LauncherModel::iterator it;
  it = model.begin();

  EXPECT_EQ(icon1, *it);
  it++;
  EXPECT_EQ(icon3, *it);
  it++;
  EXPECT_EQ(icon2, *it);
  it++;
  EXPECT_EQ(icon4, *it);
}

TEST_F(TestLauncherModel, ReorderBeforeWithPriority)
{
  icon1->SetSortPriority(0);
  icon2->SetSortPriority(1);
  icon3->SetSortPriority(2);
  icon4->SetSortPriority(3);

  model.AddIcon(icon1);
  model.AddIcon(icon2);
  model.AddIcon(icon3);
  model.AddIcon(icon4);

  model.ReorderBefore(icon3, icon2, false);

  LauncherModel::iterator it;
  it = model.begin();

  EXPECT_EQ(icon1, *it);
  it++;
  EXPECT_EQ(icon3, *it);
  it++;
  EXPECT_EQ(icon2, *it);
  it++;
  EXPECT_EQ(icon4, *it);
}

TEST_F(TestLauncherModel, ReorderAfterNext)
{
  model.AddIcon(icon1);
  model.AddIcon(icon2);
  model.AddIcon(icon3);
  model.AddIcon(icon4);

  model.ReorderAfter(icon2, icon3);

  LauncherModel::iterator it;
  it = model.begin();

  EXPECT_EQ(icon1, *it);
  it++;
  EXPECT_EQ(icon3, *it);
  it++;
  EXPECT_EQ(icon2, *it);
  it++;
  EXPECT_EQ(icon4, *it);
}

TEST_F(TestLauncherModel, ReorderAfterPrevious)
{
  model.AddIcon(icon1);
  model.AddIcon(icon2);
  model.AddIcon(icon3);
  model.AddIcon(icon4);

  model.ReorderAfter(icon4, icon1);

  LauncherModel::iterator it;
  it = model.begin();

  EXPECT_EQ(icon1, *it);
  it++;
  EXPECT_EQ(icon4, *it);
  it++;
  EXPECT_EQ(icon2, *it);
  it++;
  EXPECT_EQ(icon3, *it);
}

TEST_F(TestLauncherModel, ReorderSmart)
{
  model.AddIcon(icon1);
  model.AddIcon(icon2);
  model.AddIcon(icon3);
  model.AddIcon(icon4);

  model.ReorderSmart(icon3, icon2, false);

  LauncherModel::iterator it;
  it = model.begin();

  EXPECT_EQ(icon1, *it);
  it++;
  EXPECT_EQ(icon3, *it);
  it++;
  EXPECT_EQ(icon2, *it);
  it++;
  EXPECT_EQ(icon4, *it);
}

TEST_F(TestLauncherModel, ReorderSmartWithDifferentPriority)
{
  icon1->SetSortPriority(0);
  icon2->SetSortPriority(1);
  icon3->SetSortPriority(2);
  icon4->SetSortPriority(3);

  model.AddIcon(icon1);
  model.AddIcon(icon2);
  model.AddIcon(icon3);
  model.AddIcon(icon4);

  model.ReorderSmart(icon3, icon2, false);

  LauncherModel::iterator it;
  it = model.begin();

  EXPECT_EQ(icon1, *it);
  it++;
  EXPECT_EQ(icon3, *it);
  it++;
  EXPECT_EQ(icon2, *it);
  it++;
  EXPECT_EQ(icon4, *it);
}

TEST_F(TestLauncherModel, ReorderSmartWithSimilarPriority)
{
  icon1->SetSortPriority(0);
  icon2->SetSortPriority(0);
  icon3->SetSortPriority(1);
  icon4->SetSortPriority(1);

  model.AddIcon(icon1);
  model.AddIcon(icon2);
  model.AddIcon(icon3);
  model.AddIcon(icon4);

  model.ReorderSmart(icon4, icon3, false);

  LauncherModel::iterator it;
  it = model.begin();

  EXPECT_EQ(icon1, *it);
  it++;
  EXPECT_EQ(icon2, *it);
  it++;
  EXPECT_EQ(icon4, *it);
  it++;
  EXPECT_EQ(icon3, *it);
}

TEST_F(TestLauncherModel, ReorderSmartManyIconsWithSimilarPriority)
{
  AbstractLauncherIcon::Ptr icon5(new MockLauncherIcon);
  AbstractLauncherIcon::Ptr icon6(new MockLauncherIcon);
  icon1->SetSortPriority(0);
  icon2->SetSortPriority(0);
  icon3->SetSortPriority(1);
  icon4->SetSortPriority(1);
  icon5->SetSortPriority(1);
  icon6->SetSortPriority(2);

  model.AddIcon(icon1);
  model.AddIcon(icon2);
  model.AddIcon(icon3);
  model.AddIcon(icon4);
  model.AddIcon(icon5);
  model.AddIcon(icon6);

  model.ReorderSmart(icon6, icon4, false);

  LauncherModel::iterator it;
  it = model.begin();

  EXPECT_EQ(icon1, *it); it++;
  EXPECT_EQ(icon2, *it); it++;
  EXPECT_EQ(icon3, *it); it++;
  EXPECT_EQ(icon6, *it); it++;
  EXPECT_EQ(icon4, *it); it++;
  EXPECT_EQ(icon5, *it);
}

TEST_F(TestLauncherModel, OrderByPosition)
{
  icon1->position = AbstractLauncherIcon::Position::BEGIN;
  icon2->position = AbstractLauncherIcon::Position::FLOATING;
  icon3->position = AbstractLauncherIcon::Position::FLOATING;
  icon4->position = AbstractLauncherIcon::Position::END;

  model.AddIcon(icon3);
  model.AddIcon(icon4);
  model.AddIcon(icon2);
  model.AddIcon(icon1);

  auto it = model.begin();
  EXPECT_EQ(icon1, *it);
  it++;
  EXPECT_EQ(icon3, *it);
  it++;
  EXPECT_EQ(icon2, *it);
  it++;
  EXPECT_EQ(icon4, *it);
  it++;
  EXPECT_EQ(it, model.end());

  auto it_main = model.main_begin();
  EXPECT_EQ(icon1, *it_main);
  it_main++;
  EXPECT_EQ(icon3, *it_main);
  it_main++;
  EXPECT_EQ(icon2, *it_main);
  it_main++;
  EXPECT_EQ(it_main, model.main_end());

  auto it_shelf = model.shelf_begin();
  EXPECT_EQ(icon4, *it_shelf);
  it_shelf++;
  EXPECT_EQ(it_shelf, model.shelf_end());
}

TEST_F(TestLauncherModel, OrderByType)
{
  AbstractLauncherIcon::Ptr icon1(new MockLauncherIcon(AbstractLauncherIcon::IconType::HOME));
  AbstractLauncherIcon::Ptr icon2(new MockLauncherIcon(AbstractLauncherIcon::IconType::APPLICATION));
  AbstractLauncherIcon::Ptr icon3(new MockLauncherIcon(AbstractLauncherIcon::IconType::EXPO));
  AbstractLauncherIcon::Ptr icon4(new MockLauncherIcon(AbstractLauncherIcon::IconType::DEVICE));
  AbstractLauncherIcon::Ptr icon5(new MockLauncherIcon(AbstractLauncherIcon::IconType::TRASH));

  model.AddIcon(icon3);
  model.AddIcon(icon4);
  model.AddIcon(icon2);
  model.AddIcon(icon1);
  model.AddIcon(icon5);

  auto it = model.begin();
  EXPECT_EQ(icon1, *it);
  it++;
  EXPECT_EQ(icon2, *it);
  it++;
  EXPECT_EQ(icon3, *it);
  it++;
  EXPECT_EQ(icon4, *it);
  it++;
  EXPECT_EQ(icon5, *it);
  it++;
  EXPECT_EQ(it, model.end());
}

TEST_F(TestLauncherModel, GetClosestIcon)
{
  model.AddIcon(icon1);
  model.AddIcon(icon2);
  model.AddIcon(icon3);
  model.AddIcon(icon4);

  bool before;
  EXPECT_EQ(model.GetClosestIcon(icon1, before), icon2);
  EXPECT_FALSE(before);

  EXPECT_EQ(model.GetClosestIcon(icon2, before), icon1);
  EXPECT_TRUE(before);

  EXPECT_EQ(model.GetClosestIcon(icon3, before), icon2);
  EXPECT_TRUE(before);

  EXPECT_EQ(model.GetClosestIcon(icon4, before), icon3);
  EXPECT_TRUE(before);
}

TEST_F(TestLauncherModel, GetClosestIconWithOneIcon)
{
  model.AddIcon(icon1);

  bool before;
  EXPECT_EQ(model.GetClosestIcon(icon1, before), nullptr);
  EXPECT_TRUE(before);
}

TEST_F(TestLauncherModel, IconIndex)
{
  model.AddIcon(icon1);
  model.AddIcon(icon2);
  model.AddIcon(icon3);
  model.AddIcon(icon4);

  EXPECT_EQ(model.IconIndex(icon1), 0);
  EXPECT_EQ(model.IconIndex(icon2), 1);
  EXPECT_EQ(model.IconIndex(icon3), 2);
  EXPECT_EQ(model.IconIndex(icon4), 3);

  AbstractLauncherIcon::Ptr icon5(new MockLauncherIcon());
  EXPECT_EQ(model.IconIndex(icon5), -1);
}

TEST_F(TestLauncherModel, IconHasSister)
{
  model.AddIcon(icon1);
  EXPECT_FALSE(model.IconHasSister(icon1));

  model.AddIcon(icon2);
  model.AddIcon(icon3);
  model.AddIcon(icon4);

  EXPECT_TRUE(model.IconHasSister(icon1));
  EXPECT_TRUE(model.IconHasSister(icon2));
  EXPECT_TRUE(model.IconHasSister(icon3));
  EXPECT_TRUE(model.IconHasSister(icon4));

  EXPECT_FALSE(AbstractLauncherIcon::Ptr());
}

}
