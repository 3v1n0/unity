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
 * Authored by: Diego Sarmentero <diego.sarmentero@canonical.com>
 *
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <unity-shared/StaticCairoText.h>

#include "dash/previews/ErrorPreview.cpp"
#include "test_utils.h"

using namespace nux;
using namespace unity;
using namespace unity::dash;

namespace unity
{

namespace dash
{

class ErrorPreviewMock : public ErrorPreview
{
  public:
    MOCK_METHOD0(QueueDraw, void());
    MOCK_METHOD0(ComputeContentSize, long());
    MOCK_METHOD2(CalculateBar, void(nux::GraphicsEngine&, bool));

    ErrorPreviewMock(dash::Preview::Ptr preview_model)
      : ErrorPreview(preview_model){}
    ~ErrorPreviewMock(){}

//    nux::ObjectPtr<nux::StaticCairoText> GetText() { return static_text_; }

    using ErrorPreview::GetName;
    using ErrorPreview::GetTitle;
    using ErrorPreview::GetPrize;
    using ErrorPreview::GetBody;
    using ErrorPreview::GetFooter;
};

class TestErrorPreview : public ::testing::Test
{
  protected:
    TestErrorPreview() : Test()
    {
      action_link = new ErrorPreviewMock("action_id", "display_name");
    }
    nux::ObjectPtr<ErrorPreviewMock> action_link;
};

TEST_F(TestErrorPreview, AligmentCorrectlySetDifferent)
{
  ErrorPreviewMock link("test", "test");

  EXPECT_CALL(link, ComputeContentSize()).Times(1);
  EXPECT_CALL(link, QueueDraw()).Times(1);

  link.text_aligment.Set(nux::StaticCairoText::NUX_ALIGN_RIGHT);
}

TEST_F(TestErrorPreview, AligmentCorrectlySetSame)
{
  ErrorPreviewMock link("test", "test");

  EXPECT_CALL(link, ComputeContentSize()).Times(0);
  EXPECT_CALL(link, QueueDraw()).Times(0);

  link.text_aligment.Set(link.text_aligment.Get());
}

TEST_F(TestErrorPreview, AligmentCorrectlyRetrieved)
{
  nux::StaticCairoText::AlignState aligment =
    nux::StaticCairoText::NUX_ALIGN_RIGHT;
  action_link->aligment_ = aligment;
  EXPECT_EQ(aligment, action_link->text_aligment.Get());
}

TEST_F(TestErrorPreview, UnderlineCorrectlySetDifferent)
{
  ErrorPreviewMock link("test", "test");

  EXPECT_CALL(link, ComputeContentSize()).Times(1);
  EXPECT_CALL(link, QueueDraw()).Times(1);
  link.underline_state.Set(nux::StaticCairoText::NUX_UNDERLINE_NONE);
}

TEST_F(TestErrorPreview, UnderlineCorrectlySetSame)
{
  ErrorPreviewMock link("test", "test");

  EXPECT_CALL(link, ComputeContentSize()).Times(0);
  EXPECT_CALL(link, QueueDraw()).Times(0);
  link.underline_state.Set(link.underline_state.Get());
}

TEST_F(TestErrorPreview, UnderlineCorrectlyRetrieved)
{
  nux::StaticCairoText::UnderlineState underline =
    nux::StaticCairoText::NUX_UNDERLINE_DOUBLE;
  action_link->underline_ = underline;
  EXPECT_EQ(underline, action_link->underline_state.Get());
}

TEST_F(TestErrorPreview, FontCorrectlySetDifferent)
{
  ErrorPreviewMock link("test", "test");
  link.font_hint_ = "Ubuntu 10";

  EXPECT_CALL(link, ComputeContentSize()).Times(1);
  EXPECT_CALL(link, QueueDraw()).Times(1);
  link.font_hint.Set("Ubuntu 11");
}

TEST_F(TestErrorPreview, FontCorrectlySetSame)
{
  ErrorPreviewMock link("test", "test");
  link.font_hint_ = "Ubuntu 10";

  EXPECT_CALL(link, ComputeContentSize()).Times(0);
  EXPECT_CALL(link, QueueDraw()).Times(0);
  link.font_hint.Set(link.font_hint.Get());
}

TEST_F(TestErrorPreview, FontCorrectlyRetrieved)
{
  std::string font_hint = "Ubuntu 11";
  action_link->font_hint_ = font_hint;
  EXPECT_EQ(font_hint, action_link->font_hint.Get());
}

TEST_F(TestErrorPreview, LinkAlphaOnPressed)
{
   ButtonVisualState state = ButtonVisualState::VISUAL_STATE_PRESSED;
   EXPECT_EQ(4, action_link->GetLinkAlpha(state));
}

TEST_F(TestErrorPreview, LinkAlphaOnNormal)
{
   ButtonVisualState state = ButtonVisualState::VISUAL_STATE_NORMAL;
   EXPECT_EQ(4, action_link->GetLinkAlpha(state));
}

TEST_F(TestErrorPreview, LinkAlphaOnPrelight)
{
   ButtonVisualState state = ButtonVisualState::VISUAL_STATE_PRELIGHT;
   EXPECT_EQ(1, action_link->GetLinkAlpha(state));
}

TEST_F(TestErrorPreview, LinkAlphaOnDisabled)
{
   ButtonVisualState state = ButtonVisualState::VISUAL_STATE_DISABLED;
   EXPECT_EQ(4, action_link->GetLinkAlpha(state));
}

}

}
