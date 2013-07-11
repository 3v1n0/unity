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
#include <UnityCore/Variant.h>
#include "panel-service.h"
#include "mock_indicator_object.h"

using namespace testing;
using namespace unity;

namespace
{

struct TestPanelService : Test
{
  TestPanelService()
    : service(panel_service_get_default_with_indicators(nullptr))
  {}

  glib::Object<PanelService> service;
};

TEST_F(TestPanelService, Construction)
{
  ASSERT_TRUE(service.IsType(PANEL_TYPE_SERVICE));
  EXPECT_EQ(0, panel_service_get_n_indicators(service));
}

TEST_F(TestPanelService, Destruction)
{
  gpointer weak_ptr = reinterpret_cast<gpointer>(0xdeadbeef);
  g_object_add_weak_pointer(glib::object_cast<GObject>(service), &weak_ptr);
  ASSERT_THAT(weak_ptr, NotNull());
  service = nullptr;
  EXPECT_THAT(weak_ptr, IsNull());
}

TEST_F(TestPanelService, Singleton)
{
  ASSERT_EQ(service, panel_service_get_default());
  EXPECT_EQ(1, G_OBJECT(service.RawPtr())->ref_count);
}

TEST_F(TestPanelService, IndicatorLoading)
{
  glib::Object<IndicatorObject> object(mock_indicator_object_new());
  ASSERT_TRUE(object.IsType(INDICATOR_OBJECT_TYPE));
  ASSERT_TRUE(object.IsType(MOCK_TYPE_INDICATOR_OBJECT));

  panel_service_add_indicator(service, object);
  ASSERT_EQ(1, panel_service_get_n_indicators(service));

  EXPECT_EQ(object, panel_service_get_indicator_nth(service, 0));
}

TEST_F(TestPanelService, EmptyIndicatorObject)
{
  glib::Object<IndicatorObject> object(mock_indicator_object_new());
  panel_service_add_indicator(service, object);

  glib::Variant result(panel_service_sync(service));
  EXPECT_TRUE(result);
}

} // anonymous namespace
