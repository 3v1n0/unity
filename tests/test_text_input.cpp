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

#include "unity-shared/DashStyle.h"
#include "unity-shared/TextInput.h"
#include "unity-shared/UnitySettings.h"
#include "test_utils.h"

using namespace nux;

namespace unity
{

class TextInputMock : public TextInput
{
    public:
      using TextInput::Init;
      using TextInput::OnInputHintChanged;
      using TextInput::OnMouseButtonDown;
      using TextInput::OnEndKeyFocus;
      using TextInput::get_input_string;


      StaticCairoText* GetHint() const { return hint_; }
      IMTextEntry* GetPangoEntry() const { return pango_entry_; }
};

class TestTextInput : public ::testing::Test
{
   protected:
     TestTextInput()
     {
       entry = new TextInputMock();
       entry->Init();
       hint = entry->GetHint();
       pango_entry = entry->GetPangoEntry();
     }

     unity::Settings unity_settings_;
     dash::Style dash_style_;
     nux::ObjectPtr<TextInputMock> entry;
     StaticCairoText* hint;
     IMTextEntry* pango_entry;
};

TEST_F(TestTextInput, HintCorrectInit)
{
  nux::Color color = hint->GetTextColor();

  EXPECT_EQ(color.red, 1.0f);
  EXPECT_EQ(color.green, 1.0f);
  EXPECT_EQ(color.blue, 1.0f);
  EXPECT_EQ(color.alpha, 0.5f);
}

TEST_F(TestTextInput, InputStringCorrectSetter)
{
  // set the string and test that we do indeed set the internal va
  std::string new_input = "foo";
  entry->input_string.Set(new_input);
  EXPECT_EQ(entry->input_string.Get(), new_input);
}

TEST_F(TestTextInput, HintClearedOnInputHintChanged)
{
  // change the hint and assert that the internal value is correct
  hint->SetText("foo");
  entry->OnInputHintChanged();
  EXPECT_EQ(entry->get_input_string(), "");
}

TEST_F(TestTextInput, HintHideOnMouseButtonDown)
{
  hint->SetVisible(true);
  entry->OnMouseButtonDown(entry->GetBaseWidth()/2,
    entry->GetBaseHeight()/2  , 0, 0);
  EXPECT_FALSE(hint->IsVisible());
}

TEST_F(TestTextInput, HintVisibleOnEndKeyFocus)
{
  // set the text and ensure that later is cleared
  pango_entry->SetText("user input");
  entry->OnEndKeyFocus();

  EXPECT_FALSE(hint->IsVisible());

}

TEST_F(TestTextInput, HintHiddenOnEndKeyFocus)
{

  pango_entry->SetText("");
  entry->OnEndKeyFocus();

  EXPECT_TRUE(hint->IsVisible());
}

} // unity
