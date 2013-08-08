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
#include <pango/pango.h>
#include "test_utils.h"

using namespace unity;

namespace
{

class MockStaticCairoText : public StaticCairoText
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
  TestStaticCairoText()
    : text(new NiceMock<MockStaticCairoText>())
  {}

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

  // assert that we do call the set base size that ensures that the layout will
  // allocate enough space.
  EXPECT_CALL(*text.GetPointer(), SetBaseSize(_, _)).Times(1);
   text->PreLayoutManagement();
}

TEST_F(TestStaticCairoText, TextLeftToRightExtentIncreasesWithLength)
{
  std::string string = "Just a test string of awesome text!";
  nux::Size const& extents = text->GetTextExtents();
  ASSERT_EQ(PANGO_DIRECTION_LTR, pango_find_base_dir(string.c_str(), -1));
  ASSERT_EQ(0, extents.width);
  unsigned old_width = 0;

  for (unsigned pos = 1; pos <= string.length(); ++pos)
  {
    auto const& substr = string.substr(0, pos);
    text->SetText(substr);
    ASSERT_LT(old_width, text->GetTextExtents().width);
    old_width = text->GetTextExtents().width;
  }
}

TEST_F(TestStaticCairoText, TextRightToLeftExtentIncreasesWithLength)
{
  std::string string = "\xd7\x9e\xd7\x97\xd7\xa8\xd7\x95\xd7\x96\xd7\xaa"
                       " \xd7\x90\xd7\xa7\xd7\xa8\xd7\x90\xd7\x99\xd7\xaa"
                       "\xd7\xa2\xd7\x91\xd7\xa8\xd7\x99\xd7\xaa";

  nux::Size const& extents = text->GetTextExtents();
  ASSERT_EQ(PANGO_DIRECTION_RTL, pango_find_base_dir(string.c_str(), -1));
  ASSERT_EQ(0, extents.width);
  unsigned old_width = std::numeric_limits<unsigned>::max();

  for (int pos = 1; pos <= g_utf8_strlen(string.c_str(), -1); ++pos)
  {
    std::string substr = g_utf8_offset_to_pointer(string.c_str(), pos);
    text->SetText(substr);
    ASSERT_GT(old_width, text->GetTextExtents().width);
    old_width = text->GetTextExtents().width;
  }
}

}
