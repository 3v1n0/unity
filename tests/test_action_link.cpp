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
#include <gmock/gmock.h>

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
    ActionLinkMock(std::string const& action_hint, std::string const& label, NUX_FILE_LINE_PROTO)
      : ActionLink(action_hint, label){}
    ~ActionLinkMock(){}

    nux::ObjectPtr<nux::StaticCairoText> GetText() { return static_text_; }

    MOCK_METHOD0(GetVisualState, void()); 

    using ActionLink::GetLinkAlpha;
    using ActionLink::ComputeContentSize;
    using ActionLink::Draw;
    using ActionLink::DrawContent;
    using ActionLink::RecvClick;
    using ActionLink::GetName;
    using ActionLink::AddProperties;
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

TEST_F(TestActionLink, LinkAlphaOnPressed)
{
   ButtonVisualState state = ButtonVisualState::VISUAL_STATE_PRESSED;
   ::testing::DefaultValue<ButtonVisualState>::Set(state);
   EXPECT_EQ(4, action_link->GetLinkAlpha());
}

TEST_F(TestActionLink, LinkAlphaOnNormal)
{
   ButtonVisualState state = ButtonVisualState::VISUAL_STATE_NORMAL;
   ::testing::DefaultValue<ButtonVisualState>::Set(state);
   EXPECT_EQ(4, action_link->GetLinkAlpha());
}

TEST_F(TestActionLink, LinkAlphaOnPrelight)
{
   ButtonVisualState state = ButtonVisualState::VISUAL_STATE_PRELIGHT;
   ::testing::DefaultValue<ButtonVisualState>::Set(state);
   EXPECT_EQ(4, action_link->GetLinkAlpha());
}

TEST_F(TestActionLink, LinkAlphaOnDisabled)
{
   ButtonVisualState state = ButtonVisualState::VISUAL_STATE_DISABLED;
   ::testing::DefaultValue<ButtonVisualState>::Set(state);
   EXPECT_EQ(4, action_link->GetLinkAlpha());
}

TEST_F(TestActionLink, ActionEmittedRecvClick)
{
  action_link->ComputeContentSize();
}

}

}
