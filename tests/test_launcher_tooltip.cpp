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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "unity-shared/StaticCairoText.h"
#include "launcher/Tooltip.h"

#include <unity-shared/UnitySettings.h>
#include "test_utils.h"

namespace unity
{

class MockStaticCairoText : public nux::StaticCairoText
{
public:
  MOCK_METHOD1(SetMinimumWidth, void(int));
  MOCK_METHOD1(SetMinimumHeight, void(int));

  MockStaticCairoText(std::string const& text):StaticCairoText(text) {}

}; // StaticCairoText

class TooltipMock : public Tooltip
{
public:
  TooltipMock() : Tooltip()
  {
    // change the text and reconnect it as it should
    _tooltip_text = new MockStaticCairoText(_labelText);
    _tooltip_text->SetTextAlignment(
      nux::StaticCairoText::AlignState::NUX_ALIGN_CENTRE);
    _tooltip_text->SetTextVerticalAlignment(
      nux::StaticCairoText::AlignState::NUX_ALIGN_CENTRE);

    _tooltip_text->sigTextChanged.connect(
      sigc::mem_fun(this, &TooltipMock::RecvCairoTextChanged));
    _tooltip_text->sigFontChanged.connect(
      sigc::mem_fun(this, &TooltipMock::RecvCairoTextChanged));
  }

  using Tooltip::_tooltip_text;
  using Tooltip::RecvCairoTextChanged;
  using Tooltip::PreLayoutManagement;

}; // TooltipMock


class TestTooltip : public ::testing::Test
{
protected:
  TestTooltip() : Test()
  {
    settings = new Settings();
    tooltip = new TooltipMock();
  }

  Settings* settings;
  nux::ObjectPtr<TooltipMock> tooltip;
}; // TestTooltip

TEST_F(TestTooltip, StaticCairoTextCorrectSize)
{
  int text_width;
  int text_height;
  tooltip->_tooltip_text->GetTextExtents(text_width, text_height);

  // do assert that the methods are called with at least the min size provided
  // by the GetTextExtents method
  MockStaticCairoText* text = dynamic_cast<MockStaticCairoText*>(
    tooltip->_tooltip_text.GetPointer());

  EXPECT_CALL(*text, SetMinimumWidth(testing::Ge(text_width)));
  EXPECT_CALL(*text, SetMinimumHeight(testing::Ge(text_height)));

  tooltip->PreLayoutManagement();
}

} // unity
