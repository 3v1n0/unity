// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>
#include "test_mock_session_manager.h"
#include "SessionButton.h"
#include "SessionView.h"
#include "UnitySettings.h"

namespace unity
{
namespace session
{

struct TestSessionView : testing::Test
{
  struct ViewWrap : View
  {
    ViewWrap() : View(std::make_shared<testing::NiceMock<MockManager>>()) {}

    std::string GetTitle() const { return title_->GetText(); }
    std::string GetSubTitle() const { return subtitle_->GetText(); }

    std::list<Button*> GetButtons() const
    {
      std::list<Button*> buttons;

      for (auto const& button : buttons_layout_->GetChildren())
      {
        auto session_button = dynamic_cast<Button*>(button);
        EXPECT_NE(session_button, nullptr);

        if (!session_button)
          return buttons;

        buttons.push_back(session_button);
      }

      return buttons;
    }
  };

  unity::Settings settings;
  ViewWrap button;
};

TEST_F(TestSessionView, Construct)
{
  
}


} // session
} // unity