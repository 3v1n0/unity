/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 */

#include <list>
#include <vector>

#include <gmock/gmock.h>

#include "LensViewPrivate.h"
#include "AbstractPlacesGroup.h"

using namespace unity;
using namespace testing;

namespace
{

const int NUM_GROUPS = 4;

class TestPlacesGroupImpl : public Test
{
public:
  TestPlacesGroupImpl()
  {
    for (int i=0; i<NUM_GROUPS; ++i)
      groups_vector.push_back(new dash::AbstractPlacesGroup);
  }

  ~TestPlacesGroupImpl()
  {
    for (int i=0; i<NUM_GROUPS; ++i)
      delete groups_vector[i];
    groups_vector.clear();
  }

  std::list<dash::AbstractPlacesGroup*> GetList()
  {
    std::list<dash::AbstractPlacesGroup*> ret;
    std::copy(groups_vector.begin(), groups_vector.end(), std::back_inserter(ret));
    return ret;
  }

  void Clear()
  {
}

  std::vector<dash::AbstractPlacesGroup*> groups_vector;
};


TEST_F(TestPlacesGroupImpl, TestUpdateDrawSeparatorsEmpty)
{
  std::list<dash::AbstractPlacesGroup*> groups;

  // Just to make sure it doesn't crash.
  dash::impl::UpdateDrawSeparators(groups);
}

TEST_F(TestPlacesGroupImpl, TestUpdateDrawSeparatorsAllVisible)
{
  groups_vector[0]->SetVisible(true);
  groups_vector[1]->SetVisible(true);
  groups_vector[2]->SetVisible(true);
  groups_vector[3]->SetVisible(true);

  dash::impl::UpdateDrawSeparators(GetList());

  EXPECT_TRUE(groups_vector[0]->draw_separator);
  EXPECT_TRUE(groups_vector[1]->draw_separator);
  EXPECT_TRUE(groups_vector[2]->draw_separator);
  EXPECT_FALSE(groups_vector[3]->draw_separator);
}

TEST_F(TestPlacesGroupImpl, TestUpdateDrawSeparatorsLastInvisible)
{
  groups_vector[0]->SetVisible(true);
  groups_vector[1]->SetVisible(true);
  groups_vector[2]->SetVisible(true);
  groups_vector[3]->SetVisible(false);

  dash::impl::UpdateDrawSeparators(GetList());

  EXPECT_TRUE(groups_vector[0]->draw_separator);
  EXPECT_TRUE(groups_vector[1]->draw_separator);
  EXPECT_FALSE(groups_vector[2]->draw_separator);
}

TEST_F(TestPlacesGroupImpl, TestUpdateDrawSeparatorsLastTwoInvisible)
{
  groups_vector[0]->SetVisible(true);
  groups_vector[1]->SetVisible(true);
  groups_vector[2]->SetVisible(false);
  groups_vector[3]->SetVisible(false);

  dash::impl::UpdateDrawSeparators(GetList());

  EXPECT_TRUE(groups_vector[0]->draw_separator);
  EXPECT_FALSE(groups_vector[1]->draw_separator);
}

TEST_F(TestPlacesGroupImpl, TestUpdateDrawSeparators)
{
  groups_vector[0]->SetVisible(true);
  groups_vector[1]->SetVisible(false);
  groups_vector[2]->SetVisible(false);
  groups_vector[3]->SetVisible(true);

  dash::impl::UpdateDrawSeparators(GetList());

  EXPECT_TRUE(groups_vector[0]->draw_separator);
  EXPECT_FALSE(groups_vector[3]->draw_separator);
}

}

