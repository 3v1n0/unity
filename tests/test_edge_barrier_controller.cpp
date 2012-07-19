/*
 * Copyright 2012 Canonical Ltd.
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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 *
 */

#include <gmock/gmock.h>
#include "test_utils.h"

#include "EdgeBarrierController.h"
#include "MultiMonitor.h"

using namespace unity;
using namespace unity::ui;

namespace
{

class MockPointerBarrier : public PointerBarrierWrapper
{
public:
  MOCK_METHOD0(ConstructBarrier, void());
  MOCK_METHOD0(DestroyBarrier, void());
  MOCK_METHOD1(ReleaseBarrier, void(int));
};

class TestBarrierSubscriber : public EdgeBarrierSubscriber
{
public:
  TestBarrierSubscriber(bool handles = false)
    : handles_(false)
  {}

  bool HandleBarrierEvent(PointerBarrierWrapper* owner, BarrierEvent::Ptr event)
  {
    return handles_;
  }

  bool handles_;
};

class TestEdgeBarrierController : public testing::Test
{
public:
  virtual void SetUp()
  {
    bc.options = std::make_shared<launcher::Options>();

    for (int i = 0; i < max_num_monitors; ++i)
    {
      bc.Subscribe(&subscribers_[i], i);
    }
  }

  EdgeBarrierController bc;
  TestBarrierSubscriber subscribers_[max_num_monitors];
};

TEST_F(TestEdgeBarrierController, Construction)
{
  EXPECT_FALSE(bc.sticky_edges);
}

}
