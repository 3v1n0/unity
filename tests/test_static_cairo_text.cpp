// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include <list>
#include <gmock/gmock.h>
using namespace testing;

#include <Nux/Nux.h>
#include <unity-shared/StaticCairoText.h>
#include "test_utils.h"

using namespace unity;

namespace
{

class MockStaticCairoText : public nux::StaticCairoText
{
public:
  MOCK_METHOD2(SetBaseSize, void(int, int));

  MockStaticCairoText():StaticCairoText("") {}

  using StaticCairoText::GetTextureStartIndices;
  using StaticCairoText::GetTextureEndIndices;
  using StaticCairoText::PreLayoutManagement;
};

class TestStaticCairoText : public ::testing::Test
{

protected:
  TestStaticCairoText() : Test()
  {
    text = new MockStaticCairoText();
  }

  nux::ObjectPtr<MockStaticCairoText> text;

};

TEST_F(TestStaticCairoText, TextTextureSize)
{
  EXPECT_CALL(*text.GetPointer(), SetBaseSize(_, _)).Times(AnyNumber());
  // Test multi-texture stitching support.

  text->SetLines(-2000);
  text->SetMaximumWidth(100);

  std::stringstream ss;
  std::vector<unsigned> starts;
  std::vector<unsigned> ends;
  while(starts.size() < 3)
  {
    for (int i = 0; i < 100; i++)
      ss << "Test string\n";
    text->SetText(ss.str());

    starts = text->GetTextureStartIndices();
    ends = text->GetTextureEndIndices();
    ASSERT_TRUE(starts.size() == ends.size());

    for (unsigned int start_index = 0; start_index < starts.size(); start_index++)
    {
      if (start_index > 0)
      {
        ASSERT_EQ(starts[start_index], ends[start_index-1]+1);
      }
    }
  }
}

TEST_F(TestStaticCairoText, TextPreLayoutManagementMultipleCalls)
{
  EXPECT_CALL(*text.GetPointer(), SetBaseSize(_, _)).Times(2);
  text->PreLayoutManagement();
  
  // the first prelayout methods should have called set base size and therefore
  // we should not call it again

  EXPECT_CALL(*text.GetPointer(), SetBaseSize(_, _)).Times(0);
   text->PreLayoutManagement();
}

}
