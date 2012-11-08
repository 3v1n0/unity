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
  MockStaticCairoText():StaticCairoText("") {}

  using StaticCairoText::GetTextureStartIndices;
  using StaticCairoText::GetTextureEndIndices;
};

TEST(TestStaticCairoText, TextTextureSize)
{
  // Test multi-texture stitching support.

  nux::ObjectPtr<MockStaticCairoText> text(new MockStaticCairoText());
  text->SetLines(-2000);
  text->SetMaximumWidth(100);

  std::stringstream ss;
  std::vector<int> starts;
  std::vector<int> ends;
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

}
