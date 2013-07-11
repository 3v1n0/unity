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
#include <UnityCore/GLibWrapper.h>
#include "panel-service.h"

using namespace testing;

namespace
{

struct TestPanelService : Test
{
  TestPanelService()
    : service(panel_service_get_default_with_indicators(nullptr))
  {}

  unity::glib::Object<PanelService> service;
};

TEST_F(TestPanelService, Construct)
{
  ASSERT_TRUE(service.IsType(PANEL_TYPE_SERVICE));

  gpointer weak_ptr = reinterpret_cast<gpointer>(0xdeadbeef);
  g_object_add_weak_pointer(unity::glib::object_cast<GObject>(service), &weak_ptr);
  ASSERT_THAT(weak_ptr, NotNull());
  service = nullptr;
  EXPECT_THAT(weak_ptr, IsNull());
}

TEST_F(TestPanelService, Singleton)
{
  ASSERT_EQ(service, panel_service_get_default());
  EXPECT_EQ(G_OBJECT(service.RawPtr())->ref_count, 1);
}

} // anonymous namespace
