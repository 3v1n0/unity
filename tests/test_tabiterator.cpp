// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2013 Canonical Ltd.
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
 * Authored by: Manuel de la Pena <manuel.delapena@canonical.com>
 *
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <unity-shared/IMTextEntry.h>
#include "dash/previews/TabIterator.h"
#include "test_utils.h"

namespace unity
{

namespace dash
{

namespace previews
{

class MockedTabIterator : public TabIterator
{
public:
  MOCK_METHOD0(GetCurretFocusArea, nux::InputArea*());
  using TabIterator::RemoveAlreadyPresent;
  using TabIterator::areas_;
};

class TestTabIterator : public ::testing::Test
{
  protected:
    TestTabIterator() : Test()
    {
      tab_iterator = new MockedTabIterator();
    }

    ~TestTabIterator()
    {
      delete tab_iterator;
    }

    MockedTabIterator *tab_iterator;
};

TEST_F(TestTabIterator, DoRemoveAlreadyPresent)
{
  unity::IMTextEntry* entry = new unity::IMTextEntry();
  tab_iterator->areas_.push_front(entry);
  tab_iterator->RemoveAlreadyPresent(entry);

  std::list<nux::InputArea*>::iterator it = std::find(tab_iterator->areas_.begin(),
    tab_iterator->areas_.end(), entry);
  EXPECT_EQ(it, tab_iterator->areas_.end());
}

TEST_F(TestTabIterator, DoRemoveAlreadyMissing)
{
  unity::IMTextEntry* entry = new unity::IMTextEntry();
  tab_iterator->RemoveAlreadyPresent(entry);

  std::list<nux::InputArea*>::iterator it = std::find(tab_iterator->areas_.begin(),
    tab_iterator->areas_.end(), entry);
  EXPECT_EQ(it, tab_iterator->areas_.end());
}

TEST_F(TestTabIterator, AddAreaFirst)
{
  for(int index=0; index < 10; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  unity::IMTextEntry* entry = new unity::IMTextEntry();
  tab_iterator->AddAreaFirst(entry);

  std::list<nux::InputArea*>::iterator it = std::find(tab_iterator->areas_.begin(),
    tab_iterator->areas_.end(), entry);

  EXPECT_EQ(it, tab_iterator->areas_.begin());
}

TEST_F(TestTabIterator, AddAreaLast)
{
  for(int index=0; index < 10; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  unity::IMTextEntry* entry = new unity::IMTextEntry();
  tab_iterator->AddAreaLast(entry);
  nux::InputArea* last = tab_iterator->areas_.back();

  EXPECT_TRUE(entry == last);  // compare pointers
}

TEST_F(TestTabIterator, AddAreaIndex)
{
  for(int index=0; index < 10; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }
  unity::IMTextEntry* entry = new unity::IMTextEntry();
  tab_iterator->AddArea(entry, 5);

  std::list<nux::InputArea*>::iterator it = tab_iterator->areas_.begin();
  std::advance(it, 5);

  EXPECT_NE(tab_iterator->areas_.end(), it);
  EXPECT_TRUE(entry == (unity::IMTextEntry*)*it);
}

TEST_F(TestTabIterator, AddAreaIndexTooLarge)
{
  for(int index=0; index < 5; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  unity::IMTextEntry* entry = new unity::IMTextEntry();
  tab_iterator->AddArea(entry, 7);

  nux::InputArea* last = tab_iterator->areas_.back();

  EXPECT_TRUE(entry == last);  // compare pointers
}

TEST_F(TestTabIterator, AddAreaBefore)
{
  unity::IMTextEntry* first_entry = new unity::IMTextEntry();
  tab_iterator->areas_.push_front(first_entry);

  unity::IMTextEntry* second_entry = new unity::IMTextEntry();
  tab_iterator->AddAreaBefore(second_entry, first_entry);

  EXPECT_TRUE(second_entry == *tab_iterator->areas_.begin());
}

TEST_F(TestTabIterator, AddAreaBeforeMissing)
{
  for(int index=0; index < 5; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  unity::IMTextEntry* first_entry = new unity::IMTextEntry();
  unity::IMTextEntry* second_entry = new unity::IMTextEntry();
  tab_iterator->AddAreaBefore(second_entry, first_entry);

  std::list<nux::InputArea*>::iterator it = std::find(tab_iterator->areas_.begin(),
    tab_iterator->areas_.end(), second_entry);

  nux::InputArea* last = tab_iterator->areas_.back();

  EXPECT_TRUE(second_entry == last);
}

TEST_F(TestTabIterator, AddAreaAfter)
{
  unity::IMTextEntry* first_entry = new unity::IMTextEntry();
  tab_iterator->areas_.push_front(first_entry);

  unity::IMTextEntry* second_entry = new unity::IMTextEntry();
  tab_iterator->AddAreaAfter(second_entry, first_entry);

  nux::InputArea* last = tab_iterator->areas_.back();

  EXPECT_TRUE(second_entry == last);
}

TEST_F(TestTabIterator, AddAreaAfterMissing)
{
  for(int index=0; index < 5; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  unity::IMTextEntry* first_entry = new unity::IMTextEntry();
  unity::IMTextEntry* second_entry = new unity::IMTextEntry();
  tab_iterator->AddAreaAfter(second_entry, first_entry);

  std::list<nux::InputArea*>::iterator it = std::find(tab_iterator->areas_.begin(),
    tab_iterator->areas_.end(), second_entry);

  nux::InputArea* last = tab_iterator->areas_.back();

  EXPECT_TRUE(second_entry == last);
}

TEST_F(TestTabIterator, GetDefaultFocus)
{
  for(int index=0; index < 10; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  unity::IMTextEntry* entry = new unity::IMTextEntry();
  tab_iterator->AddAreaFirst(entry);

  EXPECT_TRUE(tab_iterator->DefaultFocus() == entry);
}

TEST_F(TestTabIterator, GetDefaultFocusEmpty)
{
  EXPECT_TRUE(tab_iterator->DefaultFocus() == nullptr);
}

TEST_F(TestTabIterator, FindKeyFocusAreaFromWindow)
{
  nux::InputArea* current_focus_area = nux::GetWindowCompositor().GetKeyFocusArea();
  // add the area to the iterator
  tab_iterator->AddAreaFirst(current_focus_area);

  EXPECT_TRUE(tab_iterator->FindKeyFocusArea(0, 0, 0) == current_focus_area);
}

TEST_F(TestTabIterator, FindKeyFocusFromIterator)
{
  for(int index=0; index < 10; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  unity::IMTextEntry* entry = new unity::IMTextEntry();
  tab_iterator->AddAreaFirst(entry);

  EXPECT_TRUE(tab_iterator->FindKeyFocusArea(0, 0, 0) == entry);
}

TEST_F(TestTabIterator, FindKeyFocusAreaEmpty)
{
  EXPECT_TRUE(tab_iterator->FindKeyFocusArea(0, 0, 0) == nullptr);
}

TEST_F(TestTabIterator, KeyNavIterationWrongDirection)
{
  nux::Area* area = tab_iterator->KeyNavIteration(nux::KEY_NAV_NONE);
  EXPECT_TRUE(area == nullptr);
}

TEST_F(TestTabIterator, KeyNavIterationNotCurrentPrevious)
{
}

TEST_F(TestTabIterator, KeyNavIterationNotCurrentNext)
{
}

TEST_F(TestTabIterator, KeyNavIterationPreviousIsBegin)
{
}

TEST_F(TestTabIterator, KeyNavIterationPreviousIsNotBegin)
{
}

TEST_F(TestTabIterator, KeyNavIterationNextIsEnd)
{
}

TEST_F(TestTabIterator, KeyNavIterationNextIsNotEnd)
{
}

} // previews

} // dash

} // unity
