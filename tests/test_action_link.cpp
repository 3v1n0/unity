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

#include <unity-shared/StaticCairoText.h>

#include "dash/previews/ActionLink.cpp"
#include "test_utils.h"

using namespace nux;
using namespace unity;
using namespace unity::dash;

namespace unity
{

namespace dash
{

class ActionLinkMock : public ActionLink
{
  public:
    MOCK_METHOD0(QueueDraw, void());
    MOCK_METHOD0(ComputeContentSize, long());
    MOCK_METHOD2(CalculateBar, void(nux::GraphicsEngine&, bool));

    ActionLinkMock(std::string const& action_hint, std::string const& label, NUX_FILE_LINE_PROTO)
      : ActionLink(action_hint, label){}
    ~ActionLinkMock(){}

    nux::ObjectPtr<StaticCairoText> GetText() { return static_text_; }

    using ActionLink::GetLinkAlpha;
    using ActionLink::Draw;
    using ActionLink::DrawContent;
    using ActionLink::RecvClick;
    using ActionLink::GetName;
    using ActionLink::AddProperties;
    using ActionLink::set_aligment;
    using ActionLink::get_aligment;
    using ActionLink::set_underline;
    using ActionLink::get_underline;
    using ActionLink::set_font_hint;
    using ActionLink::get_font_hint;
    using ActionLink::action_hint_;
    using ActionLink::font_hint_;
    using ActionLink::aligment_;
    using ActionLink::underline_;
};

class TestActionLink : public ::testing::Test
{
  protected:
    TestActionLink() : Test()
    {
      action_link = new ActionLinkMock("action_id", "display_name");
    }
    nux::ObjectPtr<ActionLinkMock> action_link;
};

TEST_F(TestActionLink, AligmentCorrectlySetDifferent)
{
  ActionLinkMock link("test", "test");

  EXPECT_CALL(link, ComputeContentSize()).Times(1);
  EXPECT_CALL(link, QueueDraw()).Times(1);

  link.text_aligment.Set(StaticCairoText::NUX_ALIGN_RIGHT);
}

TEST_F(TestActionLink, AligmentCorrectlySetSame)
{
  ActionLinkMock link("test", "test");

  EXPECT_CALL(link, ComputeContentSize()).Times(0);
  EXPECT_CALL(link, QueueDraw()).Times(0);

  link.text_aligment.Set(link.text_aligment.Get());
}

TEST_F(TestActionLink, AligmentCorrectlyRetrieved)
{
  StaticCairoText::AlignState aligment =
    StaticCairoText::NUX_ALIGN_RIGHT;
  action_link->aligment_ = aligment;
  EXPECT_EQ(aligment, action_link->text_aligment.Get());
}

TEST_F(TestActionLink, UnderlineCorrectlySetDifferent)
{
  ActionLinkMock link("test", "test");

  EXPECT_CALL(link, ComputeContentSize()).Times(1);
  EXPECT_CALL(link, QueueDraw()).Times(1);
  link.underline_state.Set(StaticCairoText::NUX_UNDERLINE_NONE);
}

TEST_F(TestActionLink, UnderlineCorrectlySetSame)
{
  ActionLinkMock link("test", "test");

  EXPECT_CALL(link, ComputeContentSize()).Times(0);
  EXPECT_CALL(link, QueueDraw()).Times(0);
  link.underline_state.Set(link.underline_state.Get());
}

TEST_F(TestActionLink, UnderlineCorrectlyRetrieved)
{
  StaticCairoText::UnderlineState underline = StaticCairoText::NUX_UNDERLINE_DOUBLE;
  action_link->underline_ = underline;
  EXPECT_EQ(underline, action_link->underline_state.Get());
}

TEST_F(TestActionLink, FontCorrectlySetDifferent)
{
  ActionLinkMock link("test", "test");
  link.font_hint_ = "Ubuntu 10";

  EXPECT_CALL(link, ComputeContentSize()).Times(1);
  EXPECT_CALL(link, QueueDraw()).Times(1);
  link.font_hint.Set("Ubuntu 11");
}

TEST_F(TestActionLink, FontCorrectlySetSame)
{
  ActionLinkMock link("test", "test");
  link.font_hint_ = "Ubuntu 10";

  EXPECT_CALL(link, ComputeContentSize()).Times(0);
  EXPECT_CALL(link, QueueDraw()).Times(0);
  link.font_hint.Set(link.font_hint.Get());
}

TEST_F(TestActionLink, FontCorrectlyRetrieved)
{
  std::string font_hint = "Ubuntu 11";
  action_link->font_hint_ = font_hint;
  EXPECT_EQ(font_hint, action_link->font_hint.Get());
}

TEST_F(TestActionLink, LinkAlphaOnPressed)
{
   ButtonVisualState state = ButtonVisualState::VISUAL_STATE_PRESSED;
   EXPECT_EQ(4, action_link->GetLinkAlpha(state, false));
}

TEST_F(TestActionLink, LinkAlphaOnNormal)
{
   ButtonVisualState state = ButtonVisualState::VISUAL_STATE_NORMAL;
   EXPECT_EQ(4, action_link->GetLinkAlpha(state, false));
}

TEST_F(TestActionLink, LinkAlphaOnNormalButKeyboardFocused)
{
   ButtonVisualState state = ButtonVisualState::VISUAL_STATE_NORMAL;
   EXPECT_EQ(1, action_link->GetLinkAlpha(state, true));
}

TEST_F(TestActionLink, LinkAlphaOnPrelight)
{
   ButtonVisualState state = ButtonVisualState::VISUAL_STATE_PRELIGHT;
   EXPECT_EQ(1, action_link->GetLinkAlpha(state, false));
}

TEST_F(TestActionLink, LinkAlphaOnDisabled)
{
   ButtonVisualState state = ButtonVisualState::VISUAL_STATE_DISABLED;
   EXPECT_EQ(4, action_link->GetLinkAlpha(state, false));
}

}

}
