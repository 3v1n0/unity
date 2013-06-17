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
  EXPECT_EQ(static_cast<unsigned int>(model->detail_selection_index), 0);
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

TEST_F(TestSwitcherModel, SelectionIsActive)
{
  SwitcherModel model(icons_);

  model.Selection()->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, false);
  EXPECT_FALSE(model.SelectionIsActive());

  model.Selection()->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);
  EXPECT_TRUE(model.SelectionIsActive());
}

TEST_F(TestSwitcherModel, TestWebAppActive)
{
  // Create a base case
  SwitcherModel::Ptr base_model(new SwitcherModel(icons_));

  // Set the first icon as Active to simulate Firefox being active
  icons_.front()->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);

  // Set the last icon as Active to simulate that it is a WebApp
  icons_.back()->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);

  SwitcherModel::Ptr model(new SwitcherModel(icons_));

  model->DetailXids();

  // model's front Window should be different than the base case due to the
  // re-sorting in DetailXids().
  EXPECT_NE(model->DetailXids().front(), base_model->DetailXids().front());
}

TEST_F(TestSwitcherModel, TestHasNextDetailRow)
{
  SwitcherModel::Ptr model(new SwitcherModel(icons_));
  model->detail_selection = true;
  model->SetRowSizes({2,2});

  EXPECT_TRUE(model->HasNextDetailRow());

  model->NextDetailRow();
  EXPECT_FALSE(model->HasNextDetailRow());
}

TEST_F(TestSwitcherModel, TestHasPrevDetailRow)
{
  SwitcherModel::Ptr model(new SwitcherModel(icons_));
  model->detail_selection = true;
  model->SetRowSizes({2,2});

  model->NextDetail();
  EXPECT_TRUE(model->HasPrevDetailRow());

  model->PrevDetailRow();
  EXPECT_FALSE(model->HasPrevDetailRow());
}

TEST_F(TestSwitcherModel, TestHasNextThenPrevDetailRow)
{
  SwitcherModel::Ptr model(new SwitcherModel(icons_));
  model->detail_selection = true;
  model->SetRowSizes({2,2});

  EXPECT_TRUE(model->HasNextDetailRow());

  model->NextDetailRow();
  EXPECT_FALSE(model->HasNextDetailRow());

  EXPECT_TRUE(model->HasPrevDetailRow());
  model->PrevDetailRow();
  EXPECT_FALSE(model->HasPrevDetailRow());
}

TEST_F(TestSwitcherModel, TestNextDetailRow)
{
  SwitcherModel::Ptr model(new SwitcherModel(icons_));
  model->detail_selection = true;
  model->SetRowSizes({2,2});

  model->NextDetailRow();
  EXPECT_EQ(static_cast<unsigned int>(model->detail_selection_index), 2);
}

TEST_F(TestSwitcherModel, TestNextDetailThenNextDetailRow)
{
  SwitcherModel::Ptr model(new SwitcherModel(icons_));
  model->detail_selection = true;
  model->SetRowSizes({2,2});

  model->NextDetail();
  model->NextDetailRow();
  EXPECT_EQ(static_cast<unsigned int>(model->detail_selection_index), 3);
}

TEST_F(TestSwitcherModel, TestPrevDetailRow)
{
  SwitcherModel::Ptr model(new SwitcherModel(icons_));
  model->detail_selection = true;
  model->SetRowSizes({2,2});

  model->NextDetailRow();
  model->PrevDetailRow();

  EXPECT_EQ(static_cast<unsigned int>(model->detail_selection_index), 0);
}

TEST_F(TestSwitcherModel, TestNextDetailThenPrevDetailRow)
{
  SwitcherModel::Ptr model(new SwitcherModel(icons_));
  model->detail_selection = true;
  model->SetRowSizes({2,2});

  model->NextDetail();
  model->NextDetailRow();

  model->PrevDetailRow();
  EXPECT_EQ(static_cast<unsigned int>(model->detail_selection_index), 1);
}

TEST_F(TestSwitcherModel, TestUnEvenNextDetailRow)
{
  SwitcherModel::Ptr model(new SwitcherModel(icons_));
  model->detail_selection = true;
  model->SetRowSizes({3,2});

  model->NextDetailRow();
  EXPECT_EQ(static_cast<unsigned int>(model->detail_selection_index), 3);
}

TEST_F(TestSwitcherModel, TestUnEvenPrevDetailRow)
{
  SwitcherModel::Ptr model(new SwitcherModel(icons_));
  model->detail_selection = true;
  model->SetRowSizes({3,2});

  model->NextDetailRow();
  model->PrevDetailRow();

  EXPECT_EQ(static_cast<unsigned int>(model->detail_selection_index), 0);
}

TEST_F(TestSwitcherModel, TestNextPrevDetailRowMovesLeftInTopRow)
{
  SwitcherModel::Ptr model(new SwitcherModel(icons_));
  model->detail_selection = true;
  model->SetRowSizes({3,2});

  model->NextDetail();
  model->NextDetail();
  model->PrevDetailRow();
  model->PrevDetailRow();

  EXPECT_EQ(static_cast<unsigned int>(model->detail_selection_index), 0);
}

}
