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
  LauncherModel model;
};

TEST_F(TestLauncherModel, TestConstructor)
{
  EXPECT_EQ(model.Size(), 0);
}

TEST_F(TestLauncherModel, TestAdd)
{
  AbstractLauncherIcon::Ptr first(new MockLauncherIcon());

  EXPECT_EQ(model.Size(), 0);
  model.AddIcon(first);
  EXPECT_EQ(model.Size(), 1);
}

TEST_F(TestLauncherModel, TestRemove)
{
  AbstractLauncherIcon::Ptr first(new MockLauncherIcon());

  EXPECT_EQ(model.Size(), 0);
  model.AddIcon(first);
  EXPECT_EQ(model.Size(), 1);
  model.RemoveIcon(first);
  EXPECT_EQ(model.Size(), 0);
}

TEST_F(TestLauncherModel, TestAddSignal)
{
  AbstractLauncherIcon::Ptr first(new MockLauncherIcon());

  bool icon_added = false;
  model.icon_added.connect([&icon_added] (AbstractLauncherIcon::Ptr) { icon_added = true; });

  model.AddIcon(first);
  EXPECT_EQ(icon_added, true);
}

TEST_F(TestLauncherModel, TestRemoveSignal)
{
  AbstractLauncherIcon::Ptr first(new MockLauncherIcon());

  bool icon_removed = false;
  model.icon_removed.connect([&icon_removed] (AbstractLauncherIcon::Ptr) { icon_removed = true; });

  model.AddIcon(first);
  EXPECT_EQ(icon_removed, false);
  model.RemoveIcon(first);
  EXPECT_EQ(icon_removed, true);
}

TEST_F(TestLauncherModel, TestSort)
{
  AbstractLauncherIcon::Ptr first(new MockLauncherIcon());
  AbstractLauncherIcon::Ptr second(new MockLauncherIcon());
  AbstractLauncherIcon::Ptr third(new MockLauncherIcon());
  AbstractLauncherIcon::Ptr fourth(new MockLauncherIcon());

  third->SetSortPriority(0);
  model.AddIcon(third);

  first->SetSortPriority(-1);
  model.AddIcon(first);

  fourth->SetSortPriority(2);
  model.AddIcon(fourth);

  second->SetSortPriority(0);
  model.AddIcon(second);

  LauncherModel::iterator it;
  it = model.begin();

  EXPECT_EQ(first, *it);
  it++;
  EXPECT_EQ(second, *it);
  it++;
  EXPECT_EQ(third, *it);
  it++;
  EXPECT_EQ(fourth, *it);
}

TEST_F(TestLauncherModel, TestReorderBefore)
{
  AbstractLauncherIcon::Ptr first(new MockLauncherIcon());
  AbstractLauncherIcon::Ptr second(new MockLauncherIcon());
  AbstractLauncherIcon::Ptr third(new MockLauncherIcon());
  AbstractLauncherIcon::Ptr fourth(new MockLauncherIcon());

  first->SetSortPriority(0);
  second->SetSortPriority(1);
  third->SetSortPriority(2);
  fourth->SetSortPriority(3);

  model.AddIcon(first);
  model.AddIcon(second);
  model.AddIcon(third);
  model.AddIcon(fourth);

  model.ReorderBefore(third, second, false);

  LauncherModel::iterator it;
  it = model.begin();

  EXPECT_EQ(first, *it);
  it++;
  EXPECT_EQ(third, *it);
  it++;
  EXPECT_EQ(second, *it);
  it++;
  EXPECT_EQ(fourth, *it);
}

TEST_F(TestLauncherModel, TestReorderSmart)
{
  AbstractLauncherIcon::Ptr first(new MockLauncherIcon());
  AbstractLauncherIcon::Ptr second(new MockLauncherIcon());
  AbstractLauncherIcon::Ptr third(new MockLauncherIcon());
  AbstractLauncherIcon::Ptr fourth(new MockLauncherIcon());

  first->SetSortPriority(0);
  second->SetSortPriority(1);
  third->SetSortPriority(2);
  fourth->SetSortPriority(3);

  model.AddIcon(first);
  model.AddIcon(second);
  model.AddIcon(third);
  model.AddIcon(fourth);

  model.ReorderSmart(third, second, false);

  LauncherModel::iterator it;
  it = model.begin();

  EXPECT_EQ(first, *it);
  it++;
  EXPECT_EQ(third, *it);
  it++;
  EXPECT_EQ(second, *it);
  it++;
  EXPECT_EQ(fourth, *it);
}

TEST_F(TestLauncherModel, TestOrderByPosition)
{
  AbstractLauncherIcon::Ptr first(new MockLauncherIcon());
  AbstractLauncherIcon::Ptr second(new MockLauncherIcon());
  AbstractLauncherIcon::Ptr third(new MockLauncherIcon());
  AbstractLauncherIcon::Ptr fourth(new MockLauncherIcon());

  first->position = AbstractLauncherIcon::Position::BEGIN;
  second->position = AbstractLauncherIcon::Position::FLOATING;
  third->position = AbstractLauncherIcon::Position::FLOATING;
  fourth->position = AbstractLauncherIcon::Position::END;

  model.AddIcon(third);
  model.AddIcon(fourth);
  model.AddIcon(second);
  model.AddIcon(first);

  auto it = model.begin();
  EXPECT_EQ(first, *it);
  it++;
  EXPECT_EQ(third, *it);
  it++;
  EXPECT_EQ(second, *it);
  it++;
  EXPECT_EQ(fourth, *it);
  it++;
  EXPECT_EQ(it, model.end());

  auto it_main = model.main_begin();
  EXPECT_EQ(first, *it_main);
  it_main++;
  EXPECT_EQ(third, *it_main);
  it_main++;
  EXPECT_EQ(second, *it_main);
  it_main++;
  EXPECT_EQ(it_main, model.main_end());

  auto it_shelf = model.shelf_begin();
  EXPECT_EQ(fourth, *it_shelf);
  it_shelf++;
  EXPECT_EQ(it_shelf, model.shelf_end());
}

}
