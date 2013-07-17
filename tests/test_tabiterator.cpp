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
 *              Marco Trevisan <marco.trevisan@canonical.com>
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

typedef nux::ObjectPtr<IMTextEntry> IMTextEntryPtr;

struct TestTabIterator : ::testing::Test
{
  struct MockedTabIterator : public TabIterator
  {
    using TabIterator::areas_;
  };

  MockedTabIterator tab_iterator;
};

TEST_F(TestTabIterator, DoRemove)
{
  auto entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.areas_.push_back(entry.GetPointer());
  tab_iterator.Remove(entry.GetPointer());

  std::list<nux::InputArea*>::iterator it = std::find(tab_iterator.areas_.begin(),
    tab_iterator.areas_.end(), entry.GetPointer());
  EXPECT_EQ(it, tab_iterator.areas_.end());
}

TEST_F(TestTabIterator, DoRemoveMissing)
{
  auto entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.Remove(entry.GetPointer());

  std::list<nux::InputArea*>::iterator it = std::find(tab_iterator.areas_.begin(),
    tab_iterator.areas_.end(), entry.GetPointer());
  EXPECT_EQ(it, tab_iterator.areas_.end());
}

TEST_F(TestTabIterator, Prepend)
{
  std::vector<IMTextEntryPtr> local_areas;

  for(int index=0; index < 10; ++index)
  {
    local_areas.push_back(IMTextEntryPtr(new IMTextEntry));
    tab_iterator.areas_.push_back(local_areas.back().GetPointer());
  }

  auto entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.Prepend(entry.GetPointer());

  std::list<nux::InputArea*>::iterator it = std::find(tab_iterator.areas_.begin(),
    tab_iterator.areas_.end(), entry.GetPointer());

  EXPECT_EQ(it, tab_iterator.areas_.begin());
}

TEST_F(TestTabIterator, Append)
{
  std::vector<IMTextEntryPtr> local_areas;

  for(int index=0; index < 10; ++index)
  {
    local_areas.push_back(IMTextEntryPtr(new IMTextEntry));
    tab_iterator.areas_.push_back(local_areas.back().GetPointer());
  }

  auto entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.Append(entry.GetPointer());
  nux::InputArea* last = tab_iterator.areas_.back();

  EXPECT_EQ(entry.GetPointer(), last);  // compare pointers
}

TEST_F(TestTabIterator, InsertIndex)
{
  std::vector<IMTextEntryPtr> local_areas;

  for(int index=0; index < 10; ++index)
  {
    local_areas.push_back(IMTextEntryPtr(new IMTextEntry));
    tab_iterator.areas_.push_back(local_areas.back().GetPointer());
  }

  auto entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.Insert(entry.GetPointer(), 5);

  std::list<nux::InputArea*>::iterator it = tab_iterator.areas_.begin();
  std::advance(it, 5);

  EXPECT_NE(tab_iterator.areas_.end(), it);
  EXPECT_EQ(entry.GetPointer(), *it);
}

TEST_F(TestTabIterator, InsertIndexTooLarge)
{
  std::vector<IMTextEntryPtr> local_areas;

  for(int index=0; index < 5; ++index)
  {
    local_areas.push_back(IMTextEntryPtr(new IMTextEntry));
    tab_iterator.areas_.push_back(local_areas.back().GetPointer());
  }

  auto entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.Insert(entry.GetPointer(), 7);

  nux::InputArea* last = tab_iterator.areas_.back();

  EXPECT_EQ(entry.GetPointer(), last);  // compare pointers
}

TEST_F(TestTabIterator, InsertBefore)
{
  auto first_entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.areas_.push_back(first_entry.GetPointer());

  auto second_entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.InsertBefore(second_entry.GetPointer(), first_entry.GetPointer());

  EXPECT_EQ(second_entry.GetPointer(), *tab_iterator.areas_.begin());
}

TEST_F(TestTabIterator, InsertBeforeMissing)
{
  std::vector<IMTextEntryPtr> local_areas;

  for(int index=0; index < 5; ++index)
  {
    local_areas.push_back(IMTextEntryPtr(new IMTextEntry));
    tab_iterator.areas_.push_back(local_areas.back().GetPointer());
  }

  auto first_entry = IMTextEntryPtr(new IMTextEntry);
  auto second_entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.InsertBefore(second_entry.GetPointer(), first_entry.GetPointer());

  std::list<nux::InputArea*>::iterator it = std::find(tab_iterator.areas_.begin(),
    tab_iterator.areas_.end(), second_entry.GetPointer());

  nux::InputArea* last = tab_iterator.areas_.back();

  EXPECT_EQ(second_entry.GetPointer(), last);
}

TEST_F(TestTabIterator, InsertAfter)
{
  auto first_entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.areas_.push_back(first_entry.GetPointer());

  auto second_entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.InsertAfter(second_entry.GetPointer(), first_entry.GetPointer());

  nux::InputArea* last = tab_iterator.areas_.back();

  EXPECT_EQ(second_entry.GetPointer(), last);
}

TEST_F(TestTabIterator, InsertAfterMissing)
{
  std::vector<IMTextEntryPtr> local_areas;

  for(int index=0; index < 5; ++index)
  {
    local_areas.push_back(IMTextEntryPtr(new IMTextEntry));
    tab_iterator.areas_.push_back(local_areas.back().GetPointer());
  }

  auto first_entry = IMTextEntryPtr(new IMTextEntry);
  auto second_entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.InsertAfter(second_entry.GetPointer(), first_entry.GetPointer());

  std::list<nux::InputArea*>::iterator it = std::find(tab_iterator.areas_.begin(),
    tab_iterator.areas_.end(), second_entry.GetPointer());

  nux::InputArea* last = tab_iterator.areas_.back();

  EXPECT_EQ(second_entry.GetPointer(), last);
}

TEST_F(TestTabIterator, GetDefaultFocus)
{
  std::vector<IMTextEntryPtr> local_areas;

  for(int index=0; index < 10; ++index)
  {
    local_areas.push_back(IMTextEntryPtr(new IMTextEntry));
    tab_iterator.areas_.push_back(local_areas.back().GetPointer());
  }

  auto entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.Prepend(entry.GetPointer());

  EXPECT_EQ(tab_iterator.DefaultFocus(), entry.GetPointer());
}

TEST_F(TestTabIterator, GetDefaultFocusEmpty)
{
  EXPECT_EQ(tab_iterator.DefaultFocus(), nullptr);
}

TEST_F(TestTabIterator, FindKeyFocusAreaFromWindow)
{
  nux::InputArea* current_focus_area = nux::GetWindowCompositor().GetKeyFocusArea();
  // add the area to the iterator
  tab_iterator.Prepend(current_focus_area);

  EXPECT_EQ(tab_iterator.FindKeyFocusArea(0, 0, 0), current_focus_area);
}

TEST_F(TestTabIterator, FindKeyFocusFromIterator)
{
  std::vector<IMTextEntryPtr> local_areas;

  for(int index=0; index < 10; ++index)
  {
    local_areas.push_back(IMTextEntryPtr(new IMTextEntry));
    tab_iterator.areas_.push_back(local_areas.back().GetPointer());
  }

  auto entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.Prepend(entry.GetPointer());

  EXPECT_EQ(tab_iterator.FindKeyFocusArea(0, 0, 0), entry.GetPointer());
}

TEST_F(TestTabIterator, FindKeyFocusAreaEmpty)
{
  EXPECT_EQ(tab_iterator.FindKeyFocusArea(0, 0, 0), nullptr);
}

TEST_F(TestTabIterator, KeyNavIterationEmpty)
{
  nux::Area* area = tab_iterator.KeyNavIteration(nux::KEY_NAV_TAB_PREVIOUS);
  EXPECT_EQ(area, nullptr);
}

TEST_F(TestTabIterator, KeyNavIterationWrongDirection)
{
  nux::Area* area = tab_iterator.KeyNavIteration(nux::KEY_NAV_NONE);
  EXPECT_EQ(area, nullptr);
}

TEST_F(TestTabIterator, KeyNavIterationWithNoCurrentSelectionAndPreviousMove)
{
  auto first_entry = IMTextEntryPtr(new IMTextEntry);
  auto second_entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.areas_.push_back(first_entry.GetPointer());
  tab_iterator.areas_.push_back(second_entry.GetPointer());

  IMTextEntry* result = (IMTextEntry*) tab_iterator.KeyNavIteration(
    nux::KEY_NAV_TAB_PREVIOUS);

  EXPECT_EQ(result, *tab_iterator.areas_.rbegin());
  EXPECT_EQ(result, second_entry.GetPointer());
}

TEST_F(TestTabIterator, KeyNavIterationNoCurrentSelectionAndNextMove)
{
  auto first_entry = IMTextEntryPtr(new IMTextEntry);
  auto second_entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.areas_.push_back(first_entry.GetPointer());
  tab_iterator.areas_.push_back(second_entry.GetPointer());

  IMTextEntry* result = (IMTextEntry*) tab_iterator.KeyNavIteration(
    nux::KEY_NAV_TAB_NEXT);

  EXPECT_EQ(result, *tab_iterator.areas_.begin());
  EXPECT_EQ(result, first_entry.GetPointer());
}

TEST_F(TestTabIterator, KeyNavIterationWithPreviousSelectionIsFirstArea)
{
  auto first_entry = IMTextEntryPtr(new IMTextEntry);
  auto second_entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.areas_.push_back(first_entry.GetPointer());
  tab_iterator.areas_.push_back(second_entry.GetPointer());

  nux::GetWindowCompositor().SetKeyFocusArea(first_entry.GetPointer());

  IMTextEntry* result = (IMTextEntry*) tab_iterator.KeyNavIteration(
    nux::KEY_NAV_TAB_PREVIOUS);

  EXPECT_EQ(result, *tab_iterator.areas_.rbegin());
  EXPECT_EQ(result, second_entry.GetPointer());
}

TEST_F(TestTabIterator, KeyNavIterationWithPreviousSelectionIsNotFirst)
{
  std::vector<IMTextEntryPtr> local_areas;

  for(int index=0; index < 10; ++index)
  {
    local_areas.push_back(IMTextEntryPtr(new IMTextEntry));
    tab_iterator.areas_.push_back(local_areas.back().GetPointer());
  }

  auto first_entry = IMTextEntryPtr(new IMTextEntry);
  auto second_entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.areas_.push_back(first_entry.GetPointer());
  tab_iterator.areas_.push_back(second_entry.GetPointer());

  for(int index=0; index < 10; ++index)
  {
    local_areas.push_back(IMTextEntryPtr(new IMTextEntry));
    tab_iterator.areas_.push_back(local_areas.back().GetPointer());
  }

  nux::GetWindowCompositor().SetKeyFocusArea(second_entry.GetPointer());

  IMTextEntry* result = (IMTextEntry*) tab_iterator.KeyNavIteration(
    nux::KEY_NAV_TAB_PREVIOUS);
  EXPECT_EQ(result, first_entry.GetPointer());
}

TEST_F(TestTabIterator, KeyNavIterationWithNextSelectionIsLast)
{
  auto first_entry = IMTextEntryPtr(new IMTextEntry);
  auto second_entry = IMTextEntryPtr(new IMTextEntry);
  auto not_in_areas = IMTextEntryPtr(new IMTextEntry);

  tab_iterator.areas_.push_back(first_entry.GetPointer());
  tab_iterator.areas_.push_back(second_entry.GetPointer());

  nux::GetWindowCompositor().SetKeyFocusArea(not_in_areas.GetPointer());

  IMTextEntry* result = (IMTextEntry*) tab_iterator.KeyNavIteration(
    nux::KEY_NAV_TAB_NEXT);

  EXPECT_EQ(result, *tab_iterator.areas_.begin());
  EXPECT_EQ(result, first_entry.GetPointer());
}

TEST_F(TestTabIterator, KeyNavIterationWithNextSelectionIsNotLast)
{
  std::vector<IMTextEntryPtr> local_areas;

  for(int index=0; index < 10; ++index)
  {
    local_areas.push_back(IMTextEntryPtr(new IMTextEntry));
    tab_iterator.areas_.push_back(local_areas.back().GetPointer());
  }

  auto first_entry = IMTextEntryPtr(new IMTextEntry);
  auto second_entry = IMTextEntryPtr(new IMTextEntry);
  tab_iterator.areas_.push_back(first_entry.GetPointer());
  tab_iterator.areas_.push_back(second_entry.GetPointer());

  for(int index=0; index < 10; ++index)
  {
    local_areas.push_back(IMTextEntryPtr(new IMTextEntry));
    tab_iterator.areas_.push_back(local_areas.back().GetPointer());
  }
  nux::GetWindowCompositor().SetKeyFocusArea(first_entry.GetPointer());

  IMTextEntry* result = (IMTextEntry*) tab_iterator.KeyNavIteration(
    nux::KEY_NAV_TAB_NEXT);
  EXPECT_EQ(result, second_entry.GetPointer());
}

} // previews

} // dash

} // unity
