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

#include <gmock/gmock.h>
#include "MenuManager.h"
#include "mock_indicators.h"
#include "mock_key_grabber.h"

namespace unity
{
namespace menu
{
namespace
{

struct MockManager : Manager
{
  typedef std::shared_ptr<MockManager> Ptr;

  MockManager()
    : Manager(std::make_shared<indicator::MockIndicators::Nice>(),
              std::make_shared<key::MockGrabber::Nice>())
  {}

  indicator::MockIndicators::Ptr indicators;
  key::MockGrabber::Ptr key_grabber;
};

} // anonymous namespace
} // menu namespace
} // unity namespace
