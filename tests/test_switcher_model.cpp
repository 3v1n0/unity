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

/**
 * An extension of the mock launcher icon class provided in the library to
 * aid in tracking individually identified icons.
 */
class MockLauncherIcon2 : public unity::launcher::MockLauncherIcon
{
public:
  MockLauncherIcon2(int id)
  : id_(id)
  { }

  int id_;
};

/**
 * A helper function for downcasting the mocked icon objects.
 */
inline int IdentityOf(AbstractLauncherIcon::Ptr const& p)
{ return ((MockLauncherIcon2*)p.GetPointer())->id_; }


class TestSwitcherModel : public testing::Test
{
public:
  TestSwitcherModel()
  {
    for (int i = 0; i < 4; ++i)
    {
      icons_.push_back(AbstractLauncherIcon::Ptr(new MockLauncherIcon2(i)));
    }
  }

protected:
  std::vector<AbstractLauncherIcon::Ptr> icons_;
};


TEST_F(TestSwitcherModel, TestConstructor)
{
  SwitcherModel::Ptr model(new SwitcherModel(icons_));

  EXPECT_EQ(model->Size(), icons_.size());
  EXPECT_EQ(model->Selection(), icons_.front());
  EXPECT_EQ(model->LastSelection(), icons_.front());
  EXPECT_EQ(model->SelectionIndex(), 0);
  EXPECT_EQ(model->LastSelectionIndex(), 0);
}


TEST_F(TestSwitcherModel, TestSelection)
{
  SwitcherModel::Ptr model(new SwitcherModel(icons_));

  EXPECT_EQ(IdentityOf(model->Selection()), 0);

  model->Next();
  EXPECT_EQ(IdentityOf(model->Selection()), 1);
  EXPECT_EQ(IdentityOf(model->LastSelection()), 0);
  model->Next();
  EXPECT_EQ(IdentityOf(model->Selection()), 2);
  EXPECT_EQ(IdentityOf(model->LastSelection()), 1);
  model->Next();
  EXPECT_EQ(IdentityOf(model->Selection()), 3);
  EXPECT_EQ(IdentityOf(model->LastSelection()), 2);
  model->Next();
  EXPECT_EQ(IdentityOf(model->Selection()), 0);
  EXPECT_EQ(IdentityOf(model->LastSelection()), 3);
  model->Next();
  EXPECT_EQ(IdentityOf(model->Selection()), 1);
  EXPECT_EQ(IdentityOf(model->LastSelection()), 0);

  model->Prev();
  EXPECT_EQ(IdentityOf(model->Selection()), 0);
  EXPECT_EQ(IdentityOf(model->LastSelection()), 1);
  model->Prev();
  EXPECT_EQ(IdentityOf(model->Selection()), 3);
  EXPECT_EQ(IdentityOf(model->LastSelection()), 0);

  model->Select(2);
  EXPECT_EQ(IdentityOf(model->Selection()), 2);
  EXPECT_EQ(IdentityOf(model->LastSelection()), 3);

  model->Select(0);
  EXPECT_EQ(IdentityOf(model->Selection()), 0);
  EXPECT_EQ(IdentityOf(model->LastSelection()), 2);
}


TEST_F(TestSwitcherModel, TestActiveDetailWindowSort)
{
  // Create a base case for the null hypothesis.
  SwitcherModel::Ptr model_detail(new SwitcherModel(icons_));
  model_detail->detail_selection = true;

  // Create a test case with an active detail window.
  icons_.front()->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);
  SwitcherModel::Ptr model_detail_active(new SwitcherModel(icons_));
  model_detail_active->detail_selection = true;

  EXPECT_TRUE(model_detail_active->DetailXids().size() > 2);
  EXPECT_TRUE(model_detail_active->DetailSelectionWindow() != model_detail->DetailSelectionWindow());

  // Move to the last detailed window
  for (unsigned int i = 0; i < model_detail_active->DetailXids().size() - 1; i++)
    model_detail_active->NextDetail();

  Window sorted, unsorted;
  sorted = model_detail_active->DetailSelectionWindow();
  unsorted = model_detail->DetailSelectionWindow();

  EXPECT_EQ(sorted, unsorted);
}

}
