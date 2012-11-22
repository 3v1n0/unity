/*
 * Copyright 2012 Canonical Ltd.
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


#include <gtest/gtest.h>

#include "unity-shared/TextInput.h"
#include "test_utils.h"

using namespace nux;

namespace unity
{

class TestTextInput : public ::testing::Test
{
   protected:
     TestTextInput()
     {
       entry = new TextInput();
       entry->Init();
     }

     TextInput* entry;
};

TEST_F(TestTextInput, HintCorrectInit)
{
  nux::Color color = entry->hint_->GetTextColor();

  EXPECT_EQ(color.red, 1.0f);
  EXPECT_EQ(color.green, 1.0f);
  EXPECT_EQ(color.blue, 1.0f);
  EXPECT_EQ(color.alpha, 0.5f);

  EXPECT_EQ(entry->hint_->GetFont(), "Ubuntu Italic 12px");
}

TEST_F(TestTextInput, InputStringCorrectSetter)
{
  // set the string and test that we do indeed set the internal va
  std::string new_input = "foo";
  entry->input_string.Set(new_input);
  EXPECT_EQ(entry->input_string.Get(), new_input);
}

TEST_F(TestTextInput, OnInputHintChanged)
{
  // change the hint and assert that the internal value is correct
  entry->hint_->SetText("foo");
  entry->OnInputHintChanged();
  EXPECT_EQ(entry->hint_->GetText(), "");
}

TEST_F(TestTextInput, OnMouseButtonDown)
{
  entry->hint_->SetVisible(true);
  entry->OnMouseButtonDown(0, 0, 0, 0);
  ASSERT_FALSE(entry->hint_->IsVisible());
}

TEST_F(TestTextInput, OnEndKeyFocus)
{
  entry->OnEndKeyFocus();
  EXPECT_EQ(entry->hint_->GetText(), " ");
}

} // unity
