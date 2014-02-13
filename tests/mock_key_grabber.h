/*
 * Copyright 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 *
 */

#include "KeyGrabber.h"
#include <gmock/gmock.h>

namespace unity
{
namespace key
{
namespace
{

struct MockGrabber : Grabber
{
  typedef std::shared_ptr<MockGrabber> Ptr;
  typedef testing::NiceMock<MockGrabber> Nice;

  MockGrabber()
  {
    ON_CALL(*this, GetActions()).WillByDefault(testing::ReturnRef(actions_));
  }

  MOCK_METHOD0(GetActions, CompAction::Vector&());
  MOCK_METHOD1(AddAction, void(CompAction const&));
  MOCK_METHOD1(RemoveAction, void(CompAction const&));

  CompAction::Vector actions_;
};

} // anonymous namespace
} // key namespace
} // unity namespace
