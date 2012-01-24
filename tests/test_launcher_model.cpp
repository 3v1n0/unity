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

class EventListener
{
  public:
    EventListener()
    {
      icon_added = false;
      icon_removed = false;
    }

    void OnIconAdded (AbstractLauncherIcon* icon)
    {
      icon_added = true;
    }

    void OnIconRemoved (AbstractLauncherIcon* icon)
    {
      icon_removed = true;
    }

    bool icon_added;
    bool icon_removed; 
};
//bool seen_result;

TEST(TestLauncherModel, TestConstructor)
{
  LauncherModel::Ptr model(new LauncherModel());
  EXPECT_EQ(model->Size(), 0);
}

TEST(TestLauncherModel, TestAdd)
{
  AbstractLauncherIcon* first  = new MockLauncherIcon();
  LauncherModel::Ptr model(new LauncherModel());

  EXPECT_EQ(model->Size(), 0);
  model->AddIcon(first);
  EXPECT_EQ(model->Size(), 1);
}

TEST(TestLauncherModel, TestRemove)
{
  AbstractLauncherIcon* first  = new MockLauncherIcon();
  LauncherModel::Ptr model(new LauncherModel());

  EXPECT_EQ(model->Size(), 0);
  model->AddIcon(first);
  EXPECT_EQ(model->Size(), 1);
  model->RemoveIcon(first);
  EXPECT_EQ(model->Size(), 0);
}

TEST(TestLauncherModel, TestAddSignal)
{
  AbstractLauncherIcon* first  = new MockLauncherIcon();
  LauncherModel::Ptr model(new LauncherModel());

  EventListener *listener = new EventListener();

  model->icon_added.connect(sigc::mem_fun(listener, &EventListener::OnIconAdded));
  model->AddIcon(first);
  EXPECT_EQ(listener->icon_added, true);

  delete listener;
}

TEST(TestLauncherModel, TestRemoveSignal)
{
  AbstractLauncherIcon* first  = new MockLauncherIcon();
  LauncherModel::Ptr model(new LauncherModel());

  EventListener *listener = new EventListener();

  model->icon_removed.connect(sigc::mem_fun(listener, &EventListener::OnIconRemoved));
  model->AddIcon(first);
  EXPECT_EQ(listener->icon_removed, false);
  model->RemoveIcon(first);
  EXPECT_EQ(listener->icon_removed, true);

  delete listener;
}

TEST(TestLauncherModel, TestSort)
{
  AbstractLauncherIcon* first = new MockLauncherIcon();
  AbstractLauncherIcon* second = new MockLauncherIcon();
  AbstractLauncherIcon* third = new MockLauncherIcon();
  AbstractLauncherIcon* fourth = new MockLauncherIcon();

  LauncherModel::Ptr model(new LauncherModel());

  first->SetSortPriority(0);
  second->SetSortPriority(1);
  third->SetSortPriority(2);
  fourth->SetSortPriority(3);

  model->AddIcon(third);
  model->AddIcon(first);
  model->AddIcon(fourth);
  model->AddIcon(second);

  LauncherModel::iterator it;

  it = model->begin();

  EXPECT_EQ(first, *it);
  it++;
  EXPECT_EQ(second, *it);
  it++;
  EXPECT_EQ(third, *it);
  it++;
  EXPECT_EQ(fourth, *it);
}

TEST(TestLauncherModel, TestReorderBefore)
{
  AbstractLauncherIcon* first = new MockLauncherIcon();
  AbstractLauncherIcon* second = new MockLauncherIcon();
  AbstractLauncherIcon* third = new MockLauncherIcon();
  AbstractLauncherIcon* fourth = new MockLauncherIcon();

  LauncherModel::Ptr model(new LauncherModel());

  first->SetSortPriority(0);
  second->SetSortPriority(1);
  third->SetSortPriority(2);
  fourth->SetSortPriority(3);

  model->AddIcon(first);
  model->AddIcon(second);
  model->AddIcon(third);
  model->AddIcon(fourth);

  model->ReorderBefore(third, second, false);

  LauncherModel::iterator it;
  it = model->begin();

  EXPECT_EQ(first, *it);
  it++;
  EXPECT_EQ(third, *it);
  it++;
  EXPECT_EQ(second, *it);
  it++;
  EXPECT_EQ(fourth, *it);
}

TEST(TestLauncherModel, TestReorderSmart)
{
  AbstractLauncherIcon* first = new MockLauncherIcon();
  AbstractLauncherIcon* second = new MockLauncherIcon();
  AbstractLauncherIcon* third = new MockLauncherIcon();
  AbstractLauncherIcon* fourth = new MockLauncherIcon();

  LauncherModel::Ptr model(new LauncherModel());

  first->SetSortPriority(0);
  second->SetSortPriority(1);
  third->SetSortPriority(2);
  fourth->SetSortPriority(3);

  model->AddIcon(first);
  model->AddIcon(second);
  model->AddIcon(third);
  model->AddIcon(fourth);

  model->ReorderSmart(third, second, false);

  LauncherModel::iterator it;
  it = model->begin();

  EXPECT_EQ(first, *it);
  it++;
  EXPECT_EQ(third, *it);
  it++;
  EXPECT_EQ(second, *it);
  it++;
  EXPECT_EQ(fourth, *it);
}

}
