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
  {}

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
      icons_.push_back(AbstractLauncherIcon::Ptr(new MockLauncherIcon2(i)));

    model = std::make_shared<SwitcherModel>(icons_, false);
  }

protected:
  std::vector<AbstractLauncherIcon::Ptr> icons_;
  SwitcherModel::Ptr model;
};


TEST_F(TestSwitcherModel, TestConstructor)
{
  EXPECT_EQ(model->Size(), icons_.size());
  EXPECT_EQ(model->Selection(), icons_.front());
  EXPECT_EQ(model->LastSelection(), icons_.front());
  EXPECT_EQ(model->SelectionIndex(), 0);
  EXPECT_EQ(model->LastSelectionIndex(), 0);
  EXPECT_FALSE(model->SelectionWindows().empty());
  EXPECT_TRUE(model->DetailXids().empty());
  EXPECT_FALSE(model->detail_selection);
  EXPECT_EQ(model->detail_selection_index, 0u);
}


TEST_F(TestSwitcherModel, TestSelection)
{
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
  auto model_detail = std::make_shared<SwitcherModel>(icons_, false);
  model_detail->detail_selection = true;

  // Create a test case with an active detail window.
  icons_.front()->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);
  auto model_detail_active = std::make_shared<SwitcherModel>(icons_, false);
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
  model->Selection()->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, false);
  EXPECT_FALSE(model->SelectionIsActive());

  model->Selection()->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);
  EXPECT_TRUE(model->SelectionIsActive());
}

TEST_F(TestSwitcherModel, DetailXidsIsValidOnSelectionOnly)
{
  model->detail_selection = true;
  EXPECT_FALSE(model->DetailXids().empty());
  EXPECT_EQ(model->DetailXids(), model->SelectionWindows());

  model->detail_selection = false;
  EXPECT_TRUE(model->DetailXids().empty());
  EXPECT_FALSE(model->SelectionWindows().empty());
}

TEST_F(TestSwitcherModel, TestWebAppActive)
{
  // Create a base case
  auto base_model = std::make_shared<SwitcherModel>(icons_, false);
  base_model->detail_selection = true;

  // Set the first icon as Active to simulate Firefox being active
  icons_.front()->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);

  // Set the last icon as Active to simulate that it is a WebApp
  icons_.back()->SetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, true);

  auto new_model = std::make_shared<SwitcherModel>(icons_, false);
  new_model->detail_selection = true;

  // model's front Window should be different than the base case due to the
  // re-sorting in DetailXids().
  EXPECT_NE(new_model->DetailXids().front(), base_model->DetailXids().front());
}

TEST_F(TestSwitcherModel, TestHasNextDetailRow)
{
  model->detail_selection = true;
  model->SetRowSizes({2,2});

  EXPECT_TRUE(model->HasNextDetailRow());
}

TEST_F(TestSwitcherModel, TestHasNextDetailRowStopsAtTheEnd)
{
  model->detail_selection = true;
  for (unsigned int i = 0; i < model->DetailXids().size() - 1 &&
       model->HasNextDetailRow(); i++)
  {
    model->NextDetailRow();
  }

  EXPECT_FALSE(model->HasNextDetailRow());
}

TEST_F(TestSwitcherModel, TestHasPrevDetailRow)
{
  model->detail_selection = true;
  model->SetRowSizes({2,2});

  model->NextDetail();
  EXPECT_TRUE(model->HasPrevDetailRow());

  model->PrevDetailRow();
  EXPECT_FALSE(model->HasPrevDetailRow());
}

TEST_F(TestSwitcherModel, TestHasNextThenPrevDetailRow)
{
  model->detail_selection = true;
  model->SetRowSizes({2,2});

  EXPECT_TRUE(model->HasNextDetailRow());

  model->NextDetailRow();

  EXPECT_TRUE(model->HasPrevDetailRow());
  model->PrevDetailRow();
  EXPECT_FALSE(model->HasPrevDetailRow());
}

TEST_F(TestSwitcherModel, TestNextDetailRow)
{
  model->detail_selection = true;
  model->SetRowSizes({2,2});

  model->NextDetailRow();

  // Expect going form index 0 -> 2
  // 0, 1
  // 2, 3
  EXPECT_EQ(static_cast<unsigned int>(model->detail_selection_index), 2);
}

TEST_F(TestSwitcherModel, TestNextDetailThenNextDetailRow)
{
  model->detail_selection = true;
  model->SetRowSizes({2,2});

  model->NextDetail();
  model->NextDetailRow();

  // Expect going form index 1 -> 3
  // 0, 1
  // 2, 3
  EXPECT_EQ(static_cast<unsigned int>(model->detail_selection_index), 3);
}

TEST_F(TestSwitcherModel, TestPrevDetailRow)
{
  model->detail_selection = true;
  model->SetRowSizes({2,2});

  model->NextDetailRow();
  model->PrevDetailRow();

  // Expect going form index 0 -> 2, then index 2 -> 0
  // 0, 1
  // 2, 3
  EXPECT_EQ(static_cast<unsigned int>(model->detail_selection_index), 0);
}

TEST_F(TestSwitcherModel, TestNextDetailThenPrevDetailRow)
{
  model->detail_selection = true;
  model->SetRowSizes({2,2});

  model->NextDetail();
  model->NextDetailRow();

  model->PrevDetailRow();

  // Expect going form index 1 -> 3, then index 3 -> 1
  // 0, 1
  // 2, 3
  EXPECT_EQ(static_cast<unsigned int>(model->detail_selection_index), 1);
}

TEST_F(TestSwitcherModel, TestUnEvenNextDetailRow)
{
  model->detail_selection = true;
  model->SetRowSizes({3,2});

  model->NextDetailRow();

  // Expect going form index 0 -> 3
  // 0, 1, 2,
  //   3, 4
  EXPECT_EQ(static_cast<unsigned int>(model->detail_selection_index), 3);
}

TEST_F(TestSwitcherModel, TestUnEvenPrevDetailRow)
{
  model->detail_selection = true;
  model->SetRowSizes({3,2});

  model->NextDetailRow();
  model->PrevDetailRow();

  // Expect going form index 0 -> 3, then 3 -> 0
  // 0, 1, 2,
  //   3, 4
  EXPECT_EQ(static_cast<unsigned int>(model->detail_selection_index), 0);
}

TEST_F(TestSwitcherModel, TestNextPrevDetailRowMovesLeftInTopRow)
{
  model->detail_selection = true;
  model->SetRowSizes({3,2});

  model->NextDetail();
  model->NextDetail();
  model->PrevDetailRow();
  model->PrevDetailRow();

  // Expect going form index 0 -> 1, then 1 -> 2, then 2 -> 1, 1 -> 0
  // since PrevDetailRow must go to the index 0 of at the top of the row
  // 0, 1, 2,
  //   3, 4
  EXPECT_EQ(static_cast<unsigned int>(model->detail_selection_index), 0);
}

}
