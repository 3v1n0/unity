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

using ::testing::Return;

namespace unity
{

namespace dash
{

namespace previews
{

class MockedTabIterator : public TabIterator
{
public:
  using TabIterator::areas_;
};

class TestTabIterator : public ::testing::Test
{
  protected:
    TestTabIterator() : Test(),
      tab_iterator(new MockedTabIterator())
    {}


    std::unique_ptr<MockedTabIterator> tab_iterator;
};

TEST_F(TestTabIterator, DoRemove)
{
  unity::IMTextEntry* entry = new unity::IMTextEntry();
  tab_iterator->areas_.push_front(entry);
  tab_iterator->Remove(entry);

  std::list<nux::InputArea*>::iterator it = std::find(tab_iterator->areas_.begin(),
    tab_iterator->areas_.end(), entry);
  EXPECT_EQ(it, tab_iterator->areas_.end());
}

TEST_F(TestTabIterator, DoRemoveMissing)
{
  unity::IMTextEntry* entry = new unity::IMTextEntry();
  tab_iterator->Remove(entry);

  std::list<nux::InputArea*>::iterator it = std::find(tab_iterator->areas_.begin(),
    tab_iterator->areas_.end(), entry);
  EXPECT_EQ(it, tab_iterator->areas_.end());
}

TEST_F(TestTabIterator, Prepend)
{
  for(int index=0; index < 10; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  unity::IMTextEntry* entry = new unity::IMTextEntry();
  tab_iterator->Prepend(entry);

  std::list<nux::InputArea*>::iterator it = std::find(tab_iterator->areas_.begin(),
    tab_iterator->areas_.end(), entry);

  EXPECT_EQ(it, tab_iterator->areas_.begin());
}

TEST_F(TestTabIterator, Append)
{
  for(int index=0; index < 10; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  unity::IMTextEntry* entry = new unity::IMTextEntry();
  tab_iterator->Append(entry);
  nux::InputArea* last = tab_iterator->areas_.back();

  EXPECT_EQ(entry, last);  // compare pointers
}

TEST_F(TestTabIterator, InsertIndex)
{
  for(int index=0; index < 10; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }
  unity::IMTextEntry* entry = new unity::IMTextEntry();
  tab_iterator->Insert(entry, 5);

  std::list<nux::InputArea*>::iterator it = tab_iterator->areas_.begin();
  std::advance(it, 5);

  EXPECT_NE(tab_iterator->areas_.end(), it);
  EXPECT_EQ(entry, (unity::IMTextEntry*)*it);
}

TEST_F(TestTabIterator, InsertIndexTooLarge)
{
  for(int index=0; index < 5; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  unity::IMTextEntry* entry = new unity::IMTextEntry();
  tab_iterator->Insert(entry, 7);

  nux::InputArea* last = tab_iterator->areas_.back();

  EXPECT_EQ(entry, last);  // compare pointers
}

TEST_F(TestTabIterator, InsertBefore)
{
  unity::IMTextEntry* first_entry = new unity::IMTextEntry();
  tab_iterator->areas_.push_front(first_entry);

  unity::IMTextEntry* second_entry = new unity::IMTextEntry();
  tab_iterator->InsertBefore(second_entry, first_entry);

  EXPECT_EQ(second_entry, *tab_iterator->areas_.begin());
}

TEST_F(TestTabIterator, InsertBeforeMissing)
{
  for(int index=0; index < 5; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  unity::IMTextEntry* first_entry = new unity::IMTextEntry();
  unity::IMTextEntry* second_entry = new unity::IMTextEntry();
  tab_iterator->InsertBefore(second_entry, first_entry);

  std::list<nux::InputArea*>::iterator it = std::find(tab_iterator->areas_.begin(),
    tab_iterator->areas_.end(), second_entry);

  nux::InputArea* last = tab_iterator->areas_.back();

  EXPECT_EQ(second_entry, last);
}

TEST_F(TestTabIterator, InsertAfter)
{
  unity::IMTextEntry* first_entry = new unity::IMTextEntry();
  tab_iterator->areas_.push_front(first_entry);

  unity::IMTextEntry* second_entry = new unity::IMTextEntry();
  tab_iterator->InsertAfter(second_entry, first_entry);

  nux::InputArea* last = tab_iterator->areas_.back();

  EXPECT_EQ(second_entry, last);
}

TEST_F(TestTabIterator, InsertAfterMissing)
{
  for(int index=0; index < 5; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  unity::IMTextEntry* first_entry = new unity::IMTextEntry();
  unity::IMTextEntry* second_entry = new unity::IMTextEntry();
  tab_iterator->InsertAfter(second_entry, first_entry);

  std::list<nux::InputArea*>::iterator it = std::find(tab_iterator->areas_.begin(),
    tab_iterator->areas_.end(), second_entry);

  nux::InputArea* last = tab_iterator->areas_.back();

  EXPECT_EQ(second_entry, last);
}

TEST_F(TestTabIterator, GetDefaultFocus)
{
  for(int index=0; index < 10; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  unity::IMTextEntry* entry = new unity::IMTextEntry();
  tab_iterator->Prepend(entry);

  EXPECT_EQ(tab_iterator->DefaultFocus(), entry);
}

TEST_F(TestTabIterator, GetDefaultFocusEmpty)
{
  EXPECT_EQ(tab_iterator->DefaultFocus(), nullptr);
}

TEST_F(TestTabIterator, FindKeyFocusAreaFromWindow)
{
  nux::InputArea* current_focus_area = nux::GetWindowCompositor().GetKeyFocusArea();
  // add the area to the iterator
  tab_iterator->Prepend(current_focus_area);

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
  tab_iterator->Prepend(entry);

  EXPECT_TRUE(tab_iterator->FindKeyFocusArea(0, 0, 0) == entry);
}

TEST_F(TestTabIterator, FindKeyFocusAreaEmpty)
{
  EXPECT_TRUE(tab_iterator->FindKeyFocusArea(0, 0, 0) == nullptr);
}

TEST_F(TestTabIterator, KeyNavIterationEmpty)
{
  nux::Area* area = tab_iterator->KeyNavIteration(nux::KEY_NAV_TAB_PREVIOUS);
  EXPECT_EQ(area, nullptr);
}

TEST_F(TestTabIterator, KeyNavIterationWrongDirection)
{
  nux::Area* area = tab_iterator->KeyNavIteration(nux::KEY_NAV_NONE);
  EXPECT_EQ(area, nullptr);
}

TEST_F(TestTabIterator, KeyNavIterationWithNoCurrentSelectionAndPreviousMove)
{
  unity::IMTextEntry* first_entry = new unity::IMTextEntry();
  unity::IMTextEntry* second_entry = new unity::IMTextEntry();
  tab_iterator->areas_.push_front(second_entry);
  tab_iterator->areas_.push_front(first_entry);

  unity::IMTextEntry* result = (unity::IMTextEntry*) tab_iterator->KeyNavIteration(
    nux::KEY_NAV_TAB_PREVIOUS);

  EXPECT_EQ(result, *tab_iterator->areas_.end());
}

TEST_F(TestTabIterator, KeyNavIterationNoCurrentSelectionAndNextMove)
{
  unity::IMTextEntry* first_entry = new unity::IMTextEntry();
  unity::IMTextEntry* second_entry = new unity::IMTextEntry();
  tab_iterator->areas_.push_front(second_entry);
  tab_iterator->areas_.push_front(first_entry);

  unity::IMTextEntry* result = (unity::IMTextEntry*) tab_iterator->KeyNavIteration(
    nux::KEY_NAV_TAB_NEXT);

  EXPECT_EQ(result, *tab_iterator->areas_.begin());
  EXPECT_EQ(result, first_entry);
}

TEST_F(TestTabIterator, KeyNavIterationWithPreviousSelectionIsFirstArea)
{
  unity::IMTextEntry* first_entry = new unity::IMTextEntry();
  unity::IMTextEntry* second_entry = new unity::IMTextEntry();
  tab_iterator->areas_.push_front(second_entry);
  tab_iterator->areas_.push_front(first_entry);

  nux::GetWindowCompositor().SetKeyFocusArea(first_entry);

  unity::IMTextEntry* result = (unity::IMTextEntry*) tab_iterator->KeyNavIteration(
    nux::KEY_NAV_TAB_PREVIOUS);

  EXPECT_EQ(result, *tab_iterator->areas_.end());
}

TEST_F(TestTabIterator, KeyNavIterationWithPreviousSelectionIsNotFirst)
{
  for(int index=0; index < 10; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  unity::IMTextEntry* first_entry = new unity::IMTextEntry();
  unity::IMTextEntry* second_entry = new unity::IMTextEntry();
  tab_iterator->areas_.push_front(second_entry);
  tab_iterator->areas_.push_front(first_entry);

  for(int index=0; index < 10; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  nux::GetWindowCompositor().SetKeyFocusArea(second_entry);

  unity::IMTextEntry* result = (unity::IMTextEntry*) tab_iterator->KeyNavIteration(
    nux::KEY_NAV_TAB_PREVIOUS);
  EXPECT_EQ(result, first_entry);
}

TEST_F(TestTabIterator, KeyNavIterationWithNextSelectionIsLast)
{
  unity::IMTextEntry* first_entry = new unity::IMTextEntry();
  unity::IMTextEntry* second_entry = new unity::IMTextEntry();
  unity::IMTextEntry* not_in_areas = new unity::IMTextEntry();

  tab_iterator->areas_.push_front(second_entry);
  tab_iterator->areas_.push_front(first_entry);

  nux::GetWindowCompositor().SetKeyFocusArea(not_in_areas);

  unity::IMTextEntry* result = (unity::IMTextEntry*) tab_iterator->KeyNavIteration(
    nux::KEY_NAV_TAB_NEXT);

  EXPECT_EQ(result, *tab_iterator->areas_.begin());
  EXPECT_EQ(result, first_entry);
}

TEST_F(TestTabIterator, KeyNavIterationWithNextSelectionIsNotLast)
{
  for(int index=0; index < 10; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  unity::IMTextEntry* first_entry = new unity::IMTextEntry();
  unity::IMTextEntry* second_entry = new unity::IMTextEntry();
  tab_iterator->areas_.push_front(second_entry);
  tab_iterator->areas_.push_front(first_entry);

  for(int index=0; index < 10; ++index)
  {
    unity::IMTextEntry* entry = new unity::IMTextEntry();
    tab_iterator->areas_.push_front(entry);
  }

  nux::GetWindowCompositor().SetKeyFocusArea(first_entry);

  unity::IMTextEntry* result = (unity::IMTextEntry*) tab_iterator->KeyNavIteration(
    nux::KEY_NAV_TAB_NEXT);
  EXPECT_EQ(result, second_entry);
}

} // previews

} // dash

} // unity
